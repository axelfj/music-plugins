// Warm Drive DSP engine: multiband saturation (1 to 4 bands) with oversampling.
//
// Pure C++ (no JUCE) so it is unit-tested headless in tests/.
//
// Signal flow per channel:
//   x * inGain -> upsample (1x / 2x / 4x) -> LR4 split into bands (filters run at the high rate)
//   per band:  u = band * drive
//              v = shape (u) / drive * autoGain          (one of five curves, see Saturation)
//              v = dcBlock (v)                           (the asymmetric curves add DC)
//              v = tone (v)                              (first-order low-pass, 20 kHz = off)
//              band' = bandMix * v + (1 - bandMix) * band
//   mix * sum (band') + (1 - mix) * sum (band) -> downsample -> * outGain = out
//   (the dry side of the mix is the band sum, so it has the same phase as the wet side and the mix never combs)
//
// Dividing by the drive after the curve keeps the slope at 0 equal to 1, so quiet material passes
// almost untouched and the drive knob really means "how hard the loud parts hit the curve".
// Auto gain (optional) measures the curve's gain on a -12 dBFS sine at setParams time and compensates it.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../../../shared/dsp/Crossover.h"
#include "../../../shared/dsp/Oversampler.h"
#include "../../../shared/dsp/Smoothing.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <vector>

namespace rommuser::warmdrive
{

constexpr int maxBands = 4;

enum class Saturation { tape, tube, transistor, softClip, rectify };
constexpr int numSaturations = 5;

// The curves, all with unity slope at 0 and bounded output.
inline float shape (float u, Saturation type)
{
    switch (type)
    {
        case Saturation::tape:
            // Smooth, symmetric: odd harmonics that fall off quickly.
            return std::tanh (u);

        case Saturation::tube:
        {
            // Asymmetric: a tanh shifted by a bias, so one half-wave compresses sooner than the other
            // and even harmonics appear (the "warm" 2nd). Re-centred so shape (0) = 0 and divided by
            // tanh' (bias) so the slope at 0 is 1.
            constexpr float bias = 0.4f;
            const float tb = std::tanh (bias);
            return (std::tanh (u + bias) - tb) / (1.0f - tb * tb);
        }

        case Saturation::transistor:
        {
            // Harder knee, more upper harmonics: a cubic soft clipper that flattens at |u| >= 1.5.
            const float a = std::clamp (u, -1.5f, 1.5f);
            return a - a * a * a / 6.75f;  // slope 1 at 0, reaches 1.0 at |u| = 1.5
        }

        case Saturation::softClip:
        {
            // Linear up to 0.5, then a tanh segment that approaches 1 (same family as Clip Ready).
            constexpr float t = 0.5f, w = 1.0f - t;
            const float a = std::fabs (u);
            if (a <= t) return u;
            const float y = t + w * std::tanh ((a - t) / w);
            return u > 0.0f ? y : -y;
        }

        case Saturation::rectify:
        default:
        {
            // Part of the signal is folded up (a rounded |u|, so the corner at 0 does not splatter
            // aliases): a strong 2nd harmonic, "octave up" grit, then soft-clipped. / 0.6 keeps the slope at 0 equal to 1.
            const float folded = std::sqrt (u * u + 0.01f) - 0.1f;
            return std::tanh (0.6f * u + 0.4f * folded) / 0.6f;
        }
    }
}

struct BandSettings
{
    Saturation type = Saturation::tape;
    float driveDb = 6.0f;      // 0 .. 36
    float toneHz = 20000.0f;   // 1000 .. 20000 (20000 = off)
    float mix = 1.0f;          // 0 .. 1
};

struct DriveParams
{
    int numBands = 1;              // 1 .. 4
    float crossoverHz[3] = { 150.0f, 800.0f, 4000.0f };
    std::array<BandSettings, maxBands> bands {};
    bool autoGain = true;
    float inDb = 0.0f;             // -24 .. 24
    float outDb = 0.0f;            // -24 .. 24
    float mix = 1.0f;              // 0 .. 1
    int oversampling = 2;          // 1, 2, 4
};

struct DriveMeters
{
    float inputPeak = 0.0f, outputPeak = 0.0f;
    std::array<float, maxBands> bandDriveDb {};  // how far (dB) each band's peak went into the curve (> 0 = saturating)
};

// Gain (linear) of shape (u * drive) / drive for a -12 dBFS sine, measured on 64 points: what auto gain undoes.
inline float curveGainFor (Saturation type, float drive)
{
    constexpr int points = 64;
    constexpr float amp = 0.25118864f;  // -12 dBFS
    double in = 0.0, out = 0.0;
    for (int i = 0; i < points; ++i)
    {
        const float x = amp * std::sin (6.283185307f * (float) i / (float) points);
        const float y = shape (x * drive, type) / drive;
        in += (double) x * x;
        out += (double) y * y;
    }
    return (float) std::sqrt (std::max (out, 1e-20) / std::max (in, 1e-20));
}

class DriveEngine
{
public:
    static constexpr int maxChannels = 2;
    static constexpr std::array<int, 3> factors { 1, 2, 4 };

