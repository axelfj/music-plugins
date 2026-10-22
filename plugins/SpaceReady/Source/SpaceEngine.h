// Space Ready: algorithmic reverb on an 8-line feedback delay network (Hadamard matrix) with
// per-line decay gains that hit the requested RT60, damping, slow modulation against metallic
// ringing, input diffusion, pre-delay, width, output EQ, ducking and freeze.
//
// Pure C++ (no JUCE) so the DSP is tested headless.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "dsp/Biquad.h"
#include "dsp/FractionalDelay.h"
#include "dsp/Smoothing.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace rommuser::spaceready
{

enum class Mode { room = 0, plate = 1, hall = 2, ambience = 3 };

struct SpaceParams
{
    Mode mode = Mode::hall;
    float size = 1.0f;           // 0.5..2 scales the delay lengths
    float decaySeconds = 2.0f;   // RT60, 0.1..30
    float preDelayMs = 10.0f;    // 0..200
    float dampingHz = 6000.0f;   // loop low-pass; >= 19999 = off
    float diffusion = 0.6f;      // 0..1 input diffusion
    float modulation = 0.5f;     // 0..1 delay-line modulation depth
    float width = 1.0f;          // 0..1 (0 = mono wet)
    float lowCutHz = 0.0f;       // output high-pass; 0 = off
    float highCutHz = 0.0f;      // output low-pass; 0 = off
    float duck = 0.0f;           // 0..1 wet ducking by the dry signal
    float mix = 0.3f;            // 0..1
    float outputDb = 0.0f;
    bool freeze = false;
};

struct SpaceMeters
{
    float wetRms = 0.0f;
    float duckGain = 1.0f;
    float outputPeak = 0.0f;
};

inline constexpr int numLines = 8;
inline constexpr float maxPreDelayMs = 200.0f;
inline constexpr float maxLineMs = 600.0f; // 283 ms * size 2 + modulation

struct ModeProfile
{
    float lengthsMs[numLines];
    float modDepthMs;   // at modulation = 1
    float inputGain;
};

inline ModeProfile profileFor (Mode m)
{
    switch (m)
    {
        case Mode::room:     return { { 23.0f, 29.0f, 37.0f, 41.0f, 47.0f, 53.0f, 59.0f, 67.0f }, 0.25f, 1.0f };
        case Mode::plate:    return { { 11.0f, 13.0f, 17.0f, 19.0f, 23.0f, 29.0f, 31.0f, 37.0f }, 0.5f, 1.0f };
        case Mode::ambience: return { { 89.0f, 113.0f, 139.0f, 167.0f, 193.0f, 227.0f, 251.0f, 283.0f }, 1.5f, 1.0f };
        case Mode::hall:
        default:             return { { 31.0f, 43.0f, 53.0f, 67.0f, 79.0f, 89.0f, 97.0f, 113.0f }, 0.4f, 1.0f };
    }
}

// Schroeder all-pass on a fractional delay line (integer length), for input diffusion.
struct DiffuserStage
{
    dsp::FractionalDelay line;
    int length = 1;
    void prepare (int len) { length = std::max (1, len); line.setMaxDelay (length + 2); }
    void reset() { line.reset(); }
    inline float process (float x, float g)
    {
        const float d = line.readInteger (length);
        const float v = x - g * d;
        line.push (v);
        return d + g * v;
    }
};

class SpaceEngine
{
public:
    static constexpr int maxChannels = 2;

    void prepare (double newSampleRate, int numChannels)
    {
        sampleRate = newSampleRate;
        channels = std::clamp (numChannels, 1, maxChannels);
        for (auto& l : lines) l.setMaxDelay ((int) std::ceil (sampleRate * maxLineMs * 0.001) + 8);
        for (auto& p : preDelay) p.setMaxDelay ((int) std::ceil (sampleRate * maxPreDelayMs * 0.001) + 8);
        static constexpr float diffMs[4] = { 4.7f, 3.6f, 12.7f, 9.3f };
        for (int ch = 0; ch < maxChannels; ++ch)
            for (int k = 0; k < 4; ++k)
                diffusers[(size_t) ch][(size_t) k].prepare ((int) std::lround (sampleRate * diffMs[k] * 0.001 * (ch == 1 ? 1.07 : 1.0)));
        sizeSmooth.prepare (sampleRate, 0.1);
        mixSmooth.prepare (sampleRate, 0.02);
        outSmooth.prepare (sampleRate, 0.02);
        widthSmooth.prepare (sampleRate, 0.05);
        diffusionSmooth.prepare (sampleRate, 0.05);
        modSmooth.prepare (sampleRate, 0.1);
        inputGainSmooth.prepare (sampleRate, 0.02);
        duckAttack = 1.0f - std::exp (-1.0f / (float) (sampleRate * 0.005));
        duckRelease = 1.0f - std::exp (-1.0f / (float) (sampleRate * 0.25));
        filtersReady = false;
        setParams (params);
        firstSet = true;
        reset();
    }

    void reset()
    {
        for (auto& l : lines) l.reset();
        for (auto& p : preDelay) p.reset();
        for (auto& ch : diffusers) for (auto& d : ch) d.reset();
        for (auto& s : dampState) s = 0.0f;
        for (auto& f : lowCut) f.reset();
        for (auto& f : highCut) f.reset();
        for (auto& p : lfoPhase) p = 0.0;
        for (int i = 0; i < numLines; ++i) lfoPhase[(size_t) i] = (double) i / numLines;
        duckEnv = 0.0f;
    }

    void setParams (const SpaceParams& p)
    {
        const bool modeChanged = p.mode != params.mode || ! filtersReady;
        params = p;
        profile = profileFor (p.mode);

        auto set = [this] (dsp::LinearSmoother& s, float v) { firstSet ? s.setImmediate (v) : s.setTarget (v); };
        set (sizeSmooth, std::clamp (p.size, 0.5f, 2.0f));
        set (mixSmooth, std::clamp (p.mix, 0.0f, 1.0f));
        set (outSmooth, dsp::dbToGain (p.outputDb));
        set (widthSmooth, std::clamp (p.width, 0.0f, 1.0f));
        set (diffusionSmooth, std::clamp (p.diffusion, 0.0f, 1.0f) * 0.7f);
        set (modSmooth, std::clamp (p.modulation, 0.0f, 1.0f));
        set (inputGainSmooth, p.freeze ? 0.0f : profile.inputGain);

        // Damping one-pole coefficient (off above 19999 Hz or while frozen).
        const bool dampOn = p.dampingHz < 19999.0f && ! p.freeze;
        dampCoef = dampOn ? 1.0f - std::exp (-2.0f * 3.14159265f * std::clamp (p.dampingHz, 500.0f, 20000.0f) / (float) sampleRate) : 1.0f;

        const float lc = p.lowCutHz, hc = p.highCutHz;
        if (modeChanged || std::fabs (lc - lastLowCut) > 1e-6f || std::fabs (hc - lastHighCut) > 1e-6f || ! filtersReady)
        {
            useLowCut = lc > 20.5f;
            useHighCut = hc > 0.0f && hc < 19999.0f;
            if (useLowCut)  { const auto c = dsp::biquad::highPass (lc, 0.7071, sampleRate); for (auto& f : lowCut) f.setCoeffs (c); }
            if (useHighCut) { const auto c = dsp::biquad::lowPass (hc, 0.7071, sampleRate); for (auto& f : highCut) f.setCoeffs (c); }
            lastLowCut = lc; lastHighCut = hc;
            filtersReady = true;
        }
        preDelaySamples = (int) std::lround (std::clamp (p.preDelayMs, 0.0f, maxPreDelayMs) * 0.001 * sampleRate);
        decaySeconds = std::clamp (p.decaySeconds, 0.05f, 60.0f);
        (void) modeChanged;
    }

    const SpaceParams& getParams() const { return params; }

    // Per-line feedback gain for the current RT60 (public for tests / display).
    float lineGain (float lengthSamples) const
    {
        if (params.freeze) return 1.0f;
        return std::pow (10.0f, -3.0f * lengthSamples / (decaySeconds * (float) sampleRate));
    }

    SpaceMeters process (float* const* io, int numChannels, int n)
    {
        firstSet = false;
        const int nch = std::min (numChannels, channels);
        SpaceMeters m;
        double wetEnergy = 0.0;
        const float duckRef = 1.0f / 0.0631f; // full ducking at -24 dBFS

        // LFO rates per line (Hz), slow and mutually different.
        static constexpr float lfoRates[numLines] = { 0.11f, 0.17f, 0.23f, 0.29f, 0.13f, 0.19f, 0.31f, 0.37f };

        for (int i = 0; i < n; ++i)
        {
            const float size = sizeSmooth.next();
            const float mix = mixSmooth.next();
            const float out = outSmooth.next();
            const float width = widthSmooth.next();
            const float diffG = diffusionSmooth.next();
            const float modDepth = modSmooth.next() * profile.modDepthMs * 0.001f * (float) sampleRate;
            const float inGain = inputGainSmooth.next();

            // Dry in, pre-delay, diffusion.
            float dryL = io[0][i];
            float dryR = nch > 1 ? io[1][i] : dryL;
            float inL = dryL, inR = dryR;
            if (preDelaySamples > 0)
            {
                preDelay[0].push (inL); preDelay[1].push (inR);
                inL = preDelay[0].readInteger (preDelaySamples);
                inR = preDelay[1].readInteger (preDelaySamples);
            }
            if (diffG > 0.0005f)
            {
                for (auto& d : diffusers[0]) inL = d.process (inL, diffG);
                for (auto& d : diffusers[1]) inR = d.process (inR, diffG);
            }
            inL *= inGain * 0.5f;
            inR *= inGain * 0.5f;

            // Read the eight lines (modulated), damp and apply the decay gain.
            float y[numLines];
            for (int k = 0; k < numLines; ++k)
            {
                const float lengthSamples = profile.lengthsMs[k] * size * 0.001f * (float) sampleRate;
                auto& ph = lfoPhase[(size_t) k];
                ph += lfoRates[k] / sampleRate;
                if (ph >= 1.0) ph -= 1.0;
                const float mod = modDepth * std::sin (6.283185307179586f * (float) ph);
                // Frozen: read whole samples so the interpolation cannot slowly dull the held tail.
                const float v = params.freeze ? lines[(size_t) k].readInteger ((int) std::lround (lengthSamples))
                                              : lines[(size_t) k].read (std::max (2.0f, lengthSamples + mod));
                auto& s = dampState[(size_t) k];
                s += dampCoef * (v - s);
                y[k] = s * lineGain (lengthSamples);
            }

            // Hadamard (orthogonal) feedback mixing, 1/sqrt(8).
            float h[numLines];
            hadamard8 (y, h);

            // Even lines get the left input, odd lines the right one.
            for (int k = 0; k < numLines; ++k)
                lines[(size_t) k].push (h[k] + ((k & 1) == 0 ? inL : inR));

            // Decorrelated stereo taps.
            float wetL = (y[0] - y[2] + y[4] - y[6]) * 0.7f;
            float wetR = (y[1] - y[3] + y[5] - y[7]) * 0.7f;

            // Width (mid/side), output EQ.
            const float mid = 0.5f * (wetL + wetR), side = 0.5f * (wetL - wetR) * width;
            wetL = mid + side; wetR = mid - side;
            if (useLowCut)  { wetL = lowCut[0].process (wetL);  wetR = lowCut[1].process (wetR); }
            if (useHighCut) { wetL = highCut[0].process (wetL); wetR = highCut[1].process (wetR); }

            // Ducking by the dry signal.
            const float level = std::max (std::fabs (dryL), std::fabs (dryR));
            duckEnv += (level > duckEnv ? duckAttack : duckRelease) * (level - duckEnv);
            const float duckGain = 1.0f - params.duck * std::min (1.0f, duckEnv * duckRef);
            wetL *= duckGain; wetR *= duckGain;

            wetEnergy += 0.5 * (wetL * wetL + wetR * wetR);
            lastDuckGain = duckGain;

            const float yL = (mix >= 1.0f ? wetL : (mix <= 0.0f ? dryL : dryL + (wetL - dryL) * mix)) * out;
            const float yR = (mix >= 1.0f ? wetR : (mix <= 0.0f ? dryR : dryR + (wetR - dryR) * mix)) * out;
            io[0][i] = yL;
            if (nch > 1) io[1][i] = yR;
            m.outputPeak = std::max ({ m.outputPeak, std::fabs (yL), std::fabs (yR) });
        }

        m.wetRms = n > 0 ? (float) std::sqrt (wetEnergy / n) : 0.0f;
        m.duckGain = lastDuckGain;
        return m;
    }

private:
    static inline void hadamard8 (const float* x, float* out)
    {
        // Three butterfly stages, then scale by 1/sqrt(8).
        float a[numLines];
        for (int k = 0; k < 4; ++k) { a[k] = x[k] + x[k + 4]; a[k + 4] = x[k] - x[k + 4]; }
        float b[numLines];
        for (int g = 0; g < 8; g += 4) for (int k = 0; k < 2; ++k) { b[g + k] = a[g + k] + a[g + k + 2]; b[g + k + 2] = a[g + k] - a[g + k + 2]; }
        for (int g = 0; g < 8; g += 2) { out[g] = (b[g] + b[g + 1]) * 0.35355339f; out[g + 1] = (b[g] - b[g + 1]) * 0.35355339f; }
    }

    double sampleRate = 48000.0;
    int channels = 2;
    SpaceParams params;
    ModeProfile profile = profileFor (Mode::hall);
    bool firstSet = true, filtersReady = false, useLowCut = false, useHighCut = false;
    float lastLowCut = -1.0f, lastHighCut = -1.0f;

    std::array<dsp::FractionalDelay, (size_t) numLines> lines;
    std::array<dsp::FractionalDelay, (size_t) maxChannels> preDelay;
    std::array<std::array<DiffuserStage, 4>, (size_t) maxChannels> diffusers;
    std::array<float, (size_t) numLines> dampState {};
    std::array<double, (size_t) numLines> lfoPhase {};
    std::array<dsp::Biquad, (size_t) maxChannels> lowCut, highCut;

    float dampCoef = 1.0f, decaySeconds = 2.0f;
    int preDelaySamples = 0;
    float duckAttack = 0.01f, duckRelease = 0.001f, duckEnv = 0.0f, lastDuckGain = 1.0f;

    dsp::LinearSmoother sizeSmooth, mixSmooth, outSmooth, widthSmooth, diffusionSmooth, modSmooth, inputGainSmooth;
};

} // namespace rommuser::spaceready
