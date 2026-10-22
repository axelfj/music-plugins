// Stutter Ready DSP engine: tempo-synced live capture and repeat ("stutter edit") with gestures.
//
// Pure C++ (no JUCE) so it is unit-tested headless in tests/.
//
// The input is recorded continuously into a ring buffer. When a gesture starts (MIDI note, UI
// button or the step sequencer), the first division of audio plays live while it is recorded and is
// then repeated from the same start position: repetition k starts exactly k * L samples after the
// gesture start, where L = division in samples at the host tempo (sample-aligned with the DAW clock
// when the trigger is quantised to the grid). Each repetition ends with a short crossfade into the
// audio that preceded the slice, so the jump back to the slice start is click-free.
//
// Gestures ramp over a configurable number of beats: division start -> end (buzz rolls), pitch
// (read rate), tape stop (rate to zero), and chained effects: gate, filter sweep, pan, bitcrush and
// a synced echo of the stutter.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../../../shared/dsp/Biquad.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace rommuser::stutterready
{

constexpr int maxChannels = 2;
constexpr int numGestures = 8;
constexpr int stepsPerBar = 16;      // 1/16 steps
constexpr int maxBars = 4;
constexpr int maxSteps = stepsPerBar * maxBars;
constexpr int numDivisions = 6;      // 1/4 .. 1/128

// Division lengths in beats (quarter notes).
constexpr double divisionBeats[numDivisions] = { 1.0, 0.5, 0.25, 0.125, 0.0625, 0.03125 };
constexpr const char* divisionNames[numDivisions] = { "1/4", "1/8", "1/16", "1/32", "1/64", "1/128" };

enum class FilterType { off = 0, lowPass, highPass };

struct Gesture
{
    int divStart = 2;             // division index at the start of the gesture
    int divEnd = 2;               // division index reached after rampBeats (equal = no ramp)
    float pitchSemis = 0.0f;      // -24 .. 24: read-rate ramp reached after rampBeats (0 = none)
    bool reverse = false;         // play each repetition backwards
    bool tapeStop = false;        // read rate 1 -> 0 over rampBeats, then silence
    float gate = 0.0f;            // 0 .. 1: depth of a 50 % gate at the current division
    FilterType filter = FilterType::off;
    float filterAmount = 0.0f;    // 0 .. 1: how far the sweep goes over rampBeats
    float pan = 0.0f;             // 0 .. 1: alternate repetitions left / right by this much
    float crush = 0.0f;           // 0 .. 1: bit depth and sample-rate reduction
    float echo = 0.0f;            // 0 .. 1: feedback of a one-division echo of the stutter
};

struct StutterParams
{
    float rampBeats = 2.0f;       // 0.25 .. 16: length of all ramps
    int quantize = 2;             // 0 = off, 1 = 1/4, 2 = 1/8, 3 = 1/16: trigger snapping to the grid
    bool seqEnabled = false;
    int seqBars = 1;              // 1, 2 or 4
    std::array<int8_t, maxSteps> steps {};   // -1 = off, else gesture index
    std::array<Gesture, numGestures> gestures {};
    float mix = 1.0f;
    float outputDb = 0.0f;
    double fallbackBpm = 120.0;   // used when the host gives no tempo

    StutterParams() { steps.fill (-1); }
};

struct Transport
{
    bool valid = false;
    bool playing = false;
    double ppqPosition = 0.0;     // at the first sample of the block
    double bpm = 120.0;
};

struct StutterMeters
{
    bool active = false;
    int gesture = -1;
    float progress = 0.0f;        // 0 .. 1 along the ramp
    float divisionBeatsNow = 0.0f;
    int step = -1;                // current sequencer step when playing, else -1
    float wetPeak = 0.0f;
};

inline const Gesture& builtInGesture (int index)
{
    static const std::array<Gesture, numGestures> table = [] {
        std::array<Gesture, numGestures> g {};
        g[0] = {};                                                                 // STUTTER 1/16
        g[1].divStart = 1; g[1].divEnd = 5;                                        // BUZZ 1/8 -> 1/128
        g[2].pitchSemis = 12.0f;                                                   // PITCH UP
        g[3].pitchSemis = -12.0f;                                                  // PITCH DOWN
        g[4].divStart = g[4].divEnd = 1; g[4].reverse = true;                      // REVERSE 1/8
        g[5].divStart = g[5].divEnd = 0; g[5].tapeStop = true;                     // TAPE STOP
        g[6].gate = 1.0f; g[6].filter = FilterType::lowPass; g[6].filterAmount = 0.6f;   // GATE
        g[7].divStart = g[7].divEnd = 3; g[7].filter = FilterType::highPass; g[7].filterAmount = 1.0f; g[7].pan = 0.8f;  // FILTER
        return g;
    }();
    return table[(size_t) std::clamp (index, 0, numGestures - 1)];
}

class StutterEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize, int numChannels)
    {
        sr = sampleRate;
        (void) maxBlockSize;
        channels = std::clamp (numChannels, 1, maxChannels);
        ringBits = 1;
        while ((1 << ringBits) < (int) std::ceil (sampleRate * 16.0)) ++ringBits;
        ringMask = (1u << ringBits) - 1u;
        for (int c = 0; c < maxChannels; ++c)
        {
            ring[(size_t) c].assign ((size_t) 1 << ringBits, 0.0f);
            echoBuf[(size_t) c].assign ((size_t) std::ceil (sampleRate * 3.1), 0.0f);
        }
        fadeSamples = std::max (8, (int) (sampleRate * 0.005));
        reset();
    }

    void reset()
    {
        for (auto& r : ring) std::fill (r.begin(), r.end(), 0.0f);
        for (auto& e : echoBuf) std::fill (e.begin(), e.end(), 0.0f);
        for (auto& f : filters) f.reset();
        inputCount = 0;
        active = false;
        pendingStart = -1;
        pendingGesture = -1;
        releaseFade = 0;
        echoWrite = 0;
        lastStep = -1;
        held.fill (false);
        uiHeld = -1;
        meters = {};
    }

    void setParams (const StutterParams& p) { params = p; }
    const StutterParams& getParams() const { return params; }
    const StutterMeters& getMeters() const { return meters; }

    // Gesture triggers. MIDI: note % 12 < 8 selects the gesture.
    void noteOn (int note) { const int g = note % 12; if (g < numGestures) { held[(size_t) g] = true; requestStart (g); } }
    void noteOff (int note) { const int g = note % 12; if (g < numGestures) { held[(size_t) g] = false; requestStopIfIdle (g); } }
    void allNotesOff() { held.fill (false); uiHeld = -1; requestStopIfIdle (-1); }
    void uiPress (int g) { uiHeld = g; requestStart (g); }
    void uiRelease() { const int g = uiHeld; uiHeld = -1; requestStopIfIdle (g); }

    // Display helpers: the last `n` recorded samples (mono mix) ending at the write position.
    int64_t getInputCount() const { return inputCount; }
    float readMono (int64_t absIndex) const
    {
        if (absIndex < 0 || absIndex >= inputCount) return 0.0f;
        float s = 0.0f;
        for (int c = 0; c < channels; ++c) s += ring[(size_t) c][(size_t) ((uint64_t) absIndex & ringMask)];
        return s / (float) channels;
    }
    int64_t getSliceStart() const { return active ? sliceStart : -1; }
    double getSliceLength() const { return active ? currentLength : 0.0; }

    void process (float* const* io, int numChannels, int n, const Transport& transport)
    {
        const int nch = std::clamp (numChannels, 1, channels);
        const double bpm = transport.valid && transport.bpm > 20.0 ? transport.bpm : params.fallbackBpm;
        const double samplesPerBeat = sr * 60.0 / bpm;
        const double ppqPerSample = 1.0 / samplesPerBeat;
        const float outGain = std::pow (10.0f, std::clamp (params.outputDb, -24.0f, 12.0f) / 20.0f);
        const float mix = std::clamp (params.mix, 0.0f, 1.0f);
        float wetPeak = 0.0f;

        for (int i = 0; i < n; ++i)
        {
            const double ppq = transport.ppqPosition + (double) i * ppqPerSample;

            // Sequencer: the current step decides the gesture while the host is playing.
            if (params.seqEnabled && transport.valid && transport.playing)
            {
                const int totalSteps = stepsPerBar * std::clamp (params.seqBars, 1, maxBars);
                const int step = (int) (std::floor (ppq * 4.0 + 1.0e-9)) % totalSteps;
                const int stepIndex = step < 0 ? step + totalSteps : step;
                if (stepIndex != lastStep)
                {
                    lastStep = stepIndex;
                    const int g = params.steps[(size_t) stepIndex];
                    const int prev = seqGesture;
                    seqGesture = g;
                    if (g >= 0 && g != prev)
                        startGesture (g, samplesPerBeat);   // step boundaries are already on the grid
                    else if (g < 0 && prev >= 0 && ! anyHeld())
                        stopGesture();
                }
                meters.step = stepIndex;
            }
            else
            {
                meters.step = -1;
                if (seqGesture >= 0) { seqGesture = -1; lastStep = -1; if (! anyHeld()) stopGesture(); }
            }

            // Quantised manual trigger.
            if (pendingGesture >= 0)
            {
                if (pendingStart < 0)
                {
                    // Decide the start sample now that we know the position.
                    const int q = params.quantize;
                    if (q <= 0 || ! transport.valid || ! transport.playing)
                        pendingStart = inputCount;
                    else
                    {
                        const double grid = q == 1 ? 1.0 : q == 2 ? 0.5 : 0.25;
                        const double next = std::ceil (ppq / grid - 1.0e-6) * grid;
                        const double wait = (next - ppq) * samplesPerBeat;
                        pendingStart = inputCount + (int64_t) std::llround (std::max (0.0, wait));
                    }
                }
                if (inputCount >= pendingStart)
                {
                    startGesture (pendingGesture, samplesPerBeat);
                    pendingGesture = -1;
                    pendingStart = -1;
                }
            }

            // Record.
            const size_t w = (size_t) ((uint64_t) inputCount & ringMask);
            float dry[maxChannels] {};
            for (int c = 0; c < nch; ++c)
            {
                dry[c] = io[c][i];
                if (! active || elapsed < recordLength)
                    ring[(size_t) c][w] = dry[c];
            }
            ++inputCount;

            float wet[maxChannels] {};
            if (active)
            {
                renderActive (wet, nch, samplesPerBeat);
                ++elapsed;
            }
            else
            {
                for (int c = 0; c < nch; ++c) wet[c] = dry[c];
            }

            // Release crossfade back to dry after a gesture ends.
            if (releaseFade > 0)
            {
                const float f = (float) releaseFade / (float) fadeSamples;
                for (int c = 0; c < nch; ++c)
                {
                    wet[c] += (releaseTail[c] - wet[c]) * f;
                    releaseTail[c] *= 0.995f;   // the held sample decays while it fades out
                }
                --releaseFade;
            }

            // Synced echo of the stutter (feedback at one division).
            if (echoLevel > 0.0f || echoActive)
            {
                const size_t len = echoBuf[0].size();
                const size_t rd = (echoWrite + len - (size_t) std::clamp ((int) std::llround (echoLengthSamples), 1, (int) len - 1)) % len;
                bool silent = true;
                for (int c = 0; c < nch; ++c)
                {
                    const float d = echoBuf[(size_t) c][rd];
                    const float in = active ? wet[c] : 0.0f;
                    const float fb = (in + d * echoLevel);
                    echoBuf[(size_t) c][echoWrite] = fb;
                    if (std::fabs (fb) > 1.0e-5f) silent = false;
                    wet[c] += d * echoLevel;
                }
                echoWrite = (echoWrite + 1) % len;
                echoActive = ! silent;
            }

            for (int c = 0; c < nch; ++c)
            {
                const float y = mix >= 1.0f ? wet[c] : dry[c] + (wet[c] - dry[c]) * mix;
                io[c][i] = y * outGain;
                wetPeak = std::max (wetPeak, std::fabs (wet[c]));
            }
        }

        meters.active = active;
        meters.gesture = active ? gestureIndex : -1;
        meters.wetPeak = wetPeak;
    }

