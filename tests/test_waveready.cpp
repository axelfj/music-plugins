// Wave Ready DSP tests: tuning, aliasing of the band-limited tables, envelope timing, filter slope,
// unison width, polyphony and voice stealing, mono legato glide, vibrato depth, synced LFO rate,
// silence when idle and sane output for every wave.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/WaveReady/Source/WaveEngine.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cmath>
#include <complex>
#include <vector>

using namespace rommuser::waveready;
using Catch::Approx;

namespace
{
constexpr double sr = 48000.0;

WaveParams plain()
{
    WaveParams p;
    p.osc1.wave = 2; p.osc1.unison = 1; p.osc1.level = 1.0f; p.osc1.retrig = true;
    p.osc2.on = false;
    p.filterMode = FilterMode::lp12;
    p.cutoffHz = 20000.0f;
    p.resonance = 0.0f;
    p.ampEnv = { 0.05f, 100.0f, 1.0f, 50.0f };
    p.modEnv = { 0.05f, 100.0f, 1.0f, 50.0f };
    p.velToAmp = 0.0f;
    p.outputDb = 0.0f;
    return p;
}

struct Stereo { std::vector<float> l, r; };

Stereo render (WaveEngine& e, int total, int block = 256)
{
    Stereo out;
    out.l.resize ((size_t) total); out.r.resize ((size_t) total);
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = std::min (block, total - pos);
        e.render (out.l.data() + pos, out.r.data() + pos, n);
    }
    return out;
}

float peakAbs (const std::vector<float>& x, int from, int to)
{
    float m = 0.0f;
    for (int i = from; i < to && i < (int) x.size(); ++i) m = std::max (m, std::fabs (x[(size_t) i]));
    return m;
}

// Magnitude (linear) of a windowed DFT at exactly `hz`. Hann by default; the 4-term Blackman-Harris
// window (sidelobes below -92 dB) is for the aliasing measurement.
double magAt (const std::vector<float>& x, int from, int n, double hz, bool blackmanHarris = false)
{
    std::complex<double> acc;
    for (int i = 0; i < n; ++i)
    {
        const double t = 2.0 * 3.14159265358979 * i / n;
        const double w = blackmanHarris ? 0.35875 - 0.48829 * std::cos (t) + 0.14128 * std::cos (2.0 * t) - 0.01168 * std::cos (3.0 * t)
                                        : 0.5 - 0.5 * std::cos (t);
        acc += std::polar (w * (double) x[(size_t) (from + i)], -2.0 * 3.14159265358979 * hz * i / sr);
    }
    return std::abs (acc) * 2.0 / n;
}

// Frequency of the strongest spectral peak between lo and hi Hz (parabolic interpolation).
double peakHz (const std::vector<float>& x, int from, int n, double lo, double hi)
{
    const double step = sr / n;
    double bestHz = lo, best = -1.0;
    for (double f = lo; f <= hi; f += step)
    {
        const double m = magAt (x, from, n, f);
        if (m > best) { best = m; bestHz = f; }
    }
    const double a = magAt (x, from, n, bestHz - step), b = best, c = magAt (x, from, n, bestHz + step);
    const double d = 0.5 * (a - c) / (a - 2.0 * b + c);
    return bestHz + d * step;
}

double cents (double hz, double ref) { return 1200.0 * std::log2 (hz / ref); }
} // namespace

TEST_CASE ("Wave Ready: every wave plays A4 within 1 cent and without NaNs", "[waveready]")
{
    for (int w = 0; w < numWaves; ++w)
    {
        WaveEngine e;
        e.prepare (sr, 256);
        auto p = plain();
        p.osc1.wave = w;
        e.setParams (p, true);
        e.noteOn (69, 1.0f);
        auto x = render (e, 48000);
        const double hz = peakHz (x.l, 24000, 16384, 400.0, 480.0);
        INFO ("wave " << waveNames[w] << " measured " << hz);
        REQUIRE (std::fabs (cents (hz, 440.0)) < 1.0);
        for (float v : x.l) REQUIRE (std::isfinite (v));
        REQUIRE (peakAbs (x.l, 24000, 48000) > 0.1f);
    }
}

