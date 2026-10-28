// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/ClipReady/Source/ClipEngine.h"
#include "TestHelpers.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

using namespace rommuser::clipready;
using namespace testhelpers;

namespace
{
constexpr double fs = 44100.0;
constexpr int hostBlock = 480; // deliberately not a power of two

// Runs a mono signal through a fresh engine in host-sized blocks.
std::vector<float> run (const ClipParams& p, std::vector<float> x, ClipMeters* meters = nullptr, int maxBlock = 512)
{
    ClipEngine e;
    e.prepare (fs, maxBlock, 1);
    e.setParams (p, true);
    ClipMeters total;
    for (size_t pos = 0; pos < x.size(); pos += hostBlock)
    {
        const int n = (int) std::min<size_t> (hostBlock, x.size() - pos);
        float* ch[] = { x.data() + pos };
        const auto m = e.process (ch, 1, n);
        total.inputPeak = std::max (total.inputPeak, m.inputPeak);
        total.outputPeak = std::max (total.outputPeak, m.outputPeak);
        total.clipDb = std::max (total.clipDb, m.clipDb);
    }
    if (meters)
        *meters = total;
    return x;
}

int latencyFor (int factor)
{
    ClipEngine e;
    e.prepare (fs, 512, 1);
    ClipParams p;
    p.oversampling = factor;
    e.setParams (p, true);
    return e.getLatencySamples();
}
} // namespace

TEST_CASE ("Clip curve is bounded, odd, monotonic and continuous at the knee", "[clipready]")
{
    for (float s : { 0.0f, 0.2f, 0.5f, 1.0f })
    {
        float prev = -2.0f;
        for (int i = -4000; i <= 4000; ++i)
        {
            const float x = (float) i / 1000.0f;
            const float y = clipCurve (x, s);
            REQUIRE (std::fabs (y) <= 1.0f);
            REQUIRE (y >= prev);
            REQUIRE (clipCurve (-x, s) == -y);
            prev = y;
        }
        const float t = 1.0f - s;
        REQUIRE (std::fabs (clipCurve (t + 1e-4f, s) - clipCurve (t, s)) < 2e-4f);
    }
}

TEST_CASE ("Mix at 0% returns the input exactly, delayed by the reported latency", "[clipready]")
{
    const int factor = GENERATE (1, 2, 4, 8);
    ClipParams p;
    p.oversampling = factor;
    p.mix = 0.0f;
    p.inputDb = 12.0f;
    p.strictCeiling = false; // with strict on, dry peaks above the ceiling would be clamped (by design)
    const auto in = whiteNoise (20000, 0.9);
    const auto out = run (p, in);
    const int lat = latencyFor (factor);
    for (size_t i = (size_t) lat; i < out.size(); ++i)
        REQUIRE (out[i] == in[i - (size_t) lat]);
}

TEST_CASE ("Output sample peak never exceeds the ceiling", "[clipready]")
{
    const int factor = GENERATE (1, 2, 4, 8);
    const float ceilingDb = GENERATE (0.0f, -0.3f, -6.0f);
    const float softness = GENERATE (0.0f, 0.5f, 1.0f);

    ClipParams p;
    p.oversampling = factor;
    p.ceilingDb = ceilingDb;
    p.softness = softness;
    p.inputDb = 24.0f;

    const float ceiling = std::pow (10.0f, ceilingDb / 20.0f);
    for (const auto& sig : { whiteNoise (30000, 1.0), sine (60.0, fs, 30000), sine (11025.0, fs, 30000) })
    {
        const auto out = run (p, sig);
        REQUIRE (peak (out) <= ceiling * (1.0f + 1e-6f));
    }
}

TEST_CASE ("Strict ceiling bounds the final output for every Mix and Output setting", "[clipready]")
{
    // Issue #34: Mix < 100 % lets dry through and Output can push wet over the ceiling.
    const float mix = GENERATE (0.0f, 0.5f, 1.0f);
    const float outputDb = GENERATE (0.0f, 12.0f);
    const bool link = GENERATE (false, true);
    ClipParams p;
    p.oversampling = 1;
    p.inputDb = 12.0f;
    p.ceilingDb = -6.0f;
    p.softness = 0.0f;
    p.mix = mix;
    p.outputDb = outputDb;
    p.gainLink = link;
    const float ceiling = std::pow (10.0f, -6.0f / 20.0f);
    const auto out = run (p, std::vector<float> (20000, 0.9f));
    INFO ("mix " << mix << " output " << outputDb << " link " << link);
    REQUIRE (peak (out) <= ceiling * (1.0f + 1e-6f));
}

