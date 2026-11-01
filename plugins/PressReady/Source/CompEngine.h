// Press Ready DSP engine: feed-forward compressor with peak/RMS detection, soft knee,
// log-domain ballistics applied to the gain reduction, sidechain filter, external sidechain,
// lookahead and three "characters" (Clean / Punch / Glue).
//
// Pure C++ (no JUCE) so it is unit-tested headless in tests/.
//
// Signal flow (stereo-linked):
//   sc = external ? sidechain : input  -> high-pass -> level (peak or RMS, in dB)
//   grRaw = gainComputer (level)                           (0 .. -inf dB, instantaneous)
//   gr    = ballistics (grRaw)                             (attack / release, in dB)
//   wet   = saturate (delay (input) * gain (gr) * makeup)  (saturation only for Punch / Glue, and only while compressing)
//   out   = mix * wet + (1 - mix) * delay (input)
//
// Smoothing the gain reduction rather than the detector level makes the attack and release
// knobs mean what they say: 63 % of the gain change happens after exactly one time constant.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../../../shared/dsp/Biquad.h"
#include "../../../shared/dsp/Oversampler.h" // DelayLine
#include "../../../shared/dsp/Smoothing.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace rommuser::pressready
{

enum class Detector { peak, rms };
enum class Character { clean, punch, glue };

struct CompParams
{
    float thresholdDb = -18.0f;  // -60 .. 0
    float ratio = 4.0f;          // 1 .. 20; >= infiniteRatio means limiter (inf:1)
    float attackMs = 10.0f;      // 0.05 .. 300
    float releaseMs = 120.0f;    // 5 .. 3000
    bool autoRelease = false;    // program-dependent: blend of a fast and a slow release
    float kneeDb = 6.0f;         // 0 .. 24
    float makeupDb = 0.0f;       // -12 .. 24
    bool autoMakeup = false;     // adds half the static gain reduction of a 0 dBFS signal
    float mix = 1.0f;            // 0 .. 1
    Detector detector = Detector::peak;
    float sidechainHpfHz = 0.0f; // 0 = off, else 20 .. 500
    bool externalSidechain = false;
    Character character = Character::clean;
    float lookaheadMs = 0.0f;    // 0 .. 10
};

constexpr float infiniteRatio = 20.0f;

struct CompMeters
{
    float inputPeak = 0.0f;      // linear
    float outputPeak = 0.0f;     // linear
    float gainReductionDb = 0.0f; // positive number of dB, max over the block
};

// Static gain computer: gain change (dB, <= 0) for a level, with a soft knee of `kneeDb` centred
// on the threshold. Shared with the editor to draw the transfer curve.
inline float gainComputerDb (float levelDb, float thresholdDb, float ratio, float kneeDb)
{
    const float slope = ratio >= infiniteRatio ? 1.0f : 1.0f - 1.0f / std::max (ratio, 1.0f);
    const float over = levelDb - thresholdDb;
    if (kneeDb <= 0.0f || over <= -kneeDb * 0.5f)
        return over > 0.0f ? -slope * over : 0.0f;
    if (over >= kneeDb * 0.5f)
        return -slope * over;
    const float t = over + kneeDb * 0.5f;
    return -slope * t * t / (2.0f * kneeDb);
}

// How a character bends the user's settings. Kept in one place so the GUI can explain it.
struct CharacterProfile
{
    float attackScale = 1.0f, releaseScale = 1.0f, kneeAddDb = 0.0f;
    float saturation = 0.0f;     // 0 = none; drive of the tanh stage at 12 dB of gain reduction
    bool forcePeak = false, forceRms = false;
};

inline CharacterProfile profileFor (Character c)
{
    switch (c)
    {
        case Character::punch: return { 1.0f, 0.7f, -3.0f, 0.6f, true, false };   // FET-like: grabs fast, lets go fast, colours
        case Character::glue:  return { 2.0f, 1.3f, 6.0f, 0.15f, false, true };  // bus-like: RMS, slow, wide knee, a little warmth
        case Character::clean:
        default:               return {};
    }
}

class CompEngine
{
public:
    static constexpr int maxChannels = 2;
    static constexpr float maxLookaheadMs = 10.0f;

    void prepare (double newSampleRate, int maxBlockSize, int numChannels)
    {
        sampleRate = newSampleRate;
        channels = std::min (numChannels, maxChannels);
        (void) maxBlockSize;
        const int maxLa = (int) std::ceil (sampleRate * maxLookaheadMs / 1000.0) + 1;
        for (auto& d : delays) d.setMaxDelay (maxLa);
        makeupGain.prepare (sampleRate, 0.02);
        mixSmooth.prepare (sampleRate, 0.02);
        satDrive.prepare (sampleRate, 0.03);
        setParams (params, true);
        reset();
    }

    void reset()
    {
        gr = grFast = grSlow = 0.0f;
        rmsState = peakLevel = 0.0f;
        holdLeft = 0;
        for (auto& d : delays) d.reset();
        for (auto& f : scFilters) f.reset();
    }

    void setParams (const CompParams& p, bool immediate = false)
    {
        params = p;
        profile = profileFor (p.character);

        const float attackMs = std::max (0.0f, p.attackMs * profile.attackScale);
        const float releaseMs = std::max (1.0f, p.releaseMs * profile.releaseScale);
        kneeDb = std::clamp (p.kneeDb + profile.kneeAddDb, 0.0f, 30.0f);
        detector = profile.forcePeak ? Detector::peak : profile.forceRms ? Detector::rms : p.detector;
        rmsCoef = coefFor (detector == Detector::rms && p.character == Character::glue ? 30.0f : 10.0f);
        // The peak hold bridges the dips between the peaks of a waveform. Up to 10 ms (half a period
        // of 50 Hz) so a fast release does not ride a bass note (2 ms gave -34 dB THD at 50 Hz with a
        // 30 ms release, 10 ms gives -113 dB), never more than a third of the release; the release
        // time constant gives the hold back so the knob still means what it says.
        const float holdMs = detector == Detector::peak ? std::clamp (releaseMs / 3.0f, peakDecayMs, maxPeakHoldMs) : 0.0f;
        attackCoef = coefFor (attackMs);
        releaseCoef = coefFor (releaseMs - holdMs);
        releaseFastCoef = coefFor (std::max (20.0f, releaseMs / 5.0f) - holdMs);
        releaseSlowCoef = coefFor (releaseMs * 3.0f - holdMs);
        holdSamples = (int) std::lround (sampleRate * holdMs / 1000.0);
        peakDecayCoef = coefFor (peakDecayMs);

        lookaheadSamples = std::clamp ((int) std::lround (sampleRate * p.lookaheadMs / 1000.0), 0,
                                       (int) std::ceil (sampleRate * maxLookaheadMs / 1000.0));
        for (auto& d : delays) d.setDelay (lookaheadSamples);

        const float makeup = p.makeupDb + (p.autoMakeup ? autoMakeupDb (p) : 0.0f);
        const float g = dsp::dbToGain (makeup);
        immediate ? makeupGain.setImmediate (g) : makeupGain.setTarget (g);
        immediate ? mixSmooth.setImmediate (p.mix) : mixSmooth.setTarget (p.mix);
        immediate ? satDrive.setImmediate (profile.saturation) : satDrive.setTarget (profile.saturation);

        if (p.sidechainHpfHz > 0.0f)
        {
            const auto c = dsp::biquad::highPass (p.sidechainHpfHz, 0.7071, sampleRate);
            for (auto& f : scFilters) f.setCoeffs (c);
        }
    }

    // Half of what a 0 dBFS signal would be turned down by: a sensible starting point.
    static float autoMakeupDb (const CompParams& p)
    {
        return -0.5f * gainComputerDb (0.0f, p.thresholdDb, p.ratio, p.kneeDb + profileFor (p.character).kneeAddDb);
    }

    // Static transfer curve for the display (dB in -> dB out, without makeup).
    float staticOutputDb (float inputDb) const
    {
        return inputDb + gainComputerDb (inputDb, params.thresholdDb, params.ratio, kneeDb);
    }

    int getLatencySamples() const { return lookaheadSamples; }
    float getGainReductionDb() const { return gr; }

    // io: audio in/out; sidechain: optional external key (same channel count), may be nullptr.
    CompMeters process (float* const* io, const float* const* sidechain, int numChannels, int n)
    {
        CompMeters meters;
        numChannels = std::min (numChannels, channels);
        if (numChannels <= 0) return meters;

        const bool useExt = params.externalSidechain && sidechain != nullptr;
        const bool hpf = params.sidechainHpfHz > 0.0f;

        for (int i = 0; i < n; ++i)
        {
            // Detector: stereo-linked, max of the channels after the sidechain filter.
            float key = 0.0f;
            for (int c = 0; c < numChannels; ++c)
            {
                float s = useExt ? sidechain[c][i] : io[c][i];
                if (hpf) s = scFilters[(size_t) c].process (s);
                key = std::max (key, std::fabs (s));
                meters.inputPeak = std::max (meters.inputPeak, std::fabs (io[c][i]));
            }

            float levelDb;
            if (detector == Detector::rms)
            {
                rmsState += rmsCoef * (key * key - rmsState);
                levelDb = 10.0f * std::log10 (std::max (rmsState, 1e-12f));
            }
            else
            {
                // Peak with a short hold, so the dips between the peaks of a waveform don't pull the
                // gain reduction back every half cycle (which would also stretch the attack time).
                if (key >= peakLevel) { peakLevel = key; holdLeft = holdSamples; }
                else if (holdLeft > 0) --holdLeft;
                else peakLevel += peakDecayCoef * (key - peakLevel);
                levelDb = 20.0f * std::log10 (std::max (peakLevel, 1e-6f));
            }

            // Gain reduction as a positive dB number, then attack / release ballistics on it.
            const float grRaw = -gainComputerDb (levelDb, params.thresholdDb, params.ratio, kneeDb);
            if (params.autoRelease)
            {
                step (grFast, grRaw, attackCoef, releaseFastCoef);
                step (grSlow, grRaw, attackCoef, releaseSlowCoef);
                gr = 0.5f * (grFast + grSlow);
            }
            else
            {
                step (gr, grRaw, attackCoef, releaseCoef);
                grFast = grSlow = gr;
            }
            meters.gainReductionDb = std::max (meters.gainReductionDb, gr);

            const float gain = dsp::dbToGain (-gr) * makeupGain.next();
            const float m = mixSmooth.next();
            const float drive = satDrive.next() * std::min (gr, 12.0f) / 12.0f;

            for (int c = 0; c < numChannels; ++c)
            {
                const float dry = delays[(size_t) c].process (io[c][i]);
                float wet = dry * gain;
                if (drive > 1e-4f)
                    wet = std::tanh (drive * wet) / drive; // unity slope at 0, gently rounds the peaks
                const float y = m * wet + (1.0f - m) * dry;
                io[c][i] = y;
                meters.outputPeak = std::max (meters.outputPeak, std::fabs (y));
            }
        }
        return meters;
    }

private:
    float coefFor (float ms) const
    {
        if (ms <= 0.0f) return 1.0f;
        return 1.0f - std::exp (-1.0f / (float) (sampleRate * ms / 1000.0));
    }

    static void step (float& state, float target, float attack, float release)
    {
        const float k = target > state ? attack : release;
        state += k * (target - state);
    }

    CompParams params;
    CharacterProfile profile;
    double sampleRate = 44100.0;
    int channels = 2, lookaheadSamples = 0;
    float attackCoef = 1.0f, releaseCoef = 1.0f, releaseFastCoef = 1.0f, releaseSlowCoef = 1.0f, rmsCoef = 1.0f, kneeDb = 6.0f;
    Detector detector = Detector::peak;
    static constexpr float peakDecayMs = 2.0f, maxPeakHoldMs = 10.0f;
    int holdSamples = 0, holdLeft = 0;
    float peakDecayCoef = 1.0f;
    float gr = 0.0f, grFast = 0.0f, grSlow = 0.0f, rmsState = 0.0f, peakLevel = 0.0f;
    std::array<dsp::DelayLine, (size_t) maxChannels> delays;
    std::array<dsp::Biquad, (size_t) maxChannels> scFilters;
    dsp::LinearSmoother makeupGain, mixSmooth, satDrive;
};

} // namespace rommuser::pressready
