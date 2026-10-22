// Space Ready DSP tests: RT60 matches the knob, no metallic resonances, pre-delay, freeze,
// width, ducking and bit-exact dry.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/SpaceReady/Source/SpaceEngine.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cmath>
#include <random>
#include <vector>

using namespace rommuser::spaceready;
using Catch::Approx;

namespace
{
constexpr double sr = 48000.0;
constexpr double pi = 3.14159265358979323846;

struct Stereo { std::vector<float> l, r; };

Stereo silence (int n) { return { std::vector<float> ((size_t) n, 0.0f), std::vector<float> ((size_t) n, 0.0f) }; }

Stereo burst (int n, int burstLen, unsigned seed = 1)
{
    auto s = silence (n);
    std::mt19937 rng (seed);
    std::uniform_real_distribution<float> d (-0.5f, 0.5f);
    for (int i = 0; i < burstLen; ++i) { s.l[(size_t) i] = d (rng); s.r[(size_t) i] = d (rng); }
    return s;
}

Stereo noise (int n, unsigned seed = 1)
{
    std::mt19937 rng (seed);
    std::uniform_real_distribution<float> d (-0.5f, 0.5f);
    auto s = silence (n);
    for (int i = 0; i < n; ++i) { s.l[(size_t) i] = d (rng); s.r[(size_t) i] = d (rng); }
    return s;
}

void run (SpaceEngine& e, Stereo& s, int block = 256)
{
    const int n = (int) s.l.size();
    for (int pos = 0; pos < n; pos += block)
    {
        float* io[2] = { s.l.data() + pos, s.r.data() + pos };
        e.process (io, 2, std::min (block, n - pos));
    }
}

float rmsDb (const std::vector<float>& x, int from, int to)
{
    double e = 0.0;
    for (int i = from; i < to; ++i) e += x[(size_t) i] * x[(size_t) i];
    return (float) (10.0 * std::log10 (std::max (e / (to - from), 1e-20)));
}

// RT60 from the energy decay slope between two times (dB per second -> seconds for 60 dB).
float measureRt60 (const std::vector<float>& x, float fromSec, float toSec)
{
    const int win = 2400; // 50 ms
    std::vector<double> t, db;
    for (int start = (int) (fromSec * sr); start + win <= (int) (toSec * sr); start += win)
    {
        t.push_back ((start + win / 2) / sr);
        db.push_back (rmsDb (x, start, start + win));
    }
    // Least squares slope.
    double mt = 0, md = 0;
    for (size_t i = 0; i < t.size(); ++i) { mt += t[i]; md += db[i]; }
    mt /= t.size(); md /= t.size();
    double num = 0, den = 0;
    for (size_t i = 0; i < t.size(); ++i) { num += (t[i] - mt) * (db[i] - md); den += (t[i] - mt) * (t[i] - mt); }
    const double slope = num / den; // dB/s (negative)
    return (float) (-60.0 / slope);
}

SpaceParams base()
{
    SpaceParams p;
    p.mode = Mode::hall; p.size = 1.0f; p.decaySeconds = 2.0f; p.preDelayMs = 0.0f; p.dampingHz = 20000.0f;
    p.diffusion = 0.5f; p.modulation = 0.0f; p.width = 1.0f; p.lowCutHz = 0.0f; p.highCutHz = 0.0f;
    p.duck = 0.0f; p.mix = 1.0f; p.freeze = false;
    return p;
}
} // namespace

TEST_CASE ("Measured RT60 matches the Decay knob within 15 %", "[spaceready]")
{
    struct Case { Mode mode; float decay; float from, to; };
    const Case cases[] = { { Mode::hall, 2.0f, 0.4f, 1.6f }, { Mode::room, 0.6f, 0.15f, 0.5f }, { Mode::plate, 1.2f, 0.25f, 1.0f }, { Mode::ambience, 6.0f, 1.0f, 4.0f } };
    for (const auto& c : cases)
    {
        SpaceEngine e;
        e.prepare (sr, 2);
        auto p = base();
        p.mode = c.mode; p.decaySeconds = c.decay;
        e.setParams (p);
        auto s = burst ((int) (sr * (c.to + 0.5f)), 480);
        run (e, s);
        const float rt60 = measureRt60 (s.l, c.from, c.to);
        INFO ("mode " << (int) c.mode << " decay " << c.decay << " measured " << rt60);
        REQUIRE (rt60 == Approx (c.decay).epsilon (0.15));
    }
}

TEST_CASE ("Tail spectrum has no metallic resonance (strongest peak within 18 dB of the median)", "[spaceready]")
{
    SpaceEngine e;
    e.prepare (sr, 2);
    auto p = base();
    p.decaySeconds = 3.0f; p.modulation = 0.5f;
    e.setParams (p);
    auto s = burst ((int) (sr * 2.0), 480);
    run (e, s);

    // DFT of the tail (0.5 .. 1.5 s) at 1024 frequencies between 150 Hz and 6 kHz.
    const int from = (int) (sr * 0.5), to = (int) (sr * 1.5);
    std::vector<float> mags;
    for (int b = 0; b < 1024; ++b)
    {
        const double f = 150.0 * std::pow (40.0, b / 1023.0);
        double re = 0.0, im = 0.0;
        for (int i = from; i < to; i += 2) // every other sample is plenty below 6 kHz
        {
            const double ph = 2.0 * pi * f * i / sr;
            re += s.l[(size_t) i] * std::cos (ph);
            im += s.l[(size_t) i] * std::sin (ph);
        }
        mags.push_back ((float) (10.0 * std::log10 (re * re + im * im + 1e-20)));
    }
    auto sorted = mags;
    std::sort (sorted.begin(), sorted.end());
    const float median = sorted[sorted.size() / 2];
    const float peak = sorted.back();
    INFO ("peak above median: " << peak - median << " dB");
    REQUIRE (peak - median < 18.0f);
}

