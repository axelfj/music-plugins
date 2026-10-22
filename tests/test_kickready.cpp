// Kick Ready DSP tests: tuning within ±1 cent, phase-0 start without a click, amplitude decay time,
// click layer energy and tone, click-free retrigger, drive / clip ceiling, velocity and glide.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/KickReady/Source/KickEngine.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cmath>
#include <vector>

using namespace rommuser::kickready;
using Catch::Approx;

namespace
{
constexpr double sr = 48000.0;

KickParams plain()
{
    KickParams p;
    p.pitchStartSemis = 0.0f;
    p.clickLevel = 0.0f;
    p.ampAttackMs = 0.0f;
    p.ampHoldMs = 0.0f;
    p.ampDecayMs = 600.0f;
    p.ampCurve = 1.0f;
    p.driveDb = 0.0f;
    p.clip = 0.0f;
    p.subDb = 0.0f;
    p.highCutHz = 20000.0f;
    p.velocitySens = 0.0f;
    return p;
}

std::vector<float> render (KickEngine& e, int n, int block = 256)
{
    std::vector<float> out ((size_t) n);
    for (int pos = 0; pos < n; pos += block)
        e.render (out.data() + pos, std::min (block, n - pos));
    return out;
}

// Frequency from upward zero crossings (linear interpolation) between two sample positions.
double zeroCrossingHz (const std::vector<float>& x, int from, int to)
{
    double first = -1.0, last = -1.0;
    int count = 0;
    for (int i = from + 1; i < to; ++i)
    {
        if (x[(size_t) i - 1] < 0.0f && x[(size_t) i] >= 0.0f)
        {
            const double frac = -x[(size_t) i - 1] / (x[(size_t) i] - x[(size_t) i - 1]);
            const double t = (i - 1) + frac;
            if (first < 0.0) first = t; else { last = t; ++count; }
        }
    }
    return count > 0 ? sr * count / (last - first) : 0.0;
}

double cents (double measured, double target) { return 1200.0 * std::log2 (measured / target); }

float peakAbs (const std::vector<float>& x, int from, int to)
{
    float m = 0.0f;
    for (int i = from; i < to && i < (int) x.size(); ++i) m = std::max (m, std::fabs (x[(size_t) i]));
    return m;
}

double energy (const std::vector<float>& x, int from, int to)
{
    double e = 0.0;
    for (int i = from; i < to && i < (int) x.size(); ++i) e += (double) x[(size_t) i] * x[(size_t) i];
    return e;
}
} // namespace

TEST_CASE ("Kick Ready: the body is tuned to the MIDI note within 1 cent after the pitch sweep", "[kickready]")
{
    for (int note : { 24, 36, 41, 48 })   // C0, C1, F1, C2
        for (float start : { 0.0f, 36.0f })
        {
            KickEngine e;
            e.prepare (sr, 256);
            auto p = plain();
            p.pitchStartSemis = start;
            p.pitchDecayMs = 60.0f;
            p.ampDecayMs = 1500.0f;
            e.setParams (p);
            e.noteOn (note, 1.0f);
            auto x = render (e, 48000);
            // Measure from 500 ms (sweep long gone: 1 % at 60 ms) to 900 ms.
            const double hz = zeroCrossingHz (x, 24000, 43200);
            const double target = midiToHz (note);
            INFO ("note " << note << " start " << start << " measured " << hz << " target " << target);
            REQUIRE (std::fabs (cents (hz, target)) < 1.0);
        }
}

TEST_CASE ("Kick Ready: fine tune shifts the body by the requested cents", "[kickready]")
{
    KickEngine e;
    e.prepare (sr, 256);
    auto p = plain();
    p.tuneSemitones = 0.5f;   // +50 cents
    p.ampDecayMs = 1500.0f;
    e.setParams (p);
    e.noteOn (36, 1.0f);
    auto x = render (e, 48000);
    REQUIRE (cents (zeroCrossingHz (x, 24000, 43200), midiToHz (36)) == Approx (50.0).margin (1.0));
}

