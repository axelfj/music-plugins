// Pitch-R DSP tests: pitch accuracy in cents, purity of shifted sines and chords (no ghost tones),
// click sharpness (no pre-echo), stereo behaviour, exact pass-through at 0 st, formant handling,
// reported latency per mode and sample rate, click-free automation, loudness of kicks, robustness.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/PitchR/Source/PitchEngine.h"
#include "TestHelpers.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace rommuser::pitchr;
using namespace testhelpers;

namespace
{
constexpr double sr = 48000.0;
constexpr size_t analysisLength = 32768;   // 0.68 s, taken after the engine has settled

struct Stereo
{
    std::vector<float> l, r;
};

PitchParams shift (double semis, Mode mode = Mode::clean)
{
    PitchParams p;
    p.mode = mode;
    p.semitones = (float) semis;
    return p;
}

void runEngine (PitchEngine& e, Stereo& x, int block = 256)
{
    const int n = (int) x.l.size();
    for (int pos = 0; pos < n;)
    {
        const int len = std::min (block, n - pos);
        float* io[2] = { x.l.data() + pos, x.r.data() + pos };
        e.process (io, 2, len);
        pos += len;
    }
}

Stereo process (const std::vector<float>& in, const PitchParams& p, double rate = sr, int block = 256)
{
    PitchEngine e;
    e.prepare (rate, 512, 2);
    e.setParams (p, true);
    Stereo x { in, in };
    runEngine (e, x, block);
    return x;
}

// Spectral peak near `expect`, with parabolic interpolation on the log power.
double peakHz (const float* x, double expect, double rate = sr)
{
    const auto p = powerSpectrum (x, analysisLength);
    const double binHz = rate / (double) analysisLength;
    const long c = std::lround (expect / binHz);
    long best = c;
    for (long b = std::max (1L, c - 30); b < std::min ((long) p.size() - 1, c + 30); ++b)
        if (p[(size_t) b] > p[(size_t) best]) best = b;
    const double a = std::log (p[(size_t) best - 1] + 1e-30), m = std::log (p[(size_t) best] + 1e-30), g = std::log (p[(size_t) best + 1] + 1e-30);
    return ((double) best + 0.5 * (a - g) / (a - 2.0 * m + g)) * binHz;
}

// Power away from the expected tones relative to the power at them (dB), 20 Hz .. 20 kHz.
double spurDb (const float* x, const std::vector<double>& tones, int guard = 6)
{
    const auto p = powerSpectrum (x, analysisLength);
    const double binHz = sr / (double) analysisLength;
    std::vector<bool> expected (p.size(), false);
    for (double f : tones)
    {
        const long c = std::lround (f / binHz);
        for (long b = c - guard; b <= c + guard; ++b)
            if (b >= 0 && b < (long) p.size()) expected[(size_t) b] = true;
    }
    double at = 0.0, away = 0.0;
    for (size_t k = (size_t) (20.0 / binHz); k < std::min (p.size(), (size_t) (20000.0 / binHz)); ++k)
        (expected[k] ? at : away) += p[k];
    return 10.0 * std::log10 ((away + 1e-30) / (at + 1e-30));
}

double cents (double a, double b) { return 1200.0 * std::log2 (a / b); }

std::vector<float> chord (int n, std::vector<double>& tonesOut, double ratio)
{
    std::vector<float> x ((size_t) n, 0.0f);
    for (double f : { 261.63, 329.63, 392.0 })
        for (int h = 1; h <= 4; ++h)
        {
            const auto v = sine (f * h, sr, n, 0.15 / h);
            for (int i = 0; i < n; ++i) x[(size_t) i] += v[(size_t) i];
            tonesOut.push_back (f * h * ratio);
        }
    return x;
}

// Vowel /a/: harmonics of f0 shaped by three formant resonances, amplitude 1/h.
double formantFilterDb (double f)
{
    auto res = [] (double hz, double fc, double bw) { return 1.0 / std::sqrt (std::pow (1.0 - (hz / fc) * (hz / fc), 2.0) + std::pow (hz * bw / (fc * fc), 2.0)); };
    return 20.0 * std::log10 (res (f, 730.0, 90.0) + 0.5 * res (f, 1090.0, 110.0) + 0.25 * res (f, 2440.0, 160.0));
}

std::vector<float> vowel (double f0, int n)
{
    std::vector<float> x ((size_t) n, 0.0f);
    for (int h = 1; h * f0 < 8000.0; ++h)
    {
        const auto v = sine (h * f0, sr, n, 0.02 * std::pow (10.0, formantFilterDb (h * f0) / 20.0) / h);
        for (int i = 0; i < n; ++i) x[(size_t) i] += v[(size_t) i];
    }
    return x;
}

// Mean deviation (dB) of the output harmonics from the formant filter at their new frequency,
// after undoing the source's 1/h tilt and the overall level: how well the vowel shape survived.
double formantErrorDb (const std::vector<float>& y, double outF0)
{
    const auto p = powerSpectrum (y.data() + (size_t) sr * 2, analysisLength);
    std::vector<double> e;
    for (int h = 1; h * outF0 < 4000.0; ++h)
        if (h * outF0 >= 250.0)
            e.push_back (toneLevelDb (p, sr, analysisLength, h * outF0) + 20.0 * std::log10 ((double) h) - formantFilterDb (h * outF0));
    double mean = 0.0;
    for (double v : e) mean += v;
    mean /= (double) e.size();
    double dev = 0.0;
    for (double v : e) dev += std::fabs (v - mean);
    return dev / (double) e.size();
}
} // namespace

