// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/PressReady/Source/CompEngine.h"
#include "TestHelpers.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

using namespace rommuser::pressready;
using namespace testhelpers;

namespace
{
constexpr double fs = 48000.0;

std::vector<float> run (CompEngine& e, std::vector<float> x, int block = 256)
{
    for (size_t pos = 0; pos < x.size(); pos += (size_t) block)
    {
        float* ch[] = { x.data() + pos };
        e.process (ch, nullptr, 1, (int) std::min<size_t> ((size_t) block, x.size() - pos));
    }
    return x;
}

// Peak level (dB) of x over a window.
double windowPeakDb (const std::vector<float>& x, size_t start, size_t len)
{
    double p = 0.0;
    for (size_t i = start; i < std::min (x.size(), start + len); ++i)
        p = std::max (p, (double) std::fabs (x[i]));
    return toDb (p);
}

// First sample index at which the 1 ms windowed peak has moved `fraction` of the way from `from` to `to` (dB).
size_t timeToFraction (const std::vector<float>& x, size_t start, double from, double to, double fraction)
{
    const size_t win = (size_t) (fs / 1000.0);
    const double goal = from + (to - from) * fraction;
    for (size_t i = start; i + win < x.size(); i += win / 4)
    {
        const double lvl = windowPeakDb (x, i, win);
        if ((to < from && lvl <= goal) || (to > from && lvl >= goal))
            return i;
    }
    return x.size();
}
} // namespace

TEST_CASE ("Gain computer: hard knee is exact, soft knee is continuous and symmetric", "[pressready]")
{
    REQUIRE (gainComputerDb (-30.0f, -20.0f, 4.0f, 0.0f) == 0.0f);
    REQUIRE (std::fabs (gainComputerDb (-10.0f, -20.0f, 4.0f, 0.0f) - (-7.5f)) < 1e-5f);
    REQUIRE (std::fabs (gainComputerDb (0.0f, -20.0f, infiniteRatio, 0.0f) - (-20.0f)) < 1e-5f);
    // Soft knee: zero at the knee start, matches the hard curve at the knee end, continuous in between.
    REQUIRE (gainComputerDb (-26.0f, -20.0f, 4.0f, 12.0f) == 0.0f);
    REQUIRE (std::fabs (gainComputerDb (-14.0f, -20.0f, 4.0f, 12.0f) - gainComputerDb (-14.0f, -20.0f, 4.0f, 0.0f)) < 1e-5f);
    float prev = gainComputerDb (-27.0f, -20.0f, 4.0f, 12.0f);
    for (float l = -27.0f; l <= -12.0f; l += 0.01f)
    {
        const float g = gainComputerDb (l, -20.0f, 4.0f, 12.0f);
        REQUIRE (g <= prev + 1e-6f);               // monotonic
        REQUIRE (std::fabs (g - prev) < 0.02f);    // no jumps
        prev = g;
    }
}

TEST_CASE ("Below the threshold the output is bit-identical to the input", "[pressready]")
{
    const auto character = GENERATE (Character::clean, Character::punch, Character::glue);
    CompEngine e;
    e.prepare (fs, 256, 1);
    CompParams p;
    p.thresholdDb = -6.0f;
    p.kneeDb = 6.0f;
    p.character = character;
    e.setParams (p, true);
    const auto in = whiteNoise (20000, 0.2); // -14 dBFS peak: below the knee start (-9 dB)
    const auto out = run (e, in);
    for (size_t i = 0; i < in.size(); ++i)
        REQUIRE (out[i] == in[i]);
}

TEST_CASE ("Infinite ratio with zero attack holds the steady-state peak at the threshold", "[pressready]")
{
    const float thresholdDb = GENERATE (-6.0f, -12.0f, -20.0f);
    CompEngine e;
    e.prepare (fs, 256, 1);
    CompParams p;
    p.thresholdDb = thresholdDb;
    p.ratio = infiniteRatio;
    p.attackMs = 0.0f;
    p.releaseMs = 300.0f;
    p.kneeDb = 0.0f;
    e.setParams (p, true);
    const auto out = run (e, sine (1000.0, fs, 48000, 1.0));
    const double steady = windowPeakDb (out, 24000, 24000);
    INFO ("threshold " << thresholdDb << " steady peak " << steady);
    REQUIRE (steady <= thresholdDb + 0.05);
    REQUIRE (steady >= thresholdDb - 0.6); // the slow release lets it breathe a little between peaks
}