TEST_CASE ("Signals below the knee pass untouched apart from the latency", "[clipready]")
{
    const int factor = GENERATE (2, 4, 8);
    ClipParams p;
    p.oversampling = factor;
    p.ceilingDb = 0.0f;
    p.softness = 0.35f; // knee starts at 0.65 (-3.7 dBFS)
    const auto in = sine (1000.0, fs, 30000, 0.1);
    const auto out = run (p, in);
    const int lat = latencyFor (factor);

    double err = 0.0, sig = 0.0;
    for (size_t i = 4096; i < out.size(); ++i)
    {
        const double d = out[i] - in[i - (size_t) lat];
        err += d * d;
        sig += (double) in[i - (size_t) lat] * in[i - (size_t) lat];
    }
    REQUIRE (10.0 * std::log10 (err / sig) < -90.0);
}

TEST_CASE ("Delta plus the clipped signal reconstructs the gained dry signal", "[clipready]")
{
    ClipParams p;
    p.inputDb = 9.0f;
    p.oversampling = 4;
    const auto in = whiteNoise (20000, 0.7);

    auto wetP = p;
    const auto wet = run (wetP, in);
    auto deltaP = p;
    deltaP.delta = true;
    const auto delta = run (deltaP, in);

    const float g = std::pow (10.0f, 9.0f / 20.0f);
    const int lat = latencyFor (4);
    for (size_t i = (size_t) lat; i < in.size(); ++i)
        REQUIRE (std::fabs (wet[i] + delta[i] - g * in[i - (size_t) lat]) < 1e-5f);
}

TEST_CASE ("Gain link keeps unclipped material at its original level", "[clipready]")
{
    ClipParams p;
    p.inputDb = 6.0f;
    p.gainLink = true;
    p.ceilingDb = 0.0f;
    p.softness = 0.2f;
    const auto in = sine (440.0, fs, 20000, 0.1);
    const auto out = run (p, in);
    const int lat = latencyFor (p.oversampling);
    REQUIRE (std::fabs (rms (out.data() + 8192, 8192) - rms (in.data() + 8192 - lat, 8192)) < 1e-4);
}

TEST_CASE ("Meters report input peak and how far it went over the ceiling", "[clipready]")
{
    ClipParams p;
    p.inputDb = 6.0f;
    p.ceilingDb = -6.0f;
    ClipMeters m;
    run (p, sine (100.0, fs, 20000, 0.5), &m);
    REQUIRE (std::fabs (m.inputPeak - 1.0f) < 0.01f);                 // 0.5 * +6 dB
    REQUIRE (std::fabs (m.clipDb - 6.0f) < 0.2f);                     // 0 dBFS peak vs -6 dBFS ceiling
    REQUIRE (m.outputPeak <= std::pow (10.0f, -6.0f / 20.0f) + 1e-6f);
}

TEST_CASE ("Oversampling keeps clipping aliases low", "[clipready][aliasing]")
{
    // A 5 kHz tone driven 12 dB into the ceiling. Measures every component in 20 Hz - 20 kHz that
    // is not a harmonic of the tone, i.e. the aliased energy.
    struct Case { int factor; float softness; double maxAliasDb; };
    const auto c = GENERATE (Case { 1, 0.0f, 0.0 },  // reference only: no oversampling is expected to alias
                             Case { 8, 0.0f, 0.0 },  // reference only: a hard clip aliases even at 8x
                             Case { 4, 0.5f, -80.0 },
                             Case { 8, 0.5f, -100.0 },
                             Case { 8, 1.0f, -130.0 });

    ClipParams p;
    p.oversampling = c.factor;
    p.softness = c.softness;
    p.inputDb = 12.0f;
    p.ceilingDb = 0.0f;
    p.strictCeiling = false;

    const size_t n = 1 << 15;
    const double f0 = fs * 3715.0 / (double) n; // ~5 kHz, bin-centred
    const auto out = run (p, sine (f0, fs, (int) n + 8192, 0.5));
    const auto spec = powerSpectrum (out.data() + 8192, n);
    const double alias = inharmonicLevelDb (spec, fs, n, f0, 20.0, 20000.0);

    INFO ("factor " << c.factor << " softness " << c.softness << " alias level " << alias << " dBFS");
    WARN ("alias level x" << c.factor << " softness " << c.softness << ": " << alias << " dBFS");
    if (c.maxAliasDb < 0.0)
        REQUIRE (alias < c.maxAliasDb);
}