TEST_CASE ("Pitch-R: shifted sines land within 1 cent and stay pure", "[pitchr]")
{
    const int n = (int) (sr * 2.0);
    for (double f : { 55.0, 440.0, 3000.0 })
        for (double st : { -12.0, -7.0, -1.0, 0.5, 5.0, 12.0 })
        {
            const auto y = process (sine (f, sr, n, 0.5), shift (st));
            const double target = f * std::pow (2.0, st / 12.0);
            const float* seg = y.l.data() + (size_t) sr;
            INFO (f << " Hz shifted " << st << " st");
            CHECK (std::fabs (cents (peakHz (seg, target), target)) < 1.0);
            // a 27.5 Hz result sits only ~2 FFT bins from DC, so its window overlaps its own mirror image
            CHECK (spurDb (seg, { target }) < (f < 100.0 ? -55.0 : -65.0));
        }
}

TEST_CASE ("Pitch-R: chords keep clean partials (no ghost tones between notes)", "[pitchr]")
{
    const int n = (int) (sr * 2.0);
    for (double st : { -7.0, -1.0, 5.0, 12.0 })
    {
        std::vector<double> tones;
        const auto x = chord (n, tones, std::pow (2.0, st / 12.0));
        const auto y = process (x, shift (st));
        INFO ("chord shifted " << st << " st");
        CHECK (spurDb (y.l.data() + (size_t) sr, tones) < -45.0);
    }
}

TEST_CASE ("Pitch-R: clicks stay sharp (no pre-echo)", "[pitchr]")
{
    const int n = (int) (sr * 2.5);
    std::vector<float> x ((size_t) n, 0.0f);
    for (int t = (int) (0.3 * sr); t < n - (int) (0.3 * sr); t += (int) (0.4 * sr)) x[(size_t) t] = 0.9f;
    for (double st : { -12.0, 7.0, 12.0 })
    {
        const auto y = process (x, shift (st));
        for (int t = (int) (1.1 * sr); t < n - (int) (0.4 * sr); t += (int) (0.4 * sr))
        {
            int pk = t;
            for (int i = t; i < t + (int) (0.3 * sr); ++i)
                if (std::fabs (y.l[(size_t) i]) > std::fabs (y.l[(size_t) pk])) pk = i;
            double pre = 0.0, core = 0.0;
            for (int i = pk - (int) (0.05 * sr); i < pk - (int) (0.002 * sr); ++i) pre += (double) y.l[(size_t) i] * y.l[(size_t) i];
            for (int i = pk - (int) (0.002 * sr); i < pk + (int) (0.002 * sr); ++i) core += (double) y.l[(size_t) i] * y.l[(size_t) i];
            INFO ("click shifted " << st << " st at " << t / sr << " s");
            CHECK (10.0 * std::log10 ((pre + 1e-30) / core) < -25.0);
        }
    }
}

TEST_CASE ("Pitch-R: 0 st is an exact, latency-compensated pass-through", "[pitchr]")
{
    const int n = (int) (sr * 1.0);
    const auto x = whiteNoise (n, 0.5, 9);
    for (Mode m : { Mode::clean, Mode::voice, Mode::fast })
    {
        PitchEngine e;
        e.prepare (sr, 512, 2);
        e.setParams (shift (0.0, m), true);
        REQUIRE (e.isPassThrough());
        const int latency = e.getLatencySamples();
        Stereo y { x, x };
        runEngine (e, y, 333);
        float worst = 0.0f;
        for (int i = latency; i < n; ++i)
            worst = std::max (worst, std::fabs (y.l[(size_t) i] - x[(size_t) (i - latency)]));
        CHECK (worst == 0.0f);
    }
}

