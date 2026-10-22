// ITU-R BS.1770-4 loudness meter: K-weighting, momentary (400 ms), short-term (3 s) and gated
// integrated loudness. Pure C++ so it is unit-tested headless and shared by several plugins.
//
// Calibration: a 997 Hz sine at -X dBFS in both channels of a stereo signal reads -X LUFS.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "Biquad.h"
#include "Oversampler.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

namespace rommuser::dsp
{

class LoudnessMeter
{
public:
    static constexpr int maxChannels = 2;
    static constexpr float silence = -120.0f;
    static constexpr float absoluteGateLufs = -70.0f;
    static constexpr float relativeGateLu = -10.0f;

    void prepare (double sampleRate, int numChannels)
    {
        sr = sampleRate;
        channels = std::clamp (numChannels, 1, maxChannels);
        blockLength = std::max (1, (int) std::lround (sr * 0.1));
        // BS.1770 pre-filter (high shelf) and RLB high-pass, designed for any sample rate from the
        // analogue prototype (same derivation as the reference 48 kHz coefficients in the standard).
        const double pi = 3.14159265358979323846;
        BiquadCoeffs shelf, hp;
        {
            const double f0 = 1681.974450955533, q = 0.7071752369554196, gainDb = 3.999843853973347;
            const double K = std::tan (pi * f0 / sr);
            const double Vh = std::pow (10.0, gainDb / 20.0);
            const double Vb = std::pow (Vh, 0.4996667741545416);
            const double a0 = 1.0 + K / q + K * K;
            shelf = biquad::normalise (Vh + Vb * K / q + K * K, 2.0 * (K * K - Vh), Vh - Vb * K / q + K * K,
                                       a0, 2.0 * (K * K - 1.0), 1.0 - K / q + K * K);
        }
        {
            const double f0 = 38.13547087602444, q = 0.5003270373238773;
            const double K = std::tan (pi * f0 / sr);
            const double a0 = 1.0 + K / q + K * K;
            // The standard's stage 2 keeps b = [1, -2, 1] unnormalised.
            hp = biquad::normalise (1.0, -2.0, 1.0, 1.0, 2.0 * (K * K - 1.0) / a0, (1.0 - K / q + K * K) / a0);
        }
        for (int c = 0; c < maxChannels; ++c)
        {
            stage1[(size_t) c].setCoeffs (shelf);
            stage2[(size_t) c].setCoeffs (hp);
        }
        reset();
    }

    void reset()
    {
        for (auto& b : stage1) b.reset();
        for (auto& b : stage2) b.reset();
        sumSquares.fill (0.0);
        blockPos = 0;
        blockPowers.fill (0.0);
        blocksSeen = 0;
        head = 0;
        gatedPowers.clear();
        shortTermPowers.clear();
        momentary = shortTerm = integrated = silence;
        maxMomentary = maxShortTerm = silence;
        loudnessRange = 0.0f;
        newBlock = false;
    }

    void process (const float* const* in, int numChannels, int n)
    {
        const int nch = std::clamp (numChannels, 1, channels);
        for (int i = 0; i < n; ++i)
        {
            for (int c = 0; c < nch; ++c)
            {
                const float y = stage2[(size_t) c].process (stage1[(size_t) c].process (in[c][i]));
                sumSquares[(size_t) c] += (double) y * y;
            }
            if (++blockPos >= blockLength)
                finishBlock();
        }
    }

    float getMomentary() const { return momentary; }
    float getShortTerm() const { return shortTerm; }
    float getIntegrated() const { return integrated; }
    float getMaxMomentary() const { return maxMomentary; }
    float getMaxShortTerm() const { return maxShortTerm; }
    // EBU Tech 3342 loudness range: gated (-70 abs, -20 LU rel) short-term values, 10th to 95th percentile.
    float getLoudnessRange() const { return loudnessRange; }
    bool hasNewBlock() { return std::exchange (newBlock, false); }