TEST_CASE ("Kick Ready: the kick starts at phase 0 with no step larger than the sine slope", "[kickready]")
{
    KickEngine e;
    e.prepare (sr, 256);
    auto p = plain();
    p.pitchStartSemis = 36.0f;   // starts 3 octaves up: fastest slope of the sweep
    p.ampAttackMs = 0.0f;
    e.setParams (p);
    e.noteOn (36, 1.0f);
    auto x = render (e, 4800);
    REQUIRE (x[0] == 0.0f);
    // Start frequency 65.4 Hz * 8 = 523 Hz: max slope of a unit sine is 2π·523/48000 ≈ 0.0685 per sample.
    float maxStep = 0.0f;
    for (size_t i = 1; i < x.size(); ++i) maxStep = std::max (maxStep, std::fabs (x[i] - x[i - 1]));
    REQUIRE (maxStep < 0.075f);
}

TEST_CASE ("Kick Ready: exponential amplitude decay reaches -60 dB at the decay time and then stops", "[kickready]")
{
    KickEngine e;
    e.prepare (sr, 256);
    auto p = plain();
    p.ampDecayMs = 300.0f;
    p.ampCurve = 1.0f;
    e.setParams (p);
    e.noteOn (36, 1.0f);
    auto x = render (e, 48000);
    const int d = (int) (0.3 * sr);
    // Level at 150 ms: exp(-6.9078 * 0.5) = -30 dB (±1.5 dB; measured on the cycle peak).
    const float half = peakAbs (x, d / 2 - 400, d / 2 + 400);
    REQUIRE (20.0f * std::log10 (half) == Approx (-30.0f).margin (1.5f));
    // Silence after the decay time (below -120 dBFS: the output filters may leave a rounding-level
    // tail, which differs between FMA and non-FMA builds) and the voice is no longer sounding.
    REQUIRE (peakAbs (x, d + 10, 48000) < 1e-6f);
    REQUIRE_FALSE (e.isSounding());
}

TEST_CASE ("Kick Ready: linear decay is a straight ramp to zero", "[kickready]")
{
    KickEngine e;
    e.prepare (sr, 256);
    auto p = plain();
    p.ampDecayMs = 300.0f;
    p.ampCurve = 0.0f;
    e.setParams (p);
    e.noteOn (36, 1.0f);
    auto x = render (e, 48000);
    const int d = (int) (0.3 * sr);
    REQUIRE (peakAbs (x, d / 2 - 400, d / 2 + 400) == Approx (0.5f).margin (0.03f));
    REQUIRE (peakAbs (x, d + 10, 48000) < 1e-6f);
    REQUIRE_FALSE (e.isSounding());
}

TEST_CASE ("Kick Ready: the click lives in the first milliseconds and above its tone frequency", "[kickready]")
{
    for (auto mode : { ClickMode::noise, ClickMode::tick })
    {
        KickEngine e;
        e.prepare (sr, 256);
        auto p = plain();
        p.ampDecayMs = 1.0f;    // 1 ms body so the click dominates
        p.clickLevel = 1.0f;
        p.clickDecayMs = 5.0f;
        p.clickToneHz = 3000.0f;
        p.clickMode = mode;
        e.setParams (p);
        e.noteOn (36, 1.0f);
        auto x = render (e, 9600);
        const int five = (int) (0.005 * sr);
        const double early = energy (x, 0, five);
        const double late = energy (x, 5 * five, 9600);   // after 25 ms: click at -300 dB, body gone
        INFO ("mode " << (int) mode);
        REQUIRE (early > 1e-3);
        REQUIRE (late < early * 1e-4);
        // Spectral check from 2 ms on (body gone): click energy below 500 Hz is small vs above 2 kHz.
        auto band = [&] (double lo, double hi)
        {
            double sum = 0.0;
            const int start = 96, n = five * 2;
            for (double f = lo; f < hi; f += sr / n)
            {
                double re = 0.0, im = 0.0;
                for (int i = 0; i < n; ++i) { const double ph = 6.283185307179586 * f * i / sr; re += x[(size_t) (start + i)] * std::cos (ph); im -= x[(size_t) (start + i)] * std::sin (ph); }
                sum += re * re + im * im;
            }
            return sum;
        };
        if (mode == ClickMode::noise)
            REQUIRE (band (100.0, 500.0) < band (2000.0, 8000.0) * 0.05);
    }
}