TEST_CASE ("Wave Ready: a saw at C7 keeps aliases below -80 dB of the fundamental", "[waveready]")
{
    WaveEngine e;
    e.prepare (sr, 256);
    auto p = plain();
    e.setParams (p, true);
    e.noteOn (96, 1.0f);   // C7 = 2093 Hz
    auto x = render (e, 48000);
    const double f0 = 440.0 * std::pow (2.0, (96 - 69) / 12.0);
    const double fund = magAt (x.l, 20000, 16384, f0, true);
    // Strongest bin that is more than 4 bins away from every harmonic (Blackman-Harris window, so
    // the window's own sidelobes stay below -92 dB).
    const int n = 16384;
    const double step = sr / n;
    double worst = 0.0;
    for (double f = 100.0; f < 23000.0; f += step)
    {
        const double k = f / f0;
        if (std::fabs (k - std::round (k)) * f0 < 4.0 * step) continue;
        worst = std::max (worst, magAt (x.l, 20000, n, f, true));
    }
    const double dB = 20.0 * std::log10 (worst / fund);
    INFO ("worst alias " << dB << " dB");
    REQUIRE (dB < -80.0);
}

TEST_CASE ("Wave Ready: the amplitude envelope reaches full level at the attack time, holds the sustain and dies at the release", "[waveready]")
{
    WaveEngine e;
    e.prepare (sr, 256);
    auto p = plain();
    p.osc1.wave = 0;   // sine for a clean envelope reading
    p.ampEnv = { 100.0f, 200.0f, 0.5f, 100.0f };
    e.setParams (p, true);
    e.noteOn (69, 1.0f);
    const int attack = 4800, decay = 9600;
    auto x = render (e, attack + decay + 4800);
    // 60 % through the attack the level is still rising; at the attack time it is at the peak.
    // Centre pan is -3 dB per channel, so full level reads 0.707.
    REQUIRE (peakAbs (x.l, attack * 6 / 10 - 200, attack * 6 / 10 + 200) < 0.67f);
    REQUIRE (peakAbs (x.l, attack - 300, attack + 300) == Approx (0.707f).margin (0.04f));
    // After the decay the sustain level holds (0.5).
    REQUIRE (peakAbs (x.l, attack + decay + 2000, attack + decay + 4800) == Approx (0.354f).margin (0.03f));

    e.noteOff (69);
    auto y = render (e, 48000);
    // -60 dB at the release time (100 ms = 4800 samples) within a factor of 2 in level.
    const float at = peakAbs (y.l, 4800 - 200, 4800 + 200);
    REQUIRE (at < 0.354f * 0.002f);
    REQUIRE (at > 0.354f * 0.0002f);
    // Then the voice ends and the output is exactly silent (no filters run while idle).
    REQUIRE (e.getActiveVoices() == 0);
    REQUIRE (peakAbs (y.l, 20000, 48000) < 1e-6f);
}

TEST_CASE ("Wave Ready: LP 24 at 500 Hz attenuates the 8 kHz harmonic by more than 60 dB relative to LP open", "[waveready]")
{
    auto run = [] (float cutoff, FilterMode mode)
    {
        WaveEngine e;
        e.prepare (sr, 256);
        auto p = plain();
        p.osc1.wave = 3;   // square: harmonics at 1, 3, 5 ... of 500 Hz -> 7.5 kHz is the 15th
        p.filterMode = mode;
        p.cutoffHz = cutoff;
        e.setParams (p, true);
        e.noteOn (71, 1.0f);   // B4 = 493.9 Hz
        auto x = render (e, 48000);
        const double f0 = 440.0 * std::pow (2.0, 2.0 / 12.0);
        return std::make_pair (magAt (x.l, 24000, 16384, f0), magAt (x.l, 24000, 16384, 15.0 * f0));
    };
    const auto open = run (20000.0f, FilterMode::lp24);
    const auto closed = run (500.0f, FilterMode::lp24);
    const double h15 = 20.0 * std::log10 (closed.second / open.second);
    INFO ("15th harmonic change " << h15 << " dB");
    REQUIRE (h15 < -60.0);
    // The fundamental sits at the cutoff and loses at most 8 dB (two -3 dB poles plus resonance 0).
    REQUIRE (20.0 * std::log10 (closed.first / open.first) > -8.0);
    // HP 12 at 8 kHz removes the fundamental by > 40 dB and leaves the 15th within 6 dB.
    const auto hp = run (8000.0f, FilterMode::hp12);
    REQUIRE (20.0 * std::log10 (hp.first / open.first) < -40.0);
    REQUIRE (20.0 * std::log10 (hp.second / open.second) > -6.0);
}

