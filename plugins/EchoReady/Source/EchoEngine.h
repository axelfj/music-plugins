// Echo Ready DSP engine: stereo / ping-pong delay with tempo sync, filtered and saturated feedback,
// diffusion, tape-style wow, ducking and click-free time changes.
//
// Pure C++ (no JUCE) so it is unit-tested headless in tests/.
//
// Per channel (stereo mode) or cross-coupled (ping-pong):
//   in -> [+ feedback] -> delay (time +- wow) -> low cut -> high cut -> diffusion -> saturate -> wet
//   feedback = wet * feedbackAmount
//   out = dry + wet * duck (input) * mix
//
// Time changes: "crossfade" runs a second read head at the new time and fades to it over 60 ms, so a
// tempo or division change never clicks; "glide" slides the read position instead (the pitch of the
// repeats bends, like a tape machine). Tempo-synced times are exact: beats * 60 / bpm * sampleRate
// samples, fractional, read with cubic interpolation.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../../../shared/dsp/Biquad.h"
#include "../../../shared/dsp/FractionalDelay.h"
#include "../../../shared/dsp/Smoothing.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace rommuser::echoready
{

enum class Mode { stereo, pingPong };
enum class Style { digital, tape, analog };
enum class TimeChange { crossfade, glide };

struct EchoParams
{
    bool sync = true;
    double beats = 0.5;              // division in quarter notes when sync (1/8 = 0.5)
    float timeMs = 375.0f;           // free time when not synced (1 .. 2000)
    double bpm = 120.0;              // host tempo (fallback 120)
    Mode mode = Mode::stereo;
    Style style = Style::digital;
    TimeChange timeChange = TimeChange::crossfade;
    float feedback = 0.4f;           // 0 .. 1.1
    float lowCutHz = 0.0f;           // 0 = off, else 20 .. 2000
    float highCutHz = 0.0f;          // 0 = off, else 500 .. 20000
    float diffusion = 0.0f;          // 0 .. 1
    float wow = 0.0f;                // 0 .. 1 (depth of the time modulation; tape adds some by itself)
    float duck = 0.0f;               // 0 .. 1
    float mix = 0.35f;               // 0 .. 1
    float outputDb = 0.0f;
};

struct EchoMeters
{
    float wetPeak = 0.0f;
    float duckGain = 1.0f;
};

// Per-style colour, applied on top of the user's settings.
struct StyleProfile
{
    float saturation = 0.0f;     // tanh drive in the feedback path (0 = clean)
    float extraWow = 0.0f;       // added modulation depth
    float highCutCapHz = 0.0f;   // 0 = none; else the loop never passes more than this
    float lowCutMinHz = 0.0f;
};

inline StyleProfile profileFor (Style s)
{
    switch (s)
    {
        case Style::tape:   return { 0.5f, 0.12f, 9000.0f, 60.0f };
        case Style::analog: return { 0.35f, 0.04f, 4500.0f, 100.0f };
        case Style::digital:
        default:            return {};
    }
}

class EchoEngine
{
public:
    static constexpr int maxChannels = 2;
    static constexpr float maxTimeSeconds = 2.2f;
    static constexpr float crossfadeSeconds = 0.06f;
    static constexpr float glideSeconds = 0.12f;
    static constexpr float wowMaxMs = 6.0f;

    void prepare (double newSampleRate, int numChannels)
    {
        sampleRate = newSampleRate;
        channels = std::clamp (numChannels, 1, maxChannels);
        const int maxDelay = (int) std::ceil (sampleRate * maxTimeSeconds) + 16;
        for (auto& d : delays) d.setMaxDelay (maxDelay);
        for (auto& ch : diffusers) for (auto& ap : ch) ap.setMaxDelay (512);
        crossfadeStep = 1.0f / std::max (1.0f, (float) (sampleRate * crossfadeSeconds));
        glideCoef = 1.0f - std::exp (-1.0f / (float) (sampleRate * glideSeconds / 2.0f));
        duckAttack = 1.0f - std::exp (-1.0f / (float) (sampleRate * 0.005));
        duckRelease = 1.0f - std::exp (-1.0f / (float) (sampleRate * 0.25));
        mixSmooth.prepare (sampleRate, 0.02);
        feedbackSmooth.prepare (sampleRate, 0.02);
        outSmooth.prepare (sampleRate, 0.02);
        wowSmooth.prepare (sampleRate, 0.05);
        firstParams = true;
        setParams (params);
        firstSet = true; // the first parameters sent before any audio apply immediately
        reset();
    }

    void reset()
    {
        for (auto& d : delays) d.reset();
        for (auto& ch : diffusers) for (auto& ap : ch) ap.reset();
        for (auto& f : lowCut) f.reset();
        for (auto& f : highCut) f.reset();
        for (auto& f : loopDc) f.reset();
        fbState.fill (0.0f);
        duckEnv = 0.0f;
        lfoPhase = 0.0f;
        currentTime = targetTime;
        oldTime = targetTime;
        fade = 1.0f;
    }

    // Delay time (samples) the parameters ask for.
    float nominalDelaySamples (const EchoParams& p) const
    {
        const double seconds = p.sync ? p.beats * 60.0 / std::max (p.bpm, 20.0) : p.timeMs / 1000.0;
        return (float) std::clamp (seconds * sampleRate, 1.0, (double) sampleRate * maxTimeSeconds);
    }

    void setParams (const EchoParams& p)
    {
        const auto profile = profileFor (p.style);
        const float newTime = nominalDelaySamples (p);
        if (firstParams)
        {
            targetTime = currentTime = oldTime = newTime;
            fade = 1.0f;
        }
        else if (std::fabs (newTime - targetTime) > 0.5f)
        {
            if (p.timeChange == TimeChange::crossfade)
            {
                // Start (or restart) a fade from where the current head is to the new time.
                oldTime = fade < 1.0f ? interpolatedTime() : currentTime;
                fade = 0.0f;
                currentTime = newTime;
            }
            targetTime = newTime;
        }
        firstParams = false;

        const bool filtersChanged = std::fabs (p.lowCutHz - params.lowCutHz) > 0.0f || std::fabs (p.highCutHz - params.highCutHz) > 0.0f
                                 || p.style != params.style || ! filtersReady;
        params = p;
        if (filtersChanged)
        {
            float lc = p.lowCutHz > 0.0f ? p.lowCutHz : 0.0f;
            if (profile.lowCutMinHz > 0.0f) lc = std::max (lc, profile.lowCutMinHz);
            float hc = p.highCutHz > 0.0f ? p.highCutHz : 0.0f;
            if (profile.highCutCapHz > 0.0f) hc = hc > 0.0f ? std::min (hc, profile.highCutCapHz) : profile.highCutCapHz;
            useLowCut = lc > 0.0f;
            useHighCut = hc > 0.0f;
            if (useLowCut)  { const auto c = dsp::biquad::highPass (lc, 0.7071, sampleRate); for (auto& f : lowCut) f.setCoeffs (c); }
            if (useHighCut) { const auto c = dsp::biquad::lowPass (hc, 0.7071, sampleRate); for (auto& f : highCut) f.setCoeffs (c); }
            const auto dc = dsp::biquad::highPass1 (8.0, sampleRate);
            for (auto& f : loopDc) f.setCoeffs (dc);
            filtersReady = true;
        }
        saturation = profile.saturation;
        auto set = [this] (dsp::LinearSmoother& s, float v) { firstSet ? s.setImmediate (v) : s.setTarget (v); };
        set (mixSmooth, std::clamp (p.mix, 0.0f, 1.0f));
        set (feedbackSmooth, std::clamp (p.feedback, 0.0f, 1.1f));
        set (outSmooth, dsp::dbToGain (p.outputDb));
        set (wowSmooth, std::clamp (p.wow + profile.extraWow, 0.0f, 1.0f) * wowMaxMs * 0.001f * (float) sampleRate);
    }

    const EchoParams& getParams() const { return params; }
    float getCurrentDelaySamples() const { return (float) interpolatedTime(); }

    EchoMeters process (float* const* io, int numChannels, int n)
    {
        firstSet = false;
        EchoMeters meters;
        numChannels = std::clamp (numChannels, 1, channels);
        const bool pingPong = params.mode == Mode::pingPong && numChannels == 2;
        const float lfoInc = (float) (0.9 / sampleRate);  // 0.9 Hz wow

        for (int i = 0; i < n; ++i)
        {
            const float mix = mixSmooth.next(), fb = feedbackSmooth.next(), out = outSmooth.next(), wowDepth = wowSmooth.next();

            // Time: glide towards the target, or crossfade between the old and the new read head.
            if (params.timeChange == TimeChange::glide)
            {
                currentTime += glideCoef * (targetTime - currentTime);
                if (std::fabs (targetTime - currentTime) < 1e-3) currentTime = targetTime;
                fade = 1.0f;
            }
            else if (fade < 1.0f)
            {
                fade = std::min (1.0f, fade + crossfadeStep);
            }

            // Wow: a slow sine plus a slower one, so it never sounds like a metronome.
            lfoPhase += lfoInc;
            if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
            const float wow = wowDepth * (0.7f * std::sin (6.2831853f * lfoPhase) + 0.3f * std::sin (6.2831853f * lfoPhase * 0.37f + 1.0f));

            // Ducking follows the dry input.
            float inPeak = 0.0f;
            for (int c = 0; c < numChannels; ++c)
                inPeak = std::max (inPeak, std::fabs (io[c][i]));
            duckEnv += (inPeak > duckEnv ? duckAttack : duckRelease) * (inPeak - duckEnv);
            // Full duck 12 dB above -36 dBFS, nothing below -36 dBFS.
            const float envDb = 20.0f * std::log10 (std::max (duckEnv, 1e-6f));
            const float duckAmt = std::clamp ((envDb + 36.0f) / 12.0f, 0.0f, 1.0f) * params.duck;
            const float duckGain = 1.0f - duckAmt;
            meters.duckGain = duckGain;

            // Read the delay lines (both heads while crossfading).
            float wet[maxChannels] = {};
            for (int c = 0; c < numChannels; ++c)
            {
                const auto ci = (size_t) c;
                // The lines are read before this sample is pushed, so "time" samples of delay relative
                // to the current input means reading (time - 1) samples behind the last pushed one.
                const float tA = std::max (1.0f, (float) currentTime - 1.0f + wow);
                float w = delays[ci].read (tA);
                if (fade < 1.0f)
                {
                    const float tB = std::max (1.0f, (float) oldTime - 1.0f + wow);
                    w = w * fade + delays[ci].read (tB) * (1.0f - fade);
                }
                wet[c] = w;
            }

            // Feedback path processing on what comes out of the lines.
            for (int c = 0; c < numChannels; ++c)
            {
                const auto ci = (size_t) c;
                float w = wet[c];
                if (useLowCut) w = lowCut[ci].process (w);
                if (useHighCut) w = highCut[ci].process (w);
                if (params.diffusion > 0.001f) w = diffuse (ci, w);
                if (saturation > 0.0f)
                    w = std::tanh (w * (1.0f + saturation * 2.0f)) / (1.0f + saturation * 2.0f);
                w = loopDc[ci].process (w);
                wet[c] = w;
            }

            // Write the lines: input + feedback (cross-coupled for ping-pong).
            if (pingPong)
            {
                const float mono = 0.5f * (io[0][i] + io[1][i]);
                // Left line gets the input plus the right line's output; right line gets the left line's output.
                delays[0].push (mono + fb * wet[1]);
                delays[1].push (wet[0]);
            }
            else
            {
                for (int c = 0; c < numChannels; ++c)
                    delays[(size_t) c].push (io[c][i] + fb * wet[c]);
            }

            for (int c = 0; c < numChannels; ++c)
            {
                const float y = (io[c][i] * (1.0f - mix) + wet[c] * duckGain * mix) * out;
                meters.wetPeak = std::max (meters.wetPeak, std::fabs (wet[c] * duckGain * mix));
                io[c][i] = y;
            }
        }
        return meters;
    }

private:
    double interpolatedTime() const
    {
        return fade < 1.0f ? oldTime + (currentTime - oldTime) * fade : currentTime;
    }

    // Two short Schroeder all-passes smear each repeat a little (0 = bypass).
    float diffuse (size_t ch, float x)
    {
        static constexpr int lengths[2] = { 149, 211 };
        const float g = 0.5f * params.diffusion;
        for (int k = 0; k < 2; ++k)
        {
            auto& ap = diffusers[ch][(size_t) k];
            const float delayed = ap.readInteger (lengths[k]);
            const float v = x - g * delayed;
            ap.push (v);
            x = delayed + g * v;
        }
        return x;
    }

    EchoParams params;
    double sampleRate = 48000.0;
    int channels = 2;
    bool firstParams = true, firstSet = true, filtersReady = false, useLowCut = false, useHighCut = false;
    float saturation = 0.0f;

    std::array<dsp::FractionalDelay, maxChannels> delays;
    std::array<std::array<dsp::FractionalDelay, 2>, maxChannels> diffusers;
    std::array<dsp::Biquad, maxChannels> lowCut, highCut, loopDc;
    std::array<float, maxChannels> fbState {};

    double targetTime = 1.0, currentTime = 1.0, oldTime = 1.0;
    float fade = 1.0f, crossfadeStep = 1.0f, glideCoef = 1.0f;
    float lfoPhase = 0.0f;
    float duckEnv = 0.0f, duckAttack = 1.0f, duckRelease = 1.0f;
    dsp::LinearSmoother mixSmooth, feedbackSmooth, outSmooth, wowSmooth;
};

} // namespace rommuser::echoready