    void prepare (double newSampleRate, int maxBlockSize, int numChannels)
    {
        sampleRate = newSampleRate;
        channels = std::clamp (numChannels, 1, maxChannels);
        blockSize = maxBlockSize;

        for (int c = 0; c < maxChannels; ++c)
            for (size_t f = 0; f < factors.size(); ++f)
            {
                oversamplers[(size_t) c][f] = std::make_unique<dsp::Oversampler> (maxBlockSize);
                oversamplers[(size_t) c][f]->prepare (factors[f], maxBlockSize);
            }
        pre.assign ((size_t) maxBlockSize, 0.0f);

        inGain.prepare (sampleRate, 0.02);
        outGain.prepare (sampleRate, 0.02);
        mixSmooth.prepare (sampleRate, 0.02);
        for (auto& b : bandState)
        {
            b.drive.prepare (sampleRate, 0.02);
            b.comp.prepare (sampleRate, 0.02);
            b.mix.prepare (sampleRate, 0.02);
        }

        currentFactorIndex = -1;
        setParams (params, true);
        reset();
    }

    void reset()
    {
        for (int c = 0; c < maxChannels; ++c)
        {
            for (auto& o : oversamplers[(size_t) c])
                if (o) o->reset();
            splitters[(size_t) c].reset();
            for (auto& f : dcBlockers[(size_t) c]) f.reset();
            for (auto& f : toneFilters[(size_t) c]) f.reset();
        }
    }

    void setParams (const DriveParams& p, bool immediate = false)
    {
        int idx = 1;
        for (size_t f = 0; f < factors.size(); ++f)
            if (factors[f] == p.oversampling)
                idx = (int) f;

        const bool rateChanged = idx != currentFactorIndex;
        bool xoverChanged = rateChanged || p.numBands != params.numBands;
        for (int i = 0; i < 3; ++i)
            xoverChanged = xoverChanged || std::fabs (p.crossoverHz[i] - params.crossoverHz[i]) > 0.0f;
        bool toneChanged = rateChanged;
        for (int b = 0; b < maxBands; ++b)
            toneChanged = toneChanged || std::fabs (p.bands[(size_t) b].toneHz - params.bands[(size_t) b].toneHz) > 0.0f;

        params = p;
        params.numBands = std::clamp (p.numBands, 1, maxBands);

        if (rateChanged)
        {
            currentFactorIndex = idx;
            const double highRate = sampleRate * factors[(size_t) idx];
            for (int c = 0; c < maxChannels; ++c)
            {
                if (oversamplers[(size_t) c][(size_t) idx])
                    oversamplers[(size_t) c][(size_t) idx]->reset();
                splitters[(size_t) c].prepare (highRate);
                const auto dc = dsp::biquad::highPass1 (10.0, highRate);
                for (auto& f : dcBlockers[(size_t) c]) { f.setCoeffs (dc); f.reset(); }
            }
        }
        if (xoverChanged)
            for (auto& s : splitters)
            {
                s.setNumBands (params.numBands);
                s.setFrequencies (p.crossoverHz[0], p.crossoverHz[1], p.crossoverHz[2]);
            }
        if (toneChanged)
        {
            const double highRate = sampleRate * factors[(size_t) currentFactorIndex];
            for (int b = 0; b < maxBands; ++b)
            {
                const float hz = p.bands[(size_t) b].toneHz;
                const bool off = hz >= 19999.0f;
                const auto c = off ? dsp::BiquadCoeffs::identity() : dsp::biquad::lowPass1 (hz, highRate);
                for (int ch = 0; ch < maxChannels; ++ch)
                    toneFilters[(size_t) ch][(size_t) b].setCoeffs (c);
            }
        }

        auto set = [immediate] (dsp::LinearSmoother& s, float v) { immediate ? s.setImmediate (v) : s.setTarget (v); };
        set (inGain, dsp::dbToGain (p.inDb));
        set (outGain, dsp::dbToGain (p.outDb));
        set (mixSmooth, std::clamp (p.mix, 0.0f, 1.0f));
        for (int b = 0; b < maxBands; ++b)
        {
            const auto& bs = p.bands[(size_t) b];
            const float drive = dsp::dbToGain (std::clamp (bs.driveDb, 0.0f, 36.0f));
            set (bandState[(size_t) b].drive, drive);
            set (bandState[(size_t) b].comp, p.autoGain ? 1.0f / std::max (curveGainFor (bs.type, drive), 0.05f) : 1.0f);
            set (bandState[(size_t) b].mix, std::clamp (bs.mix, 0.0f, 1.0f));
        }
    }

