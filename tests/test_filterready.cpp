// Filter Ready DSP tests: measured response of the TPT SVF against the bilinear biquad, 24 dB = two
// 12 dB sections, notch depth, stability at max resonance in Analog mode, click-free cutoff jumps,
// bit-exact dry at mix 0, LFO tempo sync and envelope opening.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/FilterReady/Source/FilterEngine.h"
#include "../shared/dsp/Biquad.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cmath>
#include <complex>
#include <random>
#include <vector>

using namespace rommuser::filterready;
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

FilterMeters run (FilterEngine& e, Stereo& s, const Transport& t = {}, int block = 256)
{
    FilterMeters m;
    const int n = (int) s.l.size();
    for (int pos = 0; pos < n; pos += block)
    {
        const int len = std::min (block, n - pos);
        float* io[2] = { s.l.data() + pos, s.r.data() + pos };
        Transport tt = t;
        if (t.valid) tt.ppqPosition = t.ppqPosition + (double) pos * t.bpm / 60.0 / sr;
        m = e.process (io, 2, len, tt);
    }
    return m;
}

float levelDb (const std::vector<float>& x, float freq, int skip)
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
    const double amp = 2.0 * std::sqrt (s * s + c * c) / len;
    return (float) (20.0 * std::log10 (std::max (amp, 1e-12)));
}

// Magnitude (dB) of the bilinear biquad the SVF is supposed to match, evaluated at the 2x rate.
float biquadDb (FilterType type, double fc, double q, double freq, int sections)
{
    using namespace rommuser::dsp::biquad;
    const double sr2 = sr * oversampling;
    rommuser::dsp::BiquadCoeffs c;
    switch (type)
    {
        case FilterType::lowPass:  c = lowPass (fc, q, sr2); break;
        case FilterType::highPass: c = highPass (fc, q, sr2); break;
        case FilterType::bandPass: c = bandPass (fc, q, sr2); break;
        case FilterType::notch:    c = notch (fc, q, sr2); break;
    }
    const double w = 2.0 * pi * freq / sr2;
    const std::complex<double> z1 = std::polar (1.0, -w), z2 = z1 * z1;
    const auto h = (c.b0 + c.b1 * z1 + c.b2 * z2) / (1.0 + c.a1 * z1 + c.a2 * z2);
    return (float) (20.0 * std::log10 (std::max (std::abs (h), 1e-9))) * (float) sections;
}

FilterParams base()
{
    FilterParams p;
    p.a = { true, FilterType::lowPass, Slope::db12, 1000.0f, 0.3f, 0.0f, 0.0f };
    p.b.enabled = false;
    p.routing = Routing::series;
    p.character = Character::clean;
    p.driveDb = 0.0f;
    p.lfoSync = false;
    p.mix = 1.0f;
    p.outputDb = 0.0f;
    return p;
}

float measure (const FilterParams& p, float freq, int n = 24000, int skip = 6000)
{
    FilterEngine e;
    e.prepare (sr, 256, 2);
    e.setParams (p);
    auto s = sine (freq, n);
    run (e, s);
    return levelDb (s.l, freq, skip) - 20.0f * std::log10 (0.5f);
}
} // namespace

TEST_CASE ("Filter Ready: clean 12 dB low-pass matches the bilinear biquad within 0.2 dB", "[filterready]")
{
    auto p = base();
    p.a.cutoffHz = 1000.0f;
    p.a.resonance = 0.3f;
    const double q = resonanceToQ (p.a.resonance);
    for (float f : { 100.0f, 300.0f, 700.0f, 1000.0f, 1400.0f, 2500.0f, 6000.0f, 15000.0f })
    {
        const float expected = biquadDb (FilterType::lowPass, 1000.0, q, f, 1);
        if (expected < -40.0f) continue;   // below the oversampler's stop-band floor
        INFO ("freq " << f << " expected " << expected);
        REQUIRE (measure (p, f) == Approx (expected).margin (0.2));
    }
}

