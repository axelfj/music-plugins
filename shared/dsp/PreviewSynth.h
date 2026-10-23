// A small, clean polyphonic sound for the MIDI tools, so a chord or an arpeggio is audible the
// moment the plugin is loaded, before any routing to a real instrument. Additive (fundamental plus
// two soft harmonics), so there is no aliasing to worry about; ADSR and a one-pole low-pass.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace rommuser::dsp
{

class PreviewSynth
{
public:
    static constexpr int maxVoices = 16;

    void prepare (double sr)
    {
        sampleRate = sr;
        attackCoef = 1.0f - std::exp (-1.0f / (float) (sr * 0.004));
        releaseCoef = std::exp (-6.9078f / (float) (sr * 0.35));
        decayCoef = std::exp (-6.9078f / (float) (sr * 1.5));
        toneCoef = 1.0f - std::exp (-6.2831853f * 2500.0f / (float) sr);
        reset();
    }

    void reset()
    {
        for (auto& v : voices) v = {};
        lpL = lpR = 0.0f;
    }

    void setLevel (float gain) { level = std::clamp (gain, 0.0f, 2.0f); }

    void noteOn (int note, float velocity)
    {
        Voice* v = nullptr;
        for (auto& c : voices) if (c.note == note && c.active) { v = &c; break; }
        if (v == nullptr) for (auto& c : voices) if (! c.active) { v = &c; break; }
        if (v == nullptr)
        {
            v = &voices[0];
            for (auto& c : voices) if (c.env < v->env) v = &c;
        }
        v->active = true;
        v->releasing = false;
        v->note = note;
        v->inc = 440.0 * std::pow (2.0, (note - 69) / 12.0) / sampleRate;
        v->gain = 0.25f * (0.3f + 0.7f * std::clamp (velocity, 0.0f, 1.0f));
        if (v->env <= 0.0f) v->phase = 0.0;
        v->sustainTarget = 0.6f;
    }

    void noteOff (int note)
    {
        for (auto& v : voices) if (v.active && v.note == note) v.releasing = true;
    }

    void allNotesOff() { for (auto& v : voices) v.releasing = true; }

    bool isSounding() const
    {
        for (const auto& v : voices) if (v.active) return true;
        return false;
    }

    // Adds to outL / outR.
    void render (float* outL, float* outR, int n)
    {
        for (auto& v : voices)
        {
            if (! v.active) continue;
            for (int i = 0; i < n; ++i)
            {
                if (v.releasing)
                {
                    v.env *= releaseCoef;
                    if (v.env < 1e-4f) { v.active = false; v.env = 0.0f; break; }
                }
                else if (v.env < 0.999f && ! v.decaying)
                    v.env += (1.0f - v.env) * attackCoef;
                else
                {
                    v.decaying = true;
                    v.env = v.sustainTarget + (v.env - v.sustainTarget) * decayCoef;
                }
                const float ph = (float) v.phase * 6.2831853f;
                const float s = std::sin (ph) + 0.35f * std::sin (2.0f * ph) + 0.12f * std::sin (3.0f * ph);
                v.phase += v.inc;
                if (v.phase >= 1.0) v.phase -= 1.0;
                const float x = s * v.env * v.gain * level;
                outL[i] += x;
                outR[i] += x;
            }
        }
        for (int i = 0; i < n; ++i)
        {
            lpL += (outL[i] - lpL) * toneCoef;
            lpR += (outR[i] - lpR) * toneCoef;
            outL[i] = lpL;
            outR[i] = lpR;
        }
    }

private:
    struct Voice
    {
        bool active = false, releasing = false, decaying = false;
        int note = 60;
        double phase = 0.0, inc = 0.0;
        float env = 0.0f, gain = 0.0f, sustainTarget = 0.6f;
    };
    std::array<Voice, maxVoices> voices {};
    double sampleRate = 48000.0;
    float attackCoef = 0.01f, releaseCoef = 0.999f, decayCoef = 0.9999f, toneCoef = 0.3f;
    float lpL = 0.0f, lpR = 0.0f, level = 1.0f;
};

} // namespace rommuser::dsp
