// Shape EQ DSP engine: up to 12 bands, each a cascade of biquads, per channel placement
// (stereo, left, right, mid, side), with coefficient smoothing and an output gain stage.
//
// Pure C++ (no JUCE) so it is unit-tested headless in tests/.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../../../shared/dsp/Biquad.h"
#include "../../../shared/dsp/Smoothing.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <vector>

namespace rommuser::shapeeq
{

constexpr int maxBands = 12;
constexpr int maxSections = 4; // 48 dB/oct = four 2nd-order sections

enum class Placement { stereo, left, right, mid, side };

struct BandParams
{
    bool enabled = false;
    dsp::FilterType type = dsp::FilterType::bell;
    double freq = 1000.0;
    double gainDb = 0.0;
    double q = 1.0;
    int slopeDb = 12;             // 6, 12, 18, 24, 36, 48 for cuts
    Placement placement = Placement::stereo;
};

struct EqParams
{
    std::array<BandParams, maxBands> bands {};
    double outputDb = 0.0;
    bool autoGain = false;
    bool bypass = false;
};

// Coefficient set for one band: up to maxSections biquads in series.
struct BandCoeffs
{
    int numSections = 0;
    std::array<dsp::BiquadCoeffs, maxSections> sections {};

    std::complex<double> response (double w) const
    {
        std::complex<double> h (1.0, 0.0);
        for (int i = 0; i < numSections; ++i)
            h *= sections[(size_t) i].response (w);
        return h;
    }
};

inline BandCoeffs designBand (const BandParams& p, double sampleRate)
{
    using namespace dsp;
    BandCoeffs out;
    auto add = [&] (const BiquadCoeffs& c) { if (out.numSections < maxSections) out.sections[(size_t) out.numSections++] = c; };

    switch (p.type)
    {
        case FilterType::bell:      add (biquad::bellMatched (p.freq, p.q, p.gainDb, sampleRate)); break;
        case FilterType::lowShelf:  add (biquad::lowShelf (p.freq, p.q, p.gainDb, sampleRate)); break;
        case FilterType::highShelf: add (biquad::highShelf (p.freq, p.q, p.gainDb, sampleRate)); break;
        case FilterType::notch:     add (biquad::notch (p.freq, p.q, sampleRate)); break;
        case FilterType::bandPass:  add (biquad::bandPass (p.freq, p.q, sampleRate)); break;
        case FilterType::allPass:   add (biquad::allPass (p.freq, p.q, sampleRate)); break;
        case FilterType::tilt:
            // +gain above, -gain below, pivoting at freq. Two gentle shelves of half the gain each.
            add (biquad::lowShelf (p.freq, 0.7071, -p.gainDb * 0.5, sampleRate));
            add (biquad::highShelf (p.freq, 0.7071, p.gainDb * 0.5, sampleRate));
            break;
        case FilterType::lowCut:
        case FilterType::highCut:
        {
            const bool hp = p.type == FilterType::lowCut;
            const int slope = std::clamp (p.slopeDb, 6, 48);
            if (slope == 6)
            {
                add (hp ? biquad::highPass1 (p.freq, sampleRate) : biquad::lowPass1 (p.freq, sampleRate));
                break;
            }
            const int order = slope / 6;                 // 2, 3(18 dB), 4, 6, 8
            const int fullSections = order / 2;
            for (int s = 0; s < fullSections; ++s)
            {
                // Butterworth by default; the band's Q scales the resonance of the last section
                // so a "resonant cut" is still possible from the same control.
                double q = biquad::butterworthSectionQ (order, s);
                if (s == fullSections - 1 && order == 2)
                    q = p.q;
                add (hp ? biquad::highPass (p.freq, q, sampleRate) : biquad::lowPass (p.freq, q, sampleRate));
            }
            if (order % 2 == 1)
                add (hp ? biquad::highPass1 (p.freq, sampleRate) : biquad::lowPass1 (p.freq, sampleRate));
            break;
        }
    }
    return out;
}

// Estimated loudness change of a curve, used for auto gain: the response averaged in dB over
// log-spaced frequencies 30 Hz - 16 kHz, weighted toward the mids where loudness lives.
inline double estimateAutoGainDb (const EqParams& p, double sampleRate)
{
    double sum = 0.0, wsum = 0.0;
    for (int i = 0; i <= 60; ++i)
    {
        const double f = 30.0 * std::pow (16000.0 / 30.0, i / 60.0);
        const double w = 2.0 * 3.14159265358979323846 * f / sampleRate;
        std::complex<double> h (1.0, 0.0);
        for (int b = 0; b < maxBands; ++b)
            if (p.bands[(size_t) b].enabled)
                h *= designBand (p.bands[(size_t) b], sampleRate).response (w);
        // Rough equal-loudness weight: flat 200 Hz - 5 kHz, falling outside.
        const double weight = f < 200.0 ? f / 200.0 : f > 5000.0 ? 5000.0 / f : 1.0;
        sum += weight * 20.0 * std::log10 (std::max (std::abs (h), 1e-12));
        wsum += weight;
    }
    return wsum > 0.0 ? -sum / wsum : 0.0;
}

class EqEngine
{
public:
    static constexpr int maxChannels = 2;

