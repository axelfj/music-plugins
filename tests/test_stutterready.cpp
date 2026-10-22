// Stutter Ready DSP tests: transparency, sample-aligned repeats against the DAW clock, quantised
// trigger start, click-free crossfade, division ramp, pitch ramp, reverse, tape stop, step sequencer.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/StutterReady/Source/StutterEngine.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cmath>
#include <functional>
#include <random>
#include <vector>

using namespace rommuser::stutterready;
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

// Runs a mono signal through the engine in blocks, calling `hook (blockStart)` before each block so
// tests can trigger gestures at a given sample. The transport advances from ppq 0 at `bpm`.
std::vector<float> run (StutterEngine& e, const std::vector<float>& in, double bpm, bool playing,
                        const std::function<void (int64_t)>& hook = {}, int block = 256)
{
    std::vector<float> out (in.size());
    Transport t;
    t.valid = true;
    t.playing = playing;
    t.bpm = bpm;
    for (size_t pos = 0; pos < in.size(); pos += (size_t) block)
    {
        const int len = (int) std::min ((size_t) block, in.size() - pos);
        if (hook) hook ((int64_t) pos);
        std::vector<float> buf (in.begin() + (long) pos, in.begin() + (long) pos + len);
        float* ptr = buf.data();
        t.ppqPosition = (double) pos / sr * bpm / 60.0;
        e.process (&ptr, 1, len, t);
        std::copy (buf.begin(), buf.end(), out.begin() + (long) pos);
    }
    return out;
}

double zeroCrossingHz (const std::vector<float>& v, size_t from, size_t to)
{
    int crossings = 0;
    for (size_t i = from + 1; i < to; ++i)
        if ((v[i - 1] < 0.0f) != (v[i] < 0.0f)) ++crossings;
    return 0.5 * crossings * sr / (double) (to - from);
}

float maxAbs (const std::vector<float>& v, size_t from, size_t to)
{
    float m = 0.0f;
    for (size_t i = from; i < to; ++i) m = std::max (m, std::fabs (v[i]));
    return m;
}
} // namespace

TEST_CASE ("Stutter Ready: without a gesture the signal passes bit-exact", "[stutterready]")
{
    StutterEngine e;
    e.prepare (sr, 256, 1);
    StutterParams p;
    e.setParams (p);
    const auto in = noise (48000);
    const auto out = run (e, in, 120.0, true);
    for (size_t i = 0; i < in.size(); ++i)
        REQUIRE (out[i] == in[i]);
}

TEST_CASE ("Stutter Ready: repeats are sample-aligned to the division at the host tempo", "[stutterready]")
{
    // 120 BPM, 1/16 = 0.125 s = 6000 samples exactly.
    StutterEngine e;
    e.prepare (sr, 256, 1);
    StutterParams p;
    p.quantize = 0;
    p.gestures[0] = Gesture {};   // 1/16 plain stutter
    e.setParams (p);
    const auto in = noise (96000);
    const int64_t T = 10240;      // on a block boundary so the trigger lands exactly there
    const auto out = run (e, in, 120.0, true, [&] (int64_t pos) { if (pos == T) e.noteOn (60); });
    const int L = 6000;
    const int fade = (int) (sr * 0.005);
    // First pass is the live input.
    for (int k = 0; k < L - fade - 1; ++k)
        REQUIRE (out[(size_t) (T + k)] == in[(size_t) (T + k)]);
    // Repetitions 2..6 are identical to the first pass, sample for sample.
    for (int rep = 1; rep < 6; ++rep)
        for (int k = 0; k < L; ++k)
            REQUIRE (out[(size_t) (T + rep * L + k)] == Approx (out[(size_t) (T + k)]).margin (1.0e-6));
    // Before the trigger nothing changed.
    for (int64_t i = 0; i < T; ++i)
        REQUIRE (out[(size_t) i] == in[(size_t) i]);
}

