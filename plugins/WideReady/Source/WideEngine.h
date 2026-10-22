// Wide Ready: chorus (1-3 voices, ensemble), flanger (feedback, through-zero, polarity) and
// micro-pitch (two detuned voices, L up / R down) on one modulated fractional delay core.
//
// Pure C++ (no JUCE) so the DSP is tested headless.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "dsp/FractionalDelay.h"
#include "dsp/Smoothing.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace rommuser::wideready
{

enum class Mode { chorus = 0, flanger = 1, microPitch = 2 };

struct WideParams
{
    Mode mode = Mode::chorus;
    int voices = 2;              // chorus: 1..3
    bool ensemble = false;       // chorus: each voice runs at its own rate
    float rateHz = 0.4f;         // chorus / flanger LFO rate
    float depth = 0.5f;          // 0..1
    float delayMs = 15.0f;       // chorus base delay (5..40) / flanger manual delay (0.1..10)
    float feedback = 0.0f;       // flanger: -0.95..0.95
    bool throughZero = false;    // flanger: the dry signal is delayed by `delayMs` so the sweep crosses it
    bool invert = false;         // flanger: wet polarity
    float spread = 0.5f;         // 0..1 LFO offset between L and R (up to 180 degrees)
    float cents = 12.0f;         // micro-pitch: detune, L up and R down
    float mix = 0.5f;            // 0..1
    float outputDb = 0.0f;
};

struct WideMeters
{
    float voiceDelayMs[2][3] {};  // current delay of each voice per channel (0 when unused)
    int activeVoices = 0;
    float outputPeak = 0.0f;
};

inline constexpr int maxVoices = 3;
inline constexpr float maxDelayMs = 80.0f;
inline constexpr float microPitchWindowMs = 24.0f;

class WideEngine
{
public:
    static constexpr int maxChannels = 2;

    void prepare (double newSampleRate, int numChannels)
    {
        sampleRate = newSampleRate;
        channels = std::clamp (numChannels, 1, maxChannels);
        const int maxDelay = (int) std::ceil (sampleRate * maxDelayMs * 0.001) + 8;
        for (auto& d : lines) d.setMaxDelay (maxDelay);
        for (auto& d : dryLines) d.setMaxDelay (maxDelay);
        windowSamples = (float) (sampleRate * microPitchWindowMs * 0.001);
        fadeStep = 1.0f / std::max (1.0f, (float) (sampleRate * 0.01)); // 10 ms out + 10 ms in
        depthSmooth.prepare (sampleRate, 0.05);
        delaySmooth.prepare (sampleRate, 0.05);
        feedbackSmooth.prepare (sampleRate, 0.02);
        spreadSmooth.prepare (sampleRate, 0.05);
        centsSmooth.prepare (sampleRate, 0.05);
        mixSmooth.prepare (sampleRate, 0.02);
        outSmooth.prepare (sampleRate, 0.02);
        setParams (params);
        firstSet = true; // parameters sent before any audio apply immediately
        reset();
    }

    void reset()
    {
        for (auto& d : lines) d.reset();
        for (auto& d : dryLines) d.reset();
        for (auto& f : lastWet) f = 0.0f;
        for (auto& p : phases) p = 0.0;
        for (auto& p : pitchPhase) p = 0.0;
        structureFade = 1.0f;
        fadingOut = false;
        active = pending = params;
    }

    void setParams (const WideParams& p)
    {
        params = p;
        params.voices = std::clamp (p.voices, 1, maxVoices);
        const bool structureChanged = params.mode != pending.mode || params.voices != pending.voices
                                   || params.ensemble != pending.ensemble || params.throughZero != pending.throughZero
                                   || params.invert != pending.invert;
        pending = params;

        auto set = [this] (dsp::LinearSmoother& s, float v) { firstSet ? s.setImmediate (v) : s.setTarget (v); };
        set (spreadSmooth, std::clamp (p.spread, 0.0f, 1.0f));
        set (mixSmooth, std::clamp (p.mix, 0.0f, 1.0f));
        set (outSmooth, dsp::dbToGain (p.outputDb));

        if (firstSet)
        {
            active = params;
            applyStructureValues (params, true);
        }
        else if (structureChanged || fadingOut)
        {
            // Fade the wet path out, swap the structure (and jump the time/feedback values while it is
            // silent, so a mode change does not chirp through a delay ramp), fade back in.
            fadingOut = true;
        }
        else
        {
            active = params;
            applyStructureValues (params, false);
        }
    }

    const WideParams& getParams() const { return params; }
    int getLatencySamples() const { return 0; }

    WideMeters process (float* const* io, int numChannels, int n)
    {
        firstSet = false;
        const int nch = std::min (numChannels, channels);
        WideMeters m;

        static constexpr float ensembleRatios[maxVoices] = { 1.0f, 0.73f, 1.27f };
        static constexpr float chorusBaseRatios[maxVoices] = { 1.0f, 1.35f, 0.75f };

        for (int i = 0; i < n; ++i)
        {
            const float depth = depthSmooth.next();
            const float delayMs = delaySmooth.next();
            const float fb = feedbackSmooth.next();
            const float spread = spreadSmooth.next();
            const float cents = centsSmooth.next();
            const float mix = mixSmooth.next();
            const float out = outSmooth.next();

            // Structure crossfade: wet goes to silence, the new structure takes over, wet comes back.
            if (fadingOut)
            {
                structureFade -= fadeStep;
                if (structureFade <= 0.0f)
                {
                    structureFade = 0.0f;
                    fadingOut = false;
                    active = pending;
                    applyStructureValues (pending, true);
                    for (auto& f : lastWet) f = 0.0f;
                }
            }
            else if (structureFade < 1.0f)
            {
                structureFade = std::min (1.0f, structureFade + fadeStep);
            }

            // Advance LFOs (shared by both channels; the right channel adds the spread offset).
            const double inc = std::clamp ((double) params.rateHz, 0.01, 20.0) / sampleRate;
            const int voices = active.mode == Mode::chorus ? active.voices : 1;
            for (int v = 0; v < maxVoices; ++v)
            {
                phases[(size_t) v] += inc * (active.ensemble && active.mode == Mode::chorus ? ensembleRatios[v] : 1.0f);
                if (phases[(size_t) v] >= 1.0) phases[(size_t) v] -= 1.0;
            }

            for (int ch = 0; ch < nch; ++ch)
            {
                const float dry = io[ch][i];
                auto& line = lines[(size_t) ch];
                float wet = 0.0f;
                float dryOut = dry;

                if (active.mode == Mode::chorus)
                {
                    line.push (dry);
                    const float modMs = depth * std::min (delayMs * 0.5f, 8.0f);
                    for (int v = 0; v < voices; ++v)
                    {
                        double ph = phases[(size_t) v] + (double) v / voices + (ch == 1 ? spread * 0.5 : 0.0);
                        ph -= std::floor (ph);
                        const float lfo = std::sin (6.283185307179586f * (float) ph);
                        const float dMs = std::clamp (delayMs * chorusBaseRatios[v] + modMs * lfo, 0.05f, maxDelayMs);
                        const float dSamples = (float) ((double) dMs * sampleRate / 1000.0); // exact for whole-ms delays
                        wet += line.read (dSamples);
                        m.voiceDelayMs[ch][v] = dMs;
                    }
                    wet /= std::sqrt ((float) voices);
                }
                else if (active.mode == Mode::flanger)
                {
                    line.push (dry + fb * structureFade * lastWet[(size_t) ch]); // feedback follows the wet fade
                    double ph = phases[0] + (ch == 1 ? spread * 0.5 : 0.0);
                    ph -= std::floor (ph);
                    const float lfo = std::sin (6.283185307179586f * (float) ph);
                    const float manual = std::min (delayMs, 20.0f);
                    const float dMs = std::clamp (manual * (1.0f + depth * lfo), 0.02f, maxDelayMs);
                    const float dSamples = std::max (1.0f, dMs * 0.001f * (float) sampleRate);
                    wet = line.read (dSamples);
                    // Soft limit the loop so high feedback cannot run away.
                    lastWet[(size_t) ch] = wet > 2.0f || wet < -2.0f ? std::tanh (wet * 0.5f) * 2.0f : wet;
                    if (active.invert) wet = -wet;
                    m.voiceDelayMs[ch][0] = dMs;
                    if (active.throughZero)
                    {
                        dryLines[(size_t) ch].push (dry);
                        dryOut = dryLines[(size_t) ch].read (std::max (1.0f, manual * 0.001f * (float) sampleRate));
                    }
                }
                else // micro-pitch: two crossfaded taps sliding through a window, L up and R down
                {
                    line.push (dry);
                    const float direction = (nch > 1 && ch == 1) ? -1.0f : 1.0f;
                    const float ratio = std::exp2 (direction * cents / 1200.0f);
                    const float rate = std::fabs (1.0f - ratio);
                    auto& ph = pitchPhase[(size_t) ch];
                    ph += (double) (rate / windowSamples);
                    if (ph >= 1.0) ph -= 1.0;
                    const float minDelay = 2.0f;
                    auto tap = [&] (double phase)
                    {
                        phase -= std::floor (phase);
                        const float pos = (float) phase * windowSamples;
                        const float d = ratio > 1.0f ? minDelay + windowSamples - pos : minDelay + pos;
                        const float g = std::sin (3.14159265358979f * (float) phase);
                        return line.read (d) * g * g;
                    };
                    wet = tap (ph) + tap (ph + 0.5);
                    m.voiceDelayMs[ch][0] = (minDelay + windowSamples * 0.5f) * 1000.0f / (float) sampleRate;
                }

                wet *= structureFade;
                const float blend = mix >= 1.0f ? wet : (mix <= 0.0f ? dryOut : dryOut + (wet - dryOut) * mix);
                const float y = blend * out;
                io[ch][i] = y;
                m.outputPeak = std::max (m.outputPeak, std::fabs (y));
            }
        }

        m.activeVoices = active.mode == Mode::chorus ? active.voices : 1;
        return m;
    }

private:
    void applyStructureValues (const WideParams& p, bool immediate)
    {
        auto set = [immediate] (dsp::LinearSmoother& s, float v) { immediate ? s.setImmediate (v) : s.setTarget (v); };
        set (depthSmooth, std::clamp (p.depth, 0.0f, 1.0f));
        set (delaySmooth, std::clamp (p.delayMs, 0.02f, 40.0f));
        set (feedbackSmooth, std::clamp (p.feedback, -0.95f, 0.95f));
        set (centsSmooth, std::clamp (p.cents, -100.0f, 100.0f));
    }

    double sampleRate = 48000.0;
    int channels = 2;
    WideParams params, active, pending;
    bool firstSet = true;

    std::array<dsp::FractionalDelay, (size_t) maxChannels> lines, dryLines;
    std::array<float, (size_t) maxChannels> lastWet {};
    std::array<double, (size_t) maxVoices> phases {};
    std::array<double, (size_t) maxChannels> pitchPhase {};
    float windowSamples = 1152.0f;

    float structureFade = 1.0f, fadeStep = 0.002f;
    bool fadingOut = false;

    dsp::LinearSmoother depthSmooth, delaySmooth, feedbackSmooth, spreadSmooth, centsSmooth, mixSmooth, outSmooth;
};

} // namespace rommuser::wideready
