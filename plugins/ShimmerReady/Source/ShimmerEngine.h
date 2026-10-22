// Shimmer Ready DSP engine: granular pitch-shifting delay with reverse, loop filters, freeze and an
// integrated reverb ("shimmer": echoes that climb an octave and turn into pads).
//
// Pure C++ (no JUCE) so it is unit-tested headless in tests/.
//
// Signal flow per channel:
//   in + feedback -> delay line (tempo-synced or free, forward or reverse) -> loop filters ->
//   granular pitch shifter (semitones per repetition; bypassed exactly at 0) -> soft limit -> feedback
//   wet = delay output (+ reverb of it by the Shimmer amount, Space Ready engine) -> mix with dry.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../../../shared/dsp/Biquad.h"
#include "../../../shared/dsp/FractionalDelay.h"
#include "../../../shared/dsp/Psola.h"
#include "../../../shared/dsp/Smoothing.h"
#include "../../SpaceReady/Source/SpaceEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace rommuser::shimmerready
{

constexpr int maxChannels = 2;
constexpr int numDivisions = 12;
constexpr double divisionBeats[numDivisions] = { 0.125, 1.0 / 6.0, 0.25, 1.0 / 3.0, 0.375, 0.5, 2.0 / 3.0, 0.75, 1.0, 1.5, 2.0, 4.0 };
constexpr const char* divisionNames[numDivisions] = { "1/32", "1/16T", "1/16", "1/8T", "1/16D", "1/8", "1/4T", "1/8D", "1/4", "1/4D", "1/2", "1 bar" };
constexpr double maxDelaySeconds = 4.0;

struct ShimmerParams
{
    bool sync = true;
    int division = 5;              // index into divisionBeats (1/8)
    float timeMs = 375.0f;         // free time when not synced, 10 .. 4000
    double bpm = 120.0;            // host tempo
    float pitchSemis = 12.0f;      // -24 .. 24 per repetition (0 = plain delay, bit-exact)
    float grainMs = 80.0f;         // 20 .. 200
    bool reverse = false;
    float feedback = 0.5f;         // 0 .. 1.1
    float lowCutHz = 0.0f;         // loop high-pass; 0 = off, else 20 .. 2000
    float highCutHz = 0.0f;        // loop low-pass; 0 = off, else 500 .. 20000
    bool freeze = false;           // loop holds (feedback 1, input muted)
    float shimmer = 0.0f;          // 0 .. 1 reverb amount on the wet signal
    float decaySeconds = 4.0f;     // reverb RT60
    float width = 1.0f;            // stereo offset of the right channel delay (0 .. 1 -> up to +15 %)
    float mix = 0.4f;
    float outputDb = 0.0f;
};

struct ShimmerMeters
{
    float delayMs = 0.0f;
    float wetPeak = 0.0f;
    float feedbackPeak = 0.0f;
    float inputHz = 0.0f;         // pitch tracked in the loop (0 = none)
};

// Two-grain overlap-add pitch shifter on a short delay line. ratio 1 is passed through untouched.
class GrainShifter
{
public:
    void prepare (double sampleRate, float maxGrainMs)
    {
        sr = sampleRate;
        line.setMaxDelay ((int) std::ceil (sr * maxGrainMs * 0.001) + 8);
        reset();
    }

    void reset()
    {
        line.reset();
        phase = 0.0;
    }

    void set (float ratioIn, float grainMs)
    {
        ratio = ratioIn;
        grainTarget = baseGrain = std::clamp (sr * grainMs * 0.001, 16.0, sr * 0.2);
        bypass = std::fabs (ratioIn - 1.0f) < 1.0e-6f;
    }

    // Pitch-synchronous grain. The two taps are half a grain apart, so for a tonal input of
    // frequency f they are phase-coherent (no beating, no cancellation) when f * grain / 2 is a whole
    // number m of cycles. The output is then periodic with period m / f and only has spectral lines at
    // multiples of f / m; the shifted tone r * f sits exactly on a line when r * m is an integer
    // (always true for octaves), so m is chosen near the base grain to make r * m as close to an
    // integer as possible. The new grain is applied where the sawtooth wraps, so no tap jumps.
    void setInputPitch (double hz, bool voiced)
    {
        if (! voiced || hz <= 0.0 || bypass)
        {
            grainTarget = baseGrain;
            return;
        }
        const double r = (double) ratio;
        const double cyclesPerSample = hz / sr;
        const double m0 = std::max (1.0, baseGrain * 0.5 * cyclesPerSample);
        const int mLo = std::max (1, (int) std::floor (m0 * 0.7)), mHi = std::max (mLo, (int) std::ceil (m0 * 1.3));
        int best = (int) std::round (m0);
        double bestCost = 1.0e9;
        for (int m = mLo; m <= mHi; ++m)
        {
            const double rm = r * (double) m;
            const double freqError = std::fabs (rm - std::round (rm)) / (double) m;   // relative error of r * f
            const double cost = freqError * 1000.0 + std::fabs ((double) m - m0) / m0 * 0.01;
            if (cost < bestCost) { bestCost = cost; best = m; }
        }
        grainTarget = std::clamp (2.0 * (double) best / cyclesPerSample, 16.0, sr * 0.2);
    }

    inline float process (float x)
    {
        if (bypass)
            return x;
        line.push (x);
        // Two taps half a grain apart; the delay sweeps from 0 to grain (or back) at rate 1 - ratio.
        const double prev = phase;
        phase += (1.0 - ratio) / grain;
        if (phase >= 1.0 || phase < 0.0)
        {
            phase -= std::floor (phase);
            grain = grainTarget;
        }
        (void) prev;
        float out = 0.0f;
        for (int k = 0; k < 2; ++k)
        {
            double ph = phase + 0.5 * k;
            ph -= std::floor (ph);
            const float delay = (float) (ph * grain) + 1.0f;
            const float w = 0.5f - 0.5f * std::cos (2.0 * 3.14159265358979 * ph);   // Hann
            out += line.read (delay) * w;
        }
        return out;
    }

private:
    double sr = 48000.0;
    dsp::FractionalDelay line;
    double phase = 0.0, grain = 4000.0, grainTarget = 4000.0, baseGrain = 4000.0;
    float ratio = 1.0f;
    bool bypass = true;
};

class ShimmerEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize, int numChannels)
    {
        sr = sampleRate;
        maxBlock = std::max (16, maxBlockSize);
        channels = std::clamp (numChannels, 1, maxChannels);
        const int maxDelay = (int) std::ceil (sr * maxDelaySeconds) * 3 + 16;   // reverse reads up to 3 L
        for (int c = 0; c < maxChannels; ++c)
        {
            lines[(size_t) c].setMaxDelay (maxDelay);
            shifters[(size_t) c].prepare (sr, 200.0f);
            timeSmooth[(size_t) c].prepare (sr, 0.08);
        }
        detector.prepare (sr, 60.0f);
        detectBuf.assign ((size_t) detector.getWindowSize(), 0.0f);
        detectCount = 0;
        reverb.prepare (sr, channels);
        wetScratch.assign ((size_t) (maxChannels * maxBlock), 0.0f);
        firstSet = true;
        setParams (params);
        reset();
    }

    void reset()
    {
        for (auto& l : lines) l.reset();
        for (auto& s : shifters) s.reset();
        for (auto& f : lowCut) f.reset();
        for (auto& f : highCut) f.reset();
        reverb.reset();
        reversePhase = 0.0;
        std::fill (detectBuf.begin(), detectBuf.end(), 0.0f);
        detectCount = 0;
        meters = {};
    }

    void setParams (const ShimmerParams& p)
    {
        params = p;
        const double beats = divisionBeats[std::clamp (p.division, 0, numDivisions - 1)];
        const double bpm = p.bpm > 20.0 ? p.bpm : 120.0;
        const double seconds = p.sync ? beats * 60.0 / bpm : std::clamp ((double) p.timeMs, 10.0, 4000.0) * 0.001;
        baseDelay = std::clamp (seconds * sr, 16.0, sr * maxDelaySeconds);
        for (int c = 0; c < maxChannels; ++c)
        {
            const float target = (float) (baseDelay * (c == 1 ? 1.0 + 0.15 * std::clamp (p.width, 0.0f, 1.0f) : 1.0));
            if (firstSet) timeSmooth[(size_t) c].setImmediate (target);
            else timeSmooth[(size_t) c].setTarget (target);
            shifters[(size_t) c].set (std::pow (2.0f, std::clamp (p.pitchSemis, -24.0f, 24.0f) / 12.0f), std::clamp (p.grainMs, 20.0f, 200.0f));
        }
        useLowCut = p.lowCutHz > 1.0f;
        useHighCut = p.highCutHz > 1.0f && p.highCutHz < 19999.0f;
        if (useLowCut)
        {
            const auto c = dsp::biquad::highPass (std::clamp ((double) p.lowCutHz, 20.0, 2000.0), 0.7071, sr);
            for (auto& f : lowCut) f.setCoeffs (c);
        }
        if (useHighCut)
        {
            const auto c = dsp::biquad::lowPass (std::clamp ((double) p.highCutHz, 500.0, 20000.0), 0.7071, sr);
            for (auto& f : highCut) f.setCoeffs (c);
        }
        spaceready::SpaceParams rp;
        rp.mode = spaceready::Mode::hall;
        rp.decaySeconds = std::clamp (p.decaySeconds, 0.5f, 30.0f);
        rp.mix = 1.0f;
        rp.preDelayMs = 0.0f;
        rp.dampingHz = 7000.0f;
        rp.width = 1.0f;
        reverb.setParams (rp);
        meters.delayMs = (float) (baseDelay / sr * 1000.0);
        firstSet = false;
    }

    const ShimmerParams& getParams() const { return params; }
    const ShimmerMeters& getMeters() const { return meters; }
    double getDelaySamples() const { return baseDelay; }

    void process (float* const* io, int numChannels, int n)
    {
        const int nch = std::clamp (numChannels, 1, channels);
        const float fb = params.freeze ? 1.0f : std::clamp (params.feedback, 0.0f, 1.1f);
        const float inGain = params.freeze ? 0.0f : 1.0f;
        const float mix = std::clamp (params.mix, 0.0f, 1.0f);
        const float outGain = std::pow (10.0f, std::clamp (params.outputDb, -24.0f, 12.0f) / 20.0f);
        float wetPeak = 0.0f, fbPeak = 0.0f;

        for (int i = 0; i < n; ++i)
        {
            detectSample = 0.0f;
            for (int c = 0; c < nch; ++c)
            {
                const float L = timeSmooth[(size_t) c].next();
                const float dry = io[c][i];

                // Read the tap first (relative to the previous push: L - 1 == exactly L samples back
                // once this sample is pushed), then write input + processed feedback.
                float tap;
                if (params.reverse)
                {
                    // Two half-overlapping voices reading backwards over segments of length L.
                    tap = 0.0f;
                    for (int k = 0; k < 2; ++k)
                    {
                        double ph = reversePhase + 0.5 * k;
                        ph -= std::floor (ph);
                        const float d = L + 2.0f * (float) ph * L;
                        const float w = 0.5f - 0.5f * std::cos (2.0 * 3.14159265358979 * ph);
                        tap += lines[(size_t) c].read (d - 1.0f) * w;
                    }
                }
                else
                    tap = lines[(size_t) c].read (L - 1.0f);

                // Loop processing: filters, pitch, soft limit -> fed back with the input.
                float loop = tap;
                if (useLowCut) loop = lowCut[(size_t) c].process (loop);
                if (useHighCut) loop = highCut[(size_t) c].process (loop);
                loop = shifters[(size_t) c].process (loop);
                if (fb > 0.0f)
                {
                    const float a = std::fabs (loop);
                    if (a > 0.9f) loop = (loop > 0.0f ? 1.0f : -1.0f) * (0.9f + std::tanh ((a - 0.9f) * 2.5f) * 0.4f);
                }
                lines[(size_t) c].push (dry * inGain + loop * fb);
                fbPeak = std::max (fbPeak, std::fabs (loop));
                if (c == 0) detectSample = tap; else detectSample += tap;

                wetScratch[(size_t) (c * maxBlock + i)] = tap;
            }
            // Track the pitch of what goes round the loop so the grain can lock to it.
            {
                const size_t w = detectBuf.size();
                detectBuf[(size_t) (detectCount % (int64_t) w)] = detectSample / (float) nch;
                ++detectCount;
                if (detectCount % (int64_t) (w / 4) == 0)
                {
                    // Linearise the ring into `detectLinear` (oldest first) and detect.
                    detectLinear.resize (w);
                    const size_t start = (size_t) (detectCount % (int64_t) w);
                    for (size_t k = 0; k < w; ++k) detectLinear[k] = detectBuf[(start + k) % w];
                    const auto est = detector.detect (detectLinear.data());
                    // The loop signal carries grain modulation, so accept a looser periodicity than a voice would need.
                    const bool tonal = est.hz > 0.0f && est.hz < 2000.0f && est.confidence > 0.4f;
                    for (auto& sh : shifters) sh.setInputPitch (est.hz, tonal);
                    meters.inputHz = tonal ? est.hz : 0.0f;
                }
            }
            if (params.reverse)
            {
                const float L0 = timeSmooth[0].getTarget();
                reversePhase += 1.0 / std::max (16.0f, L0);
                reversePhase -= std::floor (reversePhase);
            }
        }

        // Shimmer: reverb of the wet signal, added by amount.
        float* wetPtrs[maxChannels] = {};
        for (int c = 0; c < nch; ++c) wetPtrs[c] = wetScratch.data() + (size_t) (c * maxBlock);
        const float shimmer = std::clamp (params.shimmer, 0.0f, 1.0f);
        if (shimmer > 0.0f)
        {
            // Keep a copy of the delay output; the reverb replaces its buffer with pure wet.
            std::vector<float>& copy = reverbScratch;
            copy.assign (wetScratch.begin(), wetScratch.end());
            float* rvPtrs[maxChannels] = {};
            for (int c = 0; c < nch; ++c) rvPtrs[c] = copy.data() + (size_t) (c * maxBlock);
            reverb.process (rvPtrs, nch, n);
            for (int c = 0; c < nch; ++c)
                for (int i = 0; i < n; ++i)
                    wetPtrs[c][i] += rvPtrs[c][i] * shimmer;
        }

        for (int c = 0; c < nch; ++c)
            for (int i = 0; i < n; ++i)
            {
                const float dry = io[c][i], wet = wetPtrs[c][i];
                const float y = mix >= 1.0f ? wet : mix <= 0.0f ? dry : dry + (wet - dry) * mix;
                io[c][i] = y * outGain;
                wetPeak = std::max (wetPeak, std::fabs (wet));
            }

        meters.wetPeak = wetPeak;
        meters.feedbackPeak = fbPeak;
    }

private:
    double sr = 48000.0;
    int maxBlock = 512, channels = 2;
    ShimmerParams params;
    ShimmerMeters meters;
    bool firstSet = true;
    double baseDelay = 18000.0;

    std::array<dsp::FractionalDelay, maxChannels> lines;
    std::array<GrainShifter, maxChannels> shifters;
    std::array<dsp::LinearSmoother, maxChannels> timeSmooth;
    std::array<dsp::Biquad, maxChannels> lowCut, highCut;
    bool useLowCut = false, useHighCut = false;
    double reversePhase = 0.0;

    spaceready::SpaceEngine reverb;
    std::vector<float> wetScratch, reverbScratch;

    dsp::psola::PitchDetector detector;
    std::vector<float> detectBuf, detectLinear;
    int64_t detectCount = 0;
    float detectSample = 0.0f;
};

} // namespace rommuser::shimmerready
