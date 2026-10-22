// Chord Ready tests: diatonic triads and sevenths, fixed chord types, inversions, voicings, octave
// doublers, note-off bookkeeping (shared tones, retrigger), strum timing, velocity shaping,
// out-of-scale snapping, chord names and panic.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/ChordReady/Source/ChordEngine.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <vector>

using namespace rommuser::chordready;
using rommuser::midi::Rng;

namespace
{
constexpr double sr = 48000.0;

ChordEngine make (const ChordParams& p)
{
    ChordEngine e;
    e.prepare (sr);
    e.setParams (p);
    return e;
}

std::vector<NoteEvent> run (ChordEngine& e, const std::vector<NoteEvent>& in, int numSamples = 512)
{
    std::vector<NoteEvent> out;
    e.process (in, numSamples, out);
    return out;
}

std::vector<int> onNotes (const std::vector<NoteEvent>& ev)
{
    std::vector<int> n;
    for (const auto& e : ev) if (e.on) n.push_back (e.note);
    std::sort (n.begin(), n.end());
    return n;
}

std::vector<int> offNotes (const std::vector<NoteEvent>& ev)
{
    std::vector<int> n;
    for (const auto& e : ev) if (! e.on) n.push_back (e.note);
    std::sort (n.begin(), n.end());
    return n;
}
} // namespace

TEST_CASE ("Chord Ready: diatonic triads in C major", "[chordready]")
{
    ChordParams p;
    auto e = make (p);
    CHECK (e.chordFor (60) == std::vector<int> { 60, 64, 67 });   // C E G
    CHECK (e.chordFor (62) == std::vector<int> { 62, 65, 69 });   // D F A
    CHECK (e.chordFor (71) == std::vector<int> { 71, 74, 77 });   // B D F
    CHECK (e.chordName (60) == "C");
    CHECK (e.chordName (62) == "Dm");
    CHECK (e.chordName (71) == "Bdim");
}

TEST_CASE ("Chord Ready: sevenths and ninths follow the scale", "[chordready]")
{
    ChordParams p;
    p.add7 = true;
    auto e = make (p);
    CHECK (e.chordFor (60) == std::vector<int> { 60, 64, 67, 71 });   // Cmaj7
    CHECK (e.chordName (60) == "Cmaj7");
    CHECK (e.chordName (62) == "Dm7");
    CHECK (e.chordName (67) == "G7");
    p.add9 = true;
    e.setParams (p);
    CHECK (e.chordFor (60) == std::vector<int> { 60, 64, 67, 71, 74 });
    CHECK (e.chordName (60) == "Cmaj9");
}

TEST_CASE ("Chord Ready: minor key and other scales", "[chordready]")
{
    ChordParams p;
    p.key = 9;    // A
    p.scale = 1;  // minor
    auto e = make (p);
    CHECK (e.chordFor (69) == std::vector<int> { 69, 72, 76 });   // Am
    CHECK (e.chordName (69) == "Am");
    CHECK (e.chordName (72) == "C");
    CHECK (e.chordName (76) == "Em");
    p.scale = 7;  // major pentatonic borrows major harmony
    p.key = 0;
    e.setParams (p);
    CHECK (e.chordFor (60) == std::vector<int> { 60, 64, 67 });
}

TEST_CASE ("Chord Ready: fixed chord type on any note", "[chordready]")
{
    ChordParams p;
    p.mode = ChordMode::fixed;
    p.chordType = 1;   // Minor
    auto e = make (p);
    for (int n : { 48, 61, 70 })
        CHECK (e.chordFor (n) == std::vector<int> { n, n + 3, n + 7 });
    CHECK (e.chordName (61) == "C#m");
    p.chordType = 10;   // Dom7
    e.setParams (p);
    CHECK (e.chordFor (60) == std::vector<int> { 60, 64, 67, 70 });
    CHECK (e.chordName (60) == "C7");
    p.chordType = 6;    // Power
    e.setParams (p);
    CHECK (e.chordFor (60) == std::vector<int> { 60, 67 });
}

TEST_CASE ("Chord Ready: inversions move the outer notes by an octave", "[chordready]")
{
    ChordParams p;
    auto e = make (p);
    p.inversion = 1;  e.setParams (p);
    CHECK (e.chordFor (60) == std::vector<int> { 64, 67, 72 });
    p.inversion = 2;  e.setParams (p);
    CHECK (e.chordFor (60) == std::vector<int> { 67, 72, 76 });
    p.inversion = -1; e.setParams (p);
    CHECK (e.chordFor (60) == std::vector<int> { 55, 60, 64 });
}

