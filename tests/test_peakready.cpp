// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/PeakReady/Source/PeakEngine.h"
#include "../shared/dsp/Loudness.h"
#include "TestHelpers.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

using namespace rommuser::peakready;
using namespace testhelpers;

namespace
{
constexpr double fs = 48000.0;
constexpr int ms = 48;
constexpr int block = 256;

std::vector<float> run (PeakEngine& e, std::vector<float> x)
{
    for (size_t pos = 0; pos < x.size(); pos += (size_t) block)
    {
        float* ch[] = { x.data() + pos };
        e.process (ch, 1, (int) std::min<size_t> ((size_t) block, x.size() - pos));
    }
    return x;
}

// True peak of a signal measured with the same 4x oversampler the plugins use.
double truePeakDb (const std::vector<float>& x)
{
    rommuser::dsp::Oversampler os (block);
    os.prepare (4, block);
    double p = 0.0;
    std::vector<float> chunk ((size_t) block, 0.0f);
    for (size_t pos = 0; pos < x.size() + (size_t) block; pos += (size_t) block)
    {
        std::fill (chunk.begin(), chunk.end(), 0.0f);
        for (size_t i = 0; i < (size_t) block && pos + i < x.size(); ++i)
            chunk[i] = x[pos + i];
        const float* hi = os.upsample (chunk.data(), block);
        for (int j = 0; j < block * 4; ++j)
            p = std::max (p, (double) std::fabs (hi[j]));
    }
    return toDb (p);
}

std::vector<float> testSignal (double gainDb)
{
    const double g = std::pow (10.0, gainDb / 20.0);
    auto x = whiteNoise (40000, g, 7);
    const auto s = sine (1000.0, fs, 20000, g);
    x.insert (x.end(), s.begin(), s.end());
    // Impulses every 10 ms: the worst case for inter-sample peaks after oversampling.
    for (int i = 0; i < 20000; ++i)
        x.push_back (i % 480 == 0 ? (float) g : 0.0f);
    // A 7 kHz sine sampled near its peaks has true peaks well above the sample peaks.
    const auto hi = sine (6857.0, fs, 20000, g);
    x.insert (x.end(), hi.begin(), hi.end());
    // Silence at the end so the measured output is not cut off mid-cycle (an abrupt cut would add
    // Gibbs overshoot to the measurement that the limiter never saw).
    x.insert (x.end(), 2000, 0.0f);
    return x;
}
} // namespace

TEST_CASE ("Peak Ready: the output true peak never passes the ceiling (all styles)")
{
    for (int style = 0; style < 3; ++style)
        for (float ceilingDb : { -1.0f, -0.3f, -6.0f })
        {
            PeakEngine e;
            e.prepare (fs, block, 1);
            PeakParams p;
            p.inputDb = 12.0f;
            p.ceilingDb = ceilingDb;
            p.style = (Style) style;
            e.setParams (p);
            const auto y = run (e, testSignal (0.0));
            INFO ("style " << style << " ceiling " << ceilingDb);
            REQUIRE (truePeakDb (y) <= ceilingDb + 0.05);
            REQUIRE (toDb (peak (y)) <= ceilingDb + 0.001);
            // And it really is limiting, not just turning everything down.
            REQUIRE (toDb (peak (y)) > ceilingDb - 1.0);
        }
}

TEST_CASE ("Peak Ready: below the ceiling the output is the input, bit-exact, delayed by the reported latency")
{
    for (bool tp : { true, false })
    {
        PeakEngine e;
        e.prepare (fs, block, 1);
        PeakParams p;
        p.ceilingDb = -1.0f;
        p.truePeak = tp;
        e.setParams (p);
        const int L = e.getLatencySamples();
        REQUIRE (L > 0);
        auto x = whiteNoise (30000, 0.3, 3);   // -10 dBFS peak: true peaks stay under -1 dB
        const auto y = run (e, x);
        for (size_t i = (size_t) L; i < x.size(); ++i)
            REQUIRE (y[i] == x[i - (size_t) L]);
    }
}

TEST_CASE ("Peak Ready: a sine above the ceiling comes out exactly at the ceiling")
{
    PeakEngine e;
    e.prepare (fs, block, 1);
    PeakParams p;
    p.inputDb = 6.0f;
    p.ceilingDb = -1.0f;
    e.setParams (p);
    const auto y = run (e, sine (1000.0, fs, 48000, 0.9));
    // Steady state: level by projection over the last 500 ms.
    const size_t start = 24000, len = 24000;
    double s = 0.0, c = 0.0;
    for (size_t i = 0; i < len; ++i)
    {
        const double ph = 2.0 * pi * 1000.0 * (double) (start + i) / fs;
        s += y[start + i] * std::sin (ph);
        c += y[start + i] * std::cos (ph);
    }
    const double amp = 2.0 * std::sqrt (s * s + c * c) / (double) len;
    REQUIRE (toDb (amp) == Catch::Approx (-1.0).margin (0.1));
}

