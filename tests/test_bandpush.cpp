// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/BandPush/Source/PushEngine.h"
#include "../shared/dsp/Crossover.h"
#include "TestHelpers.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

using namespace rommuser::bandpush;
using namespace testhelpers;

namespace
{
constexpr double fs = 48000.0;

std::vector<float> run (PushEngine& e, std::vector<float> x, int block = 256)
{
    for (size_t pos = 0; pos < x.size(); pos += (size_t) block)
    {
        float* ch[] = { x.data() + pos };
        e.process (ch, 1, (int) std::min<size_t> ((size_t) block, x.size() - pos));
    }
    return x;
}

// Sine-referenced level (dB) of a tone at `f` in the steady-state part of a signal: projection onto
// sin/cos over a whole number of cycles, so it is exact for a pure sine at any frequency.
double steadyLevelDb (const std::vector<float>& x, double f, size_t skip = 24000, size_t maxLen = 16384)
{
    const double period = fs / f;
    const size_t len = (size_t) (std::floor ((double) maxLen / period) * period);
    double sc = 0.0, ss = 0.0;
    for (size_t i = skip; i < std::min (x.size(), skip + len); ++i)
    {
        const double ph = 2.0 * pi * f * (double) i / fs;
        sc += x[i] * std::cos (ph);
        ss += x[i] * std::sin (ph);
    }
    return toDb (2.0 * std::sqrt (sc * sc + ss * ss) / (double) len);
}
} // namespace

TEST_CASE ("ThreeBandSplitter: low + mid + high sums to a flat magnitude (< 0.05 dB)")
{
    for (double f : { 30.0, 80.0, 120.0, 170.0, 500.0, 1800.0, 2500.0, 3500.0, 8000.0, 16000.0 })
    {
        rommuser::dsp::ThreeBandSplitter s;
        s.prepare (fs);
        s.setFrequencies (120.0, 2500.0);
        auto x = sine (f, fs, 48000, 0.5);
        for (auto& v : x)
        {
            float lo, mid, hi;
            s.process (v, lo, mid, hi);
            v = lo + mid + hi;
        }
        INFO ("f = " << f);
        REQUIRE (std::fabs (steadyLevelDb (x, f) - toDb (0.5)) < 0.05);
    }
}

TEST_CASE ("Depth 0 leaves the level unchanged across the spectrum")
{
    for (double f : { 60.0, 120.0, 1000.0, 2500.0, 9000.0 })
    {
        PushEngine e;
        e.prepare (fs, 1);
        PushParams p;
        p.depth = 0.0f;
        e.setParams (p, true);
        const auto y = run (e, sine (f, fs, 48000, 0.25));
        INFO ("f = " << f);
        REQUIRE (std::fabs (steadyLevelDb (y, f) - toDb (0.25)) < 0.05);
    }
}

TEST_CASE ("Downward 100 % pushes a loud band down to the threshold")
{
    PushEngine e;
    e.prepare (fs, 1);
    PushParams p;
    p.depth = 1.0f;
    p.thresholdDb = -24.0f;
    for (auto& b : p.bands) { b.up = 0.0f; b.down = 1.0f; }
    e.setParams (p, true);
    // 550 Hz sits in the middle of the mid band (120 .. 2500 Hz), where the neighbours' LR4 leakage is < -50 dB.
    const auto y = run (e, sine (550.0, fs, 72000, std::pow (10.0, -6.0 / 20.0)));
    REQUIRE (steadyLevelDb (y, 550.0, 48000) == Catch::Approx (-24.0).margin (0.5));
}

TEST_CASE ("Upward 100 % pulls a quiet band up to the threshold, and never by more than 24 dB")
{
    PushEngine e;
    e.prepare (fs, 1);
    PushParams p;
    p.depth = 1.0f;
    p.thresholdDb = -24.0f;
    for (auto& b : p.bands) { b.up = 1.0f; b.down = 0.0f; }
    e.setParams (p, true);
    const auto y = run (e, sine (550.0, fs, 72000, std::pow (10.0, -40.0 / 20.0)));
    REQUIRE (steadyLevelDb (y, 550.0, 48000) == Catch::Approx (-24.0).margin (0.5));

    PushEngine e2;
    e2.prepare (fs, 1);
    e2.setParams (p, true);
    const auto y2 = run (e2, sine (550.0, fs, 72000, std::pow (10.0, -70.0 / 20.0)));
    REQUIRE (steadyLevelDb (y2, 550.0, 48000) == Catch::Approx (-70.0 + maxUpwardDb).margin (0.5));
}

TEST_CASE ("Bands are independent: squashing the low band leaves the high band alone")
{
    PushEngine e;
    e.prepare (fs, 1);
    PushParams p;
    p.depth = 1.0f;
    p.thresholdDb = -24.0f;
    p.lowXoverHz = 200.0f;
    p.bands[0] = { 1.0f, 1.0f, 0.0f };
    p.bands[1] = { 0.0f, 0.0f, 0.0f };
    p.bands[2] = { 0.0f, 0.0f, 0.0f };
    e.setParams (p, true);

    // 40 Hz is 2.3 octaves under the 200 Hz crossover: what leaks into the mid band is < -55 dB.
    auto x = sine (40.0, fs, 72000, std::pow (10.0, -6.0 / 20.0));
    const auto hi = sine (6000.0, fs, 72000, std::pow (10.0, -30.0 / 20.0));
    for (size_t i = 0; i < x.size(); ++i)
        x[i] += hi[i];
    const auto y = run (e, x);

    const size_t n = 16384;
    const auto spec = powerSpectrum (y.data() + 48000, n);
    REQUIRE (toneLevelDb (spec, fs, n, 40.0) == Catch::Approx (-24.0).margin (0.7));
    REQUIRE (toneLevelDb (spec, fs, n, 6000.0) == Catch::Approx (-30.0).margin (0.3));
}

TEST_CASE ("Band gain applies even with depth 0, and mix 0 is the dry split")
{
    PushEngine e;
    e.prepare (fs, 1);
    PushParams p;
    p.depth = 0.0f;
    p.bands[1].gainDb = 6.0f;
    e.setParams (p, true);
    auto y = run (e, sine (550.0, fs, 48000, 0.1));
    REQUIRE (steadyLevelDb (y, 550.0) == Catch::Approx (toDb (0.1) + 6.0).margin (0.05));

    p.mix = 0.0f;
    PushEngine e2;
    e2.prepare (fs, 1);
    e2.setParams (p, true);
    y = run (e2, sine (550.0, fs, 48000, 0.1));
    REQUIRE (steadyLevelDb (y, 550.0) == Catch::Approx (toDb (0.1)).margin (0.05));
}