TEST_CASE ("Chord Ready: voicings", "[chordready]")
{
    ChordParams p;
    p.add7 = true;
    auto e = make (p);
    p.voicing = Voicing::open;   e.setParams (p);
    CHECK (e.chordFor (60) == std::vector<int> { 55, 60, 64, 71 });     // drop 2
    p.voicing = Voicing::spread; e.setParams (p);
    CHECK (e.chordFor (60) == std::vector<int> { 60, 67, 76, 83 });
    p.voicing = Voicing::wide;   e.setParams (p);
    CHECK (e.chordFor (60) == std::vector<int> { 48, 64, 67, 83 });
}

TEST_CASE ("Chord Ready: bass and top octave doublers, transpose, range", "[chordready]")
{
    ChordParams p;
    p.bassOctave = true;
    auto e = make (p);
    CHECK (e.chordFor (60) == std::vector<int> { 48, 60, 64, 67 });
    p.bassOctave = false; p.topOctave = true; e.setParams (p);
    CHECK (e.chordFor (60) == std::vector<int> { 60, 64, 67, 72 });
    p.inversion = 2; e.setParams (p);     // G C E -> top root above E5 is C6
    CHECK (e.chordFor (60) == std::vector<int> { 67, 72, 76, 84 });
    p.inversion = 0; p.topOctave = false; p.transpose = -12; e.setParams (p);
    CHECK (e.chordFor (60) == std::vector<int> { 48, 52, 55 });
    p.transpose = 0; p.bassOctave = true; e.setParams (p);
    CHECK (e.chordFor (5) == std::vector<int> { 5, 9, 12 });   // bass note below 0 is dropped
}

TEST_CASE ("Chord Ready: out-of-scale keys snap to the nearest scale note", "[chordready]")
{
    ChordParams p;
    auto e = make (p);
    CHECK (e.chordFor (61) == e.chordFor (60));     // C# -> C (ties go down)
    CHECK (e.chordFor (66) == e.chordFor (65));     // F# -> F
    p.snapInput = false; e.setParams (p);
    CHECK (e.chordFor (61) == std::vector<int> { 61, 65, 68 });   // plain major triad
}

TEST_CASE ("Chord Ready: note on emits the chord, note off releases only its notes", "[chordready]")
{
    ChordParams p;
    auto e = make (p);
    auto out = run (e, { { 10, true, 60, 100, 1 } });
    REQUIRE (out.size() == 3);
    for (const auto& ev : out) { CHECK (ev.on); CHECK (ev.sample == 10); CHECK (ev.velocity == 100); }
    CHECK (onNotes (out) == std::vector<int> { 60, 64, 67 });
    CHECK (e.numHeldInputs() == 1);
    CHECK (e.lastInputNote() == 60);

    // Add Em (E G B): G is shared with C.
    out = run (e, { { 0, true, 64, 90, 1 } });
    CHECK (onNotes (out) == std::vector<int> { 64, 67, 71 });
    // The shared G (and E) is retriggered: an off right before the on.
    CHECK (offNotes (out) == std::vector<int> { 64, 67 });

    // Release C: E and G stay (held by Em), only C goes off.
    out = run (e, { { 100, false, 60, 0, 1 } });
    CHECK (offNotes (out) == std::vector<int> { 60 });
    CHECK (onNotes (out).empty());
    CHECK (e.isNoteSounding (67));
    CHECK (! e.isNoteSounding (60));

    out = run (e, { { 0, false, 64, 0, 1 } });
    CHECK (offNotes (out) == std::vector<int> { 64, 67, 71 });
    CHECK (e.numHeldInputs() == 0);
    for (int n = 0; n < 128; ++n) CHECK (! e.isNoteSounding (n));
}