TEST_CASE ("Peak Ready: the release time controls how fast the gain recovers")
{
    auto levelAfterBurst = [] (float releaseMs)
    {
        PeakEngine e;
        e.prepare (fs, block, 1);
        PeakParams p;
        p.ceilingDb = -1.0f;
        p.releaseMs = releaseMs;
        p.autoRelease = false;
        e.setParams (p);
        const int L = e.getLatencySamples();
        auto x = sine (1000.0, fs, 200 * ms, 1.0);          // 0 dBFS burst: 1 dB of limiting
        const auto quiet = sine (1000.0, fs, 400 * ms, 0.1);  // then -20 dBFS
        x.insert (x.end(), quiet.begin(), quiet.end());
        const auto y = run (e, x);
        // Level of the quiet part 100 ms after the burst, aligned by the latency.
        const size_t start = (size_t) (200 * ms + L) + 100 * (size_t) ms;
        return toDb (rms (y.data() + start, 5 * ms)) - toDb (rms (x.data() + start - (size_t) L, 5 * ms));
    };

    const double fast = levelAfterBurst (20.0f), slow = levelAfterBurst (800.0f);
    REQUIRE (fast > -0.1);     // 20 ms release: fully recovered after 5 time constants
    REQUIRE (slow < -0.5);     // 800 ms release: still turned down
}

TEST_CASE ("Peak Ready: Aggressive style needs less gain reduction than Transparent for the same loudness push")
{
    auto maxGr = [] (Style s)
    {
        PeakEngine e;
        e.prepare (fs, block, 1);
        PeakParams p;
        p.inputDb = 9.0f;
        p.style = s;
        e.setParams (p);
        auto x = whiteNoise (48128, 0.5, 11);   // a whole number of blocks
        float gr = 0.0f;
        for (size_t pos = 0; pos < x.size(); pos += (size_t) block)
        {
            float* ch[] = { x.data() + pos };
            gr = std::max (gr, e.process (ch, 1, block).gainReductionDb);
        }
        return gr;
    };
    REQUIRE (maxGr (Style::transparent) > 6.0f);
    REQUIRE (maxGr (Style::aggressive) < maxGr (Style::transparent) - 2.0f);
}

TEST_CASE ("Loudness meter: a 997 Hz sine at -20 dBFS in both channels reads -20 LUFS, and silence is gated")
{
    rommuser::dsp::LoudnessMeter m;
    m.prepare (fs, 2);
    auto l = sine (997.0, fs, 5 * 48000, std::pow (10.0, -20.0 / 20.0));
    auto r = l;
    for (size_t pos = 0; pos < l.size(); pos += (size_t) block)
    {
        const float* ch[] = { l.data() + pos, r.data() + pos };
        m.process (ch, 2, std::min (block, (int) (l.size() - pos)));
    }
    REQUIRE (m.getMomentary() == Catch::Approx (-20.0).margin (0.1));
    REQUIRE (m.getShortTerm() == Catch::Approx (-20.0).margin (0.1));
    REQUIRE (m.getIntegrated() == Catch::Approx (-20.0).margin (0.1));

    // Five seconds of silence must not pull the integrated value down (absolute gate).
    std::vector<float> z (5 * 48000, 0.0f);
    for (size_t pos = 0; pos < z.size(); pos += (size_t) block)
    {
        const float* ch[] = { z.data() + pos, z.data() + pos };
        m.process (ch, 2, std::min (block, (int) (z.size() - pos)));
    }
    // The 400 ms windows straddling the cut are quieter but still above the gates, so the integrated
    // value moves a little (as the standard prescribes); the silence itself is excluded.
    REQUIRE (m.getIntegrated() == Catch::Approx (-20.0).margin (0.3));
    REQUIRE (m.getMomentary() < -100.0f);
}

TEST_CASE ("Peak Ready: both channels get the same gain from one shared detector")
{
    PeakEngine e;
    e.prepare (fs, block, 2);
    PeakParams p;
    p.ceilingDb = -1.0f;
    p.inputDb = 6.0f;
    e.setParams (p);
    const int L = e.getLatencySamples();
    auto l = sine (1000.0, fs, 48000, 0.9);
    auto r = sine (1000.0, fs, 48000, 0.05);
    const auto lIn = l, rIn = r;
    for (size_t pos = 0; pos < l.size(); pos += (size_t) block)
    {
        float* ch[] = { l.data() + pos, r.data() + pos };
        e.process (ch, 2, (int) std::min<size_t> ((size_t) block, l.size() - pos));
    }
    const size_t start = 30000;
    // Gain relative to the raw input (the +6 dB input gain is inside the engine): the left channel
    // is pushed 6.9 dB over the ceiling and ends up at the ceiling, the quiet right channel follows
    // with exactly the same gain instead of staying untouched.
    const double gl = toDb (rms (l.data() + start, 4800)) - toDb (rms (lIn.data() + start - (size_t) L, 4800));
    const double gr = toDb (rms (r.data() + start, 4800)) - toDb (rms (rIn.data() + start - (size_t) L, 4800));
    REQUIRE (toDb (peak (l)) == Catch::Approx (-1.0).margin (0.05));
    REQUIRE (gl == Catch::Approx (-0.087).margin (0.05));   // 0.9 -> 0.891 (the ceiling): -0.087 dB net
    REQUIRE (gl == Catch::Approx (gr).margin (0.02));
}
