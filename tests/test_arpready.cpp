// Arp Ready tests: sample-exact step timing from tempo and division (straight, dotted, triplet),
// note order for every mode, octave ranges, gate length and legato retrigger, swing, rhythm pattern
// rests, latch, chord mode, random coverage, velocity modes, alignment to the host beat grid,
// stopping when keys are released and panic.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/ArpReady/Source/ArpEngine.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <set>
#include <vector>

using namespace rommuser::arpready;

namespace
{
constexpr double sr = 48000.0;

ArpParams base()
{
    ArpParams p;
    p.division = 7;     // 1/8 = 12000 samples at 120 BPM
    p.gate = 0.5f;
    p.sync = false;
    return p;
}

ArpEngine make (const ArpParams& p, double bpm = 120.0)
{
    ArpEngine e;
    e.prepare (sr);
    e.setParams (p);
    e.setTransport ({ bpm, false, 0.0 });
    return e;
}

struct Ev { int64_t sample; bool on; int note; int velocity; };

// Runs `blocks` blocks of `blockSize`, feeding `in` (absolute samples) and collecting absolute-time output.
std::vector<Ev> run (ArpEngine& e, const std::vector<Ev>& in, int blocks, int blockSize = 512)
{
    std::vector<Ev> all;
    int64_t t0 = 0;
    for (int b = 0; b < blocks; ++b, t0 += blockSize)
    {
        std::vector<NoteEvent> blockIn;
        for (const auto& i : in)
            if (i.sample >= t0 && i.sample < t0 + blockSize)
                blockIn.push_back ({ (int) (i.sample - t0), i.on, i.note, i.velocity, 1 });
        std::vector<NoteEvent> out;
        e.process (blockIn, blockSize, out);
        for (const auto& o : out) all.push_back ({ t0 + o.sample, o.on, o.note, o.velocity });
    }
    return all;
}

std::vector<Ev> ons (const std::vector<Ev>& ev)
{
    std::vector<Ev> o;
    for (const auto& e : ev) if (e.on) o.push_back (e);
    return o;
}
std::vector<int> onNotes (const std::vector<Ev>& ev, size_t count)
{
    std::vector<int> n;
    for (const auto& e : ev) if (e.on && n.size() < count) n.push_back (e.note);
    return n;
}
const std::vector<Ev> cMajorHold = { { 0, true, 60, 100 }, { 0, true, 64, 100 }, { 0, true, 67, 100 } };
} // namespace

TEST_CASE ("Arp Ready: 1/8 at 120 BPM steps every 12000 samples with a half-step gate", "[arpready]")
{
    auto e = make (base());
    auto out = run (e, cMajorHold, 100);   // 51200 samples: steps at 0, 12000, 24000, 36000, 48000
    auto on = ons (out);
    REQUIRE (on.size() == 5);
    const int notes[5] = { 60, 64, 67, 60, 64 };
    for (int i = 0; i < 5; ++i)
    {
        CHECK (on[(size_t) i].sample == 12000 * i);
        CHECK (on[(size_t) i].note == notes[i]);
        CHECK (on[(size_t) i].velocity == 100);
    }
    // Every on has its off exactly 6000 samples later.
    for (const auto& o : on)
        if (o.sample + 6000 < 51200)
            CHECK (std::any_of (out.begin(), out.end(), [&] (const Ev& x) { return ! x.on && x.note == o.note && x.sample == o.sample + 6000; }));
    CHECK (e.samplesPerStep() == 12000.0);
}

TEST_CASE ("Arp Ready: divisions, tempo and BPM fallback", "[arpready]")
{
    auto p = base();
    p.division = 9;    // 1/8T
    auto e = make (p);
    CHECK (e.samplesPerStep() == 8000.0);
    p.division = 8;    // 1/8D
    e.setParams (p);
    CHECK (e.samplesPerStep() == 18000.0);
    p.division = 10;   // 1/16
    e.setParams (p);
    e.setTransport ({ 140.0, false, 0.0 });
    CHECK (e.samplesPerStep() == 60.0 / 140.0 * 0.25 * sr);
    e.setTransport ({ 0.0, false, 0.0 });   // host gave no tempo
    CHECK (e.samplesPerStep() == 6000.0);   // 120 BPM
}

