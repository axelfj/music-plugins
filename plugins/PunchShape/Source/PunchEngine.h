// Punch Shape DSP engine: level-independent transient shaper (attack / sustain) with a detector
// filter and an optional soft clipper on the output.
//
// Pure C++ (no JUCE) so it is unit-tested headless in tests/.
//
// How it works (all channels share one detector so the stereo image never moves):
//   d        = |detector-filtered input| (max over channels)
//   fastA    = envelope of d with 0.1 ms attack, `speed` release
//   slowA    = envelope of fastA with `speed` attack and release
//   attackDb = dB(fastA) - dB(slowA)      (> 0 only while a hit is louder than the recent average)
//   fastS    = envelope of d with 1 ms attack, `tail` / 5 release
//   slowS    = envelope of fastS with 1 ms attack, `tail` release
//   sustainDb = dB(slowS) - dB(fastS)     (> 0 only while the signal is decaying after a hit)
//   gain dB  = attack * min(attackDb / 12, 1) + sustain * min(sustainDb / 12, 1)
//
// Because the two detectors are compared in dB, the result depends on the shape of the signal
// and not on its level: a hit at -20 dBFS gets exactly the same shaping as one at -6 dBFS.
// With Attack and Sustain at 0 the gain is exactly 1 and the output is bit-identical to the input.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../../../shared/dsp/Biquad.h"
#include "../../../shared/dsp/Smoothing.h"
#include "../../ClipReady/Source/ClipEngine.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace rommuser::punchshape
{

enum class Detector { full = 0, lows, highs };

struct PunchParams
{
    float attackDb = 0.0f;      // -24 .. +24: how much to boost (+) or soften (-) the hits
    float sustainDb = 0.0f;     // -24 .. +24: how much to lengthen (+) or shorten (-) the tails
    float speedMs = 20.0f;      // 5 .. 100: how long a "hit" lasts for the attack detector
    float tailMs = 300.0f;      // 50 .. 2000: how long the sustain detector keeps listening after a hit
    Detector detector = Detector::full;
    bool clip = false;          // soft clipper at -0.3 dBFS on the output (same curve as Clip Ready)
    float outputDb = 0.0f;      // -24 .. +12
};

struct PunchMeters
{
    float inputPeak = 0.0f;     // linear
    float outputPeak = 0.0f;    // linear
    float attackGainDb = 0.0f;  // signed contribution of the attack stage with the largest magnitude in the block
    float sustainGainDb = 0.0f; // same for the sustain stage
    float levelDb = -120.0f;    // detector level (slow envelope) at the end of the block
    bool clipped = false;       // the clipper touched at least one sample in the block
};

inline constexpr int maxChannels = 2;
inline constexpr float normDb = 12.0f;        // a 12 dB difference between the detectors = the full amount
inline constexpr float floorDb = -100.0f;
inline constexpr float lowsCutoffHz = 150.0f;
inline constexpr float highsCutoffHz = 2000.0f;
inline constexpr float clipCeilingDb = -0.3f;
inline constexpr float clipSoftness = 0.35f;

// One-pole peak follower with separate attack and release times.
struct EnvelopeFollower
{
    void setTimes (double attackMs, double releaseMs, double sampleRate)
    {
        aAttack = coefficient (attackMs, sampleRate);
        aRelease = coefficient (releaseMs, sampleRate);
    }

    static float coefficient (double ms, double sampleRate)
    {
        return ms <= 0.0 ? 1.0f : (float) (1.0 - std::exp (-1.0 / (ms * 0.001 * sampleRate)));
    }

    inline float process (float x)
    {
        env += (x > env ? aAttack : aRelease) * (x - env);
        return env;
    }

    void reset() { env = 0.0f; }

    float env = 0.0f, aAttack = 1.0f, aRelease = 1.0f;
};

class PunchEngine
{
public:
    void prepare (double sampleRate, int numChannels)
    {
        sr = sampleRate;
        channels = std::clamp (numChannels, 1, maxChannels);
        outGain.prepare (sampleRate, 0.02);
        setParams (params, true);
        firstSet = true;
        reset();
    }

    void reset()
    {
        fastA.reset();
        slowA.reset();
        fastS.reset();
        slowS.reset();
        for (auto& f : detectorFilters)
            f.reset();
    }

    void setParams (const PunchParams& p, bool immediate = false)
    {
        immediate = immediate || firstSet;
        firstSet = false;

        if (immediate || p.detector != params.detector)
        {
            updateDetectorFilter (p.detector);
            for (auto& f : detectorFilters)
                f.reset();
        }
        if (immediate || std::fabs (p.speedMs - params.speedMs) > 1e-6f)
        {
            fastA.setTimes (0.1, p.speedMs, sr);
            slowA.setTimes (p.speedMs, p.speedMs, sr);
        }
        if (immediate || std::fabs (p.tailMs - params.tailMs) > 1e-6f)
        {
            fastS.setTimes (1.0, p.tailMs / 5.0, sr);
            slowS.setTimes (1.0, p.tailMs, sr);
        }

        const float out = dsp::dbToGain (p.outputDb);
        immediate ? outGain.setImmediate (out) : outGain.setTarget (out);

        params = p;
    }

    const PunchParams& getParams() const { return params; }

    PunchMeters process (float* const* io, int numChannels, int n)
    {
        PunchMeters m;
        const int nch = std::clamp (numChannels, 1, maxChannels);
        const float ceiling = dsp::dbToGain (clipCeilingDb);
        const float invCeiling = 1.0f / ceiling;
        const bool useFilter = params.detector != Detector::full;
        float maxAttack = 0.0f, maxSustain = 0.0f;

        for (int i = 0; i < n; ++i)
        {
            float d = 0.0f;
            for (int c = 0; c < nch; ++c)
            {
                const float x = io[c][i];
                m.inputPeak = std::max (m.inputPeak, std::fabs (x));
                const float f = useFilter ? detectorFilters[(size_t) c].process (x) : x;
                d = std::max (d, std::fabs (f));
            }

            // The slow followers listen to the fast ones, not to the raw signal: in steady state both
            // settle on the same value (so a held note gets no gain change), and only a change in
            // level opens a gap between them.
            const float fa = fastA.process (d), sa = slowA.process (fa);
            const float fsv = fastS.process (d), ssv = slowS.process (fsv);
            const float attackDiff = std::max (0.0f, toDb (fa) - toDb (sa));
            const float sustainDiff = std::max (0.0f, toDb (ssv) - toDb (fsv));
            const float attackGainDb = params.attackDb * std::min (attackDiff / normDb, 1.0f);
            const float sustainGainDb = params.sustainDb * std::min (sustainDiff / normDb, 1.0f);
            if (std::fabs (attackGainDb) > std::fabs (maxAttack)) maxAttack = attackGainDb;
            if (std::fabs (sustainGainDb) > std::fabs (maxSustain)) maxSustain = sustainGainDb;

            const float g = dsp::dbToGain (attackGainDb + sustainGainDb) * outGain.next();

            for (int c = 0; c < nch; ++c)
            {
                float y = io[c][i] * g;
                if (params.clip)
                {
                    const float clipped = clipready::clipCurve (y * invCeiling, clipSoftness) * ceiling;
                    m.clipped = m.clipped || std::fabs (clipped - y) > 1.0e-7f;
                    y = clipped;
                }
                io[c][i] = y;
                m.outputPeak = std::max (m.outputPeak, std::fabs (y));
            }
        }

        m.attackGainDb = maxAttack;
        m.sustainGainDb = maxSustain;
        m.levelDb = toDb (slowA.env);
        return m;
    }

private:
    static inline float toDb (float v)
    {
        return v > 1.0e-5f ? 20.0f * std::log10 (v) : floorDb;   // 1e-5 = -100 dBFS floor
    }

    void updateDetectorFilter (Detector d)
    {
        dsp::BiquadCoeffs c = dsp::BiquadCoeffs::identity();
        if (d == Detector::lows)
            c = dsp::biquad::lowPass (lowsCutoffHz, 0.7071, sr);
        else if (d == Detector::highs)
            c = dsp::biquad::highPass (highsCutoffHz, 0.7071, sr);
        for (auto& f : detectorFilters)
            f.setCoeffs (c);
    }

    double sr = 48000.0;
    int channels = 2;
    bool firstSet = true;
    PunchParams params;
    EnvelopeFollower fastA, slowA, fastS, slowS;
    std::array<dsp::Biquad, maxChannels> detectorFilters;
    dsp::LinearSmoother outGain;
};

} // namespace rommuser::punchshape
