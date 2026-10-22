// Wide Ready DSP tests: fixed delay at depth 0, interpolation purity, through-zero cancellation,
// micro-pitch detune, click-free mode switching, flanger stability and bit-exact dry.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/WideReady/Source/WideEngine.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cmath>
#include <random>
#include <vector>

using namespace rommuser::wideready;
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

void run (WideEngine& e, Stereo& s, int block = 256)
{
    const int n = (int) s.l.size();
    for (int pos = 0; pos < n; pos += block)
    {
        float* io[2] = { s.l.data() + pos, s.r.data() + pos };
        e.process (io, 2, std::min (block, n - pos));
    }
}

// Fundamental amplitude and everything-else level of a steady sine (projection over whole cycles).
struct Purity { float fundamentalDb, residualDb; };
Purity purity (const std::vector<float>& x, float freq, int skip)
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
    const double as = 2.0 * s / len, ac = 2.0 * c / len;
    double residual = 0.0;
    for (int i = 0; i < len; ++i)
    {
        const double ph = 2.0 * pi * freq * (skip + i) / sr;
        const double fit = as * std::sin (ph) + ac * std::cos (ph);
        residual += (x[(size_t) (skip + i)] - fit) * (x[(size_t) (skip + i)] - fit);
    }
    const double amp = std::sqrt (as * as + ac * ac);
    const double rms = std::sqrt (residual / len);
    return { (float) (20.0 * std::log10 (std::max (amp, 1e-12))), (float) (20.0 * std::log10 (std::max (rms, 1e-12))) };
}

float zeroCrossingFrequency (const std::vector<float>& x, int from, int to)
{
    int crossings = 0, first = -1, last = -1;
    for (int i = from + 1; i < to; ++i)
        if (x[(size_t) i - 1] < 0.0f && x[(size_t) i] >= 0.0f)
        {
            if (first < 0) first = i;
            last = i;
            ++crossings;
        }
    return crossings > 1 ? (float) ((crossings - 1) * sr / (last - first)) : 0.0f;
}
} // namespace

TEST_CASE ("Depth 0 chorus is the input delayed by exactly the base delay", "[wideready]")
{
    WideEngine e;
    e.prepare (sr, 2);
    WideParams p;
    p.mode = Mode::chorus; p.voices = 1; p.depth = 0.0f; p.delayMs = 10.0f; p.mix = 1.0f; p.spread = 0.0f;
    e.setParams (p);
    auto in = noise (24000, 3);
    auto out = in;
    run (e, out);
    const int d = 480; // 10 ms at 48 kHz
    for (int i = d; i < 24000; ++i)
    {
        REQUIRE (out.l[(size_t) i] == in.l[(size_t) (i - d)]);
        REQUIRE (out.r[(size_t) i] == in.r[(size_t) (i - d)]);
    }
}

TEST_CASE ("Fractional delay interpolation keeps a 10 kHz sine pure (residual below -80 dB)", "[wideready]")
{
    WideEngine e;
    e.prepare (sr, 2);
    WideParams p;
    p.mode = Mode::flanger; p.depth = 0.0f; p.feedback = 0.0f; p.mix = 1.0f; p.spread = 0.0f;
    p.delayMs = 5.37f / 48.0f; // 5.37 samples: a fractional read
    e.setParams (p);
    auto s = sine (10000.0f, 48000);
    run (e, s);
    const auto pu = purity (s.l, 10000.0f, 8000);
    REQUIRE (pu.fundamentalDb > 20.0f * std::log10 (0.5f) - 1.5f); // Hermite droop at 10 kHz is small
    REQUIRE (pu.residualDb - pu.fundamentalDb < -80.0f);
}

TEST_CASE ("Through-zero flanger with inverted wet cancels completely at depth 0", "[wideready]")
{
    WideEngine e;
    e.prepare (sr, 2);
    WideParams p;
    p.mode = Mode::flanger; p.depth = 0.0f; p.feedback = 0.0f; p.mix = 0.5f; p.delayMs = 2.0f;
    p.throughZero = true; p.invert = true; p.spread = 0.0f;
    e.setParams (p);
    auto s = noise (24000, 5);
    run (e, s);
    float peak = 0.0f;
    for (int i = 2000; i < 24000; ++i) peak = std::max (peak, std::fabs (s.l[(size_t) i]));
    REQUIRE (peak < 1e-5f);

    // Same thing without through-zero: the dry is not delayed, so nothing cancels.
    WideEngine e2;
    e2.prepare (sr, 2);
    p.throughZero = false;
    e2.setParams (p);
    auto s2 = noise (24000, 5);
    run (e2, s2);
    float peak2 = 0.0f;
    for (int i = 2000; i < 24000; ++i) peak2 = std::max (peak2, std::fabs (s2.l[(size_t) i]));
    REQUIRE (peak2 > 0.2f);
}