TEST_CASE ("Arp Ready: note order per mode", "[arpready]")
{
    const std::vector<Ev> played = { { 0, true, 64, 100 }, { 0, true, 60, 100 }, { 0, true, 67, 100 }, { 0, true, 71, 100 } };
    auto check = [&] (ArpMode m, std::vector<int> expected)
    {
        auto p = base();
        p.mode = m;
        auto e = make (p);
        auto out = run (e, played, 300);
        CHECK (onNotes (out, expected.size()) == expected);
    };
    check (ArpMode::up,       { 60, 64, 67, 71, 60, 64, 67, 71 });
    check (ArpMode::down,     { 71, 67, 64, 60, 71, 67 });
    check (ArpMode::upDown,   { 60, 64, 67, 71, 67, 64, 60, 64, 67 });
    check (ArpMode::downUp,   { 71, 67, 64, 60, 64, 67, 71, 67 });
    check (ArpMode::asPlayed, { 64, 60, 67, 71, 64, 60 });
}

TEST_CASE ("Arp Ready: octave range and direction", "[arpready]")
{
    auto p = base();
    p.octaves = 2;
    auto e = make (p);
    CHECK (onNotes (run (e, cMajorHold, 300), 7) == std::vector<int> { 60, 64, 67, 72, 76, 79, 60 });
    p.octaveMode = OctaveMode::down;
    e = make (p);
    CHECK (onNotes (run (e, cMajorHold, 300), 7) == std::vector<int> { 60, 64, 67, 48, 52, 55, 60 });
    p.octaves = 3; p.octaveMode = OctaveMode::upDown;
    e = make (p);
    CHECK (onNotes (run (e, cMajorHold, 500), 13) == std::vector<int> { 60, 64, 67, 72, 76, 79, 84, 88, 91, 72, 76, 79, 60 });
    p.octaves = 1; p.transpose = -12; p.octaveMode = OctaveMode::up;
    e = make (p);
    CHECK (onNotes (run (e, cMajorHold, 100), 3) == std::vector<int> { 48, 52, 55 });
}

TEST_CASE ("Arp Ready: full gate ends a note right before the next one, same note retriggers", "[arpready]")
{
    auto p = base();
    p.gate = 1.0f;
    auto e = make (p);
    auto out = run (e, { { 0, true, 60, 100 } }, 100);
    // Sequence: on 60 @0, off 60 @12000, on 60 @12000, off @24000, on @24000 ...
    REQUIRE (out.size() >= 5);
    CHECK ((out[0].on && out[0].sample == 0));
    CHECK ((! out[1].on && out[1].sample == 12000));
    CHECK ((out[2].on && out[2].sample == 12000));
    CHECK ((! out[3].on && out[3].sample == 24000));
    CHECK ((out[4].on && out[4].sample == 24000));
    int sounding = 0;
    for (const auto& ev : out) { sounding += ev.on ? 1 : -1; CHECK (sounding <= 1); CHECK (sounding >= 0); }
}

TEST_CASE ("Arp Ready: swing delays every second step", "[arpready]")
{
    auto p = base();
    p.swing = 1.0f;   // a third of a step = triplet feel
    auto e = make (p);
    auto on = ons (run (e, cMajorHold, 100));
    REQUIRE (on.size() >= 4);
    CHECK (on[0].sample == 0);
    CHECK (on[1].sample == 16000);
    CHECK (on[2].sample == 24000);
    CHECK (on[3].sample == 40000);
    p.swing = 0.5f;
    e = make (p);
    on = ons (run (e, cMajorHold, 100));
    CHECK (on[1].sample == 14000);
}

TEST_CASE ("Arp Ready: pattern rests skip the step without consuming the note", "[arpready]")
{
    auto p = base();
    p.patternLength = 4;
    p.pattern = { true, false, true, true };
    auto e = make (p);
    auto on = ons (run (e, cMajorHold, 100));
    REQUIRE (on.size() >= 4);
    CHECK ((on[0].sample == 0 && on[0].note == 60));
    CHECK ((on[1].sample == 24000 && on[1].note == 64));
    CHECK ((on[2].sample == 36000 && on[2].note == 67));
    CHECK ((on[3].sample == 48000 && on[3].note == 60));
    CHECK (e.currentPatternStep() == 0);
}

TEST_CASE ("Arp Ready: releasing the keys stops after the current step, latch keeps going", "[arpready]")
{
    auto e = make (base());
    auto out = run (e, { { 0, true, 60, 100 }, { 0, true, 64, 100 }, { 13000, false, 60, 0 }, { 13000, false, 64, 0 } }, 100);
    auto on = ons (out);
    CHECK (on.size() == 2);
    CHECK (! e.isActive());
    // The second note still gets its off.
    CHECK (std::any_of (out.begin(), out.end(), [] (const Ev& x) { return ! x.on && x.note == 64 && x.sample == 18000; }));
    CHECK (! e.isNoteSounding (64));

    auto p = base();
    p.latch = true;
    auto l = make (p);
    out = run (l, { { 0, true, 60, 100 }, { 0, true, 64, 100 }, { 13000, false, 60, 0 }, { 13000, false, 64, 0 },
                    { 40000, true, 67, 100 } }, 100);
    on = ons (out);
    REQUIRE (on.size() >= 5);
    CHECK (on[2].note == 60);   // still going after release
    CHECK (on[3].note == 64);
    CHECK (on[4].note == 67);   // new chord replaced the latched one
    CHECK (on[4].sample == 48000);
    CHECK (l.numHeld() == 1);
    // Turning latch off with the keys up stops it.
    p.latch = false;
    std::vector<NoteEvent> dummy;
    l.process ({ { 0, false, 67, 0, 1 } }, 64, dummy);
    l.setParams (p);
    CHECK (! l.isActive());
}