TEST_CASE ("Stutter Ready: a quantised trigger starts exactly on the next grid line", "[stutterready]")
{
    // 100 BPM: a beat is 28800 samples; 1/8 grid = 14400. Trigger at sample 20000 -> start at 28800.
    StutterEngine e;
    e.prepare (sr, 256, 1);
    StutterParams p;
    p.quantize = 2;   // 1/8
    p.gestures[0].divStart = p.gestures[0].divEnd = 3;   // 1/32: repeats after 3600 samples
    e.setParams (p);
    const auto in = noise (96000);
    const auto out = run (e, in, 100.0, true, [&] (int64_t pos) { if (pos == 19968) e.noteOn (60); }, 256);
    const int64_t start = 28800;
    const int L = 3600;
    // Up to the grid line the output is the input; after the first pass it repeats from `start`.
    for (int64_t i = 0; i < start + L / 2; ++i)
        REQUIRE (out[(size_t) i] == in[(size_t) i]);
    for (int k = 0; k < L / 2; ++k)
        REQUIRE (out[(size_t) (start + L + k)] == Approx (in[(size_t) (start + k)]).margin (1.0e-6));
    // Sanity: a non-repeating run would differ here.
    bool differs = false;
    for (int k = 0; k < L / 2; ++k)
        if (std::fabs (out[(size_t) (start + L + k)] - in[(size_t) (start + L + k)]) > 1.0e-3f) differs = true;
    REQUIRE (differs);
}

TEST_CASE ("Stutter Ready: the loop wrap is crossfaded (no clicks on a sine)", "[stutterready]")
{
    // 1/16 at 128 BPM = 5625 samples: 440 Hz does not fit a whole number of cycles, so a hard wrap
    // would click. The largest sample-to-sample step must stay close to the sine's own slope.
    StutterEngine e;
    e.prepare (sr, 256, 1);
    StutterParams p;
    p.quantize = 0;
    e.setParams (p);
    const auto in = sine (440.0, 96000);
    const auto out = run (e, in, 128.0, true, [&] (int64_t pos) { if (pos == 4096) e.noteOn (60); });
    const float natural = 0.5f * 2.0f * (float) pi * 440.0f / (float) sr;   // max |dx| of the sine
    float worst = 0.0f;
    for (size_t i = 4097; i < out.size(); ++i)
        worst = std::max (worst, std::fabs (out[i] - out[i - 1]));
    REQUIRE (worst < natural * 1.6f);
    // And a hard wrap of the same slice really would click (shows the test has teeth).
    const int L = 5625;
    float hardStep = 0.0f;
    for (int rep = 1; rep < 4; ++rep)
        hardStep = std::max (hardStep, std::fabs (in[(size_t) (4096 + rep * L - 1)] - in[(size_t) 4096]));
    REQUIRE (hardStep > natural * 3.0f);
}

TEST_CASE ("Stutter Ready: division ramp shortens the repeats from 1/8 to 1/32", "[stutterready]")
{
    StutterEngine e;
    e.prepare (sr, 256, 1);
    StutterParams p;
    p.quantize = 0;
    p.rampBeats = 2.0f;
    p.gestures[0].divStart = 1;   // 1/8 = 12000 samples at 120 BPM
    p.gestures[0].divEnd = 3;     // 1/32 = 3000 samples
    e.setParams (p);
    // A single click at the trigger: every repeat start produces a click.
    std::vector<float> in (192000, 0.0f);
    const int64_t T = 8192;
    in[(size_t) T] = 1.0f;
    const auto out = run (e, in, 120.0, true, [&] (int64_t pos) { if (pos == T) e.noteOn (60); });
    std::vector<int64_t> clicks;
    for (size_t i = (size_t) T; i < (size_t) (T + 48000 * 3); ++i)
        if (out[i] > 0.5f) clicks.push_back ((int64_t) i);
    REQUIRE (clicks.size() >= 8);
    std::vector<int64_t> gaps;
    for (size_t i = 1; i < clicks.size(); ++i) gaps.push_back (clicks[i] - clicks[i - 1]);
    REQUIRE (gaps.front() == 12000);                        // first repeat exactly one 1/8
    for (size_t i = 1; i < gaps.size(); ++i)
        REQUIRE (gaps[i] <= gaps[i - 1]);                   // monotonically faster
    // After the 2-beat ramp (48000 samples) the gap has settled at 1/32.
    REQUIRE (gaps.back() == 3000);
    REQUIRE (gaps.front() > gaps.back());
}

TEST_CASE ("Stutter Ready: pitch ramp doubles the frequency of the repeats", "[stutterready]")
{
    StutterEngine e;
    e.prepare (sr, 256, 1);
    StutterParams p;
    p.quantize = 0;
    p.rampBeats = 1.0f;                 // 24000 samples at 120 BPM
    p.gestures[0].pitchSemis = 12.0f;
    e.setParams (p);
    const auto in = sine (300.0, 192000);
    const int64_t T = 4096;
    const auto out = run (e, in, 120.0, true, [&] (int64_t pos) { if (pos == T) e.noteOn (60); });
    // Well after the ramp, the read rate is 2 -> 600 Hz.
    const double hz = zeroCrossingHz (out, (size_t) (T + 6000 + 30000), (size_t) (T + 6000 + 60000));
    REQUIRE (hz == Approx (600.0).margin (12.0));
    // At the start it is still 300 Hz.
    REQUIRE (zeroCrossingHz (out, (size_t) T, (size_t) (T + 3000)) == Approx (300.0).margin (12.0));
}

