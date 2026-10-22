// Ess Clean DSP engine: de-esser (band or broadband) with lookahead and a Listen mode.
//
// Pure C++ (no JUCE) so it is unit-tested headless in tests/.
//
// Detector (shared by all channels, runs `lookahead` samples ahead of the audio):
//   d = max over channels |BP4(x)|  (4th-order band-pass over the detection range)
//   level = peak follower (attack / release) -> dB
//   reduction dB = min (range, soft-knee (level - threshold))       (knee 6 dB, infinite ratio)
//   g = 10^(-reduction / 20)                                          (exactly 1 below the knee)
//
// Audio (delayed by `lookahead`):
//   band mode:      y = x - (1 - g) * band        band = BP2(x), so y == x bit-exactly when g == 1
//                   (a single 2nd-order band-pass keeps the reduction flat across the range; at full
//                   reduction y is exactly an RBJ notch over the range)
//   broadband mode: y = g * x
//   listen:         y = what was removed, i.e. (1 - g) * band or (1 - g) * x
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../../../shared/dsp/Biquad.h"
#include "../../../shared/dsp/FractionalDelay.h"
#include "../../../shared/dsp/Smoothing.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace rommuser::essclean
{

enum class Mode { band = 0, broadband };

struct EssParams
{
    float lowHz = 4500.0f;      // detection range
    float highHz = 9000.0f;
    float thresholdDb = -24.0f; // -60 .. 0 (level of the band-passed signal)
    float rangeDb = 10.0f;      // 0 .. 24: maximum reduction
    float attackMs = 1.0f;      // 0.1 .. 20
    float releaseMs = 80.0f;    // 10 .. 500
    Mode mode = Mode::band;
    float lookaheadMs = 0.0f;   // 0, 1, 2, 5
    bool listen = false;
};

struct EssMeters
{
    float reductionDb = 0.0f;   // largest reduction in the block (positive dB)
    float levelDb = -120.0f;    // detector level at the end of the block
    float inputPeak = 0.0f, outputPeak = 0.0f;
};

inline constexpr int maxChannels = 2;
inline constexpr float kneeDb = 6.0f;
inline constexpr float maxLookaheadMs = 5.0f;
inline constexpr float floorDb = -120.0f;

// Soft-knee gain computer: 0 below threshold - knee/2, (level - threshold) above threshold + knee/2,
// a quadratic blend in between; capped at range. Returns the reduction in positive dB.
inline float reductionDb (float levelDb, float thresholdDb, float rangeDb)
{
    const float over = levelDb - thresholdDb;
    float r = 0.0f;
    if (over <= -kneeDb * 0.5f)
        r = 0.0f;
    else if (over >= kneeDb * 0.5f)
        r = over;
    else
    {
        const float t = over + kneeDb * 0.5f;
        r = t * t / (2.0f * kneeDb);
    }
    return std::min (r, rangeDb);
}

class EssEngine
{
public:
    void prepare (double sampleRate, int numChannels)
    {
        sr = sampleRate;
        channels = std::clamp (numChannels, 1, maxChannels);
        maxLookahead = (int) std::lround (maxLookaheadMs * 0.001 * sr);
        for (auto& d : delays)
            d.setMaxDelay (maxLookahead + 1);
        setParams (params, true);
        firstSet = true;
        reset();
    }

    void reset()
    {
        for (auto& d : delays) d.reset();
        for (auto& f : detectorFilters) for (auto& b : f) b.reset();
        for (auto& f : bandFilters) f.reset();
        env = 0.0f;
    }

    void setParams (const EssParams& p, bool immediate = false)
    {
        immediate = immediate || firstSet;
        firstSet = false;

        EssParams q = p;
        q.highHz = std::max (q.highHz, q.lowHz * 1.1f);

        if (immediate || std::fabs (q.lowHz - params.lowHz) > 1e-3f || std::fabs (q.highHz - params.highHz) > 1e-3f)
            updateFilters (q.lowHz, q.highHz);
        if (immediate || std::fabs (q.attackMs - params.attackMs) > 1e-6f || std::fabs (q.releaseMs - params.releaseMs) > 1e-6f)
        {
            aAttack = (float) (1.0 - std::exp (-1.0 / (std::max (0.01, (double) q.attackMs) * 0.001 * sr)));
            aRelease = (float) (1.0 - std::exp (-1.0 / (std::max (0.1, (double) q.releaseMs) * 0.001 * sr)));
        }
        lookaheadSamples = std::clamp ((int) std::lround (q.lookaheadMs * 0.001 * sr), 0, maxLookahead);

        params = q;
    }

    const EssParams& getParams() const { return params; }
    int getLatencySamples() const { return lookaheadSamples; }
    float getBandCentreHz() const { return std::sqrt (params.lowHz * params.highHz); }

    EssMeters process (float* const* io, int numChannels, int n)
    {
        EssMeters m;
        const int nch = std::clamp (numChannels, 1, maxChannels);
        const int L = lookaheadSamples;

        for (int i = 0; i < n; ++i)
        {
            // Detector on the undelayed input.
            float d = 0.0f;
            for (int c = 0; c < nch; ++c)
            {
                const float x = io[c][i];
                m.inputPeak = std::max (m.inputPeak, std::fabs (x));
                auto& f = detectorFilters[(size_t) c];
                d = std::max (d, std::fabs (f[1].process (f[0].process (x))));
            }
            env += (d > env ? aAttack : aRelease) * (d - env);
            const float levelDb = env > 1.0e-6f ? 20.0f * std::log10 (env) : floorDb;
            const float red = reductionDb (levelDb, params.thresholdDb, params.rangeDb);
            m.reductionDb = std::max (m.reductionDb, red);
            const float g = red > 0.0f ? dsp::dbToGain (-red) : 1.0f;
            const float removed = 1.0f - g;

            for (int c = 0; c < nch; ++c)
            {
                auto& dl = delays[(size_t) c];
                dl.push (io[c][i]);
                const float x = L > 0 ? dl.readInteger (L) : io[c][i];
                float y;
                if (params.mode == Mode::band)
                {
                    const float band = bandFilters[(size_t) c].process (x);
                    y = params.listen ? removed * band : x - removed * band;
                }
                else
                {
                    y = params.listen ? removed * x : x * g;
                }
                io[c][i] = y;
                m.outputPeak = std::max (m.outputPeak, std::fabs (y));
            }
        }

        m.levelDb = env > 1.0e-6f ? 20.0f * std::log10 (env) : floorDb;
        return m;
    }

private:
    void updateFilters (float lowHz, float highHz)
    {
        const double fc = std::sqrt ((double) lowHz * highHz);
        const double q = fc / std::max (1.0, (double) (highHz - lowHz));
        const auto c = dsp::biquad::bandPass (fc, q, sr);
        for (auto& f : detectorFilters) for (auto& b : f) b.setCoeffs (c);
        for (auto& f : bandFilters) f.setCoeffs (c);
    }

    double sr = 48000.0;
    int channels = 2, maxLookahead = 0, lookaheadSamples = 0;
    bool firstSet = true;
    EssParams params;
    float env = 0.0f, aAttack = 1.0f, aRelease = 1.0f;
    std::array<std::array<dsp::Biquad, 2>, maxChannels> detectorFilters;
    std::array<dsp::Biquad, maxChannels> bandFilters;
    std::array<dsp::FractionalDelay, maxChannels> delays;
};

} // namespace rommuser::essclean
