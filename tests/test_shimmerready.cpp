// Shimmer Ready DSP tests: plain delay at pitch 0 (sample-exact, synced to the tempo), bit-exact dry
// at mix 0, first repeat pitched within 5 cents, reverse, feedback stability, freeze, loop filters,
// shimmer reverb tail.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/ShimmerReady/Source/ShimmerEngine.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cmath>
#include <random>
#include <vector>

using namespace rommuser::shimmerready;
using Catch::Approx;

namespace
{
constexpr double sr = 48000.0;
constexpr double pi = 3.14159265358979323846;

std::vector<float> noise (int n, unsigned seed = 1)
{
    std::mt19937 rng (seed);
    std::uniform_real_distribution<float> d (-0.5f, 0.5f);
    std::vector<float> v ((size_t) n);
    for (auto& x : v) x = d (rng);
    return v;
}

std::vector<float> sine (double hz, int n, float amp = 0.5f)
{
    std::vector<float> v ((size_t) n);
    for (int i = 0; i < n; ++i) v[(size_t) i] = amp * (float) std::sin (2.0 * pi * hz * i / sr);
    return v;
}

std::vector<float> run (ShimmerEngine& e, const std::vector<float>& in, int block = 256)
{
    std::vector<float> out (in.size());
    for (size_t pos = 0; pos < in.size(); pos += (size_t) block)
    {
        const int len = (int) std::min ((size_t) block, in.size() - pos);
        std::vector<float> buf (in.begin() + (long) pos, in.begin() + (long) pos + len);
        float* ptr = buf.data();
        e.process (&ptr, 1, len);
        std::copy (buf.begin(), buf.end(), out.begin() + (long) pos);
    }
    return out;
}

// Frequency from interpolated upward zero crossings (precise on a clean-ish sine).
double crossingHz (const std::vector<float>& v, size_t from, size_t to)
{
    double first = -1.0, last = -1.0;
    int count = 0;
    for (size_t i = from + 1; i < to; ++i)
        if (v[i - 1] < 0.0f && v[i] >= 0.0f)
        {
            const double t = (double) (i - 1) + (double) (-v[i - 1]) / (double) (v[i] - v[i - 1]);
            if (first < 0.0) first = t;
            last = t;
            ++count;
        }
    return count > 1 ? (double) (count - 1) * sr / (last - first) : 0.0;
}

// Spectral peak (Hann window, direct DFT around `centreHz`, parabolic interpolation on dB).
// This is the pitch the ear hears; a delay-line pitch shifter resets its phase once per grain, so a
// long-term zero-crossing average would be biased by up to 1 / (grain / 2) Hz.
double peakHz (const std::vector<float>& v, size_t from, size_t to, double centreHz)
{
    const size_t n = to - from;
    const double binHz = sr / (double) n;
    const int b0 = (int) std::floor (centreHz * 0.75 / binHz), b1 = (int) std::ceil (centreHz * 1.25 / binHz);
    std::vector<double> mag;
    for (int b = b0; b <= b1; ++b)
    {
        double re = 0.0, im = 0.0;
        for (size_t i = 0; i < n; ++i)
        {
            const double w = 0.5 - 0.5 * std::cos (2.0 * pi * (double) i / (double) n);
            const double ph = -2.0 * pi * (double) b * (double) i / (double) n;
            re += v[from + i] * w * std::cos (ph);
            im += v[from + i] * w * std::sin (ph);
        }
        mag.push_back (20.0 * std::log10 (std::sqrt (re * re + im * im) + 1.0e-12));
    }
    size_t k = 1;
    for (size_t i = 1; i + 1 < mag.size(); ++i) if (mag[i] > mag[k]) k = i;
    const double a = mag[k - 1], b = mag[k], c = mag[k + 1];
    const double delta = 0.5 * (a - c) / (a - 2.0 * b + c);
    return ((double) (b0 + (int) k) + delta) * binHz;
}

float rms (const std::vector<float>& v, size_t from, size_t to)
{
    double e = 0.0;
    for (size_t i = from; i < to; ++i) e += (double) v[i] * v[i];
    return (float) std::sqrt (e / (double) (to - from));
}

ShimmerParams plain()
{
    ShimmerParams p;
    p.pitchSemis = 0.0f;
    p.feedback = 0.0f;
    p.mix = 1.0f;
    p.shimmer = 0.0f;
    p.width = 0.0f;
    return p;
}
} // namespace