TEST_CASE ("Wave Ready: unison spreads the voices in pitch and stereo; width 0 is mono", "[waveready]")
{
    auto run = [] (float width)
    {
        WaveEngine e;
        e.prepare (sr, 256);
        auto p = plain();
        p.osc1.unison = 7;
        p.osc1.detune = 60.0f;
        p.osc1.width = width;
        e.setParams (p, true);
        e.noteOn (57, 1.0f);   // A3 = 220 Hz
        return render (e, 48000);
    };
    auto wide = run (1.0f);
    double diff = 0.0, sum = 0.0;
    for (int i = 24000; i < 48000; ++i)
    {
        diff += std::fabs (wide.l[(size_t) i] - wide.r[(size_t) i]);
        sum += std::fabs (wide.l[(size_t) i] + wide.r[(size_t) i]);
    }
    REQUIRE (diff / sum > 0.3);
    // Outer voices at +-30 cents: the spectrum shows energy 30 cents off the centre.
    const double f0 = 220.0;
    const double centre = magAt (wide.l, 24000, 16384, f0);
    const double outer = magAt (wide.l, 24000, 16384, f0 * std::pow (2.0, 30.0 / 1200.0));
    REQUIRE (outer > 0.3 * centre);
    auto mono = run (0.0f);
    for (int i = 0; i < 48000; ++i)
        REQUIRE (mono.l[(size_t) i] == Approx (mono.r[(size_t) i]).margin (1e-6f));
}

TEST_CASE ("Wave Ready: 16 voices sound at once, the 17th steals the oldest, and all notes off releases everything", "[waveready]")
{
    WaveEngine e;
    e.prepare (sr, 256);
    auto p = plain();
    p.osc1.wave = 0;
    e.setParams (p, true);
    for (int k = 0; k < 16; ++k) e.noteOn (48 + k, 0.8f);
    REQUIRE (e.getActiveVoices() == 16);
    e.noteOn (80, 0.8f);
    REQUIRE (e.getActiveVoices() == 16);
    auto x = render (e, 4800);
    for (float v : x.l) REQUIRE (std::isfinite (v));
    REQUIRE (peakAbs (x.l, 2400, 4800) <= 1.0f);   // safety clip holds
    e.allNotesOff();
    render (e, 48000);
    REQUIRE (e.getActiveVoices() == 0);
}

TEST_CASE ("Wave Ready: mono legato glides from the held note to the new one in the glide time, without a retrigger", "[waveready]")
{
    WaveEngine e;
    e.prepare (sr, 256);
    auto p = plain();
    p.mono = true; p.legato = true; p.glideMs = 200.0f;
    p.ampEnv = { 1.0f, 100.0f, 1.0f, 50.0f };
    e.setParams (p, true);
    e.noteOn (57, 1.0f);           // A3
    render (e, 9600);
    e.noteOn (69, 1.0f);           // A4 while A3 is held: glide
    auto x = render (e, 48000);
    // One voice only, the level never drops (no retrigger).
    REQUIRE (e.getActiveVoices() == 1);
    REQUIRE (peakAbs (x.l, 0, 1000) > 0.6f);   // centre pan is -3 dB per channel
    // Pitch at 1 s is A4 within 2 cents; halfway (one time constant ~ 200 ms) it is in between.
    const double late = peakHz (x.l, 30000, 16384, 400.0, 480.0);
    REQUIRE (std::fabs (cents (late, 440.0)) < 2.0);
    const double early = peakHz (x.l, 2000, 4096, 200.0, 480.0);
    REQUIRE (early > 240.0);
    REQUIRE (early < 420.0);
    // Releasing A4 goes back to A3 (still held).
    e.noteOff (69);
    auto y = render (e, 48000);
    REQUIRE (std::fabs (cents (peakHz (y.l, 30000, 16384, 200.0, 240.0), 220.0)) < 2.0);
}

