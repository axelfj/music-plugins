// Duck Ready DSP engine: tempo-synced volume shaper ("sidechain pump" without a compressor).
//
// Pure C++ (no JUCE) so it is unit-tested headless in tests/.
//
// A gain curve is generated once per cycle (1/4, 1/8, 1/16, 1 bar...) from a few parameters:
//
//   gain
//   1 |‾‾\             __________‾‾‾‾‾‾‾‾‾‾
//     |   \___________/
//     |   attack hold   release
//     +-----------------------------------> phase (0 .. 1 = one cycle)
//
//   depth    how far the volume drops (1 = silence)
//   attack   fraction of the cycle used to drop
//   hold     fraction held at the bottom
//   release  fraction used to come back up
//   curve    bends attack and release (-1 fast start / +1 slow start, 0 linear)
//   offset   shifts the whole curve in the cycle (to taste, or to compensate the kick's transient)
//
// The phase comes from one of three triggers:
//   tempo      the host's ppq position, so the curve is sample-aligned to the DAW grid even after
//              loops, jumps or when the plugin is inserted mid-song; free-runs when the transport stops
//   midi       a note-on restarts the curve (one shot, length = one cycle)
//   sidechain  an onset on the sidechain input (the kick) restarts the curve (one shot)
//
// Optional multiband: a Linkwitz-Riley 4th-order crossover splits the input and the gain is applied to
// the low band or the high band only. LR4 bands sum back to a flat magnitude, so with depth 0 the only
// change is the crossover's phase shift.
//
// Any phase jump (retrigger, transport jump, parameter change that moves the curve) is crossfaded over
// 1 ms from the previous gain, so there is never a click; in plain tempo mode the curve itself is
// continuous and the output is sample-exact with the grid.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../../../shared/dsp/Biquad.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace rommuser::duckready
{

enum class Trigger { tempo, midi, sidechain };
enum class Band { full, low, high };

struct DuckParams
{
    float depth = 0.85f;           // 0 .. 1
    float attack = 0.02f;          // 0.005 .. 0.5  (fraction of the cycle)
    float hold = 0.05f;            // 0 .. 0.9
    float release = 0.55f;         // 0.01 .. 1
    float curve = 0.3f;            // -1 .. 1
    float offset = 0.0f;           // -0.5 .. 0.5 (fraction of the cycle)
    double beatsPerCycle = 1.0;    // 1/4 note = 1 beat at 4/4; 1 bar = 4 beats
    Trigger trigger = Trigger::tempo;
    float sidechainThresholdDb = -24.0f; // onset threshold for Trigger::sidechain
    Band band = Band::full;
    float crossoverHz = 200.0f;    // 40 .. 2000
};

// Host transport for one block. If `valid` is false the engine free-runs at `bpm`.
struct Transport
{
    bool valid = false;
    bool playing = false;
    double ppqPosition = 0.0;  // in quarter notes, at the first sample of the block
    double bpm = 120.0;
};

// Time constant so that an exponential step settles within `samples` samples to 1/e.
inline float onePoleCoeff (double timeSeconds, double sampleRate)
{
    return timeSeconds <= 0.0 ? 1.0f : (float) (1.0 - std::exp (-1.0 / (timeSeconds * sampleRate)));
}

// Bend a 0..1 ramp. curve 0 = linear, > 0 slow start, < 0 fast start. Monotonic and continuous.
inline float bend (float u, float curve)
{
    u = std::clamp (u, 0.0f, 1.0f);
    const float k = std::pow (4.0f, std::clamp (curve, -1.0f, 1.0f));  // 0.25 .. 4
    return std::pow (u, k);
}

// Normalise attack / hold / release so they fit in one cycle (sum <= 1), keeping their proportions.
struct CurveShape
{
    float attack, hold, release, depth, curve;

    explicit CurveShape (const DuckParams& p)
    {
        attack = std::max (p.attack, 0.0005f);
        hold = std::max (p.hold, 0.0f);
        release = std::max (p.release, 0.001f);
        const float sum = attack + hold + release;
        if (sum > 1.0f)
        {
            attack /= sum;
            hold /= sum;
            release /= sum;
        }
        depth = std::clamp (p.depth, 0.0f, 1.0f);
        curve = p.curve;
    }

    // Gain (0..1) at a phase in [0, 1). The curve returns to exactly 1 before the cycle ends.
    float gainAt (float phase) const
    {
        phase -= std::floor (phase);
        if (phase < attack)
            return 1.0f - depth * bend (phase / attack, curve);
        phase -= attack;
        if (phase < hold)
            return 1.0f - depth;
        phase -= hold;
        if (phase < release)
            return 1.0f - depth * (1.0f - bend (phase / release, -curve));
        return 1.0f;
    }
};

class DuckEngine
{
public:
    static constexpr int maxChannels = 2;
    static constexpr float crossfadeSeconds = 0.001f;

    void prepare (double newSampleRate, int numChannels)
    {
        sampleRate = newSampleRate;
        channels = std::clamp (numChannels, 1, maxChannels);
        crossfadeStep = 1.0f / std::max (1.0f, (float) (sampleRate * crossfadeSeconds));
        scAttack = onePoleCoeff (0.0005, sampleRate);
        scRelease = onePoleCoeff (0.040, sampleRate);
        retriggerHoldoff = (int) (sampleRate * 0.060);
        setParams (params, true);
        reset();
    }

    void reset()
    {
        for (auto& ch : lowFilters) for (auto& f : ch) f.reset();
        for (auto& ch : highFilters) for (auto& f : ch) f.reset();
        freePhase = 0.0;
        oneShotPhase = 1.0;  // idle: curve finished
        scEnvelope = 0.0f;
        scAbove = false;
        samplesSinceTrigger = retriggerHoldoff;
        fade = 1.0f;
        lastGain = 1.0f;
        lastPhase = -1.0;
    }

    void setParams (const DuckParams& p, bool force = false)
    {
        const bool crossoverChanged = force || std::fabs (p.crossoverHz - params.crossoverHz) > 0.0f;
        const bool triggerChanged = force || p.trigger != params.trigger;
        params = p;
        shape = CurveShape (p);
        if (crossoverChanged)
            updateCrossover();
        if (triggerChanged)
            oneShotPhase = 1.0;
    }

    const DuckParams& getParams() const { return params; }

    // Phase (0..1) the curve was at on the last processed sample; for the UI playhead.
    float getCurrentPhase() const { return (float) (lastPhase < 0.0 ? 0.0 : lastPhase); }
    float getCurrentGain() const { return lastGain; }

    // io: numChannels channel pointers, processed in place.
    // sidechain: may be null (then the main input is used as the sidechain source).
    // midiNoteOnOffsets / numMidi: sample offsets (within this block) of MIDI note-ons, ascending.
    void process (float* const* io, const float* const* sidechain, int numChannels, int n,
                  const Transport& transport, const int* midiNoteOnOffsets = nullptr, int numMidi = 0)
    {
        numChannels = std::clamp (numChannels, 1, channels);
        const double bpm = transport.valid && transport.bpm > 0.0 ? transport.bpm : 120.0;
        const double beatsPerSample = bpm / 60.0 / sampleRate;
        const double phaseStep = beatsPerSample / std::max (params.beatsPerCycle, 1.0 / 64.0);

        // Tempo mode: phase from the host. Snap the free-running phase to the host so stopping and
        // restarting the transport does not jump.
        double phase = freePhase;
        if (params.trigger == Trigger::tempo && transport.valid && transport.playing)
        {
            phase = transport.ppqPosition / std::max (params.beatsPerCycle, 1.0 / 64.0);
            phase -= std::floor (phase);
        }

        int nextMidi = 0;
        for (int i = 0; i < n; ++i)
        {
            // Triggers
            if (params.trigger == Trigger::midi)
            {
                while (nextMidi < numMidi && midiNoteOnOffsets[nextMidi] <= i)
                {
                    if (midiNoteOnOffsets[nextMidi] == i)
                        retrigger();
                    ++nextMidi;
                }
            }
            else if (params.trigger == Trigger::sidechain)
            {
                float sc = 0.0f;
                for (int c = 0; c < numChannels; ++c)
                    sc = std::max (sc, std::fabs (sidechain != nullptr ? sidechain[c][i] : io[c][i]));
                scEnvelope += (sc > scEnvelope ? scAttack : scRelease) * (sc - scEnvelope);
                const float thr = std::pow (10.0f, params.sidechainThresholdDb / 20.0f);
                const bool above = scEnvelope > thr;
                if (above && ! scAbove && samplesSinceTrigger >= retriggerHoldoff)
                    retrigger();
                scAbove = above;
                ++samplesSinceTrigger;
            }

            // Phase for this sample
            double ph;
            if (params.trigger == Trigger::tempo)
            {
                ph = phase + params.offset;
                ph -= std::floor (ph);
                phase += phaseStep;
                if (phase >= 1.0)
                    phase -= 1.0;
            }
            else
            {
                ph = std::min (oneShotPhase, 1.0);
                if (oneShotPhase < 1.0)
                    oneShotPhase += phaseStep;
            }

            float gain = shape.gainAt ((float) ph);
            if (ph >= 1.0 && params.trigger != Trigger::tempo)
                gain = 1.0f;

            // A phase discontinuity (retrigger, transport jump, offset change) is crossfaded from the
            // last output gain so the curve never steps.
            if (params.trigger == Trigger::tempo && lastPhase >= 0.0)
            {
                double expected = lastPhase + phaseStep;
                expected -= std::floor (expected);
                const double diff = std::fabs (ph - expected);
                if (diff > phaseStep * 4.0 + 1e-9 && diff < 1.0 - phaseStep * 4.0)
                    startFade();
            }
            if (pendingRetrigger)
            {
                pendingRetrigger = false;
                startFade();
            }
            if (fade < 1.0f)
            {
                fade = std::min (1.0f, fade + crossfadeStep);
                gain = fadeFrom + (gain - fadeFrom) * fade;
            }

            lastPhase = ph;
            lastGain = gain;

            // Apply
            if (params.band == Band::full)
            {
                for (int c = 0; c < numChannels; ++c)
                    io[c][i] *= gain;
            }
            else
            {
                for (int c = 0; c < numChannels; ++c)
                {
                    const float x = io[c][i];
                    const float lo = lowFilters[(size_t) c][1].process (lowFilters[(size_t) c][0].process (x));
                    const float hi = highFilters[(size_t) c][1].process (highFilters[(size_t) c][0].process (x));
                    io[c][i] = params.band == Band::low ? lo * gain + hi : lo + hi * gain;
                }
            }
        }

        if (params.trigger == Trigger::tempo)
            freePhase = phase;
    }

    // Restart the one-shot curve (MIDI / sidechain). Public so hosts without MIDI buffers can test it.
    void retrigger()
    {
        oneShotPhase = 0.0;
        samplesSinceTrigger = 0;
        pendingRetrigger = true;
    }

private:
    void startFade()
    {
        fadeFrom = lastGain;
        fade = 0.0f;
    }

    void updateCrossover()
    {
        // LR4 = two cascaded 2nd-order Butterworth sections (Q = 1/sqrt 2).
        const double q = 1.0 / std::sqrt (2.0);
        const auto lp = dsp::biquad::lowPass (params.crossoverHz, q, sampleRate);
        const auto hp = dsp::biquad::highPass (params.crossoverHz, q, sampleRate);
        for (auto& ch : lowFilters) for (auto& f : ch) f.setCoeffs (lp);
        for (auto& ch : highFilters) for (auto& f : ch) f.setCoeffs (hp);
    }

    DuckParams params;
    CurveShape shape { params };
    double sampleRate = 48000.0;
    int channels = 2;

    std::array<std::array<dsp::Biquad, 2>, maxChannels> lowFilters, highFilters;

    double freePhase = 0.0;
    double oneShotPhase = 1.0;
    double lastPhase = -1.0;
    float lastGain = 1.0f;

    float fade = 1.0f, fadeFrom = 1.0f, crossfadeStep = 1.0f;
    bool pendingRetrigger = false;

    float scEnvelope = 0.0f, scAttack = 1.0f, scRelease = 1.0f;
    bool scAbove = false;
    int samplesSinceTrigger = 0, retriggerHoldoff = 0;
};

} // namespace rommuser::duckready
