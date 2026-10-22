// Phase Ready: classic all-pass phaser with 4-12 stages, free or tempo-synced LFO,
// feedback, stereo spread and click-free stage switching.
//
// Pure C++ (no JUCE) so the DSP is tested headless.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "dsp/Smoothing.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace rommuser::phaseready
{

enum class LfoShape { sine = 0, triangle = 1, random = 2 };

struct PhaseParams
{
    int stages = 6;              // 4..12, even
    bool sync = false;
    double beats = 1.0;          // LFO cycle length in beats when synced
    float rateHz = 0.5f;         // free-running rate
    float bpm = 120.0f;
    float depth = 0.7f;          // 0..1 -> sweep of +-2.5 octaves around centre
    float feedback = 0.3f;       // -0.95..0.95
    float centerHz = 800.0f;     // 100..5000
    float spread = 0.5f;         // 0..1 -> 0..180 degrees between L and R LFOs
    LfoShape shape = LfoShape::sine;
    float mix = 0.5f;            // 0..1 (0.5 = classic notches)
    float outputDb = 0.0f;
};

struct Transport
{
    bool valid = false;
    bool playing = false;
    double ppqPosition = 0.0;
    double bpm = 120.0;
};

struct PhaseMeters
{
    float lfoL = 0.0f, lfoR = 0.0f;   // -1..1
    float outputPeak = 0.0f;
};

inline constexpr int minStages = 4, maxStages = 12;
inline constexpr float sweepOctaves = 2.5f;
inline constexpr float stageSpacingOctaves = 0.25f; // stages fan out around the centre

// First-order all-pass: y = a*x + x1 - a*y1, 90 degrees at its break frequency.
struct AllPass1
{
    float a = 0.0f, x1 = 0.0f, y1 = 0.0f;
    void reset() { x1 = y1 = 0.0f; }
    inline float process (float x)
    {
        const float y = a * x + x1 - a * y1;
        x1 = x;
        y1 = y;
        return y;
    }
};

inline float allPassCoefficient (float freqHz, double sampleRate)
{
    const float f = std::clamp (freqHz, 20.0f, (float) (sampleRate * 0.45));
    const float t = std::tan ((float) (3.14159265358979323846 * f / sampleRate));
    return (t - 1.0f) / (t + 1.0f);
}

// Smooth random: a new target each LFO cycle, cubic Hermite between them. Holds five values so
// the right channel can read up to half a cycle ahead of the left one (stereo spread).
class RandomLfo
{
public:
    void reset (uint32_t seed = 0x9e3779b9u)
    {
        state = seed;
        for (auto& v : values) v = nextRandom();
    }
    void advanceCycle() // called once per LFO cycle
    {
        for (size_t i = 0; i + 1 < values.size(); ++i) values[i] = values[i + 1];
        values.back() = nextRandom();
    }
    // t in 0..1 within the segment; segment 0 = values[1]..values[2], segment 1 = values[2]..values[3].
    float valueAt (float t, int segment) const
    {
        const size_t b = segment == 0 ? 1 : 2;
        const float y0 = values[b - 1], y1 = values[b], y2 = values[b + 1], y3 = values[b + 2];
        const float c0 = y1;
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return std::clamp (((c3 * t + c2) * t + c1) * t + c0, -1.0f, 1.0f);
    }

private:
    float nextRandom()
    {
        state = state * 1664525u + 1013904223u;
        return (float) (state >> 8) / 8388608.0f - 1.0f; // -1..1
    }
    uint32_t state = 1;
    std::array<float, 5> values {};
};

class PhaseEngine
{
public:
    static constexpr int maxChannels = 2;
    static constexpr float crossfadeSeconds = 0.02f;

    void prepare (double newSampleRate, int numChannels)
    {
        sampleRate = newSampleRate;
        channels = std::clamp (numChannels, 1, maxChannels);
        crossfadeStep = 1.0f / std::max (1.0f, (float) (sampleRate * crossfadeSeconds));
        depthSmooth.prepare (sampleRate, 0.02);
        feedbackSmooth.prepare (sampleRate, 0.02);
        centerSmooth.prepare (sampleRate, 0.05);
        spreadSmooth.prepare (sampleRate, 0.05);
        mixSmooth.prepare (sampleRate, 0.02);
        outSmooth.prepare (sampleRate, 0.02);
        setParams (params);
        firstSet = true; // parameters sent before any audio apply immediately
        reset();
    }

    void reset()
    {
        for (auto& ch : stages) for (auto& s : ch) s.reset();
        for (auto& f : fbState) f = 0.0f;
        random.reset();
        phase = 0.0;
        currentStages = targetStages = params.stages;
        fade = 1.0f;
        lastLfo = { 0.0f, 0.0f };
    }

    void setParams (const PhaseParams& p)
    {
        params = p;
        params.stages = std::clamp (p.stages, minStages, maxStages) & ~1;
        if (params.stages != targetStages)
        {
            if (firstSet) { currentStages = targetStages = params.stages; fade = 1.0f; }
            else { targetStages = params.stages; fade = 0.0f; }
        }
        auto set = [this] (dsp::LinearSmoother& s, float v) { firstSet ? s.setImmediate (v) : s.setTarget (v); };
        set (depthSmooth, std::clamp (p.depth, 0.0f, 1.0f));
        set (feedbackSmooth, std::clamp (p.feedback, -0.95f, 0.95f));
        set (centerSmooth, std::log2 (std::clamp (p.centerHz, 50.0f, 8000.0f)));
        set (spreadSmooth, std::clamp (p.spread, 0.0f, 1.0f));
        set (mixSmooth, std::clamp (p.mix, 0.0f, 1.0f));
        set (outSmooth, dsp::dbToGain (p.outputDb));
    }

    const PhaseParams& getParams() const { return params; }
    double getLfoPhase() const { return phase; }
    float getLastLfo (int ch) const { return lastLfo[(size_t) std::clamp (ch, 0, 1)]; }
    int getActiveStages() const { return targetStages; }

    // Current all-pass coefficients of a channel (for drawing the response).
    void getStageCoefficients (int ch, float* out, int& count) const
    {
        const auto& s = stages[(size_t) std::clamp (ch, 0, channels - 1)];
        count = targetStages;
        for (int k = 0; k < count; ++k) out[k] = s[(size_t) k].a;
    }

    PhaseMeters process (float* const* io, int numChannels, int n, const Transport& transport)
    {
        firstSet = false;
        const int nch = std::min (numChannels, channels);
        PhaseMeters m;

        // LFO phase: tempo-locked when the host is playing, free-running otherwise.
        double phaseInc;
        if (params.sync)
        {
            const double bpm = (transport.valid && transport.bpm > 20.0) ? transport.bpm : (double) params.bpm;
            const double beats = std::max (1.0 / 64.0, params.beats);
            phaseInc = bpm / 60.0 / beats / sampleRate;
            if (transport.valid && transport.playing)
            {
                const double target = transport.ppqPosition / beats;
                phase = target - std::floor (target);
            }
        }
        else
        {
            phaseInc = std::clamp ((double) params.rateHz, 0.005, 20.0) / sampleRate;
        }

        for (int i = 0; i < n; ++i)
        {
            const float depth = depthSmooth.next();
            const float fb = feedbackSmooth.next();
            const float centerLog = centerSmooth.next();
            const float spread = spreadSmooth.next();
            const float mix = mixSmooth.next();
            const float out = outSmooth.next();

            // Advance the stage crossfade (shared by both channels).
            if (fade < 1.0f)
            {
                fade += crossfadeStep;
                if (fade >= 1.0f) { fade = 1.0f; currentStages = targetStages; }
            }

            for (int ch = 0; ch < nch; ++ch)
            {
                const float lfo = lfoValue (ch, (float) phase, spread);
                lastLfo[(size_t) ch] = lfo;
                // Sweep in octaves around the (smoothed, log) centre.
                const float fLog = centerLog + lfo * depth * sweepOctaves;
                auto& st = stages[(size_t) ch];
                const int nStages = std::max (currentStages, targetStages);
                for (int k = 0; k < nStages; ++k)
                {
                    // Stages fan out: k-th stage sits (k - (N-1)/2) * spacing octaves from the sweep centre.
                    const float offset = ((float) k - (float) (targetStages - 1) * 0.5f) * stageSpacingOctaves;
                    st[(size_t) k].a = allPassCoefficient (std::exp2 (fLog + offset), sampleRate);
                }

                const float dry = io[ch][i];
                float x = dry + fb * fbState[(size_t) ch];
                float wetCurrent = 0.0f, wetTarget = 0.0f;
                for (int k = 0; k < nStages; ++k)
                {
                    x = st[(size_t) k].process (x);
                    if (k + 1 == currentStages) wetCurrent = x;
                    if (k + 1 == targetStages) wetTarget = x;
                }
                const float wet = fade < 1.0f ? wetCurrent + (wetTarget - wetCurrent) * fade : wetTarget;
                // Soft limit the feedback path so high feedback cannot run away.
                fbState[(size_t) ch] = wet > 2.0f || wet < -2.0f ? std::tanh (wet * 0.5f) * 2.0f : wet;

                const float y = (dry + (wet - dry) * mix) * out;
                io[ch][i] = y;
                m.outputPeak = std::max (m.outputPeak, std::fabs (y));
            }

            phase += phaseInc;
            if (phase >= 1.0)
            {
                phase -= std::floor (phase);
                random.advanceCycle();
            }
        }

        m.lfoL = lastLfo[0];
        m.lfoR = lastLfo[(size_t) std::min (1, nch - 1)];
        return m;
    }

private:
    float lfoValue (int ch, float ph, float spread)
    {
        float p = ph;
        if (ch == 1) p += spread * 0.5f; // up to 180 degrees apart
        const int segment = p >= 1.0f ? 1 : 0;
        p -= std::floor (p);
        switch (params.shape)
        {
            case LfoShape::triangle: return 1.0f - 4.0f * std::fabs (p - 0.5f);
            case LfoShape::random:   return random.valueAt (p, segment);
            case LfoShape::sine:
            default:                 return std::sin (6.283185307179586f * p);
        }
    }

    double sampleRate = 48000.0;
    int channels = 2;
    PhaseParams params;
    bool firstSet = true;

    std::array<std::array<AllPass1, (size_t) maxStages>, (size_t) maxChannels> stages {};
    std::array<float, (size_t) maxChannels> fbState {};
    RandomLfo random;
    std::array<float, (size_t) maxChannels> lastLfo {};

    double phase = 0.0;
    int currentStages = 6, targetStages = 6;
    float fade = 1.0f, crossfadeStep = 0.001f;

    dsp::LinearSmoother depthSmooth, feedbackSmooth, centerSmooth, spreadSmooth, mixSmooth, outSmooth;
};

} // namespace rommuser::phaseready