    void prepare (double newSampleRate, int maxBlockSize, int numChannels)
    {
        sampleRate = newSampleRate;
        channels = std::min (numChannels, maxChannels);
        // Coefficient ramps: ~ 5 ms, in blocks of `rampBlock` samples to keep the cost down.
        rampBlock = 32;
        rampSteps = std::max (1, (int) std::lround (sampleRate * 0.005 / rampBlock));
        mid.assign ((size_t) maxBlockSize, 0.0f);
        side.assign ((size_t) maxBlockSize, 0.0f);
        outGain.prepare (sampleRate, 0.02);
        setParams (params, true);
        reset();
    }

    void reset()
    {
        for (auto& ch : filters)
            for (auto& band : ch)
                for (auto& s : band)
                    s.reset();
    }

    void setParams (const EqParams& p, bool immediate = false)
    {
        params = p;
        for (int b = 0; b < maxBands; ++b)
        {
            const auto& bp = p.bands[(size_t) b];
            target[(size_t) b] = bp.enabled ? designBand (bp, sampleRate) : BandCoeffs {};
            placement[(size_t) b] = bp.placement;
            if (immediate)
            {
                current[(size_t) b] = target[(size_t) b];
                rampRemaining[(size_t) b] = 0;
            }
            else if (! sameCoeffs (current[(size_t) b], target[(size_t) b]))
            {
                // Section count changes (type/slope switches) can't be interpolated: jump.
                if (current[(size_t) b].numSections != target[(size_t) b].numSections)
                {
                    current[(size_t) b] = target[(size_t) b];
                    rampRemaining[(size_t) b] = 0;
                    for (auto& ch : filters)
                        for (auto& s : ch[(size_t) b])
                            s.reset();
                }
                else
                {
                    rampStart[(size_t) b] = current[(size_t) b];
                    rampRemaining[(size_t) b] = rampSteps;
                }
            }
        }
        for (int c = 0; c < maxChannels; ++c)
            for (int b = 0; b < maxBands; ++b)
                for (int s = 0; s < maxSections; ++s)
                    filters[(size_t) c][(size_t) b][(size_t) s].setCoeffs (current[(size_t) b].sections[(size_t) s]);

        const float g = dsp::dbToGain ((float) (p.outputDb + (p.autoGain ? autoGainDb (p) : 0.0)));
        immediate ? outGain.setImmediate (g) : outGain.setTarget (g);
    }

    // Total magnitude (dB) of the enabled bands at a frequency, for the display. Placement is
    // ignored here (the curve shows what a centred source would get); `onlyBand` draws one band.
    double magnitudeDb (double freqHz, int onlyBand = -1) const
    {
        const double w = 2.0 * 3.14159265358979323846 * freqHz / sampleRate;
        std::complex<double> h (1.0, 0.0);
        for (int b = 0; b < maxBands; ++b)
            if (onlyBand < 0 || b == onlyBand)
                h *= target[(size_t) b].response (w);
        return 20.0 * std::log10 (std::max (std::abs (h), 1e-12));
    }

    double autoGainDb (const EqParams& p) const { return estimateAutoGainDb (p, sampleRate); }

    int getLatencySamples() const { return 0; }