TEST_CASE ("Chord Ready: strum spreads the notes in time, across blocks", "[chordready]")
{
    ChordParams p;
    p.strumMs = 10.0f;   // 480 samples at 48 kHz
    p.add7 = true;
    auto e = make (p);
    std::vector<NoteEvent> all;
    for (int block = 0; block < 4; ++block)
    {
        auto out = run (e, block == 0 ? std::vector<NoteEvent> { { 100, true, 60, 100, 1 } } : std::vector<NoteEvent> {}, 512);
        for (auto ev : out) { ev.sample += block * 512; all.push_back (ev); }
    }
    REQUIRE (all.size() == 4);
    const int expected[4] = { 60, 64, 67, 71 };
    for (int i = 0; i < 4; ++i)
    {
        CHECK (all[(size_t) i].note == expected[i]);
        CHECK (all[(size_t) i].sample == 100 + 480 * i);
    }

    // Down strum starts from the top.
    p.strumDir = StrumDir::down;
    auto d = make (p);
    all.clear();
    for (int block = 0; block < 4; ++block)
    {
        auto out = run (d, block == 0 ? std::vector<NoteEvent> { { 0, true, 60, 100, 1 } } : std::vector<NoteEvent> {}, 512);
        for (auto ev : out) { ev.sample += block * 512; all.push_back (ev); }
    }
    REQUIRE (all.size() == 4);
    CHECK (all[0].note == 71);
    CHECK (all[3].note == 60);
}

TEST_CASE ("Chord Ready: releasing during a strum cancels the pending notes", "[chordready]")
{
    ChordParams p;
    p.strumMs = 50.0f;   // 2400 samples, only the first note fits in one block
    auto e = make (p);
    auto out = run (e, { { 0, true, 60, 100, 1 }, { 300, false, 60, 0, 1 } }, 512);
    CHECK (onNotes (out) == std::vector<int> { 60 });
    CHECK (offNotes (out) == std::vector<int> { 60 });
    out = run (e, {}, 4096);
    CHECK (out.empty());
    for (int n = 0; n < 128; ++n) CHECK (! e.isNoteSounding (n));
}

TEST_CASE ("Chord Ready: velocity spread and humanize stay in range", "[chordready]")
{
    ChordParams p;
    p.velocitySpread = 1.0f;
    p.add7 = true;
    auto e = make (p);
    auto out = run (e, { { 0, true, 60, 100, 1 } });
    REQUIRE (out.size() == 4);
    CHECK (out[0].velocity == 100);   // root untouched
    CHECK (out[3].velocity == 50);    // top at half
    for (size_t i = 1; i < out.size(); ++i) CHECK (out[i].velocity < out[i - 1].velocity);

    p.velocitySpread = 0.0f; p.humanize = 1.0f;
    auto h = make (p);
    bool varied = false;
    for (int k = 0; k < 40; ++k)
    {
        auto o = run (h, { { 0, true, 60, 120, 1 }, { 10, false, 60, 0, 1 } });
        for (const auto& ev : o)
            if (ev.on)
            {
                CHECK (ev.velocity >= 96);
                CHECK (ev.velocity <= 127);
                if (ev.velocity != 120) varied = true;
            }
    }
    CHECK (varied);
}

TEST_CASE ("Chord Ready: alternate and random strum orders cover every note", "[chordready]")
{
    ChordParams p;
    p.strumMs = 1.0f;
    p.strumDir = StrumDir::alternate;
    auto e = make (p);
    auto first = run (e, { { 0, true, 60, 100, 1 }, { 200, false, 60, 0, 1 } });
    auto second = run (e, { { 0, true, 60, 100, 1 }, { 200, false, 60, 0, 1 } });
    std::vector<int> f, s;
    for (const auto& ev : first) if (ev.on) f.push_back (ev.note);
    for (const auto& ev : second) if (ev.on) s.push_back (ev.note);
    CHECK (f == std::vector<int> { 60, 64, 67 });
    CHECK (s == std::vector<int> { 67, 64, 60 });

    p.strumDir = StrumDir::random;
    auto r = make (p);
    for (int k = 0; k < 20; ++k)
    {
        auto o = run (r, { { 0, true, 60, 100, 1 }, { 400, false, 60, 0, 1 } });
        CHECK (onNotes (o) == std::vector<int> { 60, 64, 67 });
        CHECK (offNotes (o) == std::vector<int> { 60, 64, 67 });
    }
}

TEST_CASE ("Chord Ready: all notes off releases everything", "[chordready]")
{
    ChordParams p;
    auto e = make (p);
    run (e, { { 0, true, 60, 100, 1 }, { 0, true, 65, 100, 1 } });
    std::vector<NoteEvent> out;
    e.allNotesOff (7, out);
    CHECK (offNotes (out) == std::vector<int> { 60, 64, 65, 67, 69, 72 });
    for (const auto& ev : out) CHECK (ev.sample == 7);
    CHECK (e.numHeldInputs() == 0);
    CHECK (e.lastInputNote() == -1);
    CHECK (run (e, {}).empty());
}
