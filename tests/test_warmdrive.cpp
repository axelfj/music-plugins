// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/WarmDrive/Source/DriveEngine.h"
#include "../shared/dsp/Crossover.h"
#include "TestHelpers.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

using namespace rommuser::warmdrive;
using namespace testhelpers;

namespace
{
constexpr double fs = 48000.0;

std::vector<float> run (const DriveParams& p, std::vector<float> x, DriveMeters* meters = nullptr, int block = 256)
{
    DriveEngine e;
    e.prepare (fs, block, 1);
    e.setParams (p, true);
    DriveMeters m;
    for (size_t pos = 0; pos < x.size(); pos += (size_t) block)
    {
        float* ch[] = { x.data() + pos };
        const auto mm = e.process (ch, 1, (int) std::min<size_t> ((size_t) block, x.size() - pos));
        m.inputPeak = std::max (m.inputPeak, mm.inputPeak);
        m.outputPeak = std::max (m.outputPeak, mm.outputPeak);
        for (size_t b = 0; b < maxBands; ++b)
            m.bandDriveDb[b] = std::max (m.bandDriveDb[b], mm.bandDriveDb[b]);
    }
    if (meters) *meters = m;
    return x;
}

int latencyFor (int factor)
{
    DriveEngine e;
    e.prepare (fs, 256, 1);
    DriveParams p;
    p.oversampling = factor;
    e.setParams (p, true);
    return e.getLatencySamples();
}

// Exact tone level (dB) by projection over whole cycles.
double toneDb (const std::vector<float>& x, double f, size_t skip, size_t maxLen = 16384)
{
    const double period = fs / f;
    const size_t len = (size_t) (std::floor ((double) maxLen / period) * period);
    double sc = 0.0, ss = 0.0;
    for (size_t i = skip; i < std::min (x.size(), skip + len); ++i)
    {
        const double ph = 2.0 * pi * f * (double) i / fs;
        sc += x[i] * std::cos (ph);
        ss += x[i] * std::sin (ph);
    }
    return toDb (2.0 * std::sqrt (sc * sc + ss * ss) / (double) len);
}
} // namespace

TEST_CASE ("Every curve has unity slope at zero and stays bounded")
{
    for (int t = 0; t < numSaturations; ++t)
    {
        const auto type = (Saturation) t;
        REQUIRE (shape (0.0f, type) == Catch::Approx (0.0f).margin (1e-6f));
        REQUIRE (shape (1e-3f, type) / 1e-3f == Catch::Approx (1.0f).margin (0.01f));
        REQUIRE (std::fabs (shape (100.0f, type)) <= 2.0f);
        REQUIRE (std::fabs (shape (-100.0f, type)) <= 2.0f);
    }
}

TEST_CASE ("FourBandSplitter sums flat (< 0.05 dB) across the spectrum")
{
    for (double f : { 40.0, 150.0, 400.0, 800.0, 2000.0, 4000.0, 9000.0, 16000.0 })
    {
        rommuser::dsp::FourBandSplitter s;
        s.prepare (fs);
        s.setFrequencies (150.0, 800.0, 4000.0);
        auto x = sine (f, fs, 48000, 0.5);
        for (auto& v : x)
        {
            float b[4];
            s.process (v, b);
            v = b[0] + b[1] + b[2] + b[3];
        }
        INFO ("f = " << f);
        REQUIRE (std::fabs (toneDb (x, f, 24000) - toDb (0.5)) < 0.05);
    }
}