private:
    bool anyHeld() const
    {
        if (uiHeld >= 0) return true;
        for (bool h : held) if (h) return true;
        return false;
    }

    void requestStart (int g)
    {
        if (active && gestureIndex == g) return;
        pendingGesture = g;
        pendingStart = -1;
    }

    void requestStopIfIdle (int g)
    {
        if (pendingGesture >= 0 && (g < 0 || pendingGesture == g)) pendingGesture = -1;
        if (anyHeld() || seqGesture >= 0) return;
        if (active && (g < 0 || gestureIndex == g)) stopGesture();
    }

    void startGesture (int g, double samplesPerBeat)
    {
        if (active)
        {
            // Switching gesture: fade the old one out.
            stopGesture();
        }
        gestureIndex = std::clamp (g, 0, numGestures - 1);
        gesture = params.gestures[(size_t) gestureIndex];
        active = true;
        sliceStart = inputCount;
        elapsed = 0;
        readPos = 0.0;
        repetition = 0;
        gateFade = 1.0f;
        crushHold = 0;
        for (auto& f : filters) f.reset();
        for (int c = 0; c < maxChannels; ++c) crushLast[c] = 0.0f;
        echoLevel = std::clamp (gesture.echo, 0.0f, 0.95f);
        if (echoLevel <= 0.0f) echoActive = false;
        lengthStart = divisionBeats[std::clamp (gesture.divStart, 0, numDivisions - 1)] * samplesPerBeat;
        lengthEnd = divisionBeats[std::clamp (gesture.divEnd, 0, numDivisions - 1)] * samplesPerBeat;
        // Record the first pass live, plus what the reverse crossfade reads past the slice end.
        recordLength = (int64_t) std::ceil (std::max (lengthStart, lengthEnd)) + fadeSamples + 4;
        rampSamples = std::max (1.0, (double) std::clamp (params.rampBeats, 0.25f, 16.0f) * samplesPerBeat);
        echoLengthSamples = lengthStart;
        currentLength = lengthStart;
    }

    void stopGesture()
    {
        if (! active) return;
        active = false;
        releaseFade = fadeSamples;
        for (int c = 0; c < maxChannels; ++c) releaseTail[c] = lastWet[c];
    }

    inline float readRing (int c, double pos) const
    {
        // 4-point Hermite around the slice start.
        const double fl = std::floor (pos);
        const float t = (float) (pos - fl);
        const int64_t base = sliceStart + (int64_t) fl;
        auto at = [&] (int64_t idx) { return ring[(size_t) c][(size_t) ((uint64_t) idx & ringMask)]; };
        const float xm1 = at (base - 1), x0 = at (base), x1 = at (base + 1), x2 = at (base + 2);
        if (t < 1.0e-7f) return x0;
        const float c0 = x0, c1 = 0.5f * (x1 - xm1), c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2, c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * t + c2) * t + c1) * t + c0;
    }

    void renderActive (float* wet, int nch, double samplesPerBeat)
    {
        // The first pass plays the slice live while it is recorded, so every ramp starts once the
        // first repetition begins. The length is fixed per repetition (chosen when it starts) so each
        // repeat stays a whole division; between repetitions the division ramps geometrically.
        const double u = std::clamp (((double) elapsed - lengthStart) / rampSamples, 0.0, 1.0);
        meters.progress = (float) u;

        double rate = 1.0;
        float stopGain = 1.0f;
        if (repetition > 0)
        {
            if (std::fabs (gesture.pitchSemis) > 0.01f)
                rate = std::pow (2.0, (double) gesture.pitchSemis * u / 12.0);
            if (gesture.tapeStop)
            {
                rate = std::max (0.0, 1.0 - u);
                stopGain = (float) std::min (1.0, rate * 8.0);     // mute only at the very end
            }
        }

        // Position inside the slice: wrap at the current length and pick the next one.
        if (readPos >= currentLength)
        {
            readPos = std::fmod (readPos, currentLength);
            ++repetition;
            currentLength = std::max (8.0, lengthStart * std::pow (lengthEnd / lengthStart, u));
            if (readPos >= currentLength) readPos = 0.0;
        }
        const double L = currentLength;
        meters.divisionBeatsNow = (float) (L / samplesPerBeat);
        const double fade = std::min (L * 0.25, (double) fadeSamples);
        const double tailStart = L - fade;
        // The first pass always plays forward (it is being recorded); reverse applies to the repeats.
        const bool reversed = gesture.reverse && repetition > 0;
        const double pos = reversed ? (L - readPos) : readPos;
        float sample[maxChannels] {};
        for (int c = 0; c < nch; ++c)
        {
            float s = readRing (c, pos);
            if (readPos > tailStart && fade > 1.0)
            {
                // Crossfade into the audio that preceded the slice so the wrap is continuous.
                const float wgt = (float) ((readPos - tailStart) / fade);
                const double pos2 = reversed ? (L - readPos) + L : readPos - L;
                s = s * (1.0f - wgt) + readRing (c, pos2) * wgt;
            }
            sample[c] = s;
        }
        readPos += rate;

        // Gate: 50 % duty at the current division, with 1 ms edges.
        float gateTarget = 1.0f;
        if (gesture.gate > 0.001f)
        {
            const bool open = readPos < L * 0.5;
            gateTarget = open ? 1.0f : 1.0f - std::clamp (gesture.gate, 0.0f, 1.0f);
        }
        const float gateStep = (float) (1.0 / std::max (8.0, sr * 0.001));
        gateFade += std::clamp (gateTarget - gateFade, -gateStep, gateStep);

        // Filter sweep.
        if (gesture.filter != FilterType::off && gesture.filterAmount > 0.001f)
        {
            const double amount = std::clamp ((double) gesture.filterAmount, 0.0, 1.0) * u;
            double hz;
            if (gesture.filter == FilterType::lowPass) hz = 18000.0 * std::pow (150.0 / 18000.0, amount);
            else hz = 30.0 * std::pow (6000.0 / 30.0, amount);
            hz = std::min (hz, sr * 0.45);
            const auto coeffs = gesture.filter == FilterType::lowPass ? dsp::biquad::lowPass (hz, 0.9, sr) : dsp::biquad::highPass (hz, 0.9, sr);
            for (int c = 0; c < nch; ++c)
            {
                filters[(size_t) c].setCoeffs (coeffs);
                sample[c] = filters[(size_t) c].process (sample[c]);
            }
        }

        // Bitcrush: bit depth 16 -> 3 and sample-and-hold up to 1/24 of the rate.
        if (gesture.crush > 0.001f)
        {
            const float amt = std::clamp (gesture.crush, 0.0f, 1.0f);
            const int holdLen = 1 + (int) (amt * amt * 23.0f);
            const float bits = 16.0f - 13.0f * amt;
            const float q = std::pow (2.0f, bits - 1.0f);
            if (crushHold <= 0)
            {
                for (int c = 0; c < nch; ++c) crushLast[c] = std::round (sample[c] * q) / q;
                crushHold = holdLen;
            }
            --crushHold;
            for (int c = 0; c < nch; ++c) sample[c] = crushLast[c];
        }

        // Pan: alternate repetitions left / right.
        float gainL = 1.0f, gainR = 1.0f;
        if (gesture.pan > 0.001f && nch == 2)
        {
            const float p = std::clamp (gesture.pan, 0.0f, 1.0f) * ((repetition & 1) ? 1.0f : -1.0f);
            const float angle = (p + 1.0f) * 0.25f * 3.14159265f;
            gainL = std::cos (angle) * 1.41421356f;
            gainR = std::sin (angle) * 1.41421356f;
        }

        for (int c = 0; c < nch; ++c)
        {
            const float g = gateFade * stopGain * (c == 0 ? gainL : gainR);
            wet[c] = sample[c] * g;
            lastWet[c] = wet[c];
        }
    }

    double sr = 48000.0;
    int channels = 2;
    int ringBits = 20;
    uint32_t ringMask = 0;
    std::array<std::vector<float>, maxChannels> ring;
    std::array<std::vector<float>, maxChannels> echoBuf;
    size_t echoWrite = 0;
    double echoLengthSamples = 1.0;
    float echoLevel = 0.0f;
    bool echoActive = false;
    int fadeSamples = 240;

    StutterParams params;
    StutterMeters meters;

    int64_t inputCount = 0;
    bool active = false;
    int gestureIndex = 0;
    Gesture gesture;
    int64_t sliceStart = 0, elapsed = 0, recordLength = 0;
    double lengthStart = 0.0, lengthEnd = 0.0, currentLength = 0.0, rampSamples = 1.0;
    double readPos = 0.0;
    int repetition = 0;
    float gateFade = 1.0f;
    int crushHold = 0;
    float crushLast[maxChannels] {};
    float lastWet[maxChannels] {}, releaseTail[maxChannels] {};
    int releaseFade = 0;
    std::array<dsp::Biquad, maxChannels> filters;

    int pendingGesture = -1;
    int64_t pendingStart = -1;
    std::array<bool, numGestures> held {};
    int uiHeld = -1;
    int seqGesture = -1, lastStep = -1;
};

} // namespace rommuser::stutterready
