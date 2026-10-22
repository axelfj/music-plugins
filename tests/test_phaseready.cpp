// Phase Ready DSP tests: flat with depth 0, notches at mix 50 %, click-free stage switching,
// tempo sync, stereo spread, feedback stability and bit-exact dry at mix 0.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/PhaseReady/Source/PhaseEngine.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cmath>
#include <random>
#include <vector>

using namespace rommuser::phaseready;
using Catch::Approx;

namespace
{
constexpr double sr = 48000.0;
constexpr double pi = 3.14159265358979323846;

struct Stereo { std::vector<float> l, r; };

Stereo sine (float freq, int n, float amp = 0.5f)
{
    Stereo s { std::vector<float> ((size_t) n), std::vector<float> ((size_t) n) };
    for (int i = 0; i < n; ++i)
        s.l[(size_t) i] = s.r[(size_t) i] = amp * (float) std::sin (2.0 * pi * freq * i / sr);
    return s;
}

Stereo noise (int n, unsigned seed = 1, float amp = 0.5f)
{
    std::mt19937 rng (seed);
    std::uniform_real_distribution<float> d (-amp, amp);
    Stereo s { std::vector<float> ((size_t) n), std::vector<float> ((size_t) n) };
    for (int i = 0; i < n; ++i) { s.l[(size_t) i] = d (rng); s.r[(size_t) i] = d (rng); }
    return s;
}

void run (PhaseEngine& e, Stereo& s, const Transport& t = {}, int block = 256)
{
    const int n = (int) s.l.size();
    for (int pos = 0; pos < n; pos += block)
    {
        const int len = std::min (block, n - pos);
        float* io[2] = { s.l.data() + pos, s.r.data() + pos };
        Transport tt = t;
        if (t.valid) tt.ppqPosition = t.ppqPosition + (double) pos * t.bpm / 60.0 / sr;
        e.process (io, 2, len, tt);
    }
}

// Exact level of a steady sine by projecting onto sin/cos over whole cycles.
float levelDb (const std::vector<float>& x, float freq, int skip)
{
    const int available = (int) x.size() - skip;
    const int cycles = (int) std::floor (available * freq / sr);
    const int len = (int) std::floor (cycles * sr / freq);
    double s = 0.0, c = 0.0;
    for (int i = 0; i < len; ++i)
    {
        const double ph = 2.0 * pi * freq * (skip + i) / sr;
        s += x[(size_t) (skip + i)] * std::sin (ph);
        c += x[(size_t) (skip + i)] * std::cos (ph);
    }
    const double amp = 2.0 * std::sqrt (s * s + c * c) / len;
    return (float) (20.0 * std::log10 (std::max (amp, 1e-12)));
}

PhaseParams base()
{
    PhaseParams p;
    p.stages = 8; p.sync = false; p.rateHz = 0.5f; p.depth = 0.0f; p.feedback = 0.0f;
    p.centerHz = 800.0f; p.spread = 0.0f; p.shape = LfoShape::sine; p.mix = 1.0f;
    return p;
}
} // namespace

TEST_CASE ("Depth 0, feedback 0, mix 100 % leaves the magnitude flat within 0.1 dB", "[phaseready]")
{
    for (int stages : { 4, 8, 12 })
        for (float f : { 60.0f, 300.0f, 800.0f, 2500.0f, 9000.0f })
        {
            PhaseEngine e;
            e.prepare (sr, 2);
            auto p = base();
            p.stages = stages;
            e.setParams (p);
            auto s = sine (f, 24000);
            run (e, s);
            INFO ("stages " << stages << " freq " << f);
            REQUIRE (levelDb (s.l, f, 4000) == Approx (20.0 * std::log10 (0.5)).margin (0.1));
        }
}

TEST_CASE ("Mix 50 % carves deep notches and passes other frequencies near 0 dB", "[phaseready]")
{
    PhaseEngine e;
    e.prepare (sr, 2);
    auto p = base();
    p.mix = 0.5f;
    p.stages = 4;
    e.setParams (p);

    float minDb = 0.0f, maxDb = -100.0f;
    for (float f = 100.0f; f < 12000.0f; f *= 1.06f)
    {
        e.reset();
        auto s = sine (f, 12000);
        run (e, s);
        const float db = levelDb (s.l, f, 2000) - 20.0f * std::log10 (0.5f);
        minDb = std::min (minDb, db);
        maxDb = std::max (maxDb, db);
    }
    REQUIRE (minDb < -20.0f);   // a real notch
    REQUIRE (maxDb < 0.2f);     // never louder than the input
    REQUIRE (maxDb > -0.5f);    // and some frequencies pass untouched
}

TEST_CASE ("Changing the number of stages does not click", "[phaseready]")
{
    PhaseEngine e;
    e.prepare (sr, 2);
    auto p = base();
    p.mix = 0.5f; p.depth = 0.5f; p.stages = 4;
    e.setParams (p);
    auto s = sine (440.0f, 48000);

    // First half with 4 stages, then switch to 12 mid-stream.
    float* io[2] = { s.l.data(), s.r.data() };
    e.process (io, 2, 24000, {});
    p.stages = 12;
    e.setParams (p);
    float* io2[2] = { s.l.data() + 24000, s.r.data() + 24000 };
    e.process (io2, 2, 24000, {});

    // The largest sample-to-sample jump around the switch must stay close to the sine's own slope.
    const float sineMaxStep = 0.5f * 2.0f * (float) std::sin (pi * 440.0 / sr) * 2.0f; // amplitude up to 1.0 at mix 50 % with notches
    float maxStep = 0.0f;
    for (int i = 23000; i < 26000; ++i)
        maxStep = std::max (maxStep, std::fabs (s.l[(size_t) i] - s.l[(size_t) i - 1]));
    REQUIRE (maxStep < sineMaxStep * 1.2f);
}

