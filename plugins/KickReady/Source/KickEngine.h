// Kick Ready DSP engine: a monophonic kick drum synthesiser.
//
// Pure C++ (no JUCE) so it is unit-tested headless in tests/.
//
// Body: a sine oscillator that always starts at phase 0 (no click at note start), swept by a pitch
// envelope (start offset in semitones, decay time, curve) and shaped by an amplitude envelope
// (attack, hold, decay, curve). Click: a short burst of filtered noise or a sine tick with its own
// decay. Then drive (tanh, peak-normalised), a soft clipper, a low shelf ("Sub"), a high cut and the
// output gain. Retriggering while a kick is still sounding fades the old one out over 2 ms and
// starts the new one at phase 0, so there is never a discontinuity; Glide slides the base pitch from
// the previous note.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../../../shared/dsp/Biquad.h"
#include "../../ClipReady/Source/ClipEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace rommuser::kickready
{

enum class ClickMode { noise = 0, tick };

struct KickParams
{
    // Tuning
    bool followMidi = true;         // false: every note plays baseNote
    int baseNote = 36;              // C1
    float tuneSemitones = 0.0f;     // -12 .. 12 (fine tune applied to the note)
    float glideMs = 0.0f;           // 0 .. 500
    float velocitySens = 0.5f;      // 0 .. 1: how much velocity changes the level

    // Pitch envelope (semitones above the note at the start of the kick)
    float pitchStartSemis = 36.0f;  // 0 .. 72
    float pitchDecayMs = 60.0f;     // 5 .. 1000: time to fall to 1 % of the start offset
    float pitchCurve = 0.5f;        // 0 .. 1: 0 = drops fast then lingers, 1 = holds then drops

    // Amplitude envelope
    float ampAttackMs = 0.1f;       // 0 .. 20
    float ampHoldMs = 0.0f;         // 0 .. 300
    float ampDecayMs = 400.0f;      // 20 .. 3000: time to -60 dB
    float ampCurve = 0.7f;          // 0 .. 1: 0 = linear decay, 1 = exponential

    // Click
    ClickMode clickMode = ClickMode::noise;
    float clickLevel = 0.3f;        // 0 .. 1
    float clickDecayMs = 6.0f;      // 1 .. 60
    float clickToneHz = 2500.0f;    // 200 .. 12000: high-pass on the noise / tick frequency

    // Dirt
    float driveDb = 0.0f;           // 0 .. 36
    float clip = 0.0f;              // 0 .. 1: 0 = off, 1 = hard clip at the ceiling
    float subDb = 0.0f;             // -12 .. 12 low shelf at 60 Hz
    float highCutHz = 20000.0f;     // 200 .. 20000
    float outputDb = 0.0f;          // -24 .. 12
};

inline constexpr float clipCeiling = 0.98f;   // ~ -0.18 dBFS

inline double midiToHz (double note) { return 440.0 * std::pow (2.0, (note - 69.0) / 12.0); }

// Decay envelopes used by the voice. u = t / decayTime, both return 1 at u = 0.
// Pitch: reaches 1 % at u = 1; the curve bends the time axis.
inline float pitchDecay (float u, float curve)
{
    const float c = 0.5f + 1.5f * std::clamp (curve, 0.0f, 1.0f);   // 0.5 .. 2
    return std::exp (-4.60517f * std::pow (std::max (u, 0.0f), c));
}
// Amplitude: -60 dB at u = 1 for the exponential shape, 0 at u = 1 for the linear shape.
inline float ampDecay (float u, float curve)
{
    if (u >= 1.0f) return 0.0f;
    const float lin = 1.0f - u;
    const float expo = (std::exp (-6.9078f * u) - 0.001f) / 0.999f;   // 1 at 0, exactly 0 at 1
    return std::max (0.0f, lin + (expo - lin) * curve);
}

class KickEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize)
    {
        sr = sampleRate;
        maxBlock = std::max (1, maxBlockSize);
        fadeSamples = std::max (1, (int) std::lround (0.002 * sr));
        setParams (params, true);
        reset();
    }

    void reset()
    {
        for (auto& v : voices) v = Voice {};
        current = 0;
        lastBaseHz = 0.0;
        sub.reset();
        highCut.reset();
    }

    void setParams (const KickParams& p, bool immediate = false)
    {
        (void) immediate;
        params = p;
        sub.setCoeffs (dsp::biquad::lowShelf (60.0, 0.707, std::clamp (p.subDb, -12.0f, 12.0f), sr));
        const double hc = std::clamp ((double) p.highCutHz, 200.0, std::min (20000.0, sr * 0.45));
        highCut.setCoeffs (hc >= 19999.0 ? dsp::BiquadCoeffs::identity() : dsp::biquad::lowPass (hc, 0.707, sr));
        clickCoeffs = dsp::biquad::highPass (std::clamp ((double) p.clickToneHz, 200.0, std::min (12000.0, sr * 0.45)), 0.707, sr);
        outGain = std::pow (10.0f, std::clamp (p.outputDb, -24.0f, 12.0f) / 20.0f);
        driveGain = std::pow (10.0f, std::clamp (p.driveDb, 0.0f, 36.0f) / 20.0f);
    }

    const KickParams& getParams() const { return params; }

    // Starts a kick. velocity 0 .. 1.
    void noteOn (int midiNote, float velocity)
    {
        Voice& old = voices[(size_t) current];
        const bool wasActive = old.active && ! old.fading;
        if (old.active)
        {
            old.fading = true;
            old.fadeRemaining = fadeSamples;
        }
        current ^= 1;
        Voice& v = voices[(size_t) current];
        v = Voice {};
        v.active = true;
        const double note = (params.followMidi ? (double) midiNote : (double) params.baseNote) + (double) params.tuneSemitones;
        v.targetBaseHz = midiToHz (note);
        v.baseHz = (wasActive && params.glideMs > 0.0f && lastBaseHz > 0.0) ? lastBaseHz : v.targetBaseHz;
        v.glideCoeff = params.glideMs > 0.0f ? (float) (1.0 - std::exp (-1.0 / (params.glideMs * 0.001 * sr))) : 1.0f;
        v.rng = 0x9E3779B9u ^ (uint32_t) midiNote;
        v.tickPhase = 0.0;
        v.clickFilter.reset();
        const float vel = std::clamp (velocity, 0.0f, 1.0f);
        v.gain = 1.0f + params.velocitySens * (vel - 1.0f);   // 1 at full velocity
    }

    void noteOff (int) {}   // kicks are one-shots

    bool isSounding() const
    {
        for (const auto& v : voices) if (v.active) return true;
        return false;
    }

    // Renders n mono samples into out (overwrites).
    void render (float* out, int n)
    {
        std::fill (out, out + n, 0.0f);
        for (auto& v : voices)
            if (v.active)
                renderVoice (v, out, n);

        for (int i = 0; i < n; ++i)
        {
            float x = out[i];
            if (params.driveDb > 0.0f)
                x = std::tanh (x * driveGain) / std::tanh (driveGain);
            if (params.clip > 0.0f)
            {
                const float softness = 1.0f - std::clamp (params.clip, 0.0f, 1.0f) * 0.9f;   // 1 .. 0.1
                x = clipready::clipCurve (x / clipCeiling, softness) * clipCeiling;
            }
            x = sub.process (x);
            x = highCut.process (x);
            out[i] = x * outGain;
        }
    }

    // Offline render of one kick (for the display and WAV export). Stops when the kick has decayed
    // or after maxSeconds.
    std::vector<float> renderOneShot (int midiNote, float velocity, double maxSeconds = 4.0)
    {
        KickEngine e;
        e.prepare (sr, 512);
        e.setParams (params, true);
        e.noteOn (midiNote, velocity);
        std::vector<float> out;
        std::vector<float> block (512);
        const int maxSamples = (int) (maxSeconds * sr);
        while ((int) out.size() < maxSamples && e.isSounding())
        {
            e.render (block.data(), 512);
            out.insert (out.end(), block.begin(), block.end());
        }
        // Trim the trailing silence to the last sample above -90 dBFS (keep 10 ms after it).
        int last = (int) out.size() - 1;
        while (last > 0 && std::fabs (out[(size_t) last]) < 3.16e-5f) --last;
        out.resize ((size_t) std::min ((int) out.size(), last + 1 + (int) (0.01 * sr)));
        return out;
    }

    // Current pitch envelope in semitones at time t (seconds), for the display.
    float pitchOffsetAt (float t) const
    {
        return params.pitchStartSemis * pitchDecay (t / (std::max (5.0f, params.pitchDecayMs) * 0.001f), params.pitchCurve);
    }