TEST_CASE ("Stutter Ready: reverse plays the repeats backwards", "[stutterready]")
{
    StutterEngine e;
    e.prepare (sr, 256, 1);
    StutterParams p;
    p.quantize = 0;
    p.gestures[0].reverse = true;       // 1/16 = 6000 samples
    e.setParams (p);
    // A ramp that rises inside the slice: reversed repeats must fall.
    std::vector<float> in (96000, 0.0f);
    const int64_t T = 4096;
    for (int k = 0; k < 6000; ++k) in[(size_t) (T + k)] = (float) k / 6000.0f;
    const auto out = run (e, in, 120.0, true, [&] (int64_t pos) { if (pos == T) e.noteOn (60); });
    // Second repetition, away from the crossfade region: out[T + L + k] == in[T + L - k].
    for (int k = 500; k < 5000; k += 50)
        REQUIRE (out[(size_t) (T + 6000 + k)] == Approx (in[(size_t) (T + 6000 - k)]).margin (1.0e-3));
}

TEST_CASE ("Stutter Ready: tape stop slows down and ends in silence", "[stutterready]")
{
    StutterEngine e;
    e.prepare (sr, 256, 1);
    StutterParams p;
    p.quantize = 0;
    p.rampBeats = 2.0f;                 // 48000 samples
    p.gestures[0].divStart = p.gestures[0].divEnd = 0;
    p.gestures[0].tapeStop = true;
    e.setParams (p);
    const auto in = sine (400.0, 192000);
    const int64_t T = 4096;
    const auto out = run (e, in, 120.0, true, [&] (int64_t pos) { if (pos == T) e.noteOn (60); });
    // First pass (24000 samples) is live; the 2-beat ramp (48000 samples) starts after it.
    const double early = zeroCrossingHz (out, (size_t) (T + 2000), (size_t) (T + 8000));
    const double late = zeroCrossingHz (out, (size_t) (T + 24000 + 36000), (size_t) (T + 24000 + 42000));   // u ~ 0.8: rate ~0.2
    REQUIRE (early == Approx (400.0).margin (10.0));
    REQUIRE (late < 0.4 * early);
    REQUIRE (maxAbs (out, (size_t) (T + 24000 + 50000), (size_t) (T + 24000 + 60000)) < 1.0e-3f);
}

TEST_CASE ("Stutter Ready: the step sequencer runs gestures on the 1/16 grid", "[stutterready]")
{
    StutterEngine e;
    e.prepare (sr, 256, 1);
    StutterParams p;
    p.seqEnabled = true;
    p.seqBars = 1;
    p.gestures[0].divStart = p.gestures[0].divEnd = 3;   // 1/32 so the second half of a step repeats the first
    for (int s = 0; s < 4; ++s) p.steps[(size_t) s] = 0;   // first four 1/16 of every bar
    e.setParams (p);
    const auto in = noise (192000);   // two bars at 120 BPM
    const auto out = run (e, in, 120.0, true);
    const int step = 6000, half = 3000;
    // Bar 1, step 0: first half live, second half repeats it.
    for (int k = 0; k < half - 300; ++k)
        REQUIRE (out[(size_t) k] == in[(size_t) k]);
    for (int k = 0; k < half; ++k)
        REQUIRE (out[(size_t) (half + k)] == Approx (out[(size_t) k]).margin (1.0e-6));
    // Steps 4 .. 15: back to the input (after the 5 ms release fade).
    for (int k = 4 * step + 400; k < 16 * step; ++k)
        REQUIRE (out[(size_t) k] == in[(size_t) k]);
    // Bar 2 repeats the pattern.
    for (int k = 0; k < half; ++k)
        REQUIRE (out[(size_t) (16 * step + half + k)] == Approx (out[(size_t) (16 * step + k)]).margin (1.0e-6));
    // Not playing: the sequencer does nothing.
    StutterEngine e2;
    e2.prepare (sr, 256, 1);
    e2.setParams (p);
    const auto out2 = run (e2, in, 120.0, false);
    for (size_t i = 0; i < in.size(); ++i)
        REQUIRE (out2[i] == in[i]);
}
