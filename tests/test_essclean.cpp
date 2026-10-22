// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/EssClean/Source/EssEngine.h"
#include "TestHelpers.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

using namespace rommuser::essclean;
using namespace testhelpers;

namespace
{
constexpr double fs = 48000.0;
constexpr int ms = 48;

std::vector<float> run (EssEngine& e, std::vector<float> x, int block = 256)
{
    for (size_t pos = 0; pos < x.size(); pos += (size_t) block)
    {
        float* ch[] = { x.data() + pos };
        e.process (ch, 1, (int) std::min<size_t> ((size_t) block, x.size() - pos));
    }
    return x;
}

// Level of a sine component by projection over whole cycles in [start, start + len).
double componentDb (const std::vector<float>& x, double freq, size_t start, size_t len)
{
    const size_t cycles = (size_t) std::floor ((double) len * freq / fs);
    const size_t n = (size_t) std::lround ((double) cycles * fs / freq);
    double s = 0.0, c = 0.0;
    for (size_t i = 0; i < n; ++i)
    {
        const double ph = 2.0 * pi * freq * (double) i / fs;
        s += x[start + i] * std::sin (ph);
        c += x[start + i] * std::cos (ph);
    }
    return toDb (2.0 * std::sqrt (s * s + c * c) / (double) n);
}

std::vector<float> add (std::vector<float> a, const std::vector<float>& b)
{
    for (size_t i = 0; i < a.size(); ++i) a[i] += b[i];
    return a;
}
} // namespace

TEST_CASE ("Ess Clean: gain computer is 0 below the knee, capped at range above")
{
    REQUIRE (reductionDb (-40.0f, -24.0f, 10.0f) == 0.0f);
    REQUIRE (reductionDb (-27.1f, -24.0f, 10.0f) == 0.0f);                    // just under threshold - knee/2
    REQUIRE (reductionDb (-24.0f, -24.0f, 10.0f) == Catch::Approx (0.75f));   // knee middle: (3^2) / 12
    REQUIRE (reductionDb (-18.0f, -24.0f, 10.0f) == Catch::Approx (6.0f));
    REQUIRE (reductionDb (0.0f, -24.0f, 10.0f) == Catch::Approx (10.0f));
}

TEST_CASE ("Ess Clean: below the threshold the output is bit-identical to the input (both modes, with and without lookahead)")
{
    auto x = whiteNoise (30000, 0.05);                       // -32 dBFS peak, far below -24 dB in band
    const auto voice = sine (300.0, fs, 30000, 0.3);
    x = add (x, voice);

    for (int mode = 0; mode < 2; ++mode)
        for (float la : { 0.0f, 5.0f })
        {
            EssEngine e;
            e.prepare (fs, 1);
            EssParams p;
            p.mode = (Mode) mode;
            p.lookaheadMs = la;
            e.setParams (p);
            const int L = e.getLatencySamples();
            REQUIRE (L == (int) std::lround (la * 0.001 * fs));
            const auto y = run (e, x);
            for (size_t i = (size_t) L + 1; i < x.size(); ++i)
                REQUIRE (y[i] == x[i - (size_t) L]);
        }
}

TEST_CASE ("Ess Clean: band mode lowers the sibilance by the range and leaves the low tone alone")
{
    EssEngine e;
    e.prepare (fs, 1);
    EssParams p;
    p.thresholdDb = -40.0f;
    p.rangeDb = 10.0f;
    e.setParams (p);

    // Reduction is the full range at the centre of the detection range and tapers to half at its edges.
    const double essHz = e.getBandCentreHz();
    const int n = 600 * ms;
    const auto ess = sine (essHz, fs, n, 0.4);
    const auto tone = sine (200.0, fs, n, 0.4);
    const auto x = add (ess, tone);
    const auto y = run (e, x);

    const size_t start = 300 * (size_t) ms, len = 200 * (size_t) ms;
    const double essIn = componentDb (x, essHz, start, len), essOut = componentDb (y, essHz, start, len);
    const double toneIn = componentDb (x, 200.0, start, len), toneOut = componentDb (y, 200.0, start, len);
    REQUIRE (essIn - essOut == Catch::Approx (10.0).margin (0.3));
    REQUIRE (std::fabs (toneIn - toneOut) < 0.3);
}

