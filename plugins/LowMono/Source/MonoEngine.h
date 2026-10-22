// Low Mono: stereo imager with mono below a crossover (Linkwitz-Riley 24 or 48 dB/oct), width
// 0-200 % on the rest, correlation meter and goniometer feed.
//
// Pure C++ (no JUCE) so the DSP is tested headless.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "dsp/Crossover.h"
#include "dsp/Smoothing.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace rommuser::lowmono
{

struct MonoParams
{
    float width = 1.0f;        // 0..2 (1 = unchanged)
    float monoBelowHz = 0.0f;  // 0 = off; 20..500
    bool steep = false;        // false = LR4 (24 dB/oct), true = LR8 (48 dB/oct)
    float lowWidth = 0.0f;     // 0..1 how much stereo to leave below the crossover (0 = full mono)
};

struct MonoMeters
{
    float correlation = 1.0f;  // -1..1, ~300 ms window
    float midPeak = 0.0f, sidePeak = 0.0f;
};

class MonoEngine
{
public:
    static constexpr int scopePoints = 512;

    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        for (auto& x : lr4) x.prepare (sampleRate);
        for (auto& x : lr8) x.prepare (sampleRate);
        widthSmooth.prepare (sampleRate, 0.02);
        lowWidthSmooth.prepare (sampleRate, 0.02);
        corrCoef = 1.0f - std::exp (-1.0f / (float) (sampleRate * 0.3));
        setParams (params);
        firstSet = true;
        reset();
    }

    void reset()
    {
        for (auto& x : lr4) x.reset();
        for (auto& x : lr8) x.reset();
        sumLR = sumLL = sumRR = 0.0f;
        scopeWrite = 0;
        for (auto& v : scopeL) v = 0.0f;
        for (auto& v : scopeR) v = 0.0f;
    }

    void setParams (const MonoParams& p)
    {
        params = p;
        params.width = std::clamp (p.width, 0.0f, 2.0f);
        params.lowWidth = std::clamp (p.lowWidth, 0.0f, 1.0f);
        monoActive = p.monoBelowHz >= 20.0f;
        if (monoActive)
        {
            const double hz = std::clamp ((double) p.monoBelowHz, 20.0, 500.0);
            for (auto& x : lr4) if (std::fabs (x.getFrequency() - hz) > 1e-6) x.setFrequency (hz);
            for (auto& x : lr8) if (std::fabs (x.getFrequency() - hz) > 1e-6) x.setFrequency (hz);
        }
        auto set = [this] (dsp::LinearSmoother& s, float v) { firstSet ? s.setImmediate (v) : s.setTarget (v); };
        set (widthSmooth, params.width);
        set (lowWidthSmooth, params.lowWidth);
    }

    const MonoParams& getParams() const { return params; }

    // Goniometer feed: the most recent scopePoints output samples (ring, oldest first from the index).
    const float* getScopeL() const { return scopeL.data(); }
    const float* getScopeR() const { return scopeR.data(); }
    int getScopeWritePosition() const { return scopeWrite; }

    MonoMeters process (float* const* io, int numChannels, int n)
    {
        firstSet = false;
        MonoMeters m;
        if (numChannels < 2)
        {
            // Mono in, nothing to image; still feed the meters.
            for (int i = 0; i < n; ++i) { updateMeters (io[0][i], io[0][i], m); }
            m.correlation = 1.0f;
            return m;
        }

        // Bypass is bit-exact: width exactly 1 and no crossover.
        const bool bypass = ! monoActive && ! widthSmooth.isSmoothing() && std::fabs (widthSmooth.getTarget() - 1.0f) < 1e-9f;

        for (int i = 0; i < n; ++i)
        {
            const float width = widthSmooth.next();
            const float lowWidth = lowWidthSmooth.next();
            float l = io[0][i], r = io[1][i];
            if (! bypass)
            {
                float loL = 0.0f, loR = 0.0f, hiL = l, hiR = r;
                if (monoActive)
                {
                    if (params.steep) { lr8[0].process (l, loL, hiL); lr8[1].process (r, loR, hiR); }
                    else              { lr4[0].process (l, loL, hiL); lr4[1].process (r, loR, hiR); }
                }
                // Low band: mono (plus whatever lowWidth leaves of its side).
                const float loM = 0.5f * (loL + loR), loS = 0.5f * (loL - loR) * lowWidth;
                // High band (or everything when the crossover is off): width on the side.
                const float hiM = 0.5f * (hiL + hiR), hiS = 0.5f * (hiL - hiR) * width;
                l = (loM + loS) + (hiM + hiS);
                r = (loM - loS) + (hiM - hiS);
            }
            io[0][i] = l;
            io[1][i] = r;
            updateMeters (l, r, m);
        }
        const float denom = std::sqrt (sumLL * sumRR);
        m.correlation = denom > 1e-12f ? std::clamp (sumLR / denom, -1.0f, 1.0f) : 1.0f;
        return m;
    }

private:
    inline void updateMeters (float l, float r, MonoMeters& m)
    {
        sumLR += corrCoef * (l * r - sumLR);
        sumLL += corrCoef * (l * l - sumLL);
        sumRR += corrCoef * (r * r - sumRR);
        m.midPeak = std::max (m.midPeak, std::fabs (0.5f * (l + r)));
        m.sidePeak = std::max (m.sidePeak, std::fabs (0.5f * (l - r)));
        scopeL[(size_t) scopeWrite] = l;
        scopeR[(size_t) scopeWrite] = r;
        scopeWrite = (scopeWrite + 1) % scopePoints;
    }

    double sampleRate = 48000.0;
    MonoParams params;
    bool firstSet = true, monoActive = false;
    std::array<dsp::LR4Crossover, 2> lr4;
    std::array<dsp::LR8Crossover, 2> lr8;
    dsp::LinearSmoother widthSmooth, lowWidthSmooth;
    float corrCoef = 0.0001f, sumLR = 0.0f, sumLL = 0.0f, sumRR = 0.0f;
    std::array<float, (size_t) scopePoints> scopeL {}, scopeR {};
    int scopeWrite = 0;
};

} // namespace rommuser::lowmono