TEST_CASE ("Mix 0 leaves the level untouched (< 0.05 dB) at every oversampling factor and reports its latency")
{
    for (int factor : { 1, 2, 4 })
    {
        DriveParams p;
        p.mix = 0.0f;
        p.oversampling = factor;
        p.numBands = 4;
        for (auto& b : p.bands) b.driveDb = 24.0f;
        for (double f : { 60.0, 1000.0, 12000.0 })
        {
            const auto out = run (p, sine (f, fs, 48000, 0.5));
            INFO ("factor " << factor << " f " << f);
            REQUIRE (std::fabs (toneDb (out, f, 24000) - toDb (0.5)) < 0.05);
        }
        // An impulse comes out `latency` samples later (the oversampler's filters are symmetric). One band,
        // so no crossover all-pass smears the impulse.
        p.numBands = 1;
        std::vector<float> imp (4096, 0.0f);
        imp[1000] = 1.0f;
        const auto out = run (p, imp);
        size_t peakAt = 0;
        for (size_t i = 1; i < out.size(); ++i)
            if (std::fabs (out[i]) > std::fabs (out[peakAt])) peakAt = i;
        REQUIRE ((int) peakAt == 1000 + latencyFor (factor));
    }
}

TEST_CASE ("Quiet material passes almost untouched (slope 1 at 0) and the dry/wet mix does not comb")
{
    // -40 dBFS 10 kHz tone, 4 bands, tape at drive 0, mix 50 %: the wet path is practically linear and
    // aligned with the dry path, so the level is unchanged.
    DriveParams p;
    p.numBands = 4;
    p.mix = 0.5f;
    p.autoGain = false;
    for (auto& b : p.bands) { b.driveDb = 0.0f; b.type = Saturation::tape; }
    const auto out = run (p, sine (10000.0, fs, 48000, 0.01));
    REQUIRE (toneDb (out, 10000.0, 24000) == Catch::Approx (toDb (0.01)).margin (0.1));
}

TEST_CASE ("Oversampling keeps aliasing low for every curve", "[aliasing]")
{
    // 5 kHz tone driven 18 dB into the curve. Everything in 20 Hz - 20 kHz that is not a harmonic is aliasing.
    const auto type = GENERATE (Saturation::tape, Saturation::tube, Saturation::transistor, Saturation::softClip, Saturation::rectify);

    DriveParams p;
    p.numBands = 1;
    p.autoGain = false;
    p.bands[0].type = type;
    p.bands[0].driveDb = 18.0f;

    const size_t n = 1 << 15;
    const double f0 = fs * 3715.0 / (double) n;
    double alias[3];
    int i = 0;
    for (int factor : { 1, 2, 4 })
    {
        p.oversampling = factor;
        const auto out = run (p, sine (f0, fs, (int) n + 8192, 0.5));
        const auto spec = powerSpectrum (out.data() + 8192, n);
        alias[i++] = inharmonicLevelDb (spec, fs, n, f0, 20.0, 20000.0);
    }
    WARN ("type " << (int) type << " alias 1x " << alias[0] << " / 2x " << alias[1] << " / 4x " << alias[2] << " dBFS");
    // Rectify folds the waveform, whose even harmonics only fall off as 1/n^2: it is the dirty mode by
    // design, so its aliasing floor is higher (still 30 dB better at 4x than without oversampling).
    REQUIRE (alias[2] < (type == Saturation::rectify ? -60.0 : -75.0));
    REQUIRE (alias[2] < alias[0] - 20.0);
}

TEST_CASE ("Tube and Rectify add even harmonics; Tape and Transistor stay odd; no DC leaves the plugin")
{
    const size_t n = 1 << 14;
    const double f0 = fs * 200.0 / (double) n;  // bin-centred ~586 Hz

    auto secondHarmonicRelDb = [&] (Saturation type, double& dcOut)
    {
        DriveParams p;
        p.numBands = 1;
        p.autoGain = false;
        p.oversampling = 4;
        p.bands[0].type = type;
        p.bands[0].driveDb = 12.0f;
        const auto out = run (p, sine (f0, fs, (int) n + 8192, 0.5));
        double mean = 0.0;
        for (size_t i = 8192; i < out.size(); ++i) mean += out[i];
        dcOut = mean / (double) n;
        const auto spec = powerSpectrum (out.data() + 8192, n);
        return toneLevelDb (spec, fs, n, 2.0 * f0) - toneLevelDb (spec, fs, n, f0);
    };

    double dc = 0.0;
    REQUIRE (secondHarmonicRelDb (Saturation::tube, dc) > -40.0);
    REQUIRE (std::fabs (dc) < 1e-3);
    REQUIRE (secondHarmonicRelDb (Saturation::rectify, dc) > -30.0);
    REQUIRE (std::fabs (dc) < 1e-3);
    REQUIRE (secondHarmonicRelDb (Saturation::tape, dc) < -80.0);
    REQUIRE (secondHarmonicRelDb (Saturation::transistor, dc) < -80.0);
}