TEST_CASE ("Ess Clean: broadband mode lowers everything by the same amount")
{
    EssEngine e;
    e.prepare (fs, 1);
    EssParams p;
    p.thresholdDb = -40.0f;
    p.rangeDb = 8.0f;
    p.mode = Mode::broadband;
    e.setParams (p);

    const int n = 600 * ms;
    const auto x = add (sine (6000.0, fs, n, 0.4), sine (200.0, fs, n, 0.4));
    const auto y = run (e, x);
    const size_t start = 300 * (size_t) ms, len = 200 * (size_t) ms;
    REQUIRE (componentDb (x, 6000.0, start, len) - componentDb (y, 6000.0, start, len) == Catch::Approx (8.0).margin (0.2));
    REQUIRE (componentDb (x, 200.0, start, len) - componentDb (y, 200.0, start, len) == Catch::Approx (8.0).margin (0.2));
}

TEST_CASE ("Ess Clean: lookahead catches the very start of the sibilance")
{
    auto onsetReduction = [] (float lookaheadMs)
    {
        EssEngine e;
        e.prepare (fs, 1);
        EssParams p;
        p.thresholdDb = -40.0f;
        p.rangeDb = 12.0f;
        p.attackMs = 2.0f;
        p.mode = Mode::broadband;   // band mode adds the band filter's own ring-up at the onset
        p.lookaheadMs = lookaheadMs;
        e.setParams (p);
        const int L = e.getLatencySamples();
        std::vector<float> x (20 * (size_t) ms, 0.0f);
        const auto s = sine (6000.0, fs, 200 * ms, 0.4);
        x.insert (x.end(), s.begin(), s.end());
        x.insert (x.end(), (size_t) L, 0.0f);
        const auto y = run (e, x);
        // Compare the first millisecond of the sibilance, output aligned by the latency.
        double pin = 0.0, pout = 0.0;
        for (size_t i = 20 * (size_t) ms; i < 21 * (size_t) ms; ++i)
        {
            pin = std::max (pin, (double) std::fabs (x[i]));
            pout = std::max (pout, (double) std::fabs (y[i + (size_t) L]));
        }
        return toDb (pin) - toDb (pout);
    };

    REQUIRE (onsetReduction (0.0f) < 4.0);     // without lookahead the first millisecond mostly slips through
    REQUIRE (onsetReduction (5.0f) > 9.0);     // with 5 ms lookahead it is already almost fully reduced
}

TEST_CASE ("Ess Clean: Listen outputs exactly what was removed")
{
    const int n = 300 * ms;
    const auto x = add (sine (6000.0, fs, n, 0.4), sine (200.0, fs, n, 0.4));
    for (int mode = 0; mode < 2; ++mode)
    {
        EssParams p;
        p.thresholdDb = -40.0f;
        p.mode = (Mode) mode;

        EssEngine normal, listen;
        normal.prepare (fs, 1);
        listen.prepare (fs, 1);
        normal.setParams (p);
        p.listen = true;
        listen.setParams (p);
        const auto yn = run (normal, x), yl = run (listen, x);
        for (size_t i = 0; i < x.size(); ++i)
            REQUIRE (yn[i] + yl[i] == Catch::Approx (x[i]).margin (1e-5));
    }
}

TEST_CASE ("Ess Clean: both channels get the same gain from one shared detector")
{
    EssEngine e;
    e.prepare (fs, 2);
    EssParams p;
    p.thresholdDb = -40.0f;
    p.rangeDb = 10.0f;
    p.mode = Mode::broadband;
    e.setParams (p);

    const int n = 400 * ms;
    auto l = sine (6000.0, fs, n, 0.4);
    auto r = sine (6000.0, fs, n, 0.01);      // quiet on the right: would not trigger alone
    const auto lIn = l, rIn = r;
    for (size_t pos = 0; pos < l.size(); pos += 256)
    {
        float* ch[] = { l.data() + pos, r.data() + pos };
        e.process (ch, 2, (int) std::min<size_t> (256, l.size() - pos));
    }
    const size_t start = 200 * (size_t) ms, len = 150 * (size_t) ms;
    const double gl = componentDb (lIn, 6000.0, start, len) - componentDb (l, 6000.0, start, len);
    const double gr = componentDb (rIn, 6000.0, start, len) - componentDb (r, 6000.0, start, len);
    REQUIRE (gl == Catch::Approx (10.0).margin (0.2));
    REQUIRE (gr == Catch::Approx (gl).margin (0.01));
}
