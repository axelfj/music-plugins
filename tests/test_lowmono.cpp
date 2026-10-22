// Low Mono DSP tests: bit-exact bypass, side rejection below the crossover, flat magnitude with the
// crossover on, width scaling and the correlation meter.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/LowMono/Source/MonoEngine.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cmath>
#include <random>
#include <vector>

using namespace rommuser::lowmono;
using Catch::Approx;

namespace
{
constexpr double sr = 48000.0;
constexpr double pi = 3.14159265358979323846;

struct Stereo { std::vector<float> l, r; };

Stereo noise (int n, unsigned seed = 1, bool independent = true)
{
    std::mt19937 rng (seed);
    std::uniform_real_distribution<float> d (-0.5f, 0.5f);
    Stereo s { std::vector<float> ((size_t) n), std::vector<float> ((size_t) n) };
    for (int i = 0; i < n; ++i) { s.l[(size_t) i] = d (rng); s.r[(size_t) i] = independent ? d (rng) : s.l[(size_t) i]; }
    return s;
}

Stereo sine (float freq, int n, float ampL, float ampR)
{
    Stereo s { std::vector<float> ((size_t) n), std::vector<float> ((size_t) n) };
    for (int i = 0; i < n; ++i)
    {
        const float v = (float) std::sin (2.0 * pi * freq * i / sr);
        s.l[(size_t) i] = ampL * v; s.r[(size_t) i] = ampR * v;
    }
    return s;
}

void run (MonoEngine& e, Stereo& s, int block = 256)
{
    const int n = (int) s.l.size();
    for (int pos = 0; pos < n; pos += block)
    {
        float* io[2] = { s.l.data() + pos, s.r.data() + pos };
        e.process (io, 2, std::min (block, n - pos));
    }
}

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
    return (float) (20.0 * std::log10 (std::max (2.0 * std::sqrt (s * s + c * c) / len, 1e-12)));
}

float rmsDb (const std::vector<float>& x, int from)
{
    double e = 0.0;
    for (size_t i = (size_t) from; i < x.size(); ++i) e += x[i] * x[i];
    return (float) (10.0 * std::log10 (std::max (e / (double) (x.size() - (size_t) from), 1e-20)));
}
} // namespace

TEST_CASE ("Width 100 % with the crossover off is bit-exact", "[lowmono]")
{
    MonoEngine e;
    e.prepare (sr);
    MonoParams p;
    p.width = 1.0f; p.monoBelowHz = 0.0f;
    e.setParams (p);
    auto in = noise (24000, 3);
    auto out = in;
    run (e, out);
    REQUIRE (out.l == in.l);
    REQUIRE (out.r == in.r);
}

TEST_CASE ("Below the crossover L-R is gone: under -80 dB with 48 dB/oct two octaves down", "[lowmono]")
{
    for (bool steep : { true, false })
    {
        MonoEngine e;
        e.prepare (sr);
        MonoParams p;
        p.monoBelowHz = 200.0f; p.steep = steep; p.width = 1.0f;
        e.setParams (p);
        auto s = sine (50.0f, 96000, 0.5f, -0.5f); // pure side signal
        run (e, s);
        std::vector<float> side (s.l.size());
        for (size_t i = 0; i < side.size(); ++i) side[i] = 0.5f * (s.l[i] - s.r[i]);
        const float sideDb = levelDb (side, 50.0f, 24000) - 20.0f * std::log10 (0.5f);
        INFO ((steep ? "LR8" : "LR4") << " side residual " << sideDb << " dB");
        REQUIRE (sideDb < (steep ? -80.0f : -40.0f));
    }
}

TEST_CASE ("With the crossover on, a mono signal keeps a flat magnitude (all-pass sum)", "[lowmono]")
{
    for (bool steep : { false, true })
        for (float f : { 40.0f, 100.0f, 120.0f, 150.0f, 400.0f, 2000.0f, 10000.0f })
        {
            MonoEngine e;
            e.prepare (sr);
            MonoParams p;
            p.monoBelowHz = 120.0f; p.steep = steep; p.width = 1.4f;
            e.setParams (p);
            auto s = sine (f, 48000, 0.5f, 0.5f);
            run (e, s);
            INFO ("steep " << steep << " f " << f);
            REQUIRE (levelDb (s.l, f, 12000) == Approx (20.0 * std::log10 (0.5)).margin (0.1));
            REQUIRE (levelDb (s.r, f, 12000) == Approx (20.0 * std::log10 (0.5)).margin (0.1));
        }
}

TEST_CASE ("Width 0 collapses to mono and width 200 % doubles the side (+6 dB)", "[lowmono]")
{
    MonoEngine e;
    e.prepare (sr);
    MonoParams p;
    p.width = 0.0f;
    e.setParams (p);
    auto s = noise (24000, 5);
    run (e, s);
    for (size_t i = 0; i < s.l.size(); ++i) REQUIRE (s.l[i] == Approx (s.r[i]).margin (1e-6));

    MonoEngine e2;
    e2.prepare (sr);
    p.width = 2.0f;
    e2.setParams (p);
    auto in = noise (48000, 6);
    auto out = in;
    run (e2, out);
    std::vector<float> sideIn (in.l.size()), sideOut (in.l.size()), midIn (in.l.size()), midOut (in.l.size());
    for (size_t i = 0; i < in.l.size(); ++i)
    {
        sideIn[i] = 0.5f * (in.l[i] - in.r[i]); sideOut[i] = 0.5f * (out.l[i] - out.r[i]);
        midIn[i] = 0.5f * (in.l[i] + in.r[i]); midOut[i] = 0.5f * (out.l[i] + out.r[i]);
    }
    REQUIRE (rmsDb (sideOut, 2000) - rmsDb (sideIn, 2000) == Approx (6.02).margin (0.1));
    REQUIRE (rmsDb (midOut, 2000) - rmsDb (midIn, 2000) == Approx (0.0).margin (0.05));
}

TEST_CASE ("Correlation meter reads +1 for mono, -1 for inverted and ~0 for independent noise", "[lowmono]")
{
    MonoEngine e;
    e.prepare (sr);
    e.setParams ({});
    auto mono = noise (48000, 7, false);
    float* io[2] = { mono.l.data(), mono.r.data() };
    REQUIRE (e.process (io, 2, 48000).correlation == Approx (1.0).margin (0.01));

    e.reset(); // the meter has a 300 ms memory
    auto inv = noise (48000, 8, false);
    for (auto& v : inv.r) v = -v;
    float* io2[2] = { inv.l.data(), inv.r.data() };
    REQUIRE (e.process (io2, 2, 48000).correlation == Approx (-1.0).margin (0.01));

    e.reset();
    auto ind = noise (96000, 9, true);
    float* io3[2] = { ind.l.data(), ind.r.data() };
    REQUIRE (std::fabs (e.process (io3, 2, 96000).correlation) < 0.1f);
}
