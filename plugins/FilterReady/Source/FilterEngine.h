// Filter Ready DSP engine: two resonant multimode filters (series or parallel) with LFO and envelope
// modulation, input drive, Clean / Analog character and 2x oversampling.
//
// Pure C++ (no JUCE) so it is unit-tested headless in tests/.
//
// Each filter is a TPT state-variable filter (Zavalishin), which is exactly the bilinear-transform
// (RBJ) biquad response at the oversampled rate; 24 dB = two identical sections in cascade. The
// Analog character soft-saturates the integrator states (level-dependent, so quiet signals stay
// clean) and the input Drive adds tanh saturation before the filters. Cutoff is smoothed in the log
// domain and the coefficients are recomputed every sample, so fast sweeps have no zipper noise.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../../../shared/dsp/FractionalDelay.h"
#include "../../../shared/dsp/Oversampler.h"
#include "../../../shared/dsp/Smoothing.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

namespace rommuser::filterready
{

enum class FilterType { lowPass = 0, highPass, bandPass, notch };
enum class Slope { db12 = 0, db24 };
enum class Character { clean = 0, analog };
enum class Routing { series = 0, parallel };
enum class LfoShape { sine = 0, triangle, saw, square, random };

struct StageParams
{
    bool enabled = true;
    FilterType type = FilterType::lowPass;
    Slope slope = Slope::db24;
    float cutoffHz = 1200.0f;   // 20 .. 20000
    float resonance = 0.4f;     // 0 .. 1  (Q 0.5 .. 20, log)
    float lfoOctaves = 0.0f;    // -5 .. 5: cutoff modulation by the LFO
    float envOctaves = 0.0f;    // -5 .. 5: cutoff modulation by the envelope follower
};

struct FilterParams
{
    StageParams a { true, FilterType::lowPass, Slope::db24, 1200.0f, 0.4f, 0.0f, 0.0f };
    StageParams b { false, FilterType::highPass, Slope::db12, 150.0f, 0.2f, 0.0f, 0.0f };
    Routing routing = Routing::series;
    Character character = Character::clean;
    float driveDb = 0.0f;           // 0 .. 36
    bool lfoSync = false;
    float lfoRateHz = 0.5f;         // 0.02 .. 20
    double lfoBeats = 4.0;          // LFO cycle in beats when synced
    float bpm = 120.0f;             // fallback when the host gives none
    LfoShape lfoShape = LfoShape::sine;
    float envAttackMs = 10.0f;      // 1 .. 200
    float envReleaseMs = 200.0f;    // 10 .. 2000
    float envSensitivityDb = 12.0f; // 0 .. 36: gain into the follower (1.0 = full modulation)
    float mix = 1.0f;               // 0 .. 1
    float outputDb = 0.0f;          // -24 .. 12
};

struct Transport
{
    bool valid = false;
    bool playing = false;
    double ppqPosition = 0.0;
    double bpm = 120.0;
};

struct FilterMeters
{
    float cutoffA = 0.0f, cutoffB = 0.0f;   // effective cutoff (Hz) at the end of the block
    float lfo = 0.0f;                        // -1 .. 1
    float env = 0.0f;                        // 0 .. 1
    float outputPeak = 0.0f;
};

inline constexpr int maxChannels = 2;
inline constexpr int oversampling = 2;
inline constexpr float minCutoffHz = 20.0f;
inline constexpr float maxCutoffHz = 20000.0f;

inline double resonanceToQ (float r) { return 0.5 * std::pow (40.0, (double) std::clamp (r, 0.0f, 1.0f)); }

// TPT state-variable filter. Outputs the RBJ-equivalent low, high, band (0 dB peak) and notch.
struct SVF
{
    float ic1eq = 0.0f, ic2eq = 0.0f;

    void reset() { ic1eq = ic2eq = 0.0f; }