TEST_CASE ("Pre-delay holds the wet signal back by the chosen time", "[spaceready]")
{
    SpaceEngine e;
    e.prepare (sr, 2);
    auto p = base();
    p.preDelayMs = 50.0f; p.diffusion = 0.0f;
    e.setParams (p);
    auto s = silence (24000);
    s.l[0] = s.r[0] = 1.0f; // impulse
    run (e, s);
    // Nothing before the pre-delay plus the shortest line (31 ms), something right after.
    const int quietUntil = (int) (sr * (0.050 + 0.031)) - 48;
    for (int i = 0; i < quietUntil; ++i) REQUIRE (s.l[(size_t) i] == 0.0f);
    float peakAfter = 0.0f;
    for (int i = quietUntil; i < quietUntil + 2000; ++i) peakAfter = std::max (peakAfter, std::fabs (s.l[(size_t) i]));
    REQUIRE (peakAfter > 0.01f);
}

TEST_CASE ("Freeze holds the tail at a constant level and mutes new input", "[spaceready]")
{
    SpaceEngine e;
    e.prepare (sr, 2);
    auto p = base();
    p.decaySeconds = 1.0f; p.modulation = 0.3f;
    e.setParams (p);
    auto s = burst (24000, 480);
    run (e, s);
    p.freeze = true;
    e.setParams (p);
    auto tail = noise (48000 * 3, 9); // new input must be ignored
    run (e, tail);
    const float early = rmsDb (tail.l, 24000, 48000);
    const float late = rmsDb (tail.l, 48000 * 2 + 24000, 48000 * 3);
    REQUIRE (std::fabs (late - early) < 1.0f);
}

TEST_CASE ("Width 0 gives a mono wet signal, width 100 % a decorrelated one", "[spaceready]")
{
    auto p = base();
    p.width = 0.0f;
    SpaceEngine e;
    e.prepare (sr, 2);
    e.setParams (p);
    auto s = burst (48000, 480);
    run (e, s);
    for (int i = 0; i < 48000; ++i) REQUIRE (s.l[(size_t) i] == Approx (s.r[(size_t) i]).margin (1e-6));

    p.width = 1.0f;
    SpaceEngine e2;
    e2.prepare (sr, 2);
    e2.setParams (p);
    auto s2 = burst (48000, 480);
    run (e2, s2);
    double lr = 0, ll = 0, rr = 0;
    for (int i = 4800; i < 48000; ++i) { lr += s2.l[(size_t) i] * s2.r[(size_t) i]; ll += s2.l[(size_t) i] * s2.l[(size_t) i]; rr += s2.r[(size_t) i] * s2.r[(size_t) i]; }
    const double correlation = lr / std::sqrt (ll * rr);
    REQUIRE (std::fabs (correlation) < 0.3);
}

TEST_CASE ("Ducking lowers the wet signal while the dry signal is loud", "[spaceready]")
{
    auto p = base();
    p.mix = 1.0f; p.decaySeconds = 4.0f;
    SpaceEngine e;
    e.prepare (sr, 2);
    e.setParams (p);
    auto s = noise (48000, 4); // loud dry all the time
    run (e, s);
    const float unducked = rmsDb (s.l, 24000, 48000);

    p.duck = 1.0f;
    SpaceEngine e2;
    e2.prepare (sr, 2);
    e2.setParams (p);
    auto s2 = noise (48000, 4);
    run (e2, s2);
    const float ducked = rmsDb (s2.l, 24000, 48000);
    REQUIRE (unducked - ducked > 20.0f);
}

TEST_CASE ("Mix 0 passes the dry signal bit-exact", "[spaceready]")
{
    SpaceEngine e;
    e.prepare (sr, 2);
    auto p = base();
    p.mix = 0.0f; p.duck = 0.5f; p.modulation = 1.0f;
    e.setParams (p);
    auto in = noise (24000, 7);
    auto out = in;
    run (e, out);
    REQUIRE (out.l == in.l);
    REQUIRE (out.r == in.r);
}

TEST_CASE ("Long ambience tail stays bounded and finite", "[spaceready]")
{
    SpaceEngine e;
    e.prepare (sr, 2);
    auto p = base();
    p.mode = Mode::ambience; p.decaySeconds = 30.0f; p.modulation = 1.0f; p.size = 2.0f; p.diffusion = 1.0f;
    e.setParams (p);
    auto s = noise (48000 * 4, 2);
    run (e, s);
    float peak = 0.0f;
    for (float v : s.l) { REQUIRE (std::isfinite (v)); peak = std::max (peak, std::fabs (v)); }
    REQUIRE (peak < 8.0f);
}