TEST_CASE ("Without strict ceiling the overshoot stays small", "[clipready]")
{
    ClipParams p;
    p.oversampling = 4;
    p.softness = 0.5f;
    p.inputDb = 12.0f;
    p.ceilingDb = 0.0f;
    p.strictCeiling = false;
    const auto out = run (p, sine (5000.0, fs, 30000, 0.5));
    const double over = toDb (peak (out));
    WARN ("overshoot without strict ceiling: " << over << " dB");
    REQUIRE (over < 1.5);
}

TEST_CASE ("HQ (ADAA) lowers clipping aliases and keeps the ceiling", "[clipready][aliasing][hq]")
{
    struct Case { int factor; float softness; };
    const auto c = GENERATE (Case { 1, 0.0f }, Case { 2, 0.0f }, Case { 4, 0.0f }, Case { 2, 0.5f }, Case { 4, 0.5f });

    ClipParams p;
    p.oversampling = c.factor;
    p.softness = c.softness;
    p.inputDb = 12.0f;
    p.ceilingDb = 0.0f;
    p.strictCeiling = false;

    const size_t n = 1 << 15;
    const double f0 = fs * 3715.0 / (double) n;
    double alias[2];
    for (int hq = 0; hq < 2; ++hq)
    {
        p.hq = hq == 1;
        const auto out = run (p, sine (f0, fs, (int) n + 8192, 0.5));
        const auto spec = powerSpectrum (out.data() + 8192, n);
        alias[hq] = inharmonicLevelDb (spec, fs, n, f0, 20.0, 20000.0);
    }
    WARN ("clip " << c.factor << "x softness " << c.softness << " alias normal " << alias[0] << " / HQ " << alias[1] << " dBFS");
    REQUIRE (alias[1] < alias[0] - 6.0);

    // The ADAA output is an average of the curve, so the strict ceiling still holds.
    p.hq = true;
    p.strictCeiling = true;
    p.ceilingDb = -6.0f;
    const auto out = run (p, whiteNoise (20000, 1.0, 5));
    REQUIRE (peak (out) <= std::pow (10.0, -6.0 / 20.0) + 1e-6);
}

TEST_CASE ("HQ below the knee: flat, and aligned with the dry path (reported latency)", "[clipready][hq]")
{
    const int factor = GENERATE (2, 4, 8);
    ClipParams p;
    p.oversampling = factor;
    p.hq = true;
    p.softness = 0.35f;
    p.ceilingDb = 0.0f;
    p.mix = 0.5f; // half dry: any misalignment between dry and wet would comb at high frequencies

    ClipEngine e;
    e.prepare (fs, 512, 1);
    e.setParams (p, true);
    const int lat = e.getLatencySamples();
    for (double f : { 1000.0, 15000.0, 19000.0 })
    {
        e.reset();
        auto x = sine (f, fs, 16384, 0.1);
        const auto in = x;
        for (size_t pos = 0; pos < x.size(); pos += 512)
        {
            float* ch[] = { x.data() + pos };
            e.process (ch, 1, (int) std::min<size_t> (512, x.size() - pos));
        }
        double err = 0.0, sig = 0.0;
        for (size_t i = 4096; i < x.size(); ++i)
        {
            const double d = (double) x[i] - in[i - (size_t) lat];
            err += d * d;
            sig += (double) in[i] * in[i];
        }
        const double nullDb = 10.0 * std::log10 (err / sig);
        INFO ("factor " << factor << " freq " << f << " null " << nullDb << " dB");
        REQUIRE (nullDb < -60.0);
    }
}