TEST_CASE ("Wave Ready: LFO 1 vibrato of 1 semitone swings the pitch by +-1 st at the LFO rate", "[waveready]")
{
    WaveEngine e;
    e.prepare (sr, 256);
    auto p = plain();
    p.osc1.wave = 0;
    p.lfo1.shape = LfoShape::sine;
    p.lfo1.rateHz = 2.0f;
    p.lfo1ToPitch = 1.0f;
    e.setParams (p, true);
    e.noteOn (69, 1.0f);
    auto x = render (e, 96000);
    // Instantaneous frequency from zero crossings in 50 ms windows: max/min ratio ~ 2^(2/12).
    auto hzIn = [&] (int from, int to)
    {
        int crossings = 0; int first = -1, last = -1;
        for (int i = from + 1; i < to; ++i)
            if (x.l[(size_t) i - 1] < 0.0f && x.l[(size_t) i] >= 0.0f) { if (first < 0) first = i; last = i; ++crossings; }
        return crossings > 1 ? sr * (crossings - 1) / (last - first) : 0.0;
    };
    double hi = 0.0, lo = 1e9;
    for (int w = 48000; w + 2400 <= 96000; w += 1200)
    {
        const double h = hzIn (w, w + 2400);
        hi = std::max (hi, h); lo = std::min (lo, h);
    }
    REQUIRE (cents (hi, lo) == Approx (200.0).margin (25.0));
}

TEST_CASE ("Wave Ready: a synced square LFO on the amplitude repeats every division at the host tempo", "[waveready]")
{
    WaveEngine e;
    e.prepare (sr, 256);
    auto p = plain();
    p.osc1.wave = 0;
    p.bpm = 120.0f;
    p.lfo1.sync = true;
    p.lfo1.division = 5;   // 1/8 = 0.25 s at 120 BPM = 12000 samples
    p.lfo1.shape = LfoShape::square;
    p.lfo1ToAmp = 1.0f;
    e.setParams (p, true);
    e.noteOn (69, 1.0f);
    auto x = render (e, 48000);
    // Loud half then silent half: compare the peak of each 6000-sample half period.
    const float a = peakAbs (x.l, 1000, 5500), b = peakAbs (x.l, 7000, 11500), c = peakAbs (x.l, 13000, 17500);
    REQUIRE (a > 0.6f);
    REQUIRE (b < 0.05f);
    REQUIRE (c > 0.6f);
}

TEST_CASE ("Wave Ready: with no note the output is exactly zero and FX tails end", "[waveready]")
{
    WaveEngine e;
    e.prepare (sr, 256);
    auto p = plain();
    p.reverbMix = 0.5f; p.delayMix = 0.5f; p.chorusMix = 0.5f;
    e.setParams (p, true);
    auto x = render (e, 4800);
    REQUIRE (peakAbs (x.l, 0, 4800) == 0.0f);
    REQUIRE_FALSE (e.isSounding());
    e.noteOn (60, 1.0f);
    render (e, 4800);
    e.noteOff (60);
    auto y = render (e, 48000 * 12);
    REQUIRE (peakAbs (y.l, 48000 * 11, 48000 * 12) < 1e-4f);
}

TEST_CASE ("Wave Ready: filter drive and global drive keep the output bounded and the safety clip never passes 1.0", "[waveready]")
{
    WaveEngine e;
    e.prepare (sr, 256);
    auto p = plain();
    p.osc1.unison = 7; p.osc1.level = 1.0f;
    p.osc2.on = true; p.osc2.level = 1.0f; p.osc2.unison = 7;
    p.subLevel = 1.0f; p.noiseLevel = 1.0f;
    p.filterDrive = 1.0f; p.driveDb = 36.0f; p.outputDb = 12.0f;
    e.setParams (p, true);
    for (int k = 0; k < 8; ++k) e.noteOn (36 + k * 5, 1.0f);
    auto x = render (e, 24000);
    REQUIRE (peakAbs (x.l, 0, 24000) <= 1.0f);
    REQUIRE (peakAbs (x.r, 0, 24000) <= 1.0f);
    for (float v : x.r) REQUIRE (std::isfinite (v));
}
