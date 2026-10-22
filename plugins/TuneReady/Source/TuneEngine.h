// Tune Ready DSP engine: real-time pitch correction for monophonic voices.
//
// Pure C++ (no JUCE) so it is unit-tested headless in tests/.
//
// Detection and resynthesis come from shared/dsp/Psola.h (YIN + pulse-centred TD-PSOLA, formants
// preserved). This file adds the musical part: the detected note is snapped to the nearest note of
// the chosen key / scale (or to a held MIDI note) and the shift glides towards it at the chosen
// speed (0 ms = hard tune); Fixed mode transposes by a constant with an independent formant shift.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../../../shared/dsp/Psola.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace rommuser::tuneready
{

using dsp::psola::PitchDetector;
using dsp::psola::PitchEstimate;
using dsp::psola::hzToMidi;
using dsp::psola::midiToHz;
using dsp::psola::maxChannels;

enum class Mode { correct = 0, fixed };

using dsp::psola::scaleMask;
using dsp::psola::snapToScale;
using dsp::psola::Scale;

struct TuneParams
{
    Mode mode = Mode::correct;
    int key = 0;                    // 0 = C .. 11 = B
    Scale scale = Scale::chromatic;
    uint16_t customMask = 0x0FFF;   // bit n = pitch class n (relative to key) allowed
    float speedMs = 20.0f;          // 0 .. 400: time constant of the correction (0 = hard)
    float amount = 1.0f;            // 0 .. 1: how much of the correction to apply
    float humanizeCents = 0.0f;     // 0 .. 50: slow random drift of the target
    float minHz = 60.0f;            // 40 .. 120: lowest pitch to track (sets window and latency)
    float fixedSemis = 0.0f;        // -24 .. 24: shift in Fixed mode
    float formantSemis = 0.0f;      // -12 .. 12: formant shift (both modes)
    bool midiPriority = true;       // held MIDI notes override the scale
    float mix = 1.0f;
    float outputDb = 0.0f;
};

struct TuneMeters
{
    bool voiced = false;
    float inputHz = 0.0f;
    float detectedMidi = 0.0f;
    float targetMidi = 0.0f;
    float shiftSemis = 0.0f;        // applied right now
    float confidence = 0.0f;
};

class TuneEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize, int numChannels)
    {
        sr = sampleRate;
        maxBlock = std::max (1, maxBlockSize);
        channels = std::clamp (numChannels, 1, maxChannels);
        configure (params.minHz);
    }

    void reset()
    {
        analyzer.reset();
        voice.reset();
        shift = shiftTarget = 0.0f;
        humanOffset = 0.0f;
        meters = {};
        heldNotes.clear();
    }

    void setParams (const TuneParams& p, bool immediate = false)
    {
        (void) immediate;
        const bool rangeChanged = std::fabs (p.minHz - params.minHz) > 0.01f;
        params = p;
        if (rangeChanged)
            configure (p.minHz);
    }

    const TuneParams& getParams() const { return params; }
    int getLatencySamples() const { return analyzer.getLatencySamples(); }
    const TuneMeters& getMeters() const { return meters; }

    void noteOn (int note) { if (std::find (heldNotes.begin(), heldNotes.end(), note) == heldNotes.end()) heldNotes.push_back (note); }
    void noteOff (int note) { heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end()); }
    void allNotesOff() { heldNotes.clear(); }
    bool hasHeldNotes() const { return ! heldNotes.empty(); }

    void process (float* const* io, int numChannels, int n)
    {
        const int nch = std::clamp (numChannels, 1, channels);
        const float formantRatio = std::pow (2.0f, std::clamp (params.formantSemis, -12.0f, 12.0f) / 12.0f);
        const float outGain = std::pow (10.0f, std::clamp (params.outputDb, -24.0f, 12.0f) / 20.0f);
        const int latency = analyzer.getLatencySamples();
        float wet[maxChannels] {};

        for (int i = 0; i < n; ++i)
        {
            analyzer.push (io, nch, i);
            if (analyzer.hopDue())
                analyse (analyzer.analyse());

            voice.renderSample (analyzer, nch, std::pow (2.0f, shift / 12.0f), formantRatio, wet);

            const int64_t outTime = analyzer.getInputCount() - 1;
            for (int c = 0; c < nch; ++c)
            {
                const float dry = analyzer.read (c, outTime - latency);
                const float y = params.mix >= 1.0f ? wet[c] : params.mix <= 0.0f ? dry : dry + (wet[c] - dry) * params.mix;
                io[c][i] = y * outGain;
            }
        }
    }

private:
    void configure (float minHz)
    {
        analyzer.prepare (sr, maxBlock, channels, minHz);
        voice.prepare (analyzer, maxBlock, channels);
        hopSeconds = (float) analyzer.getHop() / (float) sr;
        reset();
    }

    void analyse (const PitchEstimate& est)
    {
        meters.voiced = est.voiced;
        meters.confidence = est.confidence;
        if (est.voiced)
        {
            meters.inputHz = est.hz;
            meters.detectedMidi = (float) hzToMidi (est.hz);
        }

        float target = 0.0f;
        if (params.mode == Mode::fixed)
        {
            target = std::clamp (params.fixedSemis, -24.0f, 24.0f);
            meters.targetMidi = meters.detectedMidi + target;
        }
        else if (est.voiced)
        {
            const double detected = hzToMidi (est.hz);
            double targetMidi;
            if (params.midiPriority && ! heldNotes.empty())
            {
                targetMidi = heldNotes.front();
                for (int note : heldNotes)
                    if (std::fabs (note - detected) < std::fabs (targetMidi - detected)) targetMidi = note;
            }
            else
            {
                targetMidi = snapToScale (detected, params.key, scaleMask (params.scale, params.customMask));
            }
            if (params.humanizeCents > 0.0f)
            {
                rng = rng * 1664525u + 1013904223u;
                const float r = (float) (rng >> 8) / 8388608.0f - 1.0f;
                humanOffset += 0.1f * r * params.humanizeCents / 100.0f;
                humanOffset = std::clamp (humanOffset, -params.humanizeCents / 100.0f, params.humanizeCents / 100.0f);
                humanOffset *= 0.98f;
                targetMidi += humanOffset;
            }
            target = (float) (targetMidi - detected) * std::clamp (params.amount, 0.0f, 1.0f);
            meters.targetMidi = (float) targetMidi;
        }
        shiftTarget = target;
        if (params.speedMs <= 0.0f || params.mode == Mode::fixed)
            shift = shiftTarget;
        else
            shift += (shiftTarget - shift) * (1.0f - std::exp (-hopSeconds / (params.speedMs * 0.001f)));
        meters.shiftSemis = shift;
    }

    double sr = 48000.0;
    int maxBlock = 512, channels = 2;
    TuneParams params;
    TuneMeters meters;
    dsp::psola::PsolaAnalyzer analyzer;
    dsp::psola::PsolaVoice voice;
    float hopSeconds = 0.005f;
    float shift = 0.0f, shiftTarget = 0.0f, humanOffset = 0.0f;
    uint32_t rng = 777u;
    std::vector<int> heldNotes;
};

} // namespace rommuser::tuneready