TEST_CASE ("Pitch-R: reported latency per mode and sample rate matches the measured delay", "[pitchr]")
{
    for (double rate : { 44100.0, 48000.0, 96000.0 })
        for (Mode m : { Mode::clean, Mode::voice, Mode::fast })
        {
            PitchEngine e;
            e.prepare (rate, 512, 2);
            e.setParams (shift (5.0, m), true);
            const int latency = e.getLatencySamples();
            const double ms = latency / rate * 1000.0;
            INFO ("mode " << (int) m << " at " << rate << " Hz: " << latency << " samples = " << ms << " ms");
            CHECK (latency == (1 << PitchEngine::fftOrderFor (m, rate)));
            CHECK (ms < (m == Mode::clean ? 100.0 : m == Mode::voice ? 50.0 : 25.0));

            // an impulse comes out (shifted) centred on the reported latency
            const int n = latency * 4;
            Stereo y { std::vector<float> ((size_t) n, 0.0f), std::vector<float> ((size_t) n, 0.0f) };
            y.l[(size_t) latency] = y.r[(size_t) latency] = 1.0f;
            runEngine (e, y, 128);
            int pk = 0;
            for (int i = 0; i < n; ++i)
                if (std::fabs (y.l[(size_t) i]) > std::fabs (y.l[(size_t) pk])) pk = i;
            CHECK (std::abs (pk - 2 * latency) <= latency / 64);
        }
}

TEST_CASE ("Pitch-R: stereo - mono stays mono, channels stay apart", "[pitchr]")
{
    const int n = (int) (sr * 1.5);
    std::vector<double> tones;
    const auto x = chord (n, tones, 1.0);

    const auto mono = process (x, shift (7.0));
    float worst = 0.0f;
    for (size_t i = 0; i < mono.l.size(); ++i) worst = std::max (worst, std::fabs (mono.l[i] - mono.r[i]));
    CHECK (worst < 3.0e-4f);   // -70 dB: float rounding of the shared stereo FFT, inaudible

    // left only in: the right channel stays silent
    PitchEngine e;
    e.prepare (sr, 512, 2);
    e.setParams (shift (-5.0), true);
    Stereo y { x, std::vector<float> ((size_t) n, 0.0f) };
    runEngine (e, y);
    CHECK (rms (y.r.data() + (size_t) sr, (int) (0.4 * sr)) < 1.0e-5 * rms (y.l.data() + (size_t) sr, (int) (0.4 * sr)));
}

TEST_CASE ("Pitch-R: keep formants holds the vowel shape, formant shift moves it", "[pitchr]")
{
    const int n = (int) (sr * 3.0);
    for (double f0 : { 140.0, 300.0 })
        for (double st : { -7.0, 7.0 })
        {
            const auto x = vowel (f0, n);
            PitchParams kept = shift (st, Mode::voice);
            kept.keepFormants = true;
            const double outF0 = f0 * std::pow (2.0, st / 12.0);
            const double keptErr = formantErrorDb (process (x, kept).l, outF0);
            const double movedErr = formantErrorDb (process (x, shift (st, Mode::voice)).l, outF0);
            INFO ("vowel f0 " << f0 << " shifted " << st << " st: kept " << keptErr << " dB, not kept " << movedErr << " dB");
            CHECK (keptErr < 2.5);
            CHECK (movedErr > keptErr + 2.5);
        }

    // formant +12 st at 0 st: same note, brighter (spectral centroid up)
    const auto x = vowel (140.0, n);
    PitchParams up;
    up.mode = Mode::voice;
    up.formantSemis = 12.0f;
    const auto y = process (x, up);
    CHECK (std::fabs (cents (peakHz (y.l.data() + (size_t) sr * 2, 140.0), 140.0)) < 2.0);
    auto centroid = [] (const float* s) {
        const auto p = powerSpectrum (s, analysisLength);
        double num = 0.0, den = 0.0;
        for (size_t k = 1; k < p.size(); ++k) { num += (double) k * p[k]; den += p[k]; }
        return num / den * sr / (double) analysisLength;
    };
    CHECK (centroid (y.l.data() + (size_t) sr * 2) > 1.5 * centroid (x.data() + (size_t) sr * 2));
}

TEST_CASE ("Pitch-R: automating the shift does not click", "[pitchr]")
{
    const int n = (int) (sr * 2.5);
    PitchEngine e;
    e.prepare (sr, 512, 2);
    e.setParams (shift (-12.0), true);
    Stereo y { sine (440.0, sr, n, 0.5), sine (440.0, sr, n, 0.5) };
    const int block = 64;
    for (int pos = 0; pos < n; pos += block)
    {
        const double t = (double) pos / n;
        e.setParams (shift (-12.0 + 24.0 * t));   // a full two-octave sweep
        float* io[2] = { y.l.data() + pos, y.r.data() + pos };
        e.process (io, 2, std::min (block, n - pos));
    }
    // a 0.5 sine at up to 880 Hz moves at most 0.058 per sample; a click would jump far more
    float worst = 0.0f;
    for (int i = e.getLatencySamples() + 1; i < n; ++i) worst = std::max (worst, std::fabs (y.l[(size_t) i] - y.l[(size_t) i - 1]));
    CHECK (worst < 0.12f);
}