TEST_CASE ("Arp Ready: chord mode plays everything each step, random mode covers every note", "[arpready]")
{
    auto p = base();
    p.mode = ArpMode::chord;
    p.octaves = 2;
    auto e = make (p);
    auto on = ons (run (e, cMajorHold, 100));
    REQUIRE (on.size() >= 6);
    for (int i = 0; i < 3; ++i) CHECK (on[(size_t) i].sample == 0);
    std::vector<int> first { on[0].note, on[1].note, on[2].note }, second { on[3].note, on[4].note, on[5].note };
    CHECK (first == std::vector<int> { 60, 64, 67 });
    CHECK (second == std::vector<int> { 72, 76, 79 });

    p.mode = ArpMode::random;
    auto r = make (p);
    on = ons (run (r, cMajorHold, 3000));
    std::set<int> seen;
    for (size_t i = 0; i < on.size(); ++i)
    {
        seen.insert (on[i].note);
        if (i > 0) CHECK (on[i].note != on[i - 1].note);
    }
    CHECK (seen == std::set<int> { 60, 64, 67, 72, 76, 79 });
}

TEST_CASE ("Arp Ready: velocity modes and accent", "[arpready]")
{
    auto p = base();
    p.patternLength = 4;
    p.accent = 0.5f;
    auto e = make (p);
    auto on = ons (run (e, { { 0, true, 60, 80 }, { 0, true, 64, 40 } }, 100));
    REQUIRE (on.size() >= 5);
    CHECK (on[0].velocity == 100);   // 80 + 20 accent
    CHECK (on[1].velocity == 40);
    CHECK (on[4].velocity == 100);   // pattern step 0 again
    p.velocityMode = VelocityMode::fixed;
    p.velocity = 120;
    e = make (p);
    on = ons (run (e, { { 0, true, 60, 80 }, { 0, true, 64, 40 } }, 100));
    CHECK (on[0].velocity == 127);
    CHECK (on[1].velocity == 120);
}

TEST_CASE ("Arp Ready: sync puts the steps on the host beat grid", "[arpready]")
{
    auto p = base();
    p.sync = true;
    p.division = 4;   // 1/4 = 24000 samples
    ArpEngine e;
    e.prepare (sr);
    e.setParams (p);
    // Block by block with a running transport: ppq advances 512 samples per block.
    std::vector<Ev> all;
    const double ppq0 = 0.75;   // a quarter beat (6000 samples) before beat 1
    int64_t t0 = 0;
    for (int b = 0; b < 200; ++b, t0 += 512)
    {
        e.setTransport ({ 120.0, true, ppq0 + (double) t0 * 2.0 / sr });
        std::vector<NoteEvent> in;
        if (b == 0) in = { { 100, true, 60, 100, 1 }, { 100, true, 64, 100, 1 } };
        std::vector<NoteEvent> out;
        e.process (in, 512, out);
        for (const auto& o : out) all.push_back ({ t0 + o.sample, o.on, o.note, o.velocity });
    }
    auto on = ons (all);
    REQUIRE (on.size() >= 4);
    CHECK (on[0].sample == 100);      // plays at once
    CHECK (on[1].sample == 6000);     // then lands on beat 1
    CHECK (on[2].sample == 30000);
    CHECK (on[3].sample == 54000);
    CHECK (on[0].note == 60);
    CHECK (on[1].note == 64);
}

TEST_CASE ("Arp Ready: all notes off releases everything at once", "[arpready]")
{
    auto p = base();
    p.mode = ArpMode::chord;
    auto e = make (p);
    run (e, cMajorHold, 1);
    std::vector<NoteEvent> out;
    e.allNotesOff (5, out);
    std::vector<int> offs;
    for (const auto& o : out) { CHECK (! o.on); CHECK (o.sample == 5); offs.push_back (o.note); }
    std::sort (offs.begin(), offs.end());
    CHECK (offs == std::vector<int> { 60, 64, 67 });
    CHECK (! e.isActive());
    CHECK (e.numHeld() == 0);
    std::vector<NoteEvent> later;
    e.process ({}, 4096, later);
    CHECK (later.empty());
}
