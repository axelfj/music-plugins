// Chord Ready: one key in, a whole chord out. Pure C++ (no JUCE) so it is testable headlessly.
//
// Scale mode harmonises the played note inside a key: thirds stacked on the note's scale degree
// (C in C major -> C E G, D -> D F A, B -> B D F), optionally with 7th and 9th. Fixed mode applies
// one chord type to every note. Then voicing, inversion, octave doubling, transpose, strum and
// velocity shaping. Each input note remembers the output notes it created, so overlapping chords
// that share a tone keep it sounding until the last one is released.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "dsp/MidiNotes.h"

#include <algorithm>
#include <cmath>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace rommuser::chordready
{

enum class ChordMode { scale, fixed };
enum class Voicing { close, open, spread, wide };
enum class StrumDir { up, down, alternate, random };

struct ChordType
{
    const char* name;
    const char* suffix;   // for the chord name display
    int size;
    std::array<int, 6> intervals;
};

inline constexpr int numChordTypes = 20;
inline constexpr ChordType chordTypes[numChordTypes] = {
    { "Major",     "",      3, { 0, 4, 7 } },
    { "Minor",     "m",     3, { 0, 3, 7 } },
    { "Dim",       "dim",   3, { 0, 3, 6 } },
    { "Aug",       "aug",   3, { 0, 4, 8 } },
    { "Sus2",      "sus2",  3, { 0, 2, 7 } },
    { "Sus4",      "sus4",  3, { 0, 5, 7 } },
    { "Power",     "5",     2, { 0, 7 } },
    { "Octave",    "8",     2, { 0, 12 } },
    { "Maj7",      "maj7",  4, { 0, 4, 7, 11 } },
    { "Min7",      "m7",    4, { 0, 3, 7, 10 } },
    { "Dom7",      "7",     4, { 0, 4, 7, 10 } },
    { "m7b5",      "m7b5",  4, { 0, 3, 6, 10 } },
    { "Dim7",      "dim7",  4, { 0, 3, 6, 9 } },
    { "6",         "6",     4, { 0, 4, 7, 9 } },
    { "Min6",      "m6",    4, { 0, 3, 7, 9 } },
    { "Add9",      "add9",  4, { 0, 4, 7, 14 } },
    { "mAdd9",     "madd9", 4, { 0, 3, 7, 14 } },
    { "Maj9",      "maj9",  5, { 0, 4, 7, 11, 14 } },
    { "Min9",      "m9",    5, { 0, 3, 7, 10, 14 } },
    { "Dom9",      "9",     5, { 0, 4, 7, 10, 14 } },
};

struct ChordParams
{
    ChordMode mode = ChordMode::scale;
    int key = 0;               // 0 = C
    int scale = 0;             // index into midi::scales
    bool snapInput = true;     // scale mode: out-of-scale keys land on the nearest scale note
    bool add7 = false, add9 = false;
    int chordType = 0;         // fixed mode: index into chordTypes
    Voicing voicing = Voicing::close;
    int inversion = 0;         // -2..2
    bool bassOctave = false;   // adds the root an octave down
    bool topOctave = false;    // adds the root an octave above the chord
    int transpose = 0;         // semitones -24..24
    float strumMs = 0.0f;      // 0..300 between consecutive notes
    StrumDir strumDir = StrumDir::up;
    float velocitySpread = 0.0f; // 0..1: higher notes softer
    float humanize = 0.0f;     // 0..1 random velocity
};

using NoteEvent = midi::NoteEvent;

class ChordEngine
{
public:
    void prepare (double sr)
    {
        sampleRate = sr;
        reset();
    }

    void reset()
    {
        held.clear();
        pending.clear();
        refCount.fill (0);
        strumToggle = false;
        now = 0;
    }

    void setParams (const ChordParams& p) { params = p; }
    const ChordParams& getParams() const { return params; }

    // Chord (sorted ascending) for an input note with the current parameters, before strum.
    std::vector<int> chordFor (int inputNote) const
    {
        std::vector<int> notes = baseChord (inputNote);
        if (notes.empty()) return notes;
        applyInversion (notes, params.inversion);
        applyVoicing (notes, params.voicing);
        const int root = baseRoot (inputNote);
        if (params.bassOctave) notes.push_back (root - 12);
        if (params.topOctave)
        {
            // The root one octave above the highest chord tone.
            const int top = *std::max_element (notes.begin(), notes.end());
            int r = root;
            while (r <= top) r += 12;
            notes.push_back (r);
        }
        for (auto& n : notes) n += params.transpose;
        std::sort (notes.begin(), notes.end());
        notes.erase (std::unique (notes.begin(), notes.end()), notes.end());
        notes.erase (std::remove_if (notes.begin(), notes.end(), [] (int n) { return n < 0 || n > 127; }), notes.end());
        return notes;
    }

    // "Dm7", "G", "Bdim" ... for the display (root name + matched chord type suffix).
    std::string chordName (int inputNote) const
    {
        const auto notes = baseChord (inputNote);
        if (notes.empty()) return {};
        const int root = notes.front() + params.transpose;
        std::vector<int> pcs;
        for (int n : notes) pcs.push_back (midi::pitchClass (n - notes.front()));
        std::sort (pcs.begin(), pcs.end());
        pcs.erase (std::unique (pcs.begin(), pcs.end()), pcs.end());
        for (const auto& t : chordTypes)
        {
            std::vector<int> tp;
            for (int i = 0; i < t.size; ++i) tp.push_back (midi::pitchClass (t.intervals[(size_t) i]));
            std::sort (tp.begin(), tp.end());
            tp.erase (std::unique (tp.begin(), tp.end()), tp.end());
            if (tp == pcs) return std::string (midi::noteNames[midi::pitchClass (root)]) + t.suffix;
        }
        return std::string (midi::noteNames[midi::pitchClass (root)]) + "?";
    }

    // Events in `in` must be sorted by sample within the block. Output events are appended to `out`
    // in time order (input-triggered and strum-delayed ones interleaved).
    void process (const std::vector<NoteEvent>& in, int numSamples, std::vector<NoteEvent>& out)
    {
        const int64_t blockStart = now;
        for (const auto& e : in)
        {
            const int64_t t = blockStart + std::clamp (e.sample, 0, std::max (0, numSamples - 1));
            flushPending (t, blockStart, out);
            if (e.on && e.velocity > 0) inputOn (e, t);
            else inputOff (e.note, e.channel, t, out);
        }
        flushPending (blockStart + numSamples - 1, blockStart, out);
        now = blockStart + numSamples;
    }

    // Releases everything (e.g. CC 123 or transport stop); emits note-offs at `sample`.
    void allNotesOff (int sample, std::vector<NoteEvent>& out)
    {
        pending.clear();
        for (int n = 0; n < 128; ++n)
            if (refCount[(size_t) n] > 0)
            {
                out.push_back ({ sample, false, n, 0, 1 });
                refCount[(size_t) n] = 0;
            }
        held.clear();
    }

    bool isNoteSounding (int note) const { return refCount[(size_t) std::clamp (note, 0, 127)] > 0; }
    int numHeldInputs() const { return (int) held.size(); }
    // Last played input note, -1 if none held.
    int lastInputNote() const { return held.empty() ? -1 : held.back().inputNote; }

private:
    struct OutNote { int note; int velocity; bool emitted; };
    struct Held
    {
        int inputNote, channel;
        std::vector<OutNote> notes;
    };
    struct Pending
    {
        int64_t time;
        int inputNote, channel, index;   // index into Held::notes
    };

    int baseRoot (int inputNote) const
    {
        if (params.mode == ChordMode::scale && params.snapInput)
            return midi::snapToScale (midi::scales[std::clamp (params.scale, 0, midi::numScales - 1)], params.key, inputNote);
        return inputNote;
    }

    // Close-position chord, root first, no voicing yet.
    std::vector<int> baseChord (int inputNote) const
    {
        std::vector<int> notes;
        if (params.mode == ChordMode::fixed)
        {
            const auto& t = chordTypes[std::clamp (params.chordType, 0, numChordTypes - 1)];
            for (int i = 0; i < t.size; ++i) notes.push_back (inputNote + t.intervals[(size_t) i]);
            return notes;
        }
        // Scale mode: stack thirds in a 7-note harmony scale (pentatonics borrow their parent scale,
        // chromatic uses major).
        int scaleIndex = std::clamp (params.scale, 0, midi::numScales - 1);
        const auto& chosen = midi::scales[scaleIndex];
        if (chosen.size != 7)
            scaleIndex = (scaleIndex == 8) ? 1 : 0;
        const auto& s = midi::scales[scaleIndex];
        const int root = params.snapInput ? midi::snapToScale (s, params.key, inputNote) : inputNote;
        if (midi::scaleDegree (s, params.key, root) < 0)
        {
            // Not in the scale and not snapping: a plain major triad on the note.
            notes = { root, root + 4, root + 7 };
            return notes;
        }
        notes.push_back (root);
        notes.push_back (midi::stepInScale (s, params.key, root, 2));
        notes.push_back (midi::stepInScale (s, params.key, root, 4));
        if (params.add7) notes.push_back (midi::stepInScale (s, params.key, root, 6));
        if (params.add9) notes.push_back (midi::stepInScale (s, params.key, root, 8));
        return notes;
    }

    static void applyInversion (std::vector<int>& notes, int inversion)
    {
        const int n = (int) notes.size();
        if (n < 2) return;
        inversion = std::clamp (inversion, -2, 2);
        for (int i = 0; i < inversion; ++i)
        {
            std::sort (notes.begin(), notes.end());
            notes.front() += 12;
        }
        for (int i = 0; i < -inversion; ++i)
        {
            std::sort (notes.begin(), notes.end());
            notes.back() -= 12;
        }
        std::sort (notes.begin(), notes.end());
    }

    static void applyVoicing (std::vector<int>& notes, Voicing v)
    {
        std::sort (notes.begin(), notes.end());
        const int n = (int) notes.size();
        if (n < 3) return;
        switch (v)
        {
            case Voicing::close: break;
            case Voicing::open:   notes[(size_t) (n - 2)] -= 12; break;                  // drop 2
            case Voicing::spread: for (int i = 1; i < n; i += 2) notes[(size_t) i] += 12; break;
            case Voicing::wide:   notes.front() -= 12; notes.back() += 12; break;
        }
        std::sort (notes.begin(), notes.end());
    }

    void inputOn (const NoteEvent& e, int64_t t)
    {
        // Same key pressed again: release the old chord first.
        std::vector<NoteEvent> dummy;
        releaseHeld (e.note, e.channel, t, dummy, nullptr);

        auto notes = chordFor (e.note);
        if (notes.empty()) return;

        Held h;
        h.inputNote = e.note;
        h.channel = e.channel;
        const int n = (int) notes.size();
        for (int i = 0; i < n; ++i)
        {
            float v = (float) e.velocity;
            if (n > 1) v *= 1.0f - params.velocitySpread * 0.5f * (float) i / (float) (n - 1);
            if (params.humanize > 0.0f) v += params.humanize * 24.0f * (rng.unit() * 2.0f - 1.0f);
            h.notes.push_back ({ notes[(size_t) i], std::clamp ((int) std::lround (v), 1, 127), false });
        }

        // Strum order.
        std::vector<int> order ((size_t) n);
        for (int i = 0; i < n; ++i) order[(size_t) i] = i;
        bool down = params.strumDir == StrumDir::down;
        if (params.strumDir == StrumDir::alternate) { down = strumToggle; strumToggle = ! strumToggle; }
        if (down) std::reverse (order.begin(), order.end());
        if (params.strumDir == StrumDir::random)
            for (int i = n - 1; i > 0; --i) std::swap (order[(size_t) i], order[(size_t) rng.below (i + 1)]);

        const int64_t strumSamples = (int64_t) std::lround (std::clamp (params.strumMs, 0.0f, 1000.0f) * 0.001 * sampleRate);
        held.push_back (std::move (h));
        for (int k = 0; k < n; ++k)
            pending.push_back ({ t + strumSamples * k, e.note, e.channel, order[(size_t) k] });
        std::stable_sort (pending.begin(), pending.end(), [] (const Pending& a, const Pending& b) { return a.time < b.time; });
    }

    void inputOff (int note, int channel, int64_t t, std::vector<NoteEvent>& out)
    {
        releaseHeld (note, channel, t, out, &out);
    }

    // Releases the chord of an input note: cancels its pending note-ons, decrements the counts of
    // the emitted ones and sends note-offs for tones nobody else holds.
    void releaseHeld (int note, int channel, int64_t t, std::vector<NoteEvent>&, std::vector<NoteEvent>* out)
    {
        for (size_t i = 0; i < held.size();)
        {
            if (held[i].inputNote != note || held[i].channel != channel) { ++i; continue; }
            pending.erase (std::remove_if (pending.begin(), pending.end(),
                                           [&] (const Pending& p) { return p.inputNote == note && p.channel == channel; }),
                           pending.end());
            for (const auto& o : held[i].notes)
            {
                if (! o.emitted) continue;
                auto& rc = refCount[(size_t) o.note];
                if (rc > 0 && --rc == 0 && out != nullptr)
                    out->push_back ({ (int) (t - now), false, o.note, 0, channel });
            }
            held.erase (held.begin() + (long) i);
        }
    }

    void flushPending (int64_t upTo, int64_t blockStart, std::vector<NoteEvent>& out)
    {
        while (! pending.empty() && pending.front().time <= upTo)
        {
            const Pending p = pending.front();
            pending.erase (pending.begin());
            auto it = std::find_if (held.begin(), held.end(), [&] (const Held& h) { return h.inputNote == p.inputNote && h.channel == p.channel; });
            if (it == held.end()) continue;
            auto& o = it->notes[(size_t) p.index];
            const int sample = (int) (p.time - blockStart);
            auto& rc = refCount[(size_t) o.note];
            if (rc > 0)
                out.push_back ({ sample, false, o.note, 0, p.channel });   // retrigger a tone already sounding
            out.push_back ({ sample, true, o.note, o.velocity, p.channel });
            ++rc;
            o.emitted = true;
        }
    }

    double sampleRate = 48000.0;
    ChordParams params;
    std::vector<Held> held;
    std::vector<Pending> pending;
    std::array<int, 128> refCount {};
    bool strumToggle = false;
    int64_t now = 0;
    midi::Rng rng;
};

} // namespace rommuser::chordready