TEST_CASE ("Filter Ready: high-pass, band-pass and notch follow the biquad; notch is deeper than 40 dB", "[filterready]")
{
    auto p = base();
    p.a.cutoffHz = 800.0f;
    p.a.resonance = 0.4f;
    const double q = resonanceToQ (p.a.resonance);
    for (auto type : { FilterType::highPass, FilterType::bandPass })
    {
        p.a.type = type;
        for (float f : { 200.0f, 500.0f, 800.0f, 1600.0f, 4000.0f })
        {
            const float expected = biquadDb (type, 800.0, q, f, 1);
            if (expected < -40.0f) continue;
            INFO ("type " << (int) type << " freq " << f << " expected " << expected);
            REQUIRE (measure (p, f) == Approx (expected).margin (0.25));
        }
    }
    p.a.type = FilterType::notch;
    REQUIRE (measure (p, 800.0f) < -40.0f);
    REQUIRE (measure (p, 100.0f) == Approx (0.0f).margin (0.2));
    REQUIRE (measure (p, 6000.0f) == Approx (0.0f).margin (0.2));
}

TEST_CASE ("Filter Ready: 24 dB slope is the 12 dB response squared", "[filterready]")
{
    auto p = base();
    p.a.cutoffHz = 2000.0f;
    p.a.resonance = 0.5f;
    for (float f : { 500.0f, 1500.0f, 2000.0f, 3000.0f, 4500.0f })
    {
        p.a.slope = Slope::db12;
        const float one = measure (p, f);
        p.a.slope = Slope::db24;
        const float two = measure (p, f);
        INFO ("freq " << f);
        REQUIRE (two == Approx (2.0f * one).margin (0.3));
    }
}

TEST_CASE ("Filter Ready: max resonance in Analog mode with 0 dBFS noise stays finite and bounded", "[filterready]")
{
    for (auto character : { Character::clean, Character::analog })
    {
        FilterEngine e;
        e.prepare (sr, 512, 2);
        auto p = base();
        p.a.resonance = 1.0f;
        p.a.slope = Slope::db24;
        p.a.cutoffHz = 400.0f;
        p.character = character;
        p.driveDb = character == Character::analog ? 24.0f : 0.0f;
        e.setParams (p);
        auto s = noise (48000, 7, 1.0f);
        run (e, s, {}, 512);
        float peak = 0.0f;
        for (float v : s.l) { REQUIRE (std::isfinite (v)); peak = std::max (peak, std::fabs (v)); }
        INFO ("character " << (int) character);
        REQUIRE (peak < (character == Character::analog ? 6.0f : 40.0f));
    }
}

TEST_CASE ("Filter Ready: a cutoff jump on a 100 Hz tone produces no click (smoothed in the log domain)", "[filterready]")
{
    FilterEngine e;
    e.prepare (sr, 256, 2);
    auto p = base();
    p.a.cutoffHz = 8000.0f;
    p.a.resonance = 0.2f;
    e.setParams (p);
    auto s = sine (100.0f, 24000, 0.5f);
    // First half: 8 kHz cutoff; then jump to 300 Hz (still well above 100 Hz: level barely changes).
    const int half = 12000;
    {
        Stereo first { std::vector<float> (s.l.begin(), s.l.begin() + half), std::vector<float> (s.r.begin(), s.r.begin() + half) };
        run (e, first);
        p.a.cutoffHz = 300.0f;
        e.setParams (p);
        Stereo second { std::vector<float> (s.l.begin() + half, s.l.end()), std::vector<float> (s.r.begin() + half, s.r.end()) };
        run (e, second);
        std::copy (first.l.begin(), first.l.end(), s.l.begin());
        std::copy (second.l.begin(), second.l.end(), s.l.begin() + half);
    }
    // Largest sample-to-sample step must stay close to that of the clean 100 Hz sine (~0.5 * 2π·100/48k ≈ 0.0065).
    float maxStep = 0.0f;
    for (size_t i = 1; i < s.l.size(); ++i)
        maxStep = std::max (maxStep, std::fabs (s.l[i] - s.l[i - 1]));
    REQUIRE (maxStep < 0.02f);
}