TEST_CASE ("Micro-pitch detunes the left channel up and the right channel down by the chosen cents", "[wideready]")
{
    WideEngine e;
    e.prepare (sr, 2);
    WideParams p;
    p.mode = Mode::microPitch; p.cents = 25.0f; p.mix = 1.0f;
    e.setParams (p);
    const int n = 48000 * 3;
    auto s = sine (440.0f, n);
    run (e, s);
    const float expectedUp = 440.0f * std::exp2 (25.0f / 1200.0f);
    const float expectedDown = 440.0f * std::exp2 (-25.0f / 1200.0f);
    REQUIRE (zeroCrossingFrequency (s.l, 48000, n) == Approx (expectedUp).margin (0.5));
    REQUIRE (zeroCrossingFrequency (s.r, 48000, n) == Approx (expectedDown).margin (0.5));

    // Level stays steady: the two crossfaded taps are power complementary.
    float minPeak = 1.0f, maxPeak = 0.0f;
    for (int start = 48000; start + 2400 <= n; start += 2400)
    {
        float pk = 0.0f;
        for (int i = start; i < start + 2400; ++i) pk = std::max (pk, std::fabs (s.l[(size_t) i]));
        minPeak = std::min (minPeak, pk);
        maxPeak = std::max (maxPeak, pk);
    }
    REQUIRE (maxPeak / minPeak < 1.6f);
}

TEST_CASE ("Switching mode mid-stream does not click", "[wideready]")
{
    WideEngine e;
    e.prepare (sr, 2);
    WideParams p;
    p.mode = Mode::chorus; p.voices = 3; p.depth = 0.8f; p.mix = 0.5f;
    e.setParams (p);
    auto s = sine (440.0f, 48000);
    float* io[2] = { s.l.data(), s.r.data() };
    e.process (io, 2, 24000);
    p.mode = Mode::flanger; p.feedback = 0.7f; p.delayMs = 1.0f;
    e.setParams (p);
    float* io2[2] = { s.l.data() + 24000, s.r.data() + 24000 };
    e.process (io2, 2, 24000);

    // A click is a sample-to-sample jump far larger than the signal's own slope. Compare the jumps
    // around the switch with the jumps in the steady regions before and after it.
    auto maxStepIn = [&] (int from, int to)
    {
        float v = 0.0f;
        for (int i = from; i < to; ++i) v = std::max (v, std::fabs (s.l[(size_t) i] - s.l[(size_t) i - 1]));
        return v;
    };
    const float steady = std::max (maxStepIn (10000, 23000), maxStepIn (27000, 48000));
    REQUIRE (maxStepIn (23000, 27000) < steady * 1.1f);
}

TEST_CASE ("Flanger at maximum feedback stays bounded", "[wideready]")
{
    for (float fb : { 0.95f, -0.95f })
    {
        WideEngine e;
        e.prepare (sr, 2);
        WideParams p;
        p.mode = Mode::flanger; p.depth = 1.0f; p.feedback = fb; p.delayMs = 1.0f; p.rateHz = 2.0f; p.mix = 0.5f;
        e.setParams (p);
        auto s = noise (96000);
        run (e, s);
        float peak = 0.0f;
        for (float v : s.l) { REQUIRE (std::isfinite (v)); peak = std::max (peak, std::fabs (v)); }
        REQUIRE (peak < 4.0f);
    }
}

TEST_CASE ("Chorus spread 0 keeps channels identical, spread 100 % separates them", "[wideready]")
{
    WideParams p;
    p.mode = Mode::chorus; p.voices = 2; p.depth = 0.7f; p.mix = 0.5f; p.spread = 0.0f;
    WideEngine e;
    e.prepare (sr, 2);
    e.setParams (p);
    auto s = sine (500.0f, 48000);
    run (e, s);
    for (size_t i = 0; i < s.l.size(); ++i) REQUIRE (s.l[i] == s.r[i]);

    p.spread = 1.0f;
    WideEngine e2;
    e2.prepare (sr, 2);
    e2.setParams (p);
    auto s2 = sine (500.0f, 48000);
    run (e2, s2);
    float maxDiff = 0.0f;
    for (size_t i = 4000; i < s2.l.size(); ++i) maxDiff = std::max (maxDiff, std::fabs (s2.l[i] - s2.r[i]));
    REQUIRE (maxDiff > 0.05f);
}

TEST_CASE ("Mix 0 passes the dry signal bit-exact in every mode", "[wideready]")
{
    for (auto mode : { Mode::chorus, Mode::flanger, Mode::microPitch })
    {
        WideEngine e;
        e.prepare (sr, 2);
        WideParams p;
        p.mode = mode; p.mix = 0.0f; p.depth = 1.0f; p.feedback = 0.9f; p.voices = 3;
        e.setParams (p);
        auto in = noise (24000, 7);
        auto out = in;
        run (e, out);
        REQUIRE (out.l == in.l);
        REQUIRE (out.r == in.r);
    }
}