TEST_CASE ("Attack and release times match the knobs within 10 %", "[pressready]")
{
    const float attackMs = GENERATE (5.0f, 50.0f);
    const float releaseMs = GENERATE (100.0f, 400.0f);
    CompEngine e;
    e.prepare (fs, 256, 1);
    CompParams p;
    p.thresholdDb = -20.0f;
    p.ratio = 4.0f;
    p.kneeDb = 0.0f;
    p.attackMs = attackMs;
    p.releaseMs = releaseMs;
    e.setParams (p, true);

    // 0.5 s quiet (-40 dB), 1.5 s loud (0 dB), 1.5 s quiet again.
    const size_t quiet = 24000, loud = 72000;
    auto x = sine (2000.0, fs, (int) (quiet + loud + loud), 1.0);
    for (size_t i = 0; i < x.size(); ++i)
        if (i < quiet || i >= quiet + loud) x[i] *= 0.01f;
    const auto out = run (e, x);

    const double loudIn = 0.0, compressed = loudIn + gainComputerDb (0.0f, -20.0f, 4.0f, 0.0f); // -15 dB
    const size_t tA = timeToFraction (out, quiet, loudIn, compressed, 0.632) - quiet;
    const double measuredAttack = 1000.0 * (double) tA / fs;
    INFO ("attack " << attackMs << " ms measured " << measuredAttack);
    REQUIRE (std::fabs (measuredAttack - attackMs) <= attackMs * 0.1 + 1.0); // +1 ms for the 1 ms window

    // Release: after the drop the quiet signal (-40 dB) climbs back from -55 dB to -40 dB.
    const double quietCompressed = -40.0 - 15.0;
    const size_t tR = timeToFraction (out, quiet + loud, quietCompressed, -40.0, 0.632) - (quiet + loud);
    const double measuredRelease = 1000.0 * (double) tR / fs;
    INFO ("release " << releaseMs << " ms measured " << measuredRelease);
    REQUIRE (std::fabs (measuredRelease - releaseMs) <= releaseMs * 0.1 + 1.0);
}

TEST_CASE ("Switching character mid-stream does not click", "[pressready]")
{
    CompEngine e;
    e.prepare (fs, 256, 1);
    CompParams p;
    p.thresholdDb = -24.0f;
    p.ratio = 8.0f;
    p.attackMs = 1.0f;
    e.setParams (p, true);
    auto x = sine (300.0, fs, 48000, 0.9);
    float* ch[] = { x.data() };
    e.process (ch, nullptr, 1, 24000);
    for (auto c : { Character::punch, Character::glue, Character::clean })
    {
        p.character = c;
        e.setParams (p);
        float* ch2[] = { x.data() + 24000 };
        e.process (ch2, nullptr, 1, 8000);
        for (size_t i = 0; i < 8000; ++i) x[24000 + i] = ch2[0][i];
        // shift window
        std::rotate (x.begin() + 24000, x.begin() + 32000, x.end());
    }
    // A 300 Hz sine at <= 0.9 moves at most this much per sample; allow the saturation to add 30 %.
    const double maxStep = 0.9 * 2.0 * pi * 300.0 / fs;
    double worst = 0.0;
    for (size_t i = 24001; i < 48000; ++i)
        worst = std::max (worst, (double) std::fabs (x[i] - x[i - 1]));
    REQUIRE (worst < maxStep * 1.3);
}

TEST_CASE ("Lookahead reports its latency and delays the dry path exactly", "[pressready]")
{
    CompEngine e;
    e.prepare (fs, 256, 1);
    CompParams p;
    p.thresholdDb = 0.0f; // never compresses
    p.lookaheadMs = 5.0f;
    p.mix = 0.0f;
    e.setParams (p, true);
    REQUIRE (e.getLatencySamples() == 240);
    std::vector<float> in (2000, 0.0f);
    in[100] = 1.0f;
    const auto out = run (e, in);
    for (size_t i = 0; i < out.size(); ++i)
        REQUIRE (out[i] == (i == 340 ? 1.0f : 0.0f));
}

TEST_CASE ("External sidechain drives the gain reduction, not the audio", "[pressready]")
{
    CompEngine e;
    e.prepare (fs, 256, 1);
    CompParams p;
    p.thresholdDb = -30.0f;
    p.ratio = infiniteRatio;
    p.attackMs = 0.0f;
    p.kneeDb = 0.0f;
    p.externalSidechain = true;
    e.setParams (p, true);
    auto audio = sine (1000.0, fs, 9600, 0.1);        // -20 dB: would be compressed by its own level
    const auto key = sine (1000.0, fs, 9600, 0.001);  // -60 dB key: stays below the threshold
    float* io[] = { audio.data() };
    const float* sc[] = { key.data() };
    const auto m = e.process (io, sc, 1, 9600);
    REQUIRE (m.gainReductionDb == 0.0f);
    REQUIRE (std::fabs (peak (audio) - 0.1) < 1e-6);
}