TEST_CASE ("Tempo-synced LFO follows the host position and repeats exactly every cycle", "[phaseready]")
{
    PhaseEngine e;
    e.prepare (sr, 2);
    auto p = base();
    p.sync = true; p.beats = 1.0; p.depth = 0.8f; p.mix = 0.5f; p.feedback = 0.4f;
    e.setParams (p);

    Transport t; t.valid = true; t.playing = true; t.bpm = 120.0; t.ppqPosition = 3.25;
    // One beat at 120 bpm = 0.5 s = 24000 samples. 1 kHz has whole cycles in that span.
    auto s = sine (1000.0f, 24000 * 6);
    run (e, s, t);

    // After processing, the phase equals the host position modulo one beat.
    const double expected = std::fmod (3.25 + 6.0 * 1.0, 1.0);
    REQUIRE (e.getLfoPhase() == Approx (expected).margin (1e-6));

    // Blocks one LFO period apart are identical once the filters have settled.
    float maxDiff = 0.0f;
    for (int i = 24000 * 3; i < 24000 * 4; ++i)
        maxDiff = std::max (maxDiff, std::fabs (s.l[(size_t) i] - s.l[(size_t) (i + 24000)]));
    REQUIRE (maxDiff < 1e-3f);
}

TEST_CASE ("Stereo spread 0 keeps both channels identical, spread 100 % separates them", "[phaseready]")
{
    auto p = base();
    p.depth = 0.8f; p.mix = 0.5f;

    PhaseEngine e;
    e.prepare (sr, 2);
    e.setParams (p);
    auto s = sine (500.0f, 48000);
    run (e, s);
    for (size_t i = 0; i < s.l.size(); ++i)
        REQUIRE (s.l[i] == s.r[i]);

    p.spread = 1.0f;
    PhaseEngine e2;
    e2.prepare (sr, 2);
    e2.setParams (p);
    auto s2 = sine (500.0f, 48000);
    run (e2, s2);
    float maxDiff = 0.0f;
    for (size_t i = 4000; i < s2.l.size(); ++i)
        maxDiff = std::max (maxDiff, std::fabs (s2.l[i] - s2.r[i]));
    REQUIRE (maxDiff > 0.1f);
}

TEST_CASE ("Maximum feedback with 12 stages and full depth stays bounded", "[phaseready]")
{
    for (float fb : { 0.95f, -0.95f })
        for (auto shape : { LfoShape::sine, LfoShape::triangle, LfoShape::random })
        {
            PhaseEngine e;
            e.prepare (sr, 2);
            auto p = base();
            p.stages = 12; p.depth = 1.0f; p.feedback = fb; p.mix = 0.5f; p.rateHz = 3.0f; p.shape = shape;
            e.setParams (p);
            auto s = noise (96000);
            run (e, s);
            float peak = 0.0f;
            for (float v : s.l) { REQUIRE (std::isfinite (v)); peak = std::max (peak, std::fabs (v)); }
            REQUIRE (peak < 4.0f); // input peak 0.5, a resonant phaser may add up to ~18 dB
        }
}

TEST_CASE ("Random LFO is smooth (no steps)", "[phaseready]")
{
    PhaseEngine e;
    e.prepare (sr, 2);
    auto p = base();
    p.shape = LfoShape::random; p.rateHz = 4.0f; p.depth = 1.0f; p.mix = 0.5f; p.spread = 0.7f;
    e.setParams (p);
    const int n = 96000;
    std::vector<float> lfoL ((size_t) n), lfoR ((size_t) n);
    auto s = sine (200.0f, n);
    for (int i = 0; i < n; ++i)
    {
        float* io[2] = { s.l.data() + i, s.r.data() + i };
        e.process (io, 2, 1, {});
        lfoL[(size_t) i] = e.getLastLfo (0);
        lfoR[(size_t) i] = e.getLastLfo (1);
    }
    float maxStep = 0.0f, range = 0.0f;
    for (int i = 1; i < n; ++i)
    {
        maxStep = std::max ({ maxStep, std::fabs (lfoL[(size_t) i] - lfoL[(size_t) i - 1]), std::fabs (lfoR[(size_t) i] - lfoR[(size_t) i - 1]) });
        range = std::max (range, std::fabs (lfoL[(size_t) i]));
    }
    REQUIRE (range > 0.3f);          // it actually moves
    REQUIRE (maxStep < 0.002f);      // 4 Hz over 12000 samples per cycle: a full swing is ~0.0003 per sample
}

TEST_CASE ("Mix 0 passes the dry signal bit-exact", "[phaseready]")
{
    PhaseEngine e;
    e.prepare (sr, 2);
    auto p = base();
    p.mix = 0.0f; p.depth = 1.0f; p.feedback = 0.9f;
    e.setParams (p);
    auto in = noise (24000, 7);
    auto out = in;
    run (e, out);
    REQUIRE (out.l == in.l);
    REQUIRE (out.r == in.r);
}
