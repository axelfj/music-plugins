// Wave Ready: a wavetable / supersaw synthesiser for beginners.
//
// Signal path per voice: osc 1 + osc 2 (band-limited wavetables, up to 7 unison voices each with
// detune and stereo spread, morph between neighbouring waves) + sub + noise -> state-variable filter
// (LP 12/24, HP, BP, with drive, key tracking, envelope and LFO modulation) -> amplitude envelope
// -> pan. 16 voices, or mono / legato with glide. Two ADSRs, two LFOs (free or tempo-synced), a
// fixed modulation set (LFO -> pitch / cutoff / amp / morph, env 2 -> cutoff / pitch, velocity,
// mod wheel, pitch bend). Global: drive, chorus, ping-pong delay, reverb (Space Ready engine),
// master volume and a safety soft clip.
//
// Wavetables are built once from harmonic recipes with an inverse FFT, one table per octave
// (mip levels) so that no harmonic ever lands above Nyquist; the oscillator crossfades between
// the two nearest levels, so sweeps are smooth and aliasing stays below -80 dB.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../../SpaceReady/Source/SpaceEngine.h"
#include "dsp/FFT.h"
#include "dsp/FractionalDelay.h"
#include "dsp/Smoothing.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace rommuser::waveready
{

inline constexpr int maxVoices = 16;
inline constexpr int maxUnison = 7;
inline constexpr int tableSize = 2048;
inline constexpr int numLevels = 11;      // level k keeps harmonics up to 1024 >> k (k = 10: 1 harmonic)
inline constexpr int numWaves = 10;
inline constexpr const char* waveNames[numWaves] = {
    "Sine", "Triangle", "Saw", "Square", "Pulse 25", "Pulse 12", "Organ", "Formant", "Metal", "Digital"
};

// --------------------------------------------------------------------------- wavetables

class WavetableBank
{
public:
    // Returns the process-wide bank, built on first use.
    static const WavetableBank& get()
    {
        static const WavetableBank bank;
        return bank;
    }

    // table[wave][level] has tableSize + 1 samples (last = first, for interpolation).
    const float* table (int wave, int level) const
    {
        return tables[(size_t) std::clamp (wave, 0, numWaves - 1)][(size_t) std::clamp (level, 0, numLevels - 1)].data();
    }

private:
    WavetableBank()
    {
        dsp::FFT fft;
        fft.prepare (11);
        std::vector<std::complex<float>> spec ((size_t) tableSize);
        for (int w = 0; w < numWaves; ++w)
        {
            float norm = 1.0f;
            for (int level = 0; level < numLevels; ++level)
            {
                const int maxHarmonic = (tableSize / 2) >> level;
                std::fill (spec.begin(), spec.end(), std::complex<float> (0.0f, 0.0f));
                for (int n = 1; n <= maxHarmonic && n < tableSize / 2; ++n)
                {
                    float amp = 0.0f, phase = 0.0f;
                    harmonic (w, n, amp, phase);
                    if (std::fabs (amp) < 1e-9f)
                        continue;
                    // A sine of harmonic n: X[n] = -i A/2 (times N because inverse scales by 1/N).
                    // (magnitude times a unit phasor: std::polar needs a non-negative magnitude)
                    const std::complex<float> c = (0.5f * amp * (float) tableSize) * std::polar (1.0f, phase - 1.5707963f);
                    spec[(size_t) n] = c;
                    spec[(size_t) (tableSize - n)] = std::conj (c);
                }
                fft.inverse (spec.data());
                auto& t = tables[(size_t) w][(size_t) level];
                t.resize ((size_t) tableSize + 1);
                for (int i = 0; i < tableSize; ++i) t[(size_t) i] = spec[(size_t) i].real();
                if (level == 0)
                {
                    float peak = 1e-9f;
                    for (int i = 0; i < tableSize; ++i) peak = std::max (peak, std::fabs (t[(size_t) i]));
                    norm = 1.0f / peak;
                }
                for (int i = 0; i < tableSize; ++i) t[(size_t) i] *= norm;
                t[(size_t) tableSize] = t[0];
            }
        }
    }

    static void harmonic (int wave, int n, float& amp, float& phase)
    {
        const float fn = (float) n;
        const bool odd = (n % 2) == 1;
        amp = 0.0f; phase = 0.0f;
        switch (wave)
        {
            case 0: amp = n == 1 ? 1.0f : 0.0f; break;
            case 1:
                if (odd) { amp = 1.0f / (fn * fn); phase = ((n - 1) / 2) % 2 == 1 ? 3.14159265f : 0.0f; }
                break;
            case 2: amp = 1.0f / fn; phase = (n % 2 == 0) ? 3.14159265f : 0.0f; break;
            case 3: if (odd) amp = 1.0f / fn; break;
            case 4: amp = std::fabs (2.0f / (fn * 3.14159265f) * std::sin (fn * 3.14159265f * 0.25f)); break;
            case 5: amp = std::fabs (2.0f / (fn * 3.14159265f) * std::sin (fn * 3.14159265f * 0.125f)); break;
            case 6:
            {
                static constexpr float drawbars[9] = { 1.0f, 0.6f, 0.4f, 0.5f, 0.0f, 0.3f, 0.0f, 0.3f, 0.0f };
                if (n <= 8) amp = drawbars[n];
                break;
            }
            case 7: amp = std::exp (-((fn - 8.0f) * (fn - 8.0f)) / 18.0f) + 0.5f * std::exp (-((fn - 20.0f) * (fn - 20.0f)) / 32.0f)
                          + (n == 1 ? 0.6f : 0.0f);
                    break;
            case 8:
                if (n <= 64) { amp = 1.0f / std::sqrt (fn); phase = std::fmod (fn * fn * 0.37f, 6.2831853f); }
                break;
            case 9:
                if (odd && n <= 41) { amp = 1.0f / std::pow (fn, 0.7f); phase = ((n - 1) / 2) % 2 == 1 ? 3.14159265f : 0.0f; }
                break;
            default: break;
        }
    }

    std::array<std::array<std::vector<float>, numLevels>, numWaves> tables;
};

// --------------------------------------------------------------------------- parameters

enum class FilterMode { lp12, lp24, hp12, bp12 };
enum class LfoShape { sine, triangle, sawDown, square, sampleHold };
enum class NoiseColour { white, pink };

struct OscParams
{
    bool on = true;
    int wave = 2;              // index into waveNames
    float morph = 0.0f;        // 0..1 crossfade towards the next wave
    int octave = 0;            // -2..2
    int semi = 0;              // -12..12
    float fine = 0.0f;         // cents -100..100
    float level = 0.8f;        // 0..1
    float pan = 0.0f;          // -1..1
    int unison = 1;            // 1..7
    float detune = 20.0f;      // cents, total spread between the outer voices
    float width = 0.7f;        // 0..1 stereo spread of the unison voices
    float phase = 0.0f;        // 0..1 start phase (when retrig)
    bool retrig = false;       // false = free-running (random start phase)
};

struct EnvParams
{
    float attackMs = 5.0f, decayMs = 200.0f, sustain = 0.8f, releaseMs = 200.0f;
};

struct LfoParams
{
    LfoShape shape = LfoShape::sine;
    bool sync = false;
    float rateHz = 5.0f;       // 0.01..40
    int division = 4;          // index into lfoDivisions when synced
    float fadeMs = 0.0f;       // 0..5000 fade-in
    bool retrig = true;        // restart per note
};

inline constexpr int numLfoDivisions = 9;
inline constexpr float lfoDivisionBeats[numLfoDivisions] = { 16.0f, 8.0f, 4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 0.125f, 0.0625f };
inline constexpr const char* lfoDivisionNames[numLfoDivisions] = { "4 bars", "2 bars", "1 bar", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64" };

struct WaveParams
{
    OscParams osc1, osc2;
    float subLevel = 0.0f;     // 0..1
    int subOctave = 1;         // 1 or 2 below
    bool subSquare = false;
    float noiseLevel = 0.0f;   // 0..1
    NoiseColour noiseColour = NoiseColour::white;

    FilterMode filterMode = FilterMode::lp24;
    float cutoffHz = 8000.0f;  // 20..20000
    float resonance = 0.2f;    // 0..1
    float filterDrive = 0.0f;  // 0..1
    float keyTrack = 0.0f;     // 0..1 (1 = cutoff follows the note fully)
    float envToCutoff = 0.0f;  // octaves, -5..5
    float velToCutoff = 0.0f;  // octaves at full velocity, 0..5
    float envToPitch = 0.0f;   // semitones, -24..24 (env 2)

    EnvParams ampEnv, modEnv;
    float velToAmp = 0.5f;     // 0..1

    LfoParams lfo1, lfo2;
    float lfo1ToPitch = 0.0f;  // semitones 0..12 (vibrato)
    float lfo1ToCutoff = 0.0f; // octaves -5..5
    float lfo1ToAmp = 0.0f;    // 0..1 tremolo
    float lfo2ToMorph = 0.0f;  // 0..1
    float lfo2ToCutoff = 0.0f; // octaves -5..5
    float lfo2ToPitch = 0.0f;  // semitones 0..12
    float wheelToCutoff = 0.0f;   // octaves 0..5
    float wheelToVibrato = 0.0f;  // semitones 0..2 (adds to lfo1 -> pitch)

    bool mono = false;
    bool legato = true;
    float glideMs = 0.0f;      // 0..2000
    float bendRange = 2.0f;    // semitones
    float tune = 0.0f;         // cents -100..100

    float driveDb = 0.0f;      // 0..36 global drive
    float chorusMix = 0.0f;    // 0..1
    float chorusRateHz = 0.6f;
    float chorusDepth = 0.5f;
    float delayMix = 0.0f;     // 0..1
    bool delaySync = true;
    int delayDivision = 5;     // lfoDivisions index (1/8)
    float delayMs = 375.0f;
    float delayFeedback = 0.4f;
    bool delayPingPong = true;
    float delayToneHz = 6000.0f;
    float reverbMix = 0.0f;    // 0..1
    float reverbDecay = 2.0f;  // seconds
    bool reverbPlate = false;
    float outputDb = -6.0f;
    float bpm = 120.0f;
};

struct WaveMeters
{
    int activeVoices = 0;
    float outputPeak = 0.0f;
    float env1 = 0.0f;         // last voice's amp envelope (display)
    float cutoffHz = 0.0f;     // modulated cutoff of the newest voice
};

// --------------------------------------------------------------------------- helpers

inline double midiToHz (double note) { return 440.0 * std::pow (2.0, (note - 69.0) / 12.0); }

// Exponential ADSR. Attack aims past 1 so it reaches 1 in attackMs; decay and release are the time
// to fall by 60 dB. `isDone()` after the release reaches -80 dB.
class Adsr
{
public:
    void prepare (double sr) { sampleRate = sr; }
    void set (const EnvParams& p)
    {
        attackCoef = 1.0f - std::exp (-std::log (6.0f) / (float) (sampleRate * std::max (0.05f, p.attackMs) * 0.001f));
        decayCoef = std::exp (-6.9078f / (float) (sampleRate * std::max (1.0f, p.decayMs) * 0.001f));
        releaseCoef = std::exp (-6.9078f / (float) (sampleRate * std::max (1.0f, p.releaseMs) * 0.001f));
        sustain = std::clamp (p.sustain, 0.0f, 1.0f);
    }
    void noteOn() { stage = Stage::attack; }
    void noteOff() { if (stage != Stage::idle) stage = Stage::release; }
    void kill() { stage = Stage::idle; level = 0.0f; }
    bool isActive() const { return stage != Stage::idle; }
    bool isReleasing() const { return stage == Stage::release; }
    float next()
    {
        switch (stage)
        {
            case Stage::attack:
                level += (1.2f - level) * attackCoef;
                if (level >= 1.0f) { level = 1.0f; stage = Stage::decay; }
                break;
            case Stage::decay:
                level = sustain + (level - sustain) * decayCoef;
                if (level - sustain < 1e-4f) { level = sustain; stage = Stage::sustain; }
                break;
            case Stage::sustain:
                level = sustain;
                break;
            case Stage::release:
                level *= releaseCoef;
                if (level < 1e-4f) { level = 0.0f; stage = Stage::idle; }
                break;
            case Stage::idle:
                level = 0.0f;
                break;
        }
        return level;
    }
    float getLevel() const { return level; }

private:
    enum class Stage { idle, attack, decay, sustain, release };
    Stage stage = Stage::idle;
    double sampleRate = 48000.0;
    float attackCoef = 0.01f, decayCoef = 0.999f, releaseCoef = 0.999f, sustain = 1.0f, level = 0.0f;
};

class Lfo
{
public:
    void prepare (double sr) { sampleRate = sr; }
    void set (const LfoParams& p, float bpm)
    {
        shape = p.shape;
        const float hz = p.sync ? bpm / 60.0f / lfoDivisionBeats[std::clamp (p.division, 0, numLfoDivisions - 1)]
                                : std::clamp (p.rateHz, 0.01f, 40.0f);
        inc = hz / (float) sampleRate;
        fadeInc = p.fadeMs > 0.0f ? 1.0f / (float) (sampleRate * p.fadeMs * 0.001f) : 1.0f;
    }
    void trigger (float startPhase, uint32_t seed)
    {
        phase = startPhase - std::floor (startPhase);
        fade = fadeInc >= 1.0f ? 1.0f : 0.0f;
        rng = seed | 1u;
        held = nextRandom();
    }
    // -1..1
    float next()
    {
        float v = 0.0f;
        switch (shape)
        {
            case LfoShape::sine:       v = std::sin (6.2831853f * phase); break;
            case LfoShape::triangle:   v = phase < 0.5f ? 4.0f * phase - 1.0f : 3.0f - 4.0f * phase; break;
            case LfoShape::sawDown:    v = 1.0f - 2.0f * phase; break;
            case LfoShape::square:     v = phase < 0.5f ? 1.0f : -1.0f; break;
            case LfoShape::sampleHold: v = held; break;
        }
        phase += inc;
        if (phase >= 1.0f) { phase -= 1.0f; held = nextRandom(); }
        fade = std::min (1.0f, fade + fadeInc);
        return v * fade;
    }
    float getPhase() const { return phase; }

private:
    float nextRandom()
    {
        rng = rng * 1664525u + 1013904223u;
        return (float) (rng >> 8) / 8388608.0f - 1.0f;
    }
    LfoShape shape = LfoShape::sine;
    double sampleRate = 48000.0;
    float phase = 0.0f, inc = 0.0f, fade = 1.0f, fadeInc = 1.0f, held = 0.0f;
    uint32_t rng = 1u;
};

// Topology-preserving state-variable filter (Simper), one channel.
class Svf
{
public:
    void set (float cutoffHz, float resonance, double sr)
    {
        const float fc = std::clamp (cutoffHz, 10.0f, (float) sr * 0.45f);
        g = std::tan (3.14159265f * fc / (float) sr);
        k = 1.4142f - 1.3142f * std::clamp (resonance, 0.0f, 1.0f);   // Butterworth at 0, near self-oscillation at 1
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    void reset() { ic1 = ic2 = 0.0f; }
    // Returns low, band, high in the references.
    inline void process (float v0, float& low, float& band, float& high)
    {
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        low = v2; band = v1; high = v0 - k * v1 - v2;
    }

private:
    float g = 0.1f, k = 1.0f, a1 = 1.0f, a2 = 0.0f, a3 = 0.0f, ic1 = 0.0f, ic2 = 0.0f;
};

// --------------------------------------------------------------------------- the engine

class WaveEngine
{
public:
    void prepare (double newSampleRate, int maxBlockSize)
    {
        sampleRate = newSampleRate;
        maxBlock = std::max (1, maxBlockSize);
        for (auto& v : voices)
        {
            v.ampEnv.prepare (sampleRate);
            v.modEnv.prepare (sampleRate);
            v.lfo1.prepare (sampleRate);
            v.lfo2.prepare (sampleRate);
        }
        chorusLine[0].setMaxDelay ((int) (sampleRate * 0.04) + 8);
        chorusLine[1].setMaxDelay ((int) (sampleRate * 0.04) + 8);
        delayLine[0].setMaxDelay ((int) (sampleRate * 4.0) + 8);
        delayLine[1].setMaxDelay ((int) (sampleRate * 4.0) + 8);
        reverb.prepare (sampleRate, 2);
        outSmooth.prepare (sampleRate, 0.02);
        driveSmooth.prepare (sampleRate, 0.02);
        chorusMixSmooth.prepare (sampleRate, 0.02);
        delayMixSmooth.prepare (sampleRate, 0.02);
        delayTimeSmooth.prepare (sampleRate, 0.1);
        scratch.assign ((size_t) maxBlock * 2, 0.0f);
        setParams (params, true);
        reset();
    }

    void reset()
    {
        for (auto& v : voices) v.clear();
        heldNotes.clear();
        sustainPedal = false;
        chorusLine[0].reset(); chorusLine[1].reset();
        delayLine[0].reset(); delayLine[1].reset();
        delayLp[0] = delayLp[1] = 0.0f;
        chorusPhase = 0.0f;
        pinkState = {};
        reverb.reset();
        pitchBend = 0.0f;
        modWheel = 0.0f;
    }

    void setParams (const WaveParams& p, bool immediate = false)
    {
        params = p;
        for (auto& v : voices)
        {
            v.ampEnv.set (p.ampEnv);
            v.modEnv.set (p.modEnv);
            v.lfo1.set (p.lfo1, p.bpm);
            v.lfo2.set (p.lfo2, p.bpm);
        }
        // Glide time = time to cover 99 % of the interval (exponential approach).
        glideCoeff = p.glideMs > 0.0f ? 1.0f - std::exp (-4.6052f / (float) (sampleRate * p.glideMs * 0.001f)) : 1.0f;
        outSmooth.setTarget (dsp::dbToGain (p.outputDb));
        driveSmooth.setTarget (dsp::dbToGain (std::clamp (p.driveDb, 0.0f, 36.0f)));
        chorusMixSmooth.setTarget (std::clamp (p.chorusMix, 0.0f, 1.0f));
        delayMixSmooth.setTarget (std::clamp (p.delayMix, 0.0f, 1.0f));
        const float delaySeconds = p.delaySync ? 60.0f / std::max (20.0f, p.bpm) * lfoDivisionBeats[std::clamp (p.delayDivision, 0, numLfoDivisions - 1)]
                                               : std::clamp (p.delayMs, 1.0f, 4000.0f) * 0.001f;
        delayTimeSmooth.setTarget (std::min ((float) sampleRate * 3.99f, delaySeconds * (float) sampleRate));
        delayToneCoef = 1.0f - std::exp (-6.2831853f * std::clamp (p.delayToneHz, 200.0f, 20000.0f) / (float) sampleRate);
        spaceready::SpaceParams sp;
        sp.mode = p.reverbPlate ? spaceready::Mode::plate : spaceready::Mode::hall;
        sp.decaySeconds = std::clamp (p.reverbDecay, 0.1f, 30.0f);
        sp.mix = 1.0f;
        sp.preDelayMs = 10.0f;
        sp.dampingHz = 7000.0f;
        reverb.setParams (sp);
        if (immediate)
        {
            outSmooth.setImmediate (outSmooth.getTarget());
            driveSmooth.setImmediate (driveSmooth.getTarget());
            chorusMixSmooth.setImmediate (chorusMixSmooth.getTarget());
            delayMixSmooth.setImmediate (delayMixSmooth.getTarget());
            delayTimeSmooth.setImmediate (delayTimeSmooth.getTarget());
        }
    }
    const WaveParams& getParams() const { return params; }

    // MIDI
    void noteOn (int note, float velocity)
    {
        note = std::clamp (note, 0, 127);
        heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
        heldNotes.push_back (note);
        const float vel = std::clamp (velocity, 0.01f, 1.0f);
        if (params.mono)
        {
            Voice* v = monoVoice();
            if (v != nullptr && v->ampEnv.isActive() && ! v->ampEnv.isReleasing() && params.legato)
            {
                v->note = note;
                v->targetNote = (float) note;
                v->sustained = false;
                return;   // legato: glide to the new note, no retrigger
            }
            startVoice (v != nullptr ? *v : allocate (note), note, vel, v != nullptr && v->ampEnv.isActive());
            return;
        }
        // Poly: a second press of the same note retriggers that voice.
        for (auto& v : voices)
            if (v.active && v.note == note && ! v.ampEnv.isReleasing())
            {
                startVoice (v, note, vel, true);
                return;
            }
        startVoice (allocate (note), note, vel, false);
    }

    void noteOff (int note)
    {
        note = std::clamp (note, 0, 127);
        heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
        if (params.mono)
        {
            Voice* v = monoVoice();
            if (v == nullptr || v->note != note) return;
            if (! heldNotes.empty())
            {
                // Return to the previously held note (legato) without a retrigger.
                v->note = heldNotes.back();
                v->targetNote = (float) v->note;
                return;
            }
            if (sustainPedal) v->sustained = true; else v->release();
            return;
        }
        for (auto& v : voices)
            if (v.active && v.note == note && ! v.ampEnv.isReleasing())
            {
                if (sustainPedal) v.sustained = true; else v.release();
            }
    }

    void allNotesOff()
    {
        heldNotes.clear();
        for (auto& v : voices) if (v.active) v.release();
    }
    void allSoundOff() { for (auto& v : voices) v.clear(); heldNotes.clear(); }
    void setSustainPedal (bool down)
    {
        sustainPedal = down;
        if (! down)
            for (auto& v : voices)
                if (v.active && v.sustained) { v.sustained = false; v.release(); }
    }
    void setPitchBend (float semis) { pitchBend = semis; }           // already scaled by the range
    void setModWheel (float v) { modWheel = std::clamp (v, 0.0f, 1.0f); }

    int getActiveVoices() const
    {
        int n = 0;
        for (const auto& v : voices) n += v.active ? 1 : 0;
        return n;
    }
    bool isSounding() const { return getActiveVoices() > 0 || fxTail > 0; }
    const WaveMeters& getMeters() const { return meters; }

    // Stereo render: adds nothing, overwrites outL/outR.
    void render (float* outL, float* outR, int n)
    {
        std::fill (outL, outL + n, 0.0f);
        std::fill (outR, outR + n, 0.0f);
        meters.activeVoices = 0;
        for (auto& v : voices)
            if (v.active)
            {
                renderVoice (v, outL, outR, n);
                ++meters.activeVoices;
            }

        const bool anyVoice = meters.activeVoices > 0;
        if (anyVoice) fxTail = (int) (sampleRate * 10.0);
        else if (fxTail > 0) fxTail -= n;

        if (anyVoice || fxTail > 0)
        {
            // Drive -> chorus -> delay -> reverb -> output, with a safety soft clip.
            processFx (outL, outR, n);
        }
        float peak = 0.0f;
        for (int i = 0; i < n; ++i) peak = std::max (peak, std::max (std::fabs (outL[i]), std::fabs (outR[i])));
        meters.outputPeak = peak;
    }

    // Phase of a free-running LFO for the display.
    double getSampleRate() const { return sampleRate; }

private:
    struct OscState
    {
        double phase[maxUnison] = {};
        float morphNow = 0.0f;
    };

    struct Voice
    {
        bool active = false, sustained = false;
        int note = 60;
        float velocity = 1.0f;
        float currentNote = 60.0f, targetNote = 60.0f;
        uint32_t age = 0;
        OscState osc1, osc2;
        double subPhase = 0.0;
        Adsr ampEnv, modEnv;
        Lfo lfo1, lfo2;
        Svf filterL, filterR, filterL2, filterR2;   // the second pair is the second stage of LP 24
        float lastCutoff = 1000.0f;
        uint32_t rng = 1u;
        float pinkB[3] = {};
        int controlCounter = 0;
        float cutoffNow = 1000.0f;

        void clear()
        {
            active = false; sustained = false;
            ampEnv.kill(); modEnv.kill();
            filterL.reset(); filterR.reset(); filterL2.reset(); filterR2.reset();
        }
        void release() { ampEnv.noteOff(); modEnv.noteOff(); sustained = false; }
    };

    Voice* monoVoice()
    {
        for (auto& v : voices) if (v.active) return &v;
        return nullptr;
    }

    Voice& allocate (int)
    {
        // Free voice first, then the oldest releasing one, then the oldest overall.
        for (auto& v : voices) if (! v.active) return v;
        Voice* best = nullptr;
        for (auto& v : voices)
            if (v.ampEnv.isReleasing() && (best == nullptr || v.age < best->age)) best = &v;
        if (best == nullptr)
            for (auto& v : voices)
                if (best == nullptr || v.age < best->age) best = &v;
        return *best;
    }

    void startVoice (Voice& v, int note, float vel, bool retrigger)
    {
        const bool wasActive = v.active;
        v.active = true;
        v.sustained = false;
        v.note = note;
        v.velocity = vel;
        v.age = ++ageCounter;
        v.targetNote = (float) note;
        if (! wasActive || ! retrigger)
        {
            // Glide from the last played note when glide is on, else start on the note.
            v.currentNote = (params.glideMs > 0.0f && lastNote >= 0) ? (float) lastNote : (float) note;
        }
        lastNote = note;
        v.rng = ((uint32_t) note * 2654435761u) ^ (uint32_t) (v.age * 40503u) ^ 0x9E3779B9u;
        initOsc (v.osc1, params.osc1, v.rng);
        initOsc (v.osc2, params.osc2, v.rng ^ 0x5bd1e995u);
        if (! wasActive)
        {
            v.subPhase = 0.0;
            v.filterL.reset(); v.filterR.reset(); v.filterL2.reset(); v.filterR2.reset();
            v.pinkB[0] = v.pinkB[1] = v.pinkB[2] = 0.0f;
        }
        v.ampEnv.noteOn();
        v.modEnv.noteOn();
        if (params.lfo1.retrig || ! wasActive) v.lfo1.trigger (0.0f, v.rng);
        if (params.lfo2.retrig || ! wasActive) v.lfo2.trigger (0.0f, v.rng ^ 0x1234567u);
        v.controlCounter = 0;
        v.cutoffNow = params.cutoffHz;
    }

    static void initOsc (OscState& s, const OscParams& p, uint32_t seed)
    {
        for (int u = 0; u < maxUnison; ++u)
        {
            if (p.retrig)
                s.phase[u] = (double) p.phase;
            else
            {
                seed = seed * 1664525u + 1013904223u;
                s.phase[u] = (double) (seed >> 8) / 16777216.0;
            }
        }
    }

    // Reads one unison oscillator set, adds to l/r, returns nothing. `baseInc` = cycles per sample.
    inline void renderOsc (OscState& s, const OscParams& p, double baseInc, float morph, float gain, float& l, float& r)
    {
        const int unison = std::clamp (p.unison, 1, maxUnison);
        const float detuneCents = unison > 1 ? std::clamp (p.detune, 0.0f, 100.0f) : 0.0f;
        const float width = unison > 1 ? std::clamp (p.width, 0.0f, 1.0f) : 0.0f;
        const float norm = gain / std::sqrt ((float) unison);
        const int waveA = std::clamp (p.wave, 0, numWaves - 1);
        const int waveB = std::min (waveA + 1, numWaves - 1);
        const float m = std::clamp (morph, 0.0f, 1.0f);
        const auto& bank = WavetableBank::get();
        for (int u = 0; u < unison; ++u)
        {
            const float spread = unison > 1 ? (2.0f * (float) u / (float) (unison - 1) - 1.0f) : 0.0f;
            const double inc = baseInc * std::pow (2.0, (double) (spread * detuneCents * 0.5f) / 1200.0);
            // Mip level: harmonics allowed = 0.5 / inc; level k keeps 1024 >> k, so take the first
            // level whose harmonics all fit below Nyquist (no crossfade: the group that switches sits
            // at Nyquist, where it is both tiny and inaudible, and a crossfade would let it alias).
            const float allowed = (float) (0.5 / std::max (inc, 1e-9));
            const float levelF = std::clamp (std::ceil (std::log2 (1024.0f / std::max (allowed, 1.0f)) - 1e-4f), 0.0f, (float) (numLevels - 1));
            const int l0 = (int) levelF;
            const int l1 = l0;
            const float lf = 0.0f;
            const double pos = s.phase[u] * (double) tableSize;
            const int i0 = (int) pos;
            const float frac = (float) (pos - (double) i0);
            const int im = (i0 + tableSize - 1) & (tableSize - 1);
            const int i2 = (i0 + 2) & (tableSize - 1);
            // 4-point Hermite interpolation: the table is periodic, so wrap the outer taps.
            auto read = [&] (int wave, int level)
            {
                const float* t = bank.table (wave, level);
                const float y0 = t[im], y1 = t[i0], y2 = t[i0 + 1], y3 = t[i2];
                const float c1 = 0.5f * (y2 - y0);
                const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
                const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
                return ((c3 * frac + c2) * frac + c1) * frac + y1;
            };
            float a = read (waveA, l0), b = read (waveA, l1);
            float sample = a + (b - a) * lf;
            if (m > 0.0f && waveB != waveA)
            {
                const float a2 = read (waveB, l0), b2 = read (waveB, l1);
                sample += ((a2 + (b2 - a2) * lf) - sample) * m;
            }
            s.phase[u] += inc;
            if (s.phase[u] >= 1.0) s.phase[u] -= std::floor (s.phase[u]);
            const float pan = std::clamp (p.pan + spread * width, -1.0f, 1.0f);
            const float gl = std::sqrt (0.5f * (1.0f - pan)), gr = std::sqrt (0.5f * (1.0f + pan));
            l += sample * norm * gl;
            r += sample * norm * gr;
        }
    }

    inline float noiseSample (Voice& v)
    {
        v.rng = v.rng * 1664525u + 1013904223u;
        const float white = (float) (v.rng >> 8) / 8388608.0f - 1.0f;
        if (params.noiseColour == NoiseColour::white) return white;
        // Paul Kellet's economy pink filter.
        v.pinkB[0] = 0.99765f * v.pinkB[0] + white * 0.0990460f;
        v.pinkB[1] = 0.96300f * v.pinkB[1] + white * 0.2965164f;
        v.pinkB[2] = 0.57000f * v.pinkB[2] + white * 1.0526913f;
        return (v.pinkB[0] + v.pinkB[1] + v.pinkB[2] + white * 0.1848f) * 0.25f;
    }

    void renderVoice (Voice& v, float* outL, float* outR, int n)
    {
        const auto& p = params;
        const float velAmp = 1.0f + p.velToAmp * (v.velocity - 1.0f);
        const float subGain = std::clamp (p.subLevel, 0.0f, 1.0f);
        const float noiseGain = std::clamp (p.noiseLevel, 0.0f, 1.0f) * 0.5f;
        const float osc1Gain = p.osc1.on ? std::clamp (p.osc1.level, 0.0f, 1.0f) : 0.0f;
        const float osc2Gain = p.osc2.on ? std::clamp (p.osc2.level, 0.0f, 1.0f) : 0.0f;
        const double osc1Ratio = std::pow (2.0, (double) p.osc1.octave + (double) p.osc1.semi / 12.0 + (double) p.osc1.fine / 1200.0);
        const double osc2Ratio = std::pow (2.0, (double) p.osc2.octave + (double) p.osc2.semi / 12.0 + (double) p.osc2.fine / 1200.0);
        const double subRatio = p.subOctave >= 2 ? 0.25 : 0.5;
        const float driveIn = 1.0f + 9.0f * std::clamp (p.filterDrive, 0.0f, 1.0f);
        const float driveComp = 1.0f / std::tanh (driveIn * 0.7f) * 0.7f;
        float cutoffOctMod = 0.0f;   // control-rate state

        for (int i = 0; i < n; ++i)
        {
            // Envelopes and LFOs (per sample).
            const float amp = v.ampEnv.next();
            if (! v.ampEnv.isActive())
            {
                v.active = false;
                return;
            }
            const float env2 = v.modEnv.next();
            const float l1 = v.lfo1.next();
            const float l2 = v.lfo2.next();

            // Pitch: glide, bend, env 2, vibrato.
            if (std::fabs (v.currentNote - v.targetNote) > 1e-4f)
                v.currentNote += (v.targetNote - v.currentNote) * glideCoeff;
            else
                v.currentNote = v.targetNote;
            const float vibrato = l1 * (p.lfo1ToPitch + modWheel * p.wheelToVibrato) + l2 * p.lfo2ToPitch;
            const double noteNow = (double) v.currentNote + (double) pitchBend + (double) (env2 * p.envToPitch) + (double) vibrato + (double) p.tune / 100.0;
            const double hz = midiToHz (noteNow);
            const double baseInc = std::min (hz / sampleRate, 0.45);

            float l = 0.0f, r = 0.0f;
            const float morph1 = std::clamp (p.osc1.morph + l2 * p.lfo2ToMorph, 0.0f, 1.0f);
            if (osc1Gain > 0.0f) renderOsc (v.osc1, p.osc1, baseInc * osc1Ratio, morph1, osc1Gain, l, r);
            if (osc2Gain > 0.0f) renderOsc (v.osc2, p.osc2, baseInc * osc2Ratio, p.osc2.morph, osc2Gain, l, r);
            if (subGain > 0.0f)
            {
                const float s = p.subSquare ? (v.subPhase < 0.5 ? 1.0f : -1.0f) * 0.6f : (float) std::sin (6.283185307179586 * v.subPhase);
                v.subPhase += baseInc * subRatio;
                if (v.subPhase >= 1.0) v.subPhase -= 1.0;
                l += s * subGain * 0.7071f; r += s * subGain * 0.7071f;
            }
            if (noiseGain > 0.0f)
            {
                const float nz = noiseSample (v) * noiseGain;
                l += nz; r += nz;
            }

            // Filter coefficients at control rate (every 16 samples).
            if (v.controlCounter-- <= 0)
            {
                v.controlCounter = 15;
                cutoffOctMod = env2 * p.envToCutoff + l1 * p.lfo1ToCutoff + l2 * p.lfo2ToCutoff
                             + v.velocity * p.velToCutoff + modWheel * p.wheelToCutoff
                             + p.keyTrack * (v.currentNote - 60.0f) / 12.0f;
                const float cutoff = std::clamp (p.cutoffHz * std::pow (2.0f, cutoffOctMod), 20.0f, 20000.0f);
                v.cutoffNow = cutoff;
                v.filterL.set (cutoff, p.resonance, sampleRate);
                v.filterR.set (cutoff, p.resonance, sampleRate);
                v.filterL2.set (cutoff, p.resonance, sampleRate);
                v.filterR2.set (cutoff, p.resonance, sampleRate);
            }
            if (p.filterDrive > 0.0f)
            {
                l = std::tanh (l * driveIn * 0.7f) * driveComp;
                r = std::tanh (r * driveIn * 0.7f) * driveComp;
            }
            float lo, ba, hi;
            v.filterL.process (l, lo, ba, hi);
            l = filterOut (p.filterMode, lo, ba, hi);
            if (p.filterMode == FilterMode::lp24) { v.filterL2.process (l, lo, ba, hi); l = lo; }
            v.filterR.process (r, lo, ba, hi);
            r = filterOut (p.filterMode, lo, ba, hi);
            if (p.filterMode == FilterMode::lp24) { v.filterR2.process (r, lo, ba, hi); r = lo; }

            const float trem = 1.0f - p.lfo1ToAmp * 0.5f * (1.0f - l1);
            const float g = amp * velAmp * trem;
            outL[i] += l * g;
            outR[i] += r * g;
        }
        meters.env1 = v.ampEnv.getLevel();
        meters.cutoffHz = v.cutoffNow;
    }

    static inline float filterOut (FilterMode m, float lo, float ba, float hi)
    {
        switch (m)
        {
            case FilterMode::lp12: case FilterMode::lp24: return lo;
            case FilterMode::hp12: return hi;
            case FilterMode::bp12: return ba;
        }
        return lo;
    }

    void processFx (float* outL, float* outR, int n)
    {
        const auto& p = params;
        const bool useChorus = chorusMixSmooth.getTarget() > 0.0f || chorusMixSmooth.isSmoothing();
        const bool useDelay = delayMixSmooth.getTarget() > 0.0f || delayMixSmooth.isSmoothing() || delayTailActive;
        const bool useReverb = p.reverbMix > 0.0f;
        const float chorusInc = std::clamp (p.chorusRateHz, 0.05f, 10.0f) / (float) sampleRate;
        const float chorusDepthSamples = (float) sampleRate * 0.001f * (2.0f + 8.0f * std::clamp (p.chorusDepth, 0.0f, 1.0f));
        const float chorusCentre = (float) sampleRate * 0.012f;
        const float fb = std::clamp (p.delayFeedback, 0.0f, 0.95f);
        float wetPeak = 0.0f;

        for (int i = 0; i < n; ++i)
        {
            float l = outL[i], r = outR[i];
            const float drive = driveSmooth.next();
            if (drive > 1.0001f)
            {
                const float comp = 1.0f / std::tanh (drive);
                l = std::tanh (l * drive) * comp;
                r = std::tanh (r * drive) * comp;
            }
            if (useChorus)
            {
                const float mix = chorusMixSmooth.next();
                chorusLine[0].push (l); chorusLine[1].push (r);
                const float m1 = std::sin (6.2831853f * chorusPhase), m2 = std::sin (6.2831853f * (chorusPhase + 0.25f));
                chorusPhase += chorusInc; if (chorusPhase >= 1.0f) chorusPhase -= 1.0f;
                const float d1 = chorusCentre + chorusDepthSamples * m1, d2 = chorusCentre + chorusDepthSamples * m2;
                const float wl = chorusLine[0].read (std::max (1.0f, d1)), wr = chorusLine[1].read (std::max (1.0f, d2));
                l += (wl - l * 0.3f) * mix * 0.8f;
                r += (wr - r * 0.3f) * mix * 0.8f;
            }
            if (useDelay)
            {
                const float mix = delayMixSmooth.next();
                const float time = std::max (1.0f, delayTimeSmooth.next());
                const float dl = delayLine[0].read (time), dr = delayLine[1].read (time);
                delayLp[0] += (dl - delayLp[0]) * delayToneCoef;
                delayLp[1] += (dr - delayLp[1]) * delayToneCoef;
                const float inMono = 0.5f * (l + r);
                if (p.delayPingPong)
                {
                    delayLine[0].push (inMono + delayLp[1] * fb);
                    delayLine[1].push (delayLp[0] * fb);
                }
                else
                {
                    delayLine[0].push (l + delayLp[0] * fb);
                    delayLine[1].push (r + delayLp[1] * fb);
                }
                l += delayLp[0] * mix;
                r += delayLp[1] * mix;
                wetPeak = std::max (wetPeak, std::fabs (delayLp[0]) + std::fabs (delayLp[1]));
            }
            scratch[(size_t) i] = l;
            scratch[(size_t) (maxBlock + i)] = r;
        }
        delayTailActive = wetPeak > 1e-5f;

        if (useReverb)
        {
            // Reverb on a copy of the signal (mix 1 inside the engine), added by amount.
            std::vector<float>& rl = reverbScratch;
            if ((int) rl.size() < 2 * maxBlock) rl.assign ((size_t) maxBlock * 2, 0.0f);
            std::copy (scratch.begin(), scratch.begin() + n, rl.begin());
            std::copy (scratch.begin() + maxBlock, scratch.begin() + maxBlock + n, rl.begin() + maxBlock);
            float* io[2] = { rl.data(), rl.data() + maxBlock };
            reverb.process (io, 2, n);
            const float amount = std::clamp (p.reverbMix, 0.0f, 1.0f);
            for (int i = 0; i < n; ++i)
            {
                scratch[(size_t) i] += io[0][i] * amount;
                scratch[(size_t) (maxBlock + i)] += io[1][i] * amount;
            }
        }
        for (int i = 0; i < n; ++i)
        {
            const float g = outSmooth.next();
            outL[i] = softClip (scratch[(size_t) i] * g);
            outR[i] = softClip (scratch[(size_t) (maxBlock + i)] * g);
        }
    }

    // Transparent below 0.9, tanh-shaped above so the output never passes 1.0.
    static inline float softClip (float x)
    {
        const float a = std::fabs (x);
        if (a <= 0.9f) return x;
        const float y = 0.9f + 0.1f * std::tanh ((a - 0.9f) / 0.1f);
        return x < 0.0f ? -y : y;
    }

    double sampleRate = 48000.0;
    int maxBlock = 512;
    WaveParams params;
    WaveMeters meters;
    std::array<Voice, maxVoices> voices;
    std::vector<int> heldNotes;
    bool sustainPedal = false;
    float pitchBend = 0.0f, modWheel = 0.0f;
    float glideCoeff = 1.0f;
    int lastNote = -1;
    uint32_t ageCounter = 0;
    int fxTail = 0;
    bool delayTailActive = false;

    dsp::FractionalDelay chorusLine[2], delayLine[2];
    float chorusPhase = 0.0f, delayLp[2] = {}, delayToneCoef = 0.5f;
    std::array<float, 3> pinkState {};
    spaceready::SpaceEngine reverb;
    dsp::LinearSmoother outSmooth, driveSmooth, chorusMixSmooth, delayMixSmooth, delayTimeSmooth;
    std::vector<float> scratch, reverbScratch;
};

} // namespace rommuser::waveready
