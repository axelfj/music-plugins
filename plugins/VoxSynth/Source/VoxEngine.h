// Vox Synth DSP engine: harmonies, vocoder / talkbox and robot voice from one monophonic voice.
//
// Pure C++ (no JUCE) so it is unit-tested headless in tests/.
//
// All three modes share the YIN tracker and the pulse-centred TD-PSOLA from shared/dsp/Psola.h.
//   Harmony: up to four extra voices, each an interval in scale degrees (follows the key / scale),
//            in semitones, or the k-th held MIDI note; per voice detune, pan, level and formant.
//   Vocoder: a bank of log-spaced band-pass filters measures the voice's spectral envelope and
//            imposes it on a carrier: internal saw / pulse / noise (following the voice pitch, the
//            held MIDI notes or a fixed note) or the external sidechain. "Talkbox" is this with the
//            saw carrier following the voice.
//   Robot:   the voice is forced to one note (hard), with an independent formant shift (chipmunk /
//            monster).
// The dry path is delayed by the analyzer latency so everything stays aligned; with no voices and
// mix 0 the output is the input.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../../../shared/dsp/Biquad.h"
#include "../../../shared/dsp/Psola.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace rommuser::voxsynth
{

using dsp::psola::Scale;
using dsp::psola::hzToMidi;
using dsp::psola::midiToHz;
using dsp::psola::maxChannels;

enum class Mode { harmony = 0, vocoder, robot };
enum class IntervalMode { scale = 0, semitones, midi };
enum class Carrier { saw = 0, pulse, noise, external };

inline constexpr int maxVoices = 4;
inline constexpr int maxBands = 32;

struct VoiceParams
{
    bool enabled = false;
    int degrees = 2;               // -7 .. 7 scale steps (IntervalMode::scale)
    float semitones = 4.0f;        // -24 .. 24 (IntervalMode::semitones)
    float detuneCents = 0.0f;      // -50 .. 50
    float pan = 0.0f;              // -1 .. 1
    float levelDb = -6.0f;         // -40 .. 6
    float formantSemis = 0.0f;     // -12 .. 12
};

struct VoxParams
{
    Mode mode = Mode::harmony;
    float minHz = 60.0f;
    int key = 0;
    Scale scale = Scale::major;
    uint16_t customMask = 0x0FFF;
    IntervalMode intervalMode = IntervalMode::scale;
    std::array<VoiceParams, maxVoices> voices {};
    float glideMs = 20.0f;         // smoothing of the harmony shifts

    // Vocoder
    Carrier carrier = Carrier::saw;
    bool carrierFollowsPitch = true;
    int carrierNote = 48;          // C3 when not following and no MIDI held
    int bands = 20;                // 8 .. 32
    float attackMs = 10.0f;        // 1 .. 50
    float releaseMs = 60.0f;       // 5 .. 500
    float sibilance = 0.3f;        // 0 .. 1: unvoiced hiss passed to the output
    float carrierDetuneCents = 0.0f;

    // Robot
    int robotNote = 48;            // C3
    float robotFormantSemis = 0.0f;

    float dryDb = 0.0f;            // -inf (< -59) .. 6
    float mix = 1.0f;              // wet amount
    float outputDb = 0.0f;
};

struct VoxMeters
{
    bool voiced = false;
    float inputHz = 0.0f;
    float detectedMidi = 0.0f;
    std::array<float, maxVoices> voiceMidi {};   // 0 when off
    float carrierHz = 0.0f;
    std::array<float, maxBands> bandLevels {};    // vocoder envelope (0 .. 1)
    float outputPeak = 0.0f;
};

// Differentiated parabolic wave sawtooth: cheap and much less aliasing than a naive saw.
struct DpwSaw
{
    double phase = 0.0;
    float last = 0.0f;
    inline float process (double inc)
    {
        phase += inc;
        if (phase >= 1.0) phase -= 1.0;
        const float bip = (float) (2.0 * phase - 1.0);
        const float sq = bip * bip;
        const float out = (sq - last) / (float) std::max (4.0 * inc, 1e-6);
        last = sq;
        return out;
    }
    void reset() { phase = 0.0; last = 0.0f; }
};

class VoxEngine
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
        for (auto& v : voices) v.reset();
        for (auto& s : shifts) s = 0.0f;
        for (auto& t : shiftTargets) t = 0.0f;
        robotShift = 0.0f;
        for (auto& o : oscillators) o.reset();
        for (auto& e : envelopes) e = 0.0f;
        for (auto& f : modFilters) f.reset();
        for (auto& f : carFilters) f.reset();
        for (auto& f : modFilters2) f.reset();
        for (auto& f : carFilters2) f.reset();
        sibilanceEnv = 0.0f;
        sibilanceFilter.reset();
        carrierHz = midiToHz (params.carrierNote);
        heldNotes.clear();
        meters = {};
        rng = 4242u;
    }

    void setParams (const VoxParams& p, bool immediate = false)
    {
        (void) immediate;
        const bool rangeChanged = std::fabs (p.minHz - params.minHz) > 0.01f;
        const bool bandsChanged = p.bands != params.bands;
        params = p;
        params.bands = std::clamp (params.bands, 8, maxBands);
        if (rangeChanged) configure (params.minHz);
        else if (bandsChanged) designBands();
        const float atk = (float) (1.0 - std::exp (-1.0 / (std::max (0.5f, params.attackMs) * 0.001 * sr)));
        const float rel = (float) (1.0 - std::exp (-1.0 / (std::max (1.0f, params.releaseMs) * 0.001 * sr)));
        envAttack = atk;
        envRelease = rel;
        hopGlide = params.glideMs <= 0.0f ? 1.0f : (float) (1.0 - std::exp (-((double) analyzer.getHop() / sr) / (params.glideMs * 0.001)));
    }

    const VoxParams& getParams() const { return params; }
    int getLatencySamples() const { return analyzer.getLatencySamples(); }
    const VoxMeters& getMeters() const { return meters; }

    void noteOn (int note)
    {
        if (std::find (heldNotes.begin(), heldNotes.end(), note) == heldNotes.end())
        {
            heldNotes.push_back (note);
            std::sort (heldNotes.begin(), heldNotes.end());
        }
    }
    void noteOff (int note) { heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end()); }
    void allNotesOff() { heldNotes.clear(); }
    bool hasHeldNotes() const { return ! heldNotes.empty(); }

    // sidechain may be null (external carrier then falls back to the saw).
    void process (float* const* io, int numChannels, int n, const float* const* sidechain = nullptr, int sidechainChannels = 0)
    {
        const int nch = std::clamp (numChannels, 1, channels);
        const int latency = analyzer.getLatencySamples();
        const float dryGain = params.dryDb < -59.0f ? 0.0f : std::pow (10.0f, std::clamp (params.dryDb, -60.0f, 6.0f) / 20.0f);
        const float outGain = std::pow (10.0f, std::clamp (params.outputDb, -24.0f, 12.0f) / 20.0f);
        const float mix = std::clamp (params.mix, 0.0f, 1.0f);
        float peak = 0.0f;

        for (int i = 0; i < n; ++i)
        {
            analyzer.push (io, nch, i);
            if (analyzer.hopDue())
                analyse (analyzer.analyse());
            const int64_t outTime = analyzer.getInputCount() - 1;

            float wet[maxChannels] = { 0.0f, 0.0f };
            if (params.mode == Mode::harmony)
                renderHarmony (nch, wet);
            else if (params.mode == Mode::robot)
                renderRobot (nch, wet);
            else
                renderVocoder (nch, wet, outTime, sidechain, sidechainChannels, i);

            for (int c = 0; c < nch; ++c)
            {
                const float dry = analyzer.read (c, outTime - latency);
                const float y = (dry * dryGain + wet[c] * mix) * outGain;
                io[c][i] = y;
                peak = std::max (peak, std::fabs (y));
            }
        }
        meters.outputPeak = peak;
    }

