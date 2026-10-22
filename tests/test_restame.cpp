// Res Tame DSP tests: transparent at depth 0, pink noise untouched, resonant sine tamed, range,
// delta, external sidechain (unmask) and Mid/Side placement.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/ResTame/Source/ResTameEngine.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cmath>
#include <random>
#include <vector>

using namespace rommuser::restame;
using Catch::Approx;

namespace
{
constexpr double sr = 48000.0;
constexpr double pi = 3.14159265358979323846;

struct Stereo { std::vector<float> l, r; };

Stereo whiteNoise (int n, unsigned seed = 1, float sigma = 0.1f)
{
    std::mt19937 rng (seed);
    std::normal_distribution<float> d (0.0f, sigma);
    Stereo s { std::vector<float> ((size_t) n), std::vector<float> ((size_t) n) };
    for (int i = 0; i < n; ++i) { s.l[(size_t) i] = d (rng); s.r[(size_t) i] = d (rng); }
    return s;
}

// Pink-ish noise: white noise through a -3 dB/oct approximation (Paul Kellet's filter).
Stereo pinkNoise (int n, unsigned seed = 1)
{
    auto w = whiteNoise (n, seed, 0.2f);
    Stereo s { std::vector<float> ((size_t) n), std::vector<float> ((size_t) n) };
    for (int ch = 0; ch < 2; ++ch)
    {
        double b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;
        auto& in = ch == 0 ? w.l : w.r;
        auto& out = ch == 0 ? s.l : s.r;
        for (int i = 0; i < n; ++i)
        {
            const double x = in[(size_t) i];
            b0 = 0.99886 * b0 + x * 0.0555179; b1 = 0.99332 * b1 + x * 0.0750759; b2 = 0.96900 * b2 + x * 0.1538520;
            b3 = 0.86650 * b3 + x * 0.3104856; b4 = 0.55000 * b4 + x * 0.5329522; b5 = -0.7616 * b5 - x * 0.0168980;
            out[(size_t) i] = (float) ((b0 + b1 + b2 + b3 + b4 + b5 + b6 + x * 0.5362) * 0.11);
            b6 = x * 0.115926;
        }
    }
    return s;
}

void addSine (Stereo& s, float freq, float amp)
{
    for (size_t i = 0; i < s.l.size(); ++i)
    {
        const float v = amp * (float) std::sin (2.0 * pi * freq * (double) i / sr);
        s.l[i] += v; s.r[i] += v;
    }
}

void run (TameEngine& e, Stereo& s, const Stereo* sc = nullptr, int block = 256)
{
    const int n = (int) s.l.size();
    for (int pos = 0; pos < n; pos += block)
    {
        const int len = std::min (block, n - pos);
        float* io[2] = { s.l.data() + pos, s.r.data() + pos };
        const float* scp[2] = { sc ? sc->l.data() + pos : nullptr, sc ? sc->r.data() + pos : nullptr };
        e.process (io, sc ? scp : nullptr, 2, len);
    }
}

float rmsDb (const std::vector<float>& x, int from, int to)
{
    double e = 0.0;
    for (int i = from; i < to; ++i) e += x[(size_t) i] * x[(size_t) i];
    return (float) (10.0 * std::log10 (std::max (e / (to - from), 1e-20)));
}

// Amplitude of a sine in x over [from, to) by projection.
float sineAmplitude (const std::vector<float>& x, float freq, int from, int to)
{
    double s = 0.0, c = 0.0;
    for (int i = from; i < to; ++i)
    {
        const double ph = 2.0 * pi * freq * i / sr;
        s += x[(size_t) i] * std::sin (ph);
        c += x[(size_t) i] * std::cos (ph);
    }
    return (float) (2.0 * std::sqrt (s * s + c * c) / (to - from));
}

TameParams base()
{
    TameParams p;
    p.depthDb = 12.0f; p.sharpness = 0.7f; p.selectivity = 0.5f; p.thresholdDb = 6.0f;
    p.attackMs = 20.0f; p.releaseMs = 200.0f; p.lowHz = 50.0f; p.highHz = 18000.0f;
    return p;
}
} // namespace

TEST_CASE ("Depth 0 is the input delayed by the reported latency", "[restame]")
{
    TameEngine e;
    e.prepare (sr, 2);
    auto p = base();
    p.depthDb = 0.0f;
    e.setParams (p);
    auto in = whiteNoise (48000, 3, 0.3f);
    auto out = in;
    run (e, out);
    const int d = e.getLatencySamples();
    REQUIRE (d == 2048);
    float maxErr = 0.0f;
    for (int i = d; i < 48000; ++i)
        maxErr = std::max (maxErr, std::fabs (out.l[(size_t) i] - in.l[(size_t) (i - d)]));
    REQUIRE (maxErr < 1e-5f);
}

TEST_CASE ("Pink noise without resonances loses less than 0.5 dB on average", "[restame]")
{
    TameEngine e;
    e.prepare (sr, 2);
    e.setParams (base());
    auto in = pinkNoise (48000 * 4, 5);
    auto out = in;
    run (e, out);
    const int d = e.getLatencySamples();
    const float inDb = rmsDb (in.l, 24000, 48000 * 4 - d);
    const float outDb = rmsDb (out.l, 24000 + d, 48000 * 4);
    INFO ("loss " << inDb - outDb << " dB");
    REQUIRE (inDb - outDb < 0.5f);
    REQUIRE (inDb - outDb > -0.1f);
}

