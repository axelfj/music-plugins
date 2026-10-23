// Arp Ready: a classic arpeggiator as a pure C++ engine (no JUCE), so every timing claim can be
// tested sample by sample. Held notes are turned into a step sequence (up, down, up-down, down-up,
// as played, random, chord) over 1-4 octaves, clocked by tempo + division with swing, gate, a 16-step
// rhythm pattern, latch, and an optional alignment to the host's beat position.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "dsp/MidiNotes.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace rommuser::arpready
{

using NoteEvent = midi::NoteEvent;

enum class ArpMode { up, down, upDown, downUp, asPlayed, random, chord };
enum class OctaveMode { up, down, upDown };
enum class VelocityMode { played, fixed };

inline constexpr const char* modeNames[] = { "Up", "Down", "Up-Down", "Down-Up", "As played", "Random", "Chord" };
inline constexpr int numModes = 7;

// Step divisions in beats (quarter notes). D = dotted, T = triplet.
struct Division { const char* name; double beats; };
inline constexpr Division divisions[] = {
    { "1/1", 4.0 }, { "1/2", 2.0 }, { "1/2D", 3.0 }, { "1/2T", 4.0 / 3.0 },
    { "1/4", 1.0 }, { "1/4D", 1.5 }, { "1/4T", 2.0 / 3.0 },
    { "1/8", 0.5 }, { "1/8D", 0.75 }, { "1/8T", 1.0 / 3.0 },
    { "1/16", 0.25 }, { "1/16D", 0.375 }, { "1/16T", 1.0 / 6.0 },
    { "1/32", 0.125 }, { "1/32T", 1.0 / 12.0 },
};
inline constexpr int numDivisions = 15;
inline constexpr int defaultDivision = 10;   // 1/16

struct ArpParams
{
    ArpMode mode = ArpMode::up;
    int division = defaultDivision;
    int octaves = 1;                 // 1..4
    OctaveMode octaveMode = OctaveMode::up;
    float gate = 0.5f;               // 0.05..1.0 of a step (1.0 = legato: off right before the next on)
    float swing = 0.0f;              // 0..1: odd steps delayed by up to a third of a step (1 = triplet feel)
    bool latch = false;
    int patternLength = 16;          // 1..16
    std::array<bool, 16> pattern { true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true };
    VelocityMode velocityMode = VelocityMode::played;
    int velocity = 100;              // fixed velocity
    float accent = 0.0f;             // 0..1 extra on the first pattern step
    bool sync = true;                // align steps to the host beat grid when the transport runs
    int transpose = 0;
};

struct Transport
{
    double bpm = 120.0;
    bool playing = false;
    double ppq = 0.0;                // beat position at the start of the block (only when playing)
};

class ArpEngine
{
public:
    void prepare (double sr) { sampleRate = sr; reset(); }

    void reset()
    {
        held.clear();
        sounding.clear();
        pendingOff.clear();
        active = false;
        now = 0;
        nextStep = 0;
        stepCounter = 0;
        seqIndex = 0;
        octaveIndex = 0;
        octaveDir = 1;
        lastRandom = -1;
    }

    void setParams (const ArpParams& p)
    {
        if (params.latch && ! p.latch)
        {
            held.erase (std::remove_if (held.begin(), held.end(), [] (const HeldNote& h) { return h.released; }), held.end());
            if (held.empty()) active = false;
        }
        params = p;
        params.octaves = std::clamp (params.octaves, 1, 4);
        params.patternLength = std::clamp (params.patternLength, 1, 16);
        params.gate = std::clamp (params.gate, 0.05f, 1.0f);
        params.swing = std::clamp (params.swing, 0.0f, 1.0f);
        params.division = std::clamp (params.division, 0, numDivisions - 1);
    }
    const ArpParams& getParams() const { return params; }

    void setTransport (const Transport& t)
    {
        const bool wasPlaying = transport.playing;
        transport = t;
        if (! (t.bpm > 20.0 && t.bpm < 999.0)) transport.bpm = 120.0;
        if (params.sync && transport.playing)
            alignToGrid (! wasPlaying);
    }

    // `in` must be sorted by sample. Output note-ons/offs are appended to `out` in time order.
    void process (const std::vector<NoteEvent>& in, int numSamples, std::vector<NoteEvent>& out)
    {
        const int64_t blockStart = now;
        const int64_t blockEnd = now + numSamples;
        size_t next = 0;
        while (true)
        {
            // Next input event or next step, whichever comes first.
            const int64_t inTime = next < in.size() ? blockStart + std::clamp (in[next].sample, 0, std::max (0, numSamples - 1)) : blockEnd;
            const int64_t stepTime = active ? nextStep : blockEnd;
            if (inTime >= blockEnd && stepTime >= blockEnd) break;
            if (inTime <= stepTime)
            {
                flushOffs (inTime, blockStart, out);
                handleInput (in[next], inTime, blockStart, out);
                ++next;
            }
            else
            {
                flushOffs (stepTime, blockStart, out);
                doStep (stepTime, blockStart, out);
            }
        }
        flushOffs (blockEnd - 1, blockStart, out);
        now = blockEnd;
    }

    void allNotesOff (int sample, std::vector<NoteEvent>& out)
    {
        for (const auto& s : sounding) out.push_back ({ sample, false, s.note, 0, s.channel });
        sounding.clear();
        pendingOff.clear();
        held.clear();
        active = false;
    }

    bool isActive() const { return active; }
    int numHeld() const { return (int) held.size(); }
    bool isNoteSounding (int note) const
    {
        return std::any_of (sounding.begin(), sounding.end(), [note] (const Sounding& s) { return s.note == note; });
    }
    int currentPatternStep() const { return active ? (int) ((stepCounter - 1) % (int64_t) params.patternLength) : -1; }
    double samplesPerStep() const { return 60.0 / transport.bpm * divisions[params.division].beats * sampleRate; }

private:
    struct HeldNote { int note, velocity, channel; bool released; };
    struct Sounding { int note, channel; };
    struct PendingOff { int64_t time; int note, channel; };

    void handleInput (const NoteEvent& e, int64_t t, int64_t blockStart, std::vector<NoteEvent>& out)
    {
        if (e.on && e.velocity > 0)
        {
            if (params.latch && allReleased())
                held.clear();   // a new chord after everything was let go replaces the latched one
            held.erase (std::remove_if (held.begin(), held.end(), [&] (const HeldNote& h) { return h.note == e.note; }), held.end());
            held.push_back ({ e.note, e.velocity, e.channel, false });
            if (! active)
            {
                active = true;
                seqIndex = 0;
                octaveIndex = 0;
                octaveDir = 1;
                if (params.sync && transport.playing)
                {
                    // Play right away, then fall onto the grid.
                    stepCounter = gridStepAt (t);
                    doStep (t, blockStart, out);
                }
                else
                {
                    stepCounter = 0;
                    nextStep = t;
                }
            }
        }
        else
        {
            for (auto& h : held)
                if (h.note == e.note)
                    h.released = true;
            if (! params.latch)
                held.erase (std::remove_if (held.begin(), held.end(), [] (const HeldNote& h) { return h.released; }), held.end());
            if (held.empty())
                active = false;   // sounding notes finish their gate via pendingOff
        }
    }

    bool allReleased() const
    {
        return std::all_of (held.begin(), held.end(), [] (const HeldNote& h) { return h.released; });
    }

    // Sorted ascending copy of the held notes (unique).
    std::vector<HeldNote> sortedHeld() const
    {
        auto v = held;
        std::sort (v.begin(), v.end(), [] (const HeldNote& a, const HeldNote& b) { return a.note < b.note; });
        return v;
    }

    // The note sequence for the current mode, over one octave (octaves are added by the caller).
    std::vector<HeldNote> sequence() const
    {
        auto up = sortedHeld();
        switch (params.mode)
        {
            case ArpMode::up: return up;
            case ArpMode::down: std::reverse (up.begin(), up.end()); return up;
            case ArpMode::upDown:
            {
                auto s = up;
                for (int i = (int) up.size() - 2; i >= 1; --i) s.push_back (up[(size_t) i]);
                return s;
            }
            case ArpMode::downUp:
            {
                auto s = up;
                std::reverse (s.begin(), s.end());
                for (int i = 1; i + 1 < (int) up.size(); ++i) s.push_back (up[(size_t) i]);
                return s;
            }
            case ArpMode::asPlayed: return held;
            case ArpMode::random: return up;
            case ArpMode::chord: return up;
        }
        return up;
    }

    int octaveOffsetForStep()
    {
        if (params.octaves <= 1) return 0;
        const int idx = octaveIndex;
        switch (params.octaveMode)
        {
            case OctaveMode::up: return 12 * idx;
            case OctaveMode::down: return -12 * idx;
            case OctaveMode::upDown: return 12 * idx;
        }
        return 0;
    }

    void advanceOctave()
    {
        if (params.octaves <= 1) { octaveIndex = 0; return; }
        if (params.octaveMode == OctaveMode::upDown)
        {
            octaveIndex += octaveDir;
            if (octaveIndex >= params.octaves - 1) { octaveIndex = params.octaves - 1; octaveDir = -1; }
            else if (octaveIndex <= 0) { octaveIndex = 0; octaveDir = 1; }
        }
        else
            octaveIndex = (octaveIndex + 1) % params.octaves;
    }

    void doStep (int64_t t, int64_t blockStart, std::vector<NoteEvent>& out)
    {
        const double stepLen = samplesPerStep();
        const int patternPos = (int) (stepCounter % (int64_t) params.patternLength);
        const bool rest = ! params.pattern[(size_t) patternPos];
        const bool odd = (stepCounter % 2) != 0;
        const double swingOffset = odd ? params.swing * stepLen / 3.0 : 0.0;
        const int64_t onTime = t + (int64_t) std::llround (swingOffset);
        const int64_t gateEnd = onTime + std::max<int64_t> (1, (int64_t) std::llround (params.gate * stepLen));

        if (! rest && ! held.empty())
        {
            auto seq = sequence();
            if (seq.empty()) return;
            std::vector<HeldNote> picks;
            if (params.mode == ArpMode::chord)
                picks = seq;
            else if (params.mode == ArpMode::random)
            {
                // Any held note in any of the octaves, never the same one twice in a row.
                const int total = (int) seq.size() * params.octaves;
                int i = rng.below (total);
                if (total > 1 && i == lastRandom) i = (i + 1 + rng.below (total - 1)) % total;
                lastRandom = i;
                auto h = seq[(size_t) (i % (int) seq.size())];
                h.note += 12 * (i / (int) seq.size()) * (params.octaveMode == OctaveMode::down ? -1 : 1);
                picks.push_back (h);
            }
            else
            {
                if (seqIndex >= (int) seq.size()) { seqIndex = 0; advanceOctave(); }
                picks.push_back (seq[(size_t) seqIndex]);
            }
            const int octave = params.mode == ArpMode::random ? 0 : octaveOffsetForStep();
            for (const auto& h : picks)
            {
                const int note = h.note + octave + params.transpose;
                if (note < 0 || note > 127) continue;
                int v = params.velocityMode == VelocityMode::fixed ? params.velocity : h.velocity;
                if (patternPos == 0 && params.accent > 0.0f) v = (int) std::lround (v + params.accent * 40.0f);
                v = std::clamp (v, 1, 127);
                // Retrigger if still sounding (legato gate or long swing).
                if (isNoteSounding (note))
                {
                    out.push_back ({ (int) (onTime - blockStart), false, note, 0, h.channel });
                    sounding.erase (std::remove_if (sounding.begin(), sounding.end(), [note] (const Sounding& s) { return s.note == note; }), sounding.end());
                    pendingOff.erase (std::remove_if (pendingOff.begin(), pendingOff.end(), [note] (const PendingOff& p) { return p.note == note; }), pendingOff.end());
                }
                out.push_back ({ (int) (onTime - blockStart), true, note, v, h.channel });
                sounding.push_back ({ note, h.channel });
                pendingOff.push_back ({ gateEnd, note, h.channel });
            }
            if (params.mode == ArpMode::chord) advanceOctave();
            else if (params.mode != ArpMode::random) ++seqIndex;
        }
        ++stepCounter;
        nextStep = t + (int64_t) std::llround (stepLen);
        if (params.sync && transport.playing)
            nextStep = gridTimeAfter (t);
        nextStep = std::max (nextStep, t + 1);
        if (held.empty()) active = false;
    }

    // Offs due at or before `upTo` (sorted by time so the output stays ordered).
    void flushOffs (int64_t upTo, int64_t blockStart, std::vector<NoteEvent>& out)
    {
        std::sort (pendingOff.begin(), pendingOff.end(), [] (const PendingOff& a, const PendingOff& b) { return a.time < b.time; });
        while (! pendingOff.empty() && pendingOff.front().time <= upTo)
        {
            const auto p = pendingOff.front();
            pendingOff.erase (pendingOff.begin());
            out.push_back ({ (int) std::max<int64_t> (0, p.time - blockStart), false, p.note, 0, p.channel });
            sounding.erase (std::remove_if (sounding.begin(), sounding.end(), [&] (const Sounding& s) { return s.note == p.note && s.channel == p.channel; }), sounding.end());
        }
    }

    // Host grid: beat position `ppq` corresponds to sample `now` (block start).
    double ppqAt (int64_t sample) const
    {
        return transport.ppq + (double) (sample - now) * transport.bpm / 60.0 / sampleRate;
    }
    int64_t sampleAtPpq (double ppq) const
    {
        return now + (int64_t) std::llround ((ppq - transport.ppq) * 60.0 / transport.bpm * sampleRate);
    }
    int64_t gridStepAt (int64_t sample) const
    {
        return (int64_t) std::floor (ppqAt (sample) / divisions[params.division].beats + 1e-4);
    }
    int64_t gridTimeAfter (int64_t sample) const
    {
        const double beats = divisions[params.division].beats;
        const int64_t k = gridStepAt (sample) + 1;
        return sampleAtPpq ((double) k * beats);
    }

    // At the start of a block with the transport running: put the next step on the grid.
    void alignToGrid (bool justStarted)
    {
        if (! active) return;
        const double beats = divisions[params.division].beats;
        const int64_t k = (int64_t) std::floor (transport.ppq / beats + 1e-4);
        const double onGrid = (double) k * beats;
        const bool exactlyOnGrid = std::fabs (transport.ppq - onGrid) * 60.0 / transport.bpm * sampleRate < 0.5;
        const int64_t target = exactlyOnGrid ? now : sampleAtPpq ((double) (k + 1) * beats);
        const int64_t targetStep = exactlyOnGrid ? k : k + 1;
        if (justStarted || std::llabs (target - nextStep) > 1)
        {
            nextStep = target;
            stepCounter = targetStep;
        }
    }

    double sampleRate = 48000.0;
    ArpParams params;
    Transport transport;
    std::vector<HeldNote> held;
    std::vector<Sounding> sounding;
    std::vector<PendingOff> pendingOff;
    bool active = false;
    int64_t now = 0, nextStep = 0, stepCounter = 0;
    int seqIndex = 0, octaveIndex = 0, octaveDir = 1, lastRandom = -1;
    midi::Rng rng;
};

} // namespace rommuser::arpready