private:
    void configure (float minHz)
    {
        analyzer.prepare (sr, maxBlock, channels, minHz);
        for (auto& v : voices) v.prepare (analyzer, maxBlock, channels);
        robotVoice.prepare (analyzer, maxBlock, channels);
        designBands();
        reset();
    }

    void designBands()
    {
        const int nb = std::clamp (params.bands, 8, maxBands);
        const double lo = 100.0, hi = std::min (8000.0, sr * 0.4);
        const double ratio = std::pow (hi / lo, 1.0 / (nb - 1));
        // Two cascaded 2nd-order band-passes per band (24 dB/oct skirts) so neighbouring bands do not
        // smear the envelope; the cascade narrows the band, so widen each section a little.
        const double q = 0.75 * std::sqrt (ratio) / (ratio - 1.0);
        for (int b = 0; b < nb; ++b)
        {
            const double fc = lo * std::pow (ratio, b);
            bandCoeffs[(size_t) b] = dsp::biquad::bandPass (fc, q, sr);
            modFilters[(size_t) b].setCoeffs (bandCoeffs[(size_t) b]);
            carFilters[(size_t) b].setCoeffs (bandCoeffs[(size_t) b]);
            modFilters2[(size_t) b].setCoeffs (bandCoeffs[(size_t) b]);
            carFilters2[(size_t) b].setCoeffs (bandCoeffs[(size_t) b]);
            bandHz[(size_t) b] = (float) fc;
        }
        sibilanceFilter.setCoeffs (dsp::biquad::highPass (std::min (5000.0, sr * 0.4), 0.707, sr));
    }

    void analyse (const dsp::psola::PitchEstimate& est)
    {
        meters.voiced = est.voiced;
        if (est.voiced)
        {
            meters.inputHz = est.hz;
            meters.detectedMidi = (float) hzToMidi (est.hz);
        }
        const double detected = est.voiced ? hzToMidi (est.hz) : 0.0;
        const uint16_t mask = dsp::psola::scaleMask (params.scale, params.customMask);

        // Harmony shifts.
        for (int k = 0; k < maxVoices; ++k)
        {
            const auto& v = params.voices[(size_t) k];
            float target = 0.0f;
            if (v.enabled && est.voiced)
            {
                if (params.intervalMode == IntervalMode::midi)
                {
                    if ((int) heldNotes.size() > k)
                        target = (float) (heldNotes[(size_t) k] - detected);
                }
                else if (params.intervalMode == IntervalMode::semitones)
                {
                    target = std::clamp (v.semitones, -24.0f, 24.0f);
                }
                else
                {
                    const int base = (int) dsp::psola::snapToScale (detected, params.key, mask);
                    const int note = dsp::psola::scaleStep (base, std::clamp (v.degrees, -7, 7), params.key, mask);
                    target = (float) (note - base);   // relative to the snapped note: the voice's own vibrato is kept
                }
                target += v.detuneCents / 100.0f;
                meters.voiceMidi[(size_t) k] = (float) detected + target;
            }
            else
                meters.voiceMidi[(size_t) k] = 0.0f;
            shiftTargets[(size_t) k] = target;
            if (params.intervalMode == IntervalMode::midi || hopGlide >= 1.0f)
                shifts[(size_t) k] = target;
            else
                shifts[(size_t) k] += (target - shifts[(size_t) k]) * hopGlide;
        }

        // Robot.
        robotShift = est.voiced ? (float) (params.robotNote - detected) : 0.0f;

        // Vocoder carrier pitch.
        if (! heldNotes.empty())
            carrierHz = 0.0;   // per-note oscillators below
        else if (params.carrierFollowsPitch && est.voiced)
            carrierHz = est.hz;
        else if (! params.carrierFollowsPitch)
            carrierHz = midiToHz (params.carrierNote);
        meters.carrierHz = (float) carrierHz;
    }

    void renderHarmony (int nch, float* wet)
    {
        float buf[maxChannels] {};
        for (int k = 0; k < maxVoices; ++k)
        {
            const auto& v = params.voices[(size_t) k];
            if (! v.enabled) continue;
            const float ratio = std::pow (2.0f, shifts[(size_t) k] / 12.0f);
            const float formant = std::pow (2.0f, std::clamp (v.formantSemis, -12.0f, 12.0f) / 12.0f);
            voices[(size_t) k].renderSample (analyzer, nch, ratio, formant, buf);
            const float gain = std::pow (10.0f, std::clamp (v.levelDb, -40.0f, 6.0f) / 20.0f);
            if (nch == 1)
                wet[0] += buf[0] * gain;
            else
            {
                // Constant-power pan of the (mono-summed) voice.
                const float mono = 0.5f * (buf[0] + buf[1]);
                const float angle = (std::clamp (v.pan, -1.0f, 1.0f) + 1.0f) * 0.25f * 3.14159265f;
                wet[0] += mono * gain * std::cos (angle);
                wet[1] += mono * gain * std::sin (angle);
            }
        }
    }

    void renderRobot (int nch, float* wet)
    {
        float buf[maxChannels] {};
        const float ratio = std::pow (2.0f, robotShift / 12.0f);
        const float formant = std::pow (2.0f, std::clamp (params.robotFormantSemis, -12.0f, 12.0f) / 12.0f);
        robotVoice.renderSample (analyzer, nch, ratio, formant, buf);
        for (int c = 0; c < nch; ++c) wet[c] = buf[c];
    }

    void renderVocoder (int nch, float* wet, int64_t outTime, const float* const* sidechain, int scChannels, int i)
    {
        const int latency = analyzer.getLatencySamples();
        // Modulator: the voice, delayed like the dry path.
        const float mod = analyzer.readMono (outTime - latency);

        // Carrier sample.
        float car = 0.0f;
        if (params.carrier == Carrier::external && sidechain != nullptr && scChannels > 0)
        {
            for (int c = 0; c < scChannels; ++c) car += sidechain[c][i];
            car /= (float) scChannels;
        }
        else if (params.carrier == Carrier::noise)
        {
            rng = rng * 1664525u + 1013904223u;
            car = (float) (rng >> 8) / 8388608.0f - 1.0f;
        }
        else
        {
            const float detune = std::pow (2.0f, params.carrierDetuneCents / 1200.0f);
            int count = 0;
            if (! heldNotes.empty())
            {
                for (int k = 0; k < (int) std::min<size_t> (heldNotes.size(), maxVoices); ++k)
                    car += oscillatorSample ((size_t) k, midiToHz (heldNotes[(size_t) k]) * detune), ++count;
            }
            else if (carrierHz > 0.0)
            {
                car = oscillatorSample (0, carrierHz * detune);
                count = 1;
            }
            if (count > 1) car /= (float) count;
        }

        // Band bank: carrier bands weighted by the modulator band envelopes.
        const int nb = std::clamp (params.bands, 8, maxBands);
        float out = 0.0f;
        for (int b = 0; b < nb; ++b)
        {
            const float m = std::fabs (modFilters2[(size_t) b].process (modFilters[(size_t) b].process (mod)));
            float& env = envelopes[(size_t) b];
            env += (m > env ? envAttack : envRelease) * (m - env);
            out += carFilters2[(size_t) b].process (carFilters[(size_t) b].process (car)) * env;
        }
        // Sibilance: the voice's own hiss above 5 kHz, gated by its envelope.
        if (params.sibilance > 0.0f)
        {
            const float hiss = sibilanceFilter.process (mod);
            const float level = std::fabs (hiss);
            sibilanceEnv += (level > sibilanceEnv ? envAttack : envRelease) * (level - sibilanceEnv);
            out += hiss * params.sibilance;
        }
        out *= 2.0f;   // band-pass bank gain make-up
        for (int c = 0; c < nch; ++c) wet[c] = out;
        if ((outTime & 1023) == 0)
            for (int b = 0; b < nb; ++b) meters.bandLevels[(size_t) b] = envelopes[(size_t) b];
    }

    inline float oscillatorSample (size_t k, double hz)
    {
        const double inc = std::clamp (hz, 20.0, sr * 0.45) / sr;
        const float saw = oscillators[k].process (inc);
        if (params.carrier == Carrier::pulse)
        {
            // Pulse = saw minus a half-period-shifted saw.
            DpwSaw& o2 = oscillators2[k];
            if (! o2Primed[k]) { o2.phase = oscillators[k].phase + 0.5; if (o2.phase >= 1.0) o2.phase -= 1.0; o2Primed[k] = true; }
            return 0.5f * (saw - o2.process (inc));
        }
        return saw;
    }

    double sr = 48000.0;
    int maxBlock = 512, channels = 2;
    VoxParams params;
    VoxMeters meters;
    dsp::psola::PsolaAnalyzer analyzer;
    std::array<dsp::psola::PsolaVoice, maxVoices> voices;
    dsp::psola::PsolaVoice robotVoice;
    std::array<float, maxVoices> shifts {}, shiftTargets {};
    float robotShift = 0.0f, hopGlide = 1.0f;

    std::array<DpwSaw, maxVoices> oscillators, oscillators2;
    std::array<bool, maxVoices> o2Primed {};
    double carrierHz = 130.81;
    std::array<dsp::BiquadCoeffs, maxBands> bandCoeffs;
    std::array<float, maxBands> bandHz {};
    std::array<dsp::Biquad, maxBands> modFilters, carFilters, modFilters2, carFilters2;
    std::array<float, maxBands> envelopes {};
    float envAttack = 0.01f, envRelease = 0.001f;
    dsp::Biquad sibilanceFilter;
    float sibilanceEnv = 0.0f;
    uint32_t rng = 4242u;
    std::vector<int> heldNotes;
};

} // namespace rommuser::voxsynth