    inline float process (float v0, float g, float k, FilterType type, bool analog)
    {
        const float a1 = 1.0f / (1.0f + g * (g + k));
        const float a2 = g * a1;
        const float a3 = g * a2;
        const float v3 = v0 - ic2eq;
        const float v1 = a1 * ic1eq + a2 * v3;
        const float v2 = ic2eq + a2 * ic1eq + a3 * v3;
        ic1eq = 2.0f * v1 - ic1eq;
        ic2eq = 2.0f * v2 - ic2eq;
        if (analog)
        {
            // Soft saturation of the integrator states: the resonance compresses as it gets loud.
            ic1eq = saturate (ic1eq);
            ic2eq = saturate (ic2eq);
        }
        switch (type)
        {
            case FilterType::lowPass:  return v2;
            case FilterType::highPass: return v0 - k * v1 - v2;
            case FilterType::bandPass: return k * v1;
            case FilterType::notch:    return v0 - k * v1;
        }
        return v2;
    }

    static inline float saturate (float x)
    {
        constexpr float d = 0.6f;
        return std::tanh (x * d) / d;
    }
};

class FilterEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize, int numChannels)
    {
        sr = sampleRate;
        sr2 = sampleRate * oversampling;
        channels = std::clamp (numChannels, 1, maxChannels);
        maxBlock = std::max (1, maxBlockSize);
        for (int c = 0; c < maxChannels; ++c)
        {
            oversamplers[(size_t) c] = std::make_unique<dsp::Oversampler> (maxBlock);
            oversamplers[(size_t) c]->prepare (oversampling, maxBlock);
        }
        latency = oversamplers[0]->getLatencySamples();
        for (auto& d : dryDelay) d.setMaxDelay (latency + maxBlock + 1);
        wet.assign ((size_t) maxBlock, 0.0f);

        for (auto* s : { &cutLogA, &cutLogB, &resA, &resB, &driveSmooth, &mixSmooth, &outSmooth })
            s->prepare (sampleRate, 0.02);
        lfoSmoothCoeff = (float) (1.0 - std::exp (-1.0 / (0.002 * sr)));
        setParams (params, true);
        firstSet = true;
        reset();
    }

    void reset()
    {
        for (auto& o : oversamplers) if (o) o->reset();
        for (auto& d : dryDelay) d.reset();
        for (auto& ch : svf) for (auto& st : ch) for (auto& s : st) s.reset();
        phase = 0.0;
        lfoValue = lfoSmoothed = 0.0f;
        randomValue = 0.0f;
        env = 0.0f;
    }

    void setParams (const FilterParams& p, bool immediate = false)
    {
        immediate = immediate || firstSet;
        firstSet = false;

        auto set = [immediate] (dsp::LinearSmoother& s, float v) { immediate ? s.setImmediate (v) : s.setTarget (v); };
        set (cutLogA, std::log2 (std::clamp (p.a.cutoffHz, minCutoffHz, maxCutoffHz)));
        set (cutLogB, std::log2 (std::clamp (p.b.cutoffHz, minCutoffHz, maxCutoffHz)));
        set (resA, std::clamp (p.a.resonance, 0.0f, 1.0f));
        set (resB, std::clamp (p.b.resonance, 0.0f, 1.0f));
        set (driveSmooth, std::clamp (p.driveDb, 0.0f, 36.0f));
        set (mixSmooth, std::clamp (p.mix, 0.0f, 1.0f));
        set (outSmooth, dsp::dbToGain (p.outputDb));

        if (immediate || std::fabs (p.envAttackMs - params.envAttackMs) > 1e-6f || std::fabs (p.envReleaseMs - params.envReleaseMs) > 1e-6f)
        {
            envAttack = (float) (1.0 - std::exp (-1.0 / (std::max (0.1f, p.envAttackMs) * 0.001 * sr)));
            envRelease = (float) (1.0 - std::exp (-1.0 / (std::max (1.0f, p.envReleaseMs) * 0.001 * sr)));
        }
        if (p.a.enabled != params.a.enabled || p.a.type != params.a.type || p.a.slope != params.a.slope)
            for (auto& ch : svf) for (auto& s : ch[0]) s.reset();
        if (p.b.enabled != params.b.enabled || p.b.type != params.b.type || p.b.slope != params.b.slope)
            for (auto& ch : svf) for (auto& s : ch[1]) s.reset();
        params = p;
    }

    const FilterParams& getParams() const { return params; }
    int getLatencySamples() const { return latency; }
    double getLfoPhase() const { return phase; }

    FilterMeters process (float* const* io, int numChannels, int n, const Transport& transport = {})
    {
        FilterMeters m;
        const int nch = std::clamp (numChannels, 1, maxChannels);

        // LFO rate: tempo-locked when the host is playing, free otherwise.
        double phaseInc;
        if (params.lfoSync)
        {
            const double bpm = (transport.valid && transport.bpm > 20.0) ? transport.bpm : (double) params.bpm;
            const double beats = std::max (1.0 / 64.0, params.lfoBeats);
            phaseInc = bpm / 60.0 / beats / sr;
            if (transport.valid && transport.playing)
            {
                const double target = transport.ppqPosition / beats;
                phase = target - std::floor (target);
            }
        }
        else
        {
            phaseInc = std::clamp ((double) params.lfoRateHz, 0.01, 20.0) / sr;
        }
        const float envGain = dsp::dbToGain (std::clamp (params.envSensitivityDb, 0.0f, 36.0f));
        const bool analog = params.character == Character::analog;
        const bool aOn = params.a.enabled, bOn = params.b.enabled;
        const bool parallel = params.routing == Routing::parallel;
        const float nyq = (float) (sr2 * 0.45);

        // Upsample all channels first (the filters run at 2x).
        std::array<float*, maxChannels> hi {};
        for (int c = 0; c < nch; ++c)
        {
            for (int i = 0; i < n; ++i)
                dryDelay[(size_t) c].push (io[c][i]);
            hi[(size_t) c] = oversamplers[(size_t) c]->upsample (io[c], n);
        }

        float fcA = 0.0f, fcB = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            // Modulators (per base sample, shared by all channels).
            float level = 0.0f;
            for (int c = 0; c < nch; ++c)
                level = std::max (level, std::fabs (io[c][i]));
            level = std::min (1.0f, level * envGain);
            env += (level > env ? envAttack : envRelease) * (level - env);

            advanceLfo (phaseInc);
            lfoSmoothed += (lfoValue - lfoSmoothed) * lfoSmoothCoeff;

            const float cutA = cutLogA.next(), cutB = cutLogB.next();
            const float rA = resA.next(), rB = resB.next();
            const float drive = driveSmooth.next();
            fcA = std::clamp (std::exp2 (cutA + params.a.lfoOctaves * lfoSmoothed + params.a.envOctaves * env), minCutoffHz, nyq);
            fcB = std::clamp (std::exp2 (cutB + params.b.lfoOctaves * lfoSmoothed + params.b.envOctaves * env), minCutoffHz, nyq);
            const float gA = std::tan (3.14159265f * fcA / (float) sr2), kA = (float) (1.0 / resonanceToQ (rA));
            const float gB = std::tan (3.14159265f * fcB / (float) sr2), kB = (float) (1.0 / resonanceToQ (rB));
            const float driveGain = dsp::dbToGain (drive);
            const float driveMix = std::min (1.0f, drive / 6.0f);

            for (int c = 0; c < nch; ++c)
            {
                auto& stages = svf[(size_t) c];
                for (int k = 0; k < oversampling; ++k)
                {
                    float x = hi[(size_t) c][i * oversampling + k];
                    if (driveMix > 0.0f)
                        x += driveMix * (std::tanh (x * driveGain) / driveGain * std::sqrt (driveGain) - x);

                    auto runStage = [&] (int s, float in, float g, float kk, const StageParams& sp)
                    {
                        float y = stages[(size_t) s][0].process (in, g, kk, sp.type, analog);
                        if (sp.slope == Slope::db24)
                            y = stages[(size_t) s][1].process (y, g, kk, sp.type, analog);
                        return y;
                    };

                    float y;
                    if (parallel)
                    {
                        float sum = 0.0f;
                        int count = 0;
                        if (aOn) { sum += runStage (0, x, gA, kA, params.a); ++count; }
                        if (bOn) { sum += runStage (1, x, gB, kB, params.b); ++count; }
                        y = count > 0 ? sum : x;
                    }
                    else
                    {
                        y = x;
                        if (aOn) y = runStage (0, y, gA, kA, params.a);
                        if (bOn) y = runStage (1, y, gB, kB, params.b);
                    }
                    hi[(size_t) c][i * oversampling + k] = y;
                }
            }
        }

        for (int c = 0; c < nch; ++c)
        {
            oversamplers[(size_t) c]->downsample (hi[(size_t) c], wet.data(), n);
            // Rewind the smoothers' per-sample state is shared: run mix / output per sample on channel 0
            // and reuse the values for channel 1 via a small buffer.
            for (int i = 0; i < n; ++i)
            {
                if (c == 0)
                {
                    mixBuf[(size_t) i] = mixSmooth.next();
                    outBuf[(size_t) i] = outSmooth.next();
                }
                const float mix = mixBuf[(size_t) i], out = outBuf[(size_t) i];
                const float dry = dryDelay[(size_t) c].readInteger (latency + (n - 1 - i));
                const float w = wet[(size_t) i];
                float y = mix >= 1.0f ? w : mix <= 0.0f ? dry : dry + (w - dry) * mix;
                y *= out;
                io[c][i] = y;
                m.outputPeak = std::max (m.outputPeak, std::fabs (y));
            }
        }

        m.cutoffA = fcA;
        m.cutoffB = fcB;
        m.lfo = lfoSmoothed;
        m.env = env;
        return m;
    }