    void process (float* const* io, int numChannels, int n)
    {
        if (params.bypass)
            return;

        numChannels = std::min (numChannels, channels);
        const bool stereo = numChannels == 2;

        int offset = 0;
        while (offset < n)
        {
            const int len = std::min (rampBlock, n - offset);
            advanceRamps();

            if (stereo && usesMidSide())
            {
                float* L = io[0] + offset;
                float* R = io[1] + offset;
                for (int i = 0; i < len; ++i)
                {
                    mid[(size_t) i] = 0.5f * (L[i] + R[i]);
                    side[(size_t) i] = 0.5f * (L[i] - R[i]);
                }
                // Stereo / L / R bands run on L and R; mid/side bands run on M and S. Doing L/R
                // first then M/S is fine because all stages are linear and time-invariant per block.
                runBands (L, 0, len, { Placement::stereo, Placement::left });
                runBands (R, 1, len, { Placement::stereo, Placement::right });
                for (int i = 0; i < len; ++i)
                {
                    mid[(size_t) i] = 0.5f * (L[i] + R[i]);
                    side[(size_t) i] = 0.5f * (L[i] - R[i]);
                }
                runBands (mid.data(), 0, len, { Placement::mid, Placement::mid });
                runBands (side.data(), 1, len, { Placement::side, Placement::side });
                for (int i = 0; i < len; ++i)
                {
                    L[i] = mid[(size_t) i] + side[(size_t) i];
                    R[i] = mid[(size_t) i] - side[(size_t) i];
                }
            }
            else
            {
                for (int c = 0; c < numChannels; ++c)
                    runBands (io[c] + offset, c, len,
                              { Placement::stereo, c == 0 ? Placement::left : Placement::right, true });
            }

            for (int i = 0; i < len; ++i)
            {
                const float g = outGain.next();
                for (int c = 0; c < numChannels; ++c)
                    io[c][offset + i] *= g;
            }
            offset += len;
        }
    }

private:
    struct Accept
    {
        Placement a, b;
        bool monoAcceptsMidSide = false; // in mono, mid/side bands act on the single channel

        bool operator() (Placement p) const
        {
            if (p == a || p == b) return true;
            return monoAcceptsMidSide && (p == Placement::mid || p == Placement::side);
        }
    };

    bool usesMidSide() const
    {
        for (int b = 0; b < maxBands; ++b)
            if (target[(size_t) b].numSections > 0 && (placement[(size_t) b] == Placement::mid || placement[(size_t) b] == Placement::side))
                return true;
        return false;
    }

    void runBands (float* x, int channel, int n, Accept accept)
    {
        for (int b = 0; b < maxBands; ++b)
        {
            const auto& c = current[(size_t) b];
            if (c.numSections == 0 || ! accept (placement[(size_t) b]))
                continue;
            for (int s = 0; s < c.numSections; ++s)
            {
                auto& f = filters[(size_t) channel][(size_t) b][(size_t) s];
                for (int i = 0; i < n; ++i)
                    x[i] = f.process (x[i]);
            }
        }
    }

    static bool sameCoeffs (const BandCoeffs& a, const BandCoeffs& b)
    {
        if (a.numSections != b.numSections) return false;
        for (int s = 0; s < a.numSections; ++s)
        {
            // Bitwise compare: a difference of -0.0 vs 0.0 only costs a harmless ramp.
            if (std::memcmp (&a.sections[(size_t) s], &b.sections[(size_t) s], sizeof (dsp::BiquadCoeffs)) != 0)
                return false;
        }
        return true;
    }

    void advanceRamps()
    {
        for (int b = 0; b < maxBands; ++b)
        {
            auto& remaining = rampRemaining[(size_t) b];
            if (remaining <= 0)
                continue;
            --remaining;
            const double t = 1.0 - (double) remaining / (double) rampSteps;
            auto& cur = current[(size_t) b];
            const auto& from = rampStart[(size_t) b];
            const auto& to = target[(size_t) b];
            for (int s = 0; s < cur.numSections; ++s)
            {
                auto& c = cur.sections[(size_t) s];
                const auto& f = from.sections[(size_t) s];
                const auto& g = to.sections[(size_t) s];
                c.b0 = f.b0 + (g.b0 - f.b0) * t;
                c.b1 = f.b1 + (g.b1 - f.b1) * t;
                c.b2 = f.b2 + (g.b2 - f.b2) * t;
                c.a1 = f.a1 + (g.a1 - f.a1) * t;
                c.a2 = f.a2 + (g.a2 - f.a2) * t;
                for (int ch = 0; ch < maxChannels; ++ch)
                    filters[(size_t) ch][(size_t) b][(size_t) s].setCoeffs (c);
            }
        }
    }

    EqParams params;
    double sampleRate = 44100.0;
    int channels = 2, rampBlock = 32, rampSteps = 1;
    std::array<BandCoeffs, maxBands> current {}, target {}, rampStart {};
    std::array<int, maxBands> rampRemaining {};
    std::array<Placement, maxBands> placement {};
    std::array<std::array<std::array<dsp::Biquad, maxSections>, maxBands>, maxChannels> filters {};
    std::vector<float> mid, side;
    dsp::LinearSmoother outGain;
};

} // namespace rommuser::shapeeq