    const DriveParams& getParams() const { return params; }

    int getLatencySamples() const
    {
        const auto& o = oversamplers[0u][(size_t) std::max (0, currentFactorIndex)];
        return o ? o->getLatencySamples() : 0;
    }

    DriveMeters process (float* const* io, int numChannels, int n)
    {
        DriveMeters meters;
        int offset = 0;
        while (offset < n)
        {
            const int len = std::min (blockSize, n - offset);
            processChunk (io, std::min (numChannels, channels), offset, len, meters);
            offset += len;
        }
        return meters;
    }

private:
    struct BandState { dsp::LinearSmoother drive, comp, mix; };

    void processChunk (float* const* io, int numChannels, int offset, int n, DriveMeters& meters)
    {
        // Snapshot the smoothers so every channel sees the same ramps.
        const auto startIn = inGain, startOut = outGain, startMix = mixSmooth;
        const auto startBands = bandState;
        const int nb = params.numBands;

        for (int c = 0; c < numChannels; ++c)
        {
            const auto ci = (size_t) c;
            auto sIn = startIn, sOut = startOut, sMix = startMix;
            auto bands = startBands;
            float* x = io[c] + offset;
            auto& os = *oversamplers[ci][(size_t) currentFactorIndex];
            const int factor = os.getFactor();

            for (int i = 0; i < n; ++i)
            {
                const float v = x[i] * sIn.next();
                meters.inputPeak = std::max (meters.inputPeak, std::fabs (v));
                pre[(size_t) i] = v;
            }

            float* hi = os.upsample (pre.data(), n);
            for (int i = 0; i < n; ++i)
            {
                float drive[maxBands], comp[maxBands], bmix[maxBands];
                const float m = sMix.next();
                for (int b = 0; b < nb; ++b)
                {
                    drive[b] = bands[(size_t) b].drive.next();
                    comp[b] = bands[(size_t) b].comp.next();
                    bmix[b] = bands[(size_t) b].mix.next();
                }
                for (int k = 0; k < factor; ++k)
                {
                    float& s = hi[i * factor + k];
                    float split[maxBands];
                    splitters[ci].process (s, split);
                    float wetSum = 0.0f, drySum = 0.0f;
                    for (int b = 0; b < nb; ++b)
                    {
                        const auto bi = (size_t) b;
                        const float u = split[b] * drive[b];
                        if (c == 0)
                        {
                            const float over = std::fabs (u);
                            if (over > 1.0f)
                                meters.bandDriveDb[bi] = std::max (meters.bandDriveDb[bi], 20.0f * std::log10 (over));
                        }
                        float y = shape (u, params.bands[bi].type) / drive[b] * comp[b];
                        y = dcBlockers[ci][bi].process (y);
                        y = toneFilters[ci][bi].process (y);
                        wetSum += bmix[b] * y + (1.0f - bmix[b]) * split[b];
                        drySum += split[b];
                    }
                    s = m * wetSum + (1.0f - m) * drySum;
                }
            }

            os.downsample (hi, pre.data(), n);

            for (int i = 0; i < n; ++i)
            {
                const float y = pre[(size_t) i] * sOut.next();
                x[i] = y;
                meters.outputPeak = std::max (meters.outputPeak, std::fabs (y));
            }
        }

        inGain = startIn; outGain = startOut; mixSmooth = startMix;
        bandState = startBands;
        for (int i = 0; i < n; ++i)
        {
            inGain.next(); outGain.next(); mixSmooth.next();
            for (auto& b : bandState) { b.drive.next(); b.comp.next(); b.mix.next(); }
        }
    }

    DriveParams params;
    double sampleRate = 48000.0;
    int channels = 2, blockSize = 512, currentFactorIndex = -1;

    std::array<std::array<std::unique_ptr<dsp::Oversampler>, 3>, maxChannels> oversamplers;
    std::array<dsp::MultibandSplitter, maxChannels> splitters;
    std::array<std::array<dsp::Biquad, maxBands>, maxChannels> dcBlockers, toneFilters;
    std::vector<float> pre;

    dsp::LinearSmoother inGain, outGain, mixSmooth;
    std::array<BandState, maxBands> bandState;
};

} // namespace rommuser::warmdrive
