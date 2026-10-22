// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/PunchShape/Source/PunchEngine.h"
#include "TestHelpers.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

using namespace rommuser::punchshape;
using namespace testhelpers;

namespace
{
constexpr double fs = 48000.0;

std::vector<float> run (PunchEngine& e, std::vector<float> x, int block = 256)
{
    for (size_t pos = 0; pos < x.size(); pos += (size_t) block)
    {
        float* ch[] = { x.data() + pos };
        e.process (ch, 1, (int) std::min<size_t> ((size_t) block, x.size() - pos));
    }
    return x;
}

// Silence, then a tone that starts at full level and holds (a "hit" with a long sustain).
std::vector<float> burst (double freq, double amp, int silence, int length, int tailSilence = 0)
{
    std::vector<float> x ((size_t) silence, 0.0f);
    for (int i = 0; i < length; ++i)
        x.push_back ((float) (amp * std::sin (2.0 * pi * freq * i / fs)));
    x.insert (x.end(), (size_t) tailSilence, 0.0f);
    return x;
}

// Silence, then a tone that decays exponentially (like a drum hit).
std::vector<float> decayingHit (double freq, double amp, int silence, int length, double decayMs)
{
    std::vector<float> x ((size_t) silence, 0.0f);
    for (int i = 0; i < length; ++i)
        x.push_back ((float) (amp * std::exp (-(double) i / (decayMs * 0.001 * fs)) * std::sin (2.0 * pi * freq * i / fs)));
    return x;
}

// Gain of y relative to x measured by RMS over [start, start + len).
double gainDb (const std::vector<float>& x, const std::vector<float>& y, size_t start, size_t len)
{
    return toDb (rms (y.data() + start, (int) len)) - toDb (rms (x.data() + start, (int) len));
}

constexpr int ms = 48;   // samples per millisecond at 48 kHz
} // namespace

TEST_CASE ("Punch Shape: with Attack and Sustain at 0 the output is bit-identical to the input")
{
    PunchEngine e;
    e.prepare (fs, 1);
    PunchParams p;
    e.setParams (p);

    auto x = whiteNoise (20000, 0.5);
    auto hit = decayingHit (200.0, 0.9, 1000, 20000, 150.0);
    x.insert (x.end(), hit.begin(), hit.end());
    const auto y = run (e, x);
    REQUIRE (y.size() == x.size());
    for (size_t i = 0; i < x.size(); ++i)
        REQUIRE (y[i] == x[i]);
}

TEST_CASE ("Punch Shape: positive Attack boosts the onset and leaves the steady part alone")
{
    PunchEngine e;
    e.prepare (fs, 1);
    PunchParams p;
    p.attackDb = 12.0f;
    p.speedMs = 20.0f;
    e.setParams (p);

    const auto x = burst (1000.0, 0.3, 10 * ms, 400 * ms);
    const auto y = run (e, x);

    const double onset = gainDb (x, y, 10 * (size_t) ms, 2 * (size_t) ms);
    const double steady = gainDb (x, y, 300 * (size_t) ms, 50 * (size_t) ms);
    REQUIRE (onset > 8.0);        // the hit is clearly boosted (full +12 dB at the very first samples)
    REQUIRE (onset <= 12.05);     // never more than the Attack amount
    REQUIRE (std::fabs (steady) < 0.1);   // the sustained part is untouched
}

TEST_CASE ("Punch Shape: negative Attack softens the onset")
{
    PunchEngine e;
    e.prepare (fs, 1);
    PunchParams p;
    p.attackDb = -12.0f;
    e.setParams (p);

    const auto x = burst (1000.0, 0.3, 10 * ms, 400 * ms);
    const auto y = run (e, x);
    REQUIRE (gainDb (x, y, 10 * (size_t) ms, 2 * (size_t) ms) < -8.0);
    REQUIRE (std::fabs (gainDb (x, y, 300 * (size_t) ms, 50 * (size_t) ms)) < 0.1);
}

TEST_CASE ("Punch Shape: the shaping does not depend on the input level")
{
    auto gains = [] (double amp)
    {
        PunchEngine e;
        e.prepare (fs, 1);
        PunchParams p;
        p.attackDb = 9.0f;
        p.sustainDb = -9.0f;
        e.setParams (p);
        const auto x = decayingHit (300.0, amp, 10 * ms, 500 * ms, 120.0);
        const auto y = run (e, x);
        std::vector<double> g;
        for (size_t start = 10 * (size_t) ms; start + 5 * (size_t) ms <= 300 * (size_t) ms; start += 5 * (size_t) ms)
            g.push_back (gainDb (x, y, start, 5 * (size_t) ms));
        return g;
    };

    const auto loud = gains (std::pow (10.0, -6.0 / 20.0));
    const auto quiet = gains (std::pow (10.0, -20.0 / 20.0));
    REQUIRE (loud.size() == quiet.size());
    double maxDiff = 0.0, maxGain = 0.0, minGain = 0.0;
    for (size_t i = 0; i < loud.size(); ++i)
    {
        maxDiff = std::max (maxDiff, std::fabs (loud[i] - quiet[i]));
        maxGain = std::max (maxGain, loud[i]);
        minGain = std::min (minGain, loud[i]);
    }
    REQUIRE (maxDiff < 0.05);
    REQUIRE (maxGain > 4.0);      // and the shaping actually did something
    REQUIRE (minGain < -4.0);
}