    static float toLufs (double power) { return power > 1.0e-12 ? (float) (-0.691 + 10.0 * std::log10 (power)) : silence; }

private:
    void finishBlock()
    {
        double power = 0.0;
        for (int c = 0; c < channels; ++c)
            power += sumSquares[(size_t) c] / (double) blockLength;
        sumSquares.fill (0.0);
        blockPos = 0;

        blockPowers[(size_t) head] = power;
        head = (head + 1) % (int) blockPowers.size();
        ++blocksSeen;

        auto meanOfLast = [this] (int count)
        {
            count = std::min (count, blocksSeen);
            double s = 0.0;
            for (int k = 1; k <= count; ++k)
                s += blockPowers[(size_t) ((head - k + (int) blockPowers.size()) % (int) blockPowers.size())];
            return count > 0 ? s / (double) count : 0.0;
        };

        const double m = meanOfLast (4);
        momentary = toLufs (m);
        const double st = meanOfLast (30);
        shortTerm = toLufs (st);
        if (blocksSeen >= 4) maxMomentary = std::max (maxMomentary, momentary);
        if (blocksSeen >= 30) maxShortTerm = std::max (maxShortTerm, shortTerm);
        newBlock = true;

        // Loudness range from the gated short-term values (EBU Tech 3342).
        if (blocksSeen >= 30 && shortTerm > absoluteGateLufs)
        {
            shortTermPowers.push_back (st);
            double sum = 0.0;
            for (double p : shortTermPowers) sum += p;
            const float relGate = toLufs (sum / (double) shortTermPowers.size()) - 20.0f;
            std::vector<float> kept;
            kept.reserve (shortTermPowers.size());
            for (double p : shortTermPowers)
            {
                const float l = toLufs (p);
                if (l > relGate) kept.push_back (l);
            }
            if (kept.size() >= 2)
            {
                std::sort (kept.begin(), kept.end());
                auto percentile = [&kept] (double q)
                {
                    const double pos = q * (double) (kept.size() - 1);
                    const size_t lo = (size_t) std::floor (pos);
                    const size_t hi = std::min (lo + 1, kept.size() - 1);
                    const double frac = pos - (double) lo;
                    return (float) ((1.0 - frac) * kept[lo] + frac * kept[hi]);
                };
                loudnessRange = std::max (0.0f, percentile (0.95) - percentile (0.10));
            }
        }

        // Integrated: every 100 ms a 400 ms window enters the gate.
        if (blocksSeen >= 4 && momentary > absoluteGateLufs)
        {
            gatedPowers.push_back (m);
            double sum = 0.0;
            for (double p : gatedPowers) sum += p;
            const float relGate = toLufs (sum / (double) gatedPowers.size()) + relativeGateLu;
            double sum2 = 0.0;
            int count2 = 0;
            for (double p : gatedPowers)
                if (toLufs (p) > relGate) { sum2 += p; ++count2; }
            integrated = count2 > 0 ? toLufs (sum2 / (double) count2) : silence;
        }
    }

    double sr = 48000.0;
    int channels = 2, blockLength = 4800, blockPos = 0, blocksSeen = 0, head = 0;
    std::array<Biquad, maxChannels> stage1, stage2;
    std::array<double, maxChannels> sumSquares {};
    std::array<double, 30> blockPowers {};
    std::vector<double> gatedPowers, shortTermPowers;
    float momentary = silence, shortTerm = silence, integrated = silence;
    float maxMomentary = silence, maxShortTerm = silence, loudnessRange = 0.0f;
    bool newBlock = false;
};

// True-peak meter (BS.1770 Annex 2): 4x oversampled absolute maximum, per block and held.
class TruePeakMeter
{
public:
    static constexpr int maxChannels = 2;

    void prepare (double sampleRate, int maxBlockSize)
    {
        factor = sampleRate >= 176400.0 ? 1 : sampleRate >= 88200.0 ? 2 : 4;
        maxBlock = std::max (1, maxBlockSize);
        for (auto& o : oversamplers)
        {
            o = std::make_unique<Oversampler> (maxBlock);
            o->prepare (factor, maxBlock);
        }
        reset();
    }

    void reset()
    {
        for (auto& o : oversamplers) if (o) o->reset();
        held = 0.0f;
        samplePeakHeld = 0.0f;
    }

    // Returns the true peak of this block (linear); the held maximum is kept in getHeld().
    float process (const float* const* in, int numChannels, int n)
    {
        float blockPeak = 0.0f;
        const int nch = std::clamp (numChannels, 1, maxChannels);
        for (int c = 0; c < nch; ++c)
        {
            for (int i = 0; i < n; ++i)
            {
                const float a = std::fabs (in[c][i]);
                blockPeak = std::max (blockPeak, a);
                samplePeakHeld = std::max (samplePeakHeld, a);
            }
            if (factor > 1 && oversamplers[(size_t) c])
            {
                for (int pos = 0; pos < n; pos += maxBlock)
                {
                    const int len = std::min (maxBlock, n - pos);
                    const float* hi = oversamplers[(size_t) c]->upsample (in[c] + pos, len);
                    for (int j = 0; j < len * factor; ++j)
                        blockPeak = std::max (blockPeak, std::fabs (hi[j]));
                }
            }
        }
        held = std::max (held, blockPeak);
        return blockPeak;
    }

    float getHeld() const { return held; }
    float getSamplePeakHeld() const { return samplePeakHeld; }

private:
    int factor = 4, maxBlock = 512;
    std::array<std::unique_ptr<Oversampler>, maxChannels> oversamplers;
    float held = 0.0f, samplePeakHeld = 0.0f;
};

} // namespace rommuser::dsp