private:
    struct Voice
    {
        bool active = false, fading = false;
        int fadeRemaining = 0;
        int t = 0;                    // samples since note on
        double phase = 0.0;           // 0 .. 1
        double baseHz = 50.0, targetBaseHz = 50.0;
        float glideCoeff = 1.0f;
        float gain = 1.0f;
        uint32_t rng = 1u;
        double tickPhase = 0.0;
        dsp::Biquad clickFilter;
    };

    void renderVoice (Voice& v, float* out, int n)
    {
        const float pitchT = std::max (5.0f, params.pitchDecayMs) * 0.001f * (float) sr;
        const float attackS = std::max (0.0f, params.ampAttackMs) * 0.001f * (float) sr;
        const float holdS = std::max (0.0f, params.ampHoldMs) * 0.001f * (float) sr;
        const float decayS = std::max (1.0f, params.ampDecayMs) * 0.001f * (float) sr;
        const float clickS = std::max (0.5f, params.clickDecayMs) * 0.001f * (float) sr;
        const bool click = params.clickLevel > 0.0f;
        v.clickFilter.setCoeffs (clickCoeffs);
        const double tickInc = std::clamp ((double) params.clickToneHz, 200.0, sr * 0.45) / sr;

        for (int i = 0; i < n; ++i)
        {
            // Base pitch glide (log domain) and pitch envelope.
            if (std::fabs (v.baseHz - v.targetBaseHz) > 1e-6)
                v.baseHz = std::exp (std::log (v.baseHz) + (std::log (v.targetBaseHz) - std::log (v.baseHz)) * v.glideCoeff);
            const float semis = params.pitchStartSemis * pitchDecay ((float) v.t / pitchT, params.pitchCurve);
            const double hz = std::min (v.baseHz * std::pow (2.0, semis / 12.0), sr * 0.45);

            // Amplitude envelope.
            float amp;
            const float tf = (float) v.t;
            if (tf < attackS)                 amp = tf / attackS;
            else if (tf < attackS + holdS)    amp = 1.0f;
            else                              amp = ampDecay ((tf - attackS - holdS) / decayS, params.ampCurve);

            float y = (float) std::sin (6.283185307179586 * v.phase) * amp;
            v.phase += hz / sr;
            if (v.phase >= 1.0) v.phase -= 1.0;

            if (click)
            {
                const float cEnv = std::exp (-6.9078f * (float) v.t / clickS);
                if (cEnv > 1e-4f)
                {
                    float c;
                    if (params.clickMode == ClickMode::noise)
                    {
                        v.rng = v.rng * 1664525u + 1013904223u;
                        const float white = (float) (v.rng >> 8) / 8388608.0f - 1.0f;
                        c = v.clickFilter.process (white) * 0.7f;
                    }
                    else
                    {
                        c = (float) std::sin (6.283185307179586 * v.tickPhase);
                        v.tickPhase += tickInc;
                        if (v.tickPhase >= 1.0) v.tickPhase -= 1.0;
                    }
                    y += c * cEnv * params.clickLevel;
                }
            }

            y *= v.gain;
            if (v.fading)
            {
                y *= (float) v.fadeRemaining / (float) fadeSamples;
                if (--v.fadeRemaining <= 0) { v.active = false; out[i] += y; break; }
            }
            out[i] += y;
            ++v.t;

            // Voice ends when the amplitude envelope has finished and the click is gone.
            if (! v.fading && tf >= attackS + holdS + decayS && v.t > (int) (clickS * 1.5f))
            {
                v.active = false;
                break;
            }
            if (! v.fading && i == n - 1)
                lastBaseHz = v.baseHz;
        }
    }

    double sr = 48000.0;
    int maxBlock = 512, fadeSamples = 96;
    KickParams params;
    std::array<Voice, 2> voices;
    int current = 0;
    double lastBaseHz = 0.0;
    dsp::Biquad sub, highCut;
    dsp::BiquadCoeffs clickCoeffs;
    float outGain = 1.0f, driveGain = 1.0f;
};

} // namespace rommuser::kickready
