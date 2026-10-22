// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/DuckReady/Source/DuckEngine.h"
#include "TestHelpers.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

using namespace rommuser::duckready;
using namespace testhelpers;

namespace
{
constexpr double fs = 48000.0;

// Runs a constant 1.0 signal so the output IS the gain curve.
std::vector<float> gainCurve (DuckEngine& e, int n, Transport t, int block = 256, double ppqStart = 0.0)
{
    std::vector<float> x ((size_t) n, 1.0f);
    const double beatsPerSample = t.bpm / 60.0 / fs;
    for (int pos = 0; pos < n; pos += block)
    {
        float* ch[] = { x.data() + pos };
        t.ppqPosition = ppqStart + pos * beatsPerSample;
        e.process (ch, nullptr, 1, std::min (block, n - pos), t);
    }
    return x;
}

Transport playing120()
{
    Transport t;
    t.valid = t.playing = true;
    t.bpm = 120.0;
    return t;
}

size_t argmin (const std::vector<float>& v, size_t from, size_t to)
{
    size_t best = from;
    for (size_t i = from; i < std::min (to, v.size()); ++i)
        if (v[i] < v[best])
            best = i;
    return best;
}
} // namespace

TEST_CASE ("Depth 0 leaves the signal bit-exact in full-band mode")
{
    DuckEngine e;
    e.prepare (fs, 2);
    DuckParams p;
    p.depth = 0.0f;
    e.setParams (p, true);

    auto l = whiteNoise (8192, 0.8, 1), r = whiteNoise (8192, 0.8, 2);
    const auto l0 = l, r0 = r;
    float* ch[] = { l.data(), r.data() };
    e.process (ch, nullptr, 2, 8192, playing120());
    REQUIRE (l == l0);
    REQUIRE (r == r0);
}

TEST_CASE ("The LR4 crossover sums flat: depth 0 in low-band mode changes the magnitude by < 0.05 dB")
{
    for (double f : { 50.0, 150.0, 200.0, 280.0, 1000.0, 8000.0 })
    {
        DuckEngine e;
        e.prepare (fs, 1);
        DuckParams p;
        p.depth = 0.0f;
        p.band = Band::low;
        p.crossoverHz = 200.0f;
        e.setParams (p, true);

        auto x = sine (f, fs, 48000, 0.5);
        float* ch[] = { x.data() };
        e.process (ch, nullptr, 1, 48000, playing120());
        const double level = toDb (rms (x.data() + 8000, 32000) * std::sqrt (2.0));
        INFO ("f = " << f << " Hz, level = " << level << " dB");
        REQUIRE (std::fabs (level - toDb (0.5)) < 0.05);
    }
}

TEST_CASE ("Tempo trigger: the curve is sample-aligned to the host ppq position")
{
    // 120 bpm, 1/4 note cycle -> 24000 samples. attack 10 %, hold 20 %, release 40 %: the bottom of
    // the curve spans samples 2400 .. 7200 and the curve is back to 1 from sample 16800 on.
    DuckEngine e;
    e.prepare (fs, 1);
    DuckParams p;
    p.depth = 1.0f;
    p.attack = 0.1f;
    p.hold = 0.2f;
    p.release = 0.4f;
    p.curve = 0.0f;
    p.offset = 0.0f;
    p.beatsPerCycle = 1.0;
    e.setParams (p, true);

    auto g = gainCurve (e, 48000, playing120(), 256, 0.0);
    REQUIRE (g[0] == Catch::Approx (1.0f));
    REQUIRE (g[1200] == Catch::Approx (0.5f).margin (0.002f));   // half way down the linear attack
    REQUIRE (g[2400] == Catch::Approx (0.0f).margin (1e-6f));
    REQUIRE (g[7199] == Catch::Approx (0.0f).margin (1e-6f));
    REQUIRE (g[12000] == Catch::Approx (0.5f).margin (0.002f));  // half way up the linear release
    REQUIRE (g[16800] == Catch::Approx (1.0f).margin (1e-4f));
    REQUIRE (g[23999] == Catch::Approx (1.0f));
    // Second cycle at exactly the same place.
    REQUIRE (g[24000 + 1200] == Catch::Approx (g[1200]).margin (1e-5f));
    REQUIRE (g[24000 + 12000] == Catch::Approx (g[12000]).margin (1e-5f));

    // Starting the transport half a beat in (ppq 0.5): the first block starts at phase 0.5 (release).
    DuckEngine e2;
    e2.prepare (fs, 1);
    e2.setParams (p, true);
    auto g2 = gainCurve (e2, 24000, playing120(), 256, 0.5);
    REQUIRE (g2[0] == Catch::Approx (g[12000]).margin (1e-5f));
    REQUIRE (g2[12000 + 1200] == Catch::Approx (g[1200]).margin (1e-5f));
}

TEST_CASE ("Tempo trigger: no sample-to-sample gain step bigger than the curve's own slope (no clicks)")
{
    DuckEngine e;
    e.prepare (fs, 1);
    DuckParams p;  // defaults = "Classic" preset, attack 2 % of a 1/4 at 120 bpm = 480 samples
    e.setParams (p, true);
    auto g = gainCurve (e, 96000, playing120(), 192);
    float maxStep = 0.0f;
    for (size_t i = 1; i < g.size(); ++i)
        maxStep = std::max (maxStep, std::fabs (g[i] - g[i - 1]));
    // Linear part of the attack: depth / attackSamples. The bend (curve 0.3) makes the end of the
    // attack steeper, by at most the exponent 4^0.3 ~ 1.52.
    REQUIRE (maxStep < 1.6f * p.depth / 480.0f);
}