TEST_CASE ("Shimmer Ready: pitch 0 and mix 100 % is a plain delay, sample-exact at the synced time", "[shimmerready]")
{
    ShimmerEngine e;
    e.prepare (sr, 256, 1);
    auto p = plain();
    p.sync = true;
    p.division = 5;      // 1/8 at 120 BPM = 12000 samples
    p.bpm = 120.0;
    e.setParams (p);
    REQUIRE (e.getDelaySamples() == Approx (12000.0));
    const auto in = noise (60000);
    const auto out = run (e, in);
    for (size_t i = 0; i < 12000; ++i)
        REQUIRE (std::fabs (out[i]) < 1.0e-7f);
    for (size_t i = 12000; i < in.size(); ++i)
        REQUIRE (out[i] == Approx (in[i - 12000]).margin (1.0e-6));

    // With feedback 0.5 the second repeat is half the first.
    ShimmerEngine e2;
    e2.prepare (sr, 256, 1);
    p.feedback = 0.5f;
    e2.setParams (p);
    const auto out2 = run (e2, in);
    for (size_t i = 24000; i < 36000; ++i)
        REQUIRE (out2[i] == Approx (in[i - 12000] + 0.5f * in[i - 24000]).margin (1.0e-5));
}

TEST_CASE ("Shimmer Ready: mix 0 is bit-exact dry", "[shimmerready]")
{
    ShimmerEngine e;
    e.prepare (sr, 256, 1);
    auto p = plain();
    p.pitchSemis = 12.0f;
    p.feedback = 0.7f;
    p.shimmer = 0.5f;
    p.mix = 0.0f;
    e.setParams (p);
    const auto in = noise (48000);
    const auto out = run (e, in);
    for (size_t i = 0; i < in.size(); ++i)
        REQUIRE (out[i] == in[i]);
}

TEST_CASE ("Shimmer Ready: the first repeat is pitched +12 st within 5 cents", "[shimmerready]")
{
    // The shifter sits in the feedback path: the first echo is the input as is, the pitch applies from
    // the second repeat on (each repeat climbs again). Measure the first pitched repeat.
    ShimmerEngine e;
    e.prepare (sr, 256, 1);
    auto p = plain();
    p.sync = false;
    p.timeMs = 250.0f;   // 12000 samples
    p.pitchSemis = 12.0f;
    p.feedback = 0.9f;
    e.setParams (p);
    const int burst = 12000;
    auto in = sine (220.0, 120000);
    for (size_t i = (size_t) burst; i < in.size(); ++i) in[i] = 0.0f;   // one 250 ms burst
    const auto out = run (e, in);
    // Repeat 1 (samples 12000..24000) is the burst unshifted; repeat 2 (24000..36000) is +12 st.
    const double hz1 = crossingHz (out, 13000, 23000);
    const double hz2 = peakHz (out, 25000, 35000, 440.0);
    REQUIRE (hz1 == Approx (220.0).epsilon (0.003));
    const double cents = 1200.0 * std::log2 (hz2 / 440.0);
    REQUIRE (std::fabs (cents) < 5.0);
    // Repeat 3 climbs another octave (measured once the grain has locked to the new input pitch).
    const double hz3 = peakHz (out, 40000, 47000, 880.0);
    REQUIRE (std::fabs (1200.0 * std::log2 (hz3 / 880.0)) < 10.0);
}

TEST_CASE ("Shimmer Ready: -12 st per repeat and a small grain still land within 5 cents", "[shimmerready]")
{
    ShimmerEngine e;
    e.prepare (sr, 256, 1);
    auto p = plain();
    p.sync = false;
    p.timeMs = 250.0f;
    p.pitchSemis = -12.0f;
    p.grainMs = 30.0f;
    p.feedback = 0.9f;
    e.setParams (p);
    auto in = sine (880.0, 120000);
    for (size_t i = 12000; i < in.size(); ++i) in[i] = 0.0f;
    const auto out = run (e, in);
    const double hz2 = peakHz (out, 25000, 35000, 440.0);
    REQUIRE (std::fabs (1200.0 * std::log2 (hz2 / 440.0)) < 5.0);
}