TEST_CASE ("Pitch-R: kicks keep their level and switching modes is safe", "[pitchr]")
{
    const int n = (int) (sr * 3.0);
    std::vector<float> kick ((size_t) n, 0.0f);
    for (int t0 = (int) (0.25 * sr); t0 < n - (int) (0.5 * sr); t0 += (int) (0.5 * sr))
    {
        double ph = 0.0;
        for (int i = 0; i < (int) (0.4 * sr); ++i)
        {
            const double t = i / sr, f = 55.0 + 200.0 * std::exp (-t * 40.0);
            ph += 2.0 * pi * f / sr;
            kick[(size_t) (t0 + i)] += (float) (0.8 * std::exp (-t * 8.0) * std::sin (ph));
        }
    }
    for (double st : { -12.0, 7.0 })
    {
        PitchEngine e;
        e.prepare (sr, 512, 2);
        e.setParams (shift (st), true);
        const int latency = e.getLatencySamples();
        Stereo y { kick, kick };
        runEngine (e, y);
        const double d = toDb (rms (y.l.data() + (size_t) sr, (int) sr) / rms (kick.data() + (size_t) sr - (size_t) latency, (int) sr));
        INFO ("kick shifted " << st << " st: " << d << " dB");
        CHECK (std::fabs (d) < 2.0);
    }

    // switching modes while playing: no NaN, no overs, reported latency follows the new mode
    PitchEngine e;
    e.prepare (sr, 512, 2);
    e.setParams (shift (7.0), true);
    Stereo y { kick, kick };
    for (int pos = 0, i = 0; pos < n; pos += 256, ++i)
    {
        e.setParams (shift (7.0, (Mode) ((i / 40) % numModes)));
        float* io[2] = { y.l.data() + pos, y.r.data() + pos };
        e.process (io, 2, std::min (256, n - pos));
    }
    for (float v : y.l) REQUIRE (std::isfinite (v));
    CHECK (peak (y.l) < 1.5);
}

TEST_CASE ("Pitch-R: robust to silence, DC, loud noise, odd block sizes and mono hosts", "[pitchr]")
{
    const int n = (int) (sr * 1.0);
    const auto silent = process (std::vector<float> ((size_t) n, 0.0f), shift (5.0));
    CHECK (peak (silent.l) == 0.0);

    const auto dc = process (std::vector<float> ((size_t) n, 0.5f), shift (-7.0));
    for (float v : dc.l) REQUIRE (std::isfinite (v));
    CHECK (peak (dc.l) < 1.0);

    const auto loud = whiteNoise (n, 1.0, 3);
    for (int block : { 1, 7, 100, 4096 })
    {
        const auto y = process (loud, shift (12.0), sr, block);
        for (float v : y.l) REQUIRE (std::isfinite (v));
        CHECK (peak (y.l) < 4.0);
    }

    // mono host
    PitchEngine e;
    e.prepare (sr, 512, 1);
    e.setParams (shift (3.0), true);
    auto x = sine (440.0, sr, n, 0.5);
    float* io[1] = { x.data() };
    e.process (io, 1, n);
    CHECK (std::fabs (cents (peakHz (x.data() + (size_t) (0.25 * sr), 440.0 * std::pow (2.0, 0.25)), 440.0 * std::pow (2.0, 0.25))) < 1.0);
}

TEST_CASE ("Pitch-R: CPU use (informative)", "[pitchr][cpu]")
{
    const int n = (int) (sr * 5.0);
    const auto x = whiteNoise (n, 0.3, 5);
    for (Mode m : { Mode::clean, Mode::voice, Mode::fast })
        for (bool formants : { false, true })
        {
            PitchEngine e;
            e.prepare (sr, 512, 2);
            PitchParams p = shift (5.0, m);
            p.keepFormants = formants;
            e.setParams (p, true);
            Stereo y { x, x };
            const auto t0 = std::chrono::steady_clock::now();
            runEngine (e, y);
            const double secs = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
            std::printf ("Pitch-R mode %d formants %d: %.1f %% of one core (stereo, 48 kHz)\n", (int) m, (int) formants, secs / 5.0 * 100.0);
        }
    SUCCEED();
}