TEST_CASE ("Tempo trigger: a transport jump is crossfaded over 1 ms instead of stepping")
{
    DuckEngine e;
    e.prepare (fs, 1);
    DuckParams p;
    p.depth = 1.0f;
    p.attack = 0.1f;
    p.hold = 0.2f;
    p.release = 0.4f;
    p.curve = 0.0f;
    e.setParams (p, true);

    // Block 1: phase 0.2 (gain 0, at the bottom). Block 2: jumps to phase 0.9 (gain 1).
    std::vector<float> x (512, 1.0f);
    auto t = playing120();
    float* ch[] = { x.data() };
    t.ppqPosition = 0.2;
    e.process (ch, nullptr, 1, 256, t);
    t.ppqPosition = 0.9;
    float* ch2[] = { x.data() + 256 };
    e.process (ch2, nullptr, 1, 256, t);

    REQUIRE (x[255] == Catch::Approx (0.0f).margin (1e-6f));
    REQUIRE (x[256] < 0.05f);                              // did not step to 1
    REQUIRE (x[256 + 24] == Catch::Approx (0.5f).margin (0.03f));  // half way through 48 samples
    REQUIRE (x[256 + 60] == Catch::Approx (1.0f).margin (1e-5f));  // done
}

TEST_CASE ("MIDI trigger: a note-on restarts the curve at that exact sample, one shot")
{
    DuckEngine e;
    e.prepare (fs, 1);
    DuckParams p;
    p.depth = 1.0f;
    p.attack = 0.1f;
    p.hold = 0.2f;
    p.release = 0.4f;
    p.curve = 0.0f;
    p.trigger = Trigger::midi;
    e.setParams (p, true);

    std::vector<float> x (72000, 1.0f);
    const auto t = playing120();
    float* ch[] = { x.data() };
    // Idle before the note: gain 1.
    e.process (ch, nullptr, 1, 1000, t);
    const int noteOffset = 37;
    float* ch2[] = { x.data() + 1000 };
    e.process (ch2, nullptr, 1, 71000, t, &noteOffset, 1);

    const size_t on = 1000 + (size_t) noteOffset;
    REQUIRE (x[on - 1] == Catch::Approx (1.0f));
    REQUIRE (x[on] == Catch::Approx (1.0f));            // attack starts at 1 ...
    REQUIRE (x[on + 1200] == Catch::Approx (0.5f).margin (0.002f));  // ... and is half way down 1200 samples later
    REQUIRE (x[on + 2400] == Catch::Approx (0.0f).margin (1e-6f));
    REQUIRE (x[on + 16800] == Catch::Approx (1.0f).margin (1e-4f));
    // One shot: no second dip a cycle later.
    REQUIRE (x[on + 24000 + 2400] == Catch::Approx (1.0f));
}

TEST_CASE ("Sidechain trigger: a kick on the sidechain restarts the curve within 2 ms and does not retrigger during the hit")
{
    DuckEngine e;
    e.prepare (fs, 1);
    DuckParams p;
    p.depth = 1.0f;
    p.attack = 0.1f;
    p.hold = 0.2f;
    p.release = 0.4f;
    p.curve = 0.0f;
    p.trigger = Trigger::sidechain;
    p.sidechainThresholdDb = -24.0f;
    e.setParams (p, true);

    const int n = 48000;
    std::vector<float> x ((size_t) n, 1.0f);
    std::vector<float> sc ((size_t) n, 0.0f);
    const size_t kickAt = 5000;
    auto burst = sine (60.0, fs, 4800, 0.8);  // 100 ms kick
    for (size_t i = 0; i < burst.size(); ++i)
        sc[kickAt + i] = burst[i];

    float* ch[] = { x.data() };
    const float* scCh[] = { sc.data() };
    e.process (ch, scCh, 1, n, playing120());

    REQUIRE (x[kickAt - 1] == Catch::Approx (1.0f));
    // The attack (1 to 0 over 2400 samples) must have started within 2 ms (96 samples) of the kick.
    const size_t bottom = argmin (x, kickAt, kickAt + 4000);
    REQUIRE (bottom >= kickAt + 2400);
    REQUIRE (bottom <= kickAt + 2400 + 96);
    // Only one trigger: back to 1 after the release and no second dip while the burst is still sounding.
    REQUIRE (x[kickAt + 20000] == Catch::Approx (1.0f).margin (1e-4f));
    REQUIRE (x[n - 1] == Catch::Approx (1.0f));
}

TEST_CASE ("Low-band mode ducks only below the crossover")
{
    DuckEngine e;
    e.prepare (fs, 1);
    DuckParams p;
    p.depth = 1.0f;
    p.attack = 0.01f;
    p.hold = 0.98f;
    p.release = 0.01f;
    p.band = Band::low;
    p.crossoverHz = 200.0f;
    e.setParams (p, true);

    // 40 Hz + 5 kHz, measured in the middle of the hold.
    auto x = sine (40.0, fs, 24000, 0.5);
    const auto hi = sine (5000.0, fs, 24000, 0.5);
    for (size_t i = 0; i < x.size(); ++i)
        x[i] += hi[i];
    float* ch[] = { x.data() };
    e.process (ch, nullptr, 1, 24000, playing120());

    const size_t n = 8192;
    const auto spec = powerSpectrum (x.data() + 8000, n);
    REQUIRE (toneLevelDb (spec, fs, n, 40.0) < toDb (0.5) - 40.0);
    REQUIRE (toneLevelDb (spec, fs, n, 5000.0) == Catch::Approx (toDb (0.5)).margin (0.1));
}