TEST_CASE ("Filter Ready: mix 0 returns the input bit-exact, delayed by the reported latency", "[filterready]")
{
    FilterEngine e;
    e.prepare (sr, 256, 2);
    auto p = base();
    p.mix = 0.0f;
    p.a.resonance = 0.9f;
    p.driveDb = 20.0f;
    e.setParams (p);
    auto in = noise (8192, 3);
    auto s = in;
    run (e, s, {}, 256);
    const int latency = e.getLatencySamples();
    REQUIRE (latency > 0);
    for (int i = latency; i < 8192; ++i)
    {
        REQUIRE (s.l[(size_t) i] == in.l[(size_t) (i - latency)]);
        REQUIRE (s.r[(size_t) i] == in.r[(size_t) (i - latency)]);
    }
}

TEST_CASE ("Filter Ready: synced LFO phase follows the host ppq position", "[filterready]")
{
    FilterEngine e;
    e.prepare (sr, 256, 2);
    auto p = base();
    p.lfoSync = true;
    p.lfoBeats = 4.0;   // one cycle per bar at 4/4
    p.a.lfoOctaves = 1.0f;
    e.setParams (p);
    Transport t;
    t.valid = true;
    t.playing = true;
    t.bpm = 120.0;
    // Start at beat 7 of the bar (ppq 7 → 1.75 cycles → phase 0.75 before the block).
    t.ppqPosition = 7.0;
    auto s = sine (200.0f, 256);
    run (e, s, t, 256);
    // After 256 samples at 120 bpm: 256 / 48000 * 2 beats/s = 0.01067 beats → phase 0.75 + 0.01067/4
    const double expected = 0.75 + (256.0 / sr) * 2.0 / 4.0;
    REQUIRE (e.getLfoPhase() == Approx (expected).margin (0.002));
}

TEST_CASE ("Filter Ready: envelope follower opens the cutoff on loud input and closes on silence", "[filterready]")
{
    FilterEngine e;
    e.prepare (sr, 256, 2);
    auto p = base();
    p.a.cutoffHz = 500.0f;
    p.a.envOctaves = 3.0f;
    p.envAttackMs = 5.0f;
    p.envReleaseMs = 50.0f;
    p.envSensitivityDb = 6.0f;
    e.setParams (p);

    auto loud = sine (100.0f, 4800, 0.5f);   // 0.5 * 2 (6 dB) = 1.0 → env → 1
    const auto mLoud = run (e, loud);
    REQUIRE (mLoud.env > 0.8f);
    REQUIRE (mLoud.cutoffA > 500.0f * std::exp2 (3.0f * 0.8f));

    Stereo silence { std::vector<float> (24000, 0.0f), std::vector<float> (24000, 0.0f) };
    const auto mQuiet = run (e, silence);
    REQUIRE (mQuiet.env < 0.01f);
    REQUIRE (mQuiet.cutoffA == Approx (500.0f).margin (5.0f));
}

TEST_CASE ("Filter Ready: parallel routing sums A and B, series multiplies", "[filterready]")
{
    auto p = base();
    p.a = { true, FilterType::lowPass, Slope::db24, 300.0f, 0.1f, 0.0f, 0.0f };
    p.b = { true, FilterType::highPass, Slope::db24, 3000.0f, 0.1f, 0.0f, 0.0f };
    p.routing = Routing::parallel;
    // Parallel LP@300 + HP@3k: 100 Hz and 10 kHz both pass near 0 dB; 1 kHz is heavily attenuated.
    REQUIRE (measure (p, 100.0f) == Approx (0.0f).margin (0.5));
    REQUIRE (measure (p, 10000.0f) == Approx (0.0f).margin (0.5));
    REQUIRE (measure (p, 1000.0f) < -15.0f);
    // Series LP@300 then HP@3k kills everything.
    p.routing = Routing::series;
    REQUIRE (measure (p, 100.0f) < -40.0f);
    REQUIRE (measure (p, 10000.0f) < -40.0f);
}