TEST_CASE ("Punch Shape: Sustain lengthens or shortens a decaying tail")
{
    auto tailGain = [] (float sustainDb)
    {
        PunchEngine e;
        e.prepare (fs, 1);
        PunchParams p;
        p.sustainDb = sustainDb;
        p.tailMs = 300.0f;
        e.setParams (p);
        const auto x = decayingHit (200.0, 0.8, 10 * ms, 600 * ms, 150.0);
        const auto y = run (e, x);
        // Onset is untouched by the sustain stage, the tail (200 ms in) is shaped.
        return std::pair { gainDb (x, y, 10 * (size_t) ms, 3 * (size_t) ms), gainDb (x, y, 200 * (size_t) ms, 20 * (size_t) ms) };
    };

    const auto [onsetUp, tailUp] = tailGain (12.0f);
    const auto [onsetDown, tailDown] = tailGain (-12.0f);
    REQUIRE (std::fabs (onsetUp) < 0.3);
    REQUIRE (std::fabs (onsetDown) < 0.3);
    REQUIRE (tailUp > 3.0);
    REQUIRE (tailDown < -3.0);
    REQUIRE (tailUp == Catch::Approx (-tailDown).margin (0.1));   // symmetric in dB by construction
}

TEST_CASE ("Punch Shape: the Lows detector ignores a high-frequency hit on top of a bass, and Highs the reverse")
{
    // The detector is level-independent, so a filter alone cannot "ignore" a sound that plays by itself;
    // what it does is decide which part of a mix the detector follows. Steady tone + hit of the other band.
    auto onsetGain = [] (Detector d, double steadyFreq, double hitFreq)
    {
        PunchEngine e;
        e.prepare (fs, 1);
        PunchParams p;
        p.attackDb = 12.0f;
        p.detector = d;
        e.setParams (p);
        auto x = sine (steadyFreq, fs, 400 * ms, 0.3);
        const auto hit = burst (hitFreq, 0.3, 200 * ms, 200 * ms);
        for (size_t i = 0; i < x.size(); ++i)
            x[i] += hit[i];
        const auto y = run (e, x);
        return gainDb (x, y, 200 * (size_t) ms + 2, 3 * (size_t) ms);
    };

    REQUIRE (onsetGain (Detector::lows, 5000.0, 60.0) > 6.0);      // bass hit over a steady hi tone: reacts
    REQUIRE (onsetGain (Detector::lows, 60.0, 5000.0) < 0.5);      // hi hit over a steady bass: ignored
    REQUIRE (onsetGain (Detector::highs, 60.0, 5000.0) > 6.0);
    REQUIRE (onsetGain (Detector::highs, 5000.0, 60.0) < 0.5);
    REQUIRE (onsetGain (Detector::full, 60.0, 5000.0) > 2.0);      // without the filter both hits count
}

TEST_CASE ("Punch Shape: the output clipper keeps boosted hits under the ceiling")
{
    PunchEngine e;
    e.prepare (fs, 1);
    PunchParams p;
    p.attackDb = 24.0f;
    p.clip = true;
    e.setParams (p);

    const auto x = burst (1000.0, 0.8, 10 * ms, 200 * ms);
    const auto y = run (e, x);
    REQUIRE (toDb (peak (y)) <= clipCeilingDb + 0.01);
    REQUIRE (toDb (peak (y)) > clipCeilingDb - 1.0);   // and it really reached the ceiling

    // Without the clipper the same settings go far above 0 dBFS.
    PunchEngine e2;
    e2.prepare (fs, 1);
    p.clip = false;
    e2.setParams (p);
    REQUIRE (toDb (peak (run (e2, x))) > 6.0);
}

TEST_CASE ("Punch Shape: stereo channels share one detector so the image does not move")
{
    PunchEngine e;
    e.prepare (fs, 2);
    PunchParams p;
    p.attackDb = 12.0f;
    e.setParams (p);

    // Left has the hit, right is a quiet steady tone: both must receive the same gain.
    auto l = burst (1000.0, 0.5, 10 * ms, 200 * ms);
    auto r = sine (1000.0, fs, (int) l.size(), 0.01);
    const auto rIn = r;
    for (size_t pos = 0; pos < l.size(); pos += 256)
    {
        float* ch[] = { l.data() + pos, r.data() + pos };
        e.process (ch, 2, (int) std::min<size_t> (256, l.size() - pos));
    }
    const double gl = gainDb (burst (1000.0, 0.5, 10 * ms, 200 * ms), l, 10 * (size_t) ms, 2 * (size_t) ms);
    const double gr = gainDb (rIn, r, 10 * (size_t) ms, 2 * (size_t) ms);
    REQUIRE (gl > 8.0);
    REQUIRE (gl == Catch::Approx (gr).margin (0.01));
}