TEST_CASE ("A sine 12 dB above the noise is attenuated by at least 0.8 x Depth", "[restame]")
{
    TameEngine e;
    e.prepare (sr, 2);
    auto p = base();
    p.depthDb = 12.0f;
    e.setParams (p);
    // Per-bin noise power with a Hann window is sigma^2 * 3N/8; a sine of amplitude A peaks at (A N/4)^2.
    // 12 dB above -> A = sigma * sqrt (15.85 * 6 / N).
    const float sigma = 0.1f;
    const float amp = sigma * std::sqrt (15.85f * 6.0f / 2048.0f);
    auto in = whiteNoise (48000 * 3, 11, sigma);
    addSine (in, 1000.0f, amp);
    auto out = in;
    run (e, out);
    const int d = e.getLatencySamples();
    const float before = sineAmplitude (in.l, 1000.0f, 48000, 48000 * 3 - d);
    const float after = sineAmplitude (out.l, 1000.0f, 48000 + d, 48000 * 3);
    const float reductionDb = 20.0f * std::log10 (before / after);
    INFO ("reduction " << reductionDb << " dB");
    REQUIRE (reductionDb >= 0.8f * 12.0f);
    REQUIRE (reductionDb <= 13.0f);
}

TEST_CASE ("Resonances outside the frequency range are left alone", "[restame]")
{
    TameEngine e;
    e.prepare (sr, 2);
    auto p = base();
    p.lowHz = 2000.0f; p.highHz = 10000.0f;
    e.setParams (p);
    const float sigma = 0.1f, amp = sigma * std::sqrt (15.85f * 6.0f / 2048.0f) * 2.0f;
    auto in = whiteNoise (48000 * 3, 13, sigma);
    addSine (in, 300.0f, amp);
    auto out = in;
    run (e, out);
    const int d = e.getLatencySamples();
    const float before = sineAmplitude (in.l, 300.0f, 48000, 48000 * 3 - d);
    const float after = sineAmplitude (out.l, 300.0f, 48000 + d, 48000 * 3);
    REQUIRE (std::fabs (20.0f * std::log10 (before / after)) < 0.5f);
}

TEST_CASE ("Delta outputs exactly what was removed", "[restame]")
{
    auto p = base();
    const float sigma = 0.1f, amp = sigma * std::sqrt (15.85f * 6.0f / 2048.0f) * 2.0f;
    auto in = whiteNoise (48000 * 2, 17, sigma);
    addSine (in, 1500.0f, amp);

    TameEngine normal, delta;
    normal.prepare (sr, 2); delta.prepare (sr, 2);
    normal.setParams (p);
    p.delta = true;
    delta.setParams (p);
    auto a = in, b = in;
    run (normal, a);
    run (delta, b);
    const int d = normal.getLatencySamples();
    float maxErr = 0.0f;
    for (int i = d; i < 48000 * 2; ++i)
        maxErr = std::max (maxErr, std::fabs ((in.l[(size_t) (i - d)] - a.l[(size_t) i]) - b.l[(size_t) i]));
    REQUIRE (maxErr < 1e-5f);
    // And the delta really carries the sine that was cut.
    REQUIRE (sineAmplitude (b.l, 1500.0f, 48000 + d, 96000) > amp * 0.5f);
}

TEST_CASE ("External sidechain cuts this track where the other track peaks (unmask)", "[restame]")
{
    TameEngine e;
    e.prepare (sr, 2);
    auto p = base();
    p.externalSidechain = true;
    p.depthDb = 12.0f;
    e.setParams (p);
    auto main = whiteNoise (48000 * 3, 19, 0.1f);
    auto sc = whiteNoise (48000 * 3, 23, 0.02f);
    addSine (sc, 2000.0f, 0.3f); // the other track has a strong 2 kHz component
    auto out = main;
    run (e, out, &sc);
    const int d = e.getLatencySamples();
    // Band energy around 2 kHz drops a lot, overall energy barely moves.
    auto bandDb = [&] (const std::vector<float>& x, int from, int to)
    {
        double e2 = 0.0;
        for (float f = 1950.0f; f <= 2050.0f; f += 23.4375f) { const float a = sineAmplitude (x, f, from, to); e2 += a * a; }
        return (float) (10.0 * std::log10 (e2 + 1e-20));
    };
    const float bandBefore = bandDb (main.l, 48000, 48000 * 3 - d), bandAfter = bandDb (out.l, 48000 + d, 48000 * 3);
    const float totalBefore = rmsDb (main.l, 48000, 48000 * 3 - d), totalAfter = rmsDb (out.l, 48000 + d, 48000 * 3);
    INFO ("band drop " << bandBefore - bandAfter << " dB, total drop " << totalBefore - totalAfter << " dB");
    REQUIRE (bandBefore - bandAfter > 8.0f);
    REQUIRE (totalBefore - totalAfter < 0.7f);
}

TEST_CASE ("Side placement leaves a mono signal untouched", "[restame]")
{
    TameEngine e;
    e.prepare (sr, 2);
    auto p = base();
    p.placement = Placement::side;
    e.setParams (p);
    auto in = whiteNoise (48000 * 2, 29, 0.1f);
    in.r = in.l; // mono
    addSine (in, 800.0f, 0.1f);
    auto out = in;
    run (e, out);
    const int d = e.getLatencySamples();
    float maxErr = 0.0f;
    for (int i = d; i < 48000 * 2; ++i)
        maxErr = std::max (maxErr, std::fabs (out.l[(size_t) i] - in.l[(size_t) (i - d)]));
    REQUIRE (maxErr < 1e-4f);
}