TEST_CASE ("Shimmer Ready: reverse plays each repeat backwards", "[shimmerready]")
{
    ShimmerEngine e;
    e.prepare (sr, 256, 1);
    auto p = plain();
    p.sync = false;
    p.timeMs = 250.0f;   // L = 12000
    p.reverse = true;
    e.setParams (p);
    // A rising ramp over one delay length: the echo must fall.
    std::vector<float> in (72000, 0.0f);
    for (int k = 0; k < 12000; ++k) in[(size_t) k] = (float) k / 12000.0f;
    const auto out = run (e, in);
    // Find where the reversed segment lands (within the first three delay lengths) and check its slope.
    double best = 1.0;
    for (int start = 12000; start <= 36000; start += 100)
    {
        double sum = 0.0;
        int cnt = 0;
        for (int k = 1000; k < 11000; k += 100)
        {
            const float a = out[(size_t) (start + k)], b = out[(size_t) (start + k + 100)];
            sum += (double) (b - a);
            ++cnt;
        }
        best = std::min (best, sum / cnt);
    }
    REQUIRE (best < -0.004);   // falling on average (ramp slope is 1/12000 per sample = -0.0083 per 100 when reversed)
    // And a forward run of the same input rises.
    ShimmerEngine f;
    f.prepare (sr, 256, 1);
    p.reverse = false;
    f.setParams (p);
    const auto fwd = run (f, in);
    double sum = 0.0;
    for (int k = 1000; k < 11000; k += 100) sum += (double) (fwd[(size_t) (12000 + k + 100)] - fwd[(size_t) (12000 + k)]);
    REQUIRE (sum > 0.0);
}

TEST_CASE ("Shimmer Ready: 110 % feedback stays bounded", "[shimmerready]")
{
    ShimmerEngine e;
    e.prepare (sr, 256, 1);
    auto p = plain();
    p.sync = false;
    p.timeMs = 100.0f;
    p.pitchSemis = 7.0f;
    p.feedback = 1.1f;
    e.setParams (p);
    const auto in = noise (48000 * 10);
    const auto out = run (e, in);
    float peak = 0.0f;
    for (float v : out) peak = std::max (peak, std::fabs (v));
    REQUIRE (peak < 2.0f);
    REQUIRE (std::isfinite (peak));
}

TEST_CASE ("Shimmer Ready: freeze holds the loop and ignores new input", "[shimmerready]")
{
    ShimmerEngine e;
    e.prepare (sr, 256, 1);
    auto p = plain();
    p.sync = false;
    p.timeMs = 200.0f;   // 9600 samples
    p.feedback = 0.5f;
    e.setParams (p);
    // Fill the loop with a burst, then freeze while feeding a different, loud signal.
    auto first = sine (300.0, 9600, 0.3f);
    const auto fill = run (e, first);
    (void) fill;
    p.freeze = true;
    e.setParams (p);
    const auto later = noise (9600 * 10, 7);
    const auto out = run (e, later);
    // Level stays within 1 dB across the frozen loops (each 9600 samples).
    const float r0 = rms (out, 0, 9600), r9 = rms (out, 9600 * 9, 9600 * 10);
    REQUIRE (std::fabs (20.0f * std::log10 (r9 / r0)) < 1.0f);
    // And it is still the 300 Hz tone, not the noise.
    REQUIRE (crossingHz (out, 9600 * 9, 9600 * 10) == Approx (300.0).epsilon (0.01));
}

TEST_CASE ("Shimmer Ready: loop high cut darkens each repeat", "[shimmerready]")
{
    ShimmerEngine e;
    e.prepare (sr, 256, 1);
    auto p = plain();
    p.sync = false;
    p.timeMs = 250.0f;
    p.feedback = 0.9f;
    p.highCutHz = 1000.0f;
    e.setParams (p);
    auto in = sine (6000.0, 120000);
    for (size_t i = 12000; i < in.size(); ++i) in[i] = 0.0f;
    const auto out = run (e, in);
    const float r1 = rms (out, 13000, 23000), r2 = rms (out, 25000, 35000);
    // Repeat 1 is unfiltered (the filter is in the loop), repeat 2 passed the 1 kHz low-pass once:
    // a 6 kHz tone loses about 31 dB (2nd order) on top of the 0.9 feedback.
    REQUIRE (20.0f * std::log10 (r2 / r1) < -25.0f);
}

TEST_CASE ("Shimmer Ready: shimmer adds a reverb tail after the echoes", "[shimmerready]")
{
    auto measure = [] (float shimmer)
    {
        ShimmerEngine e;
        e.prepare (sr, 256, 1);
        auto p = plain();
        p.sync = false;
        p.timeMs = 100.0f;    // 4800 samples
        p.feedback = 0.0f;
        p.shimmer = shimmer;
        p.decaySeconds = 3.0f;
        e.setParams (p);
        auto in = noise (48000 * 3);
        for (size_t i = 4800; i < in.size(); ++i) in[i] = 0.0f;
        const auto out = run (e, in);
        return rms (out, 48000, 48000 * 2);   // 1 .. 2 s: the single echo ended at 0.2 s
    };
    REQUIRE (measure (0.0f) < 1.0e-6f);
    REQUIRE (measure (0.8f) > 1.0e-3f);
}