private:
    void advanceLfo (double inc)
    {
        phase += inc;
        if (phase >= 1.0)
        {
            phase -= std::floor (phase);
            randomValue = nextRandom();
        }
        const float ph = (float) phase;
        switch (params.lfoShape)
        {
            case LfoShape::sine:     lfoValue = std::sin (6.2831853f * ph); break;
            case LfoShape::triangle: lfoValue = 1.0f - 4.0f * std::fabs (ph - 0.5f); break;
            case LfoShape::saw:      lfoValue = 1.0f - 2.0f * ph; break;
            case LfoShape::square:   lfoValue = ph < 0.5f ? 1.0f : -1.0f; break;
            case LfoShape::random:   lfoValue = randomValue; break;
        }
    }

    float nextRandom()
    {
        rng = rng * 1664525u + 1013904223u;
        return (float) (rng >> 8) / 8388608.0f - 1.0f;   // -1 .. 1
    }

    double sr = 48000.0, sr2 = 96000.0;
    int channels = 2, maxBlock = 512, latency = 0;
    bool firstSet = true;
    FilterParams params;

    std::array<std::unique_ptr<dsp::Oversampler>, maxChannels> oversamplers;
    std::array<dsp::FractionalDelay, maxChannels> dryDelay;
    std::vector<float> wet;
    std::array<float, 8192> mixBuf {}, outBuf {};
    std::array<std::array<std::array<SVF, 2>, 2>, maxChannels> svf;   // [channel][stage A/B][cascade]

    dsp::LinearSmoother cutLogA, cutLogB, resA, resB, driveSmooth, mixSmooth, outSmooth;
    double phase = 0.0;
    float lfoValue = 0.0f, lfoSmoothed = 0.0f, lfoSmoothCoeff = 0.01f, randomValue = 0.0f;
    uint32_t rng = 12345u;
    float env = 0.0f, envAttack = 0.01f, envRelease = 0.001f;
};

} // namespace rommuser::filterready