TEST_CASE ("Kick Ready: retriggering while a kick sounds never produces a step larger than the signal slope", "[kickready]")
{
    KickEngine e;
    e.prepare (sr, 256);
    auto p = plain();
    p.pitchStartSemis = 24.0f;
    p.pitchDecayMs = 40.0f;
    p.ampDecayMs = 800.0f;
    e.setParams (p);
    e.noteOn (36, 1.0f);
    auto a = render (e, 2000);          // retrigger 41.7 ms in, mid-cycle at near full level
    e.noteOn (36, 1.0f);
    auto b = render (e, 4000);
    std::vector<float> x (a);
    x.insert (x.end(), b.begin(), b.end());
    float maxStep = 0.0f;
    for (size_t i = 1; i < x.size(); ++i) maxStep = std::max (maxStep, std::fabs (x[i] - x[i - 1]));
    // Start frequency 65.4 * 4 = 262 Hz: unit sine slope 0.034 / sample; the 2 ms fade of the old
    // voice adds at most 1/96 per sample. A hard restart would step by up to 1.0.
    REQUIRE (maxStep < 0.06f);
}

TEST_CASE ("Kick Ready: drive stays peak-normalised and the clipper never exceeds the ceiling", "[kickready]")
{
    for (float drive : { 0.0f, 12.0f, 36.0f })
        for (float clip : { 0.0f, 1.0f })
        {
            KickEngine e;
            e.prepare (sr, 256);
            auto p = plain();
            p.driveDb = drive;
            p.clip = clip;
            p.clickLevel = 0.5f;
            p.subDb = 6.0f;
            e.setParams (p);
            e.noteOn (36, 1.0f);
            auto x = render (e, 24000);
            INFO ("drive " << drive << " clip " << clip);
            for (float v : x) REQUIRE (std::isfinite (v));
            if (clip > 0.0f)
                REQUIRE (peakAbs (x, 0, 24000) <= clipCeiling * std::pow (10.0f, 6.0f / 20.0f) + 1e-3f);   // shelf after the clipper may add up to +6 dB
            else
                REQUIRE (peakAbs (x, 0, 24000) < 2.5f);
        }
    // Pure drive without shelf: peak stays at or below 1.
    KickEngine e;
    e.prepare (sr, 256);
    auto p = plain();
    p.driveDb = 24.0f;
    e.setParams (p);
    e.noteOn (36, 1.0f);
    auto x = render (e, 24000);
    REQUIRE (peakAbs (x, 0, 24000) <= 1.0f + 1e-4f);
    REQUIRE (peakAbs (x, 0, 24000) > 0.9f);
}

TEST_CASE ("Kick Ready: velocity sensitivity scales the level and glide slides the base pitch", "[kickready]")
{
    {
        KickEngine e;
        e.prepare (sr, 256);
        auto p = plain();
        p.velocitySens = 1.0f;
        e.setParams (p);
        e.noteOn (36, 1.0f);
        auto full = render (e, 4800);
        e.reset();
        e.noteOn (36, 0.5f);
        auto half = render (e, 4800);
        REQUIRE (peakAbs (half, 0, 4800) / peakAbs (full, 0, 4800) == Approx (0.5f).margin (0.01f));
    }
    {
        KickEngine e;
        e.prepare (sr, 256);
        auto p = plain();
        p.glideMs = 100.0f;
        p.ampDecayMs = 2000.0f;
        e.setParams (p);
        e.noteOn (36, 1.0f);
        render (e, 24000);
        e.noteOn (48, 1.0f);           // an octave up, glides from C1
        auto x = render (e, 48000);
        // Early (0 .. 20 ms) still near C1; late (800 .. 1000 ms) at C2 within 1 cent.
        const double early = zeroCrossingHz (x, 0, 960);
        const double late = zeroCrossingHz (x, 38400, 48000);
        REQUIRE (early < midiToHz (36) * 1.3);
        REQUIRE (std::fabs (cents (late, midiToHz (48))) < 1.0);
    }
}

TEST_CASE ("Kick Ready: renderOneShot returns the whole kick and trims the silence", "[kickready]")
{
    KickEngine e;
    e.prepare (sr, 256);
    auto p = plain();
    p.ampDecayMs = 250.0f;
    e.setParams (p);
    auto x = e.renderOneShot (36, 1.0f);
    REQUIRE ((int) x.size() >= (int) (0.25 * sr));
    REQUIRE ((int) x.size() <= (int) (0.27 * sr));
    REQUIRE (x[0] == 0.0f);
    REQUIRE (peakAbs (x, 0, (int) x.size()) > 0.9f);
}