TEST_CASE ("Auto gain keeps a -12 dBFS tone within 1 dB for every curve at heavy drive")
{
    for (int t = 0; t < numSaturations; ++t)
    {
        DriveParams p;
        p.numBands = 1;
        p.autoGain = true;
        p.bands[0].type = (Saturation) t;
        p.bands[0].driveDb = 24.0f;
        const auto out = run (p, sine (1000.0, fs, 48000, 0.25118864));
        INFO ("type " << t);
        REQUIRE (std::fabs (toDb (rms (out.data() + 24000, 16384) * std::sqrt (2.0)) - (-12.0)) < 1.0);
    }
}

TEST_CASE ("Only the driven band is distorted: a clean band next to a saturated one keeps its level")
{
    DriveParams p;
    p.numBands = 3;
    p.crossoverHz[0] = 200.0f;
    p.crossoverHz[1] = 2500.0f;
    p.autoGain = false;
    p.oversampling = 2;
    p.bands[0] = { Saturation::transistor, 30.0f, 20000.0f, 1.0f };  // low band: crushed
    p.bands[1] = { Saturation::tape, 0.0f, 20000.0f, 0.0f };          // mid: clean (mix 0)
    p.bands[2] = { Saturation::tape, 0.0f, 20000.0f, 0.0f };          // high: clean

    auto x = sine (40.0, fs, 48000, 0.5);
    const auto hi = sine (6000.0, fs, 48000, 0.05);
    for (size_t i = 0; i < x.size(); ++i) x[i] += hi[i];
    const auto out = run (p, x);
    REQUIRE (toneDb (out, 6000.0, 24000) == Catch::Approx (toDb (0.05)).margin (0.2));
    // The low band did get squashed.
    REQUIRE (toneDb (out, 40.0, 24000) < toDb (0.5) - 3.0);
}

TEST_CASE ("HQ (ADAA) lowers aliasing for every curve at the same oversampling", "[aliasing][hq]")
{
    const auto type = GENERATE (Saturation::tape, Saturation::tube, Saturation::transistor, Saturation::softClip, Saturation::rectify);
    const int factor = GENERATE (1, 2);

    DriveParams p;
    p.numBands = 1;
    p.autoGain = false;
    p.bands[0].type = type;
    p.bands[0].driveDb = 18.0f;
    p.oversampling = factor;

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
    WARN ("type " << (int) type << " " << factor << "x alias normal " << alias[0] << " / HQ " << alias[1] << " dBFS");
    REQUIRE (alias[1] < alias[0] - 6.0);
}

TEST_CASE ("HQ keeps the level and the mix stays comb-free", "[hq]")
{
    // At low drive the curve is almost linear: HQ must not change the level of a mid tone, and a
    // 50 % band mix must not comb (dry and wet get the same two-sample average).
    DriveParams p;
    p.numBands = 1;
    p.autoGain = false;
    p.hq = true;
    p.oversampling = 2;
    p.bands[0].driveDb = 0.0f;
    p.bands[0].mix = 0.5f;
    for (double f : { 1000.0, 10000.0 })
    {
        const auto out = run (p, sine (f, fs, 48000, 0.01));
        INFO ("freq " << f);
        REQUIRE (toneDb (out, f, 24000) == Catch::Approx (toDb (0.01)).margin (0.25));
    }
}
