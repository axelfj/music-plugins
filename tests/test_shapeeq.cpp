// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/ShapeEQ/Source/EqEngine.h"
#include "TestHelpers.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

using namespace rommuser::shapeeq;
using namespace rommuser::dsp;
using namespace testhelpers;

namespace
{
constexpr double fs = 44100.0;

// Analog magnitude (dB) of the prototype each cookbook filter approximates.
double analogBellDb (double f, double f0, double q, double gainDb)
{
    const double A = std::pow (10.0, gainDb / 40.0);
    const std::complex<double> s (0.0, f / f0);
    const auto h = (s * s + s * (A / q) + 1.0) / (s * s + s / (A * q) + 1.0);
    return 20.0 * std::log10 (std::abs (h));
}

double analogButterworthLowCutDb (double f, double f0, int order)
{
    return -10.0 * std::log10 (1.0 + std::pow (f0 / f, 2.0 * order));
}

// Runs a mono signal through the engine in 480-sample blocks.
std::vector<float> run (EqEngine& e, std::vector<float> x)
{
    for (size_t pos = 0; pos < x.size(); pos += 480)
    {
        float* ch[] = { x.data() + pos };
        e.process (ch, 1, (int) std::min<size_t> (480, x.size() - pos));
    }
    return x;
}

// Measured gain (dB) of a steady sine through the engine vs the dry sine.
double measuredGainDb (EqEngine& e, double f)
{
    const int n = 1 << 15;
    const auto in = sine (f, fs, n + 8192, 0.25);
    const auto out = run (e, in);
    return toDb (rms (out.data() + 8192, n)) - toDb (rms (in.data() + 8192, n));
}
} // namespace

TEST_CASE ("Matched bell follows its analog prototype up to Nyquist", "[shapeeq]")
{
    const double f0 = GENERATE (100.0, 1000.0, 4000.0, 10000.0);
    const double gain = GENERATE (-12.0, 6.0);
    const auto c = biquad::bellMatched (f0, 1.0, gain, fs);
    for (double f : { f0 * 0.25, f0 * 0.5, f0, f0 * 2.0, f0 * 4.0 })
    {
        if (f > 16000.0) continue; // matching degrades in the last half-octave below Nyquist (~1 dB at 20 kHz)
        const double got = c.magnitudeDb (f, fs), want = analogBellDb (f, f0, 1.0, gain);
        INFO ("f0 " << f0 << " f " << f << " got " << got << " want " << want);
        // Exact at the centre and within 0.1 dB over the audible band; the top octave is matched
        // rather than exact, where the cookbook bell would already be several dB off.
        REQUIRE (std::fabs (got - want) < (f < 10000.0 ? 0.1 : 0.25));
    }
    REQUIRE (std::fabs (c.magnitudeDb (f0, fs) - gain) < 0.01);
}

TEST_CASE ("Low cut slopes follow Butterworth responses", "[shapeeq]")
{
    const int slope = GENERATE (6, 12, 18, 24, 36, 48);
    BandParams p;
    p.enabled = true;
    p.type = FilterType::lowCut;
    p.freq = 200.0;
    p.q = 0.7071;
    p.slopeDb = slope;
    const auto band = designBand (p, fs);
    const int order = slope / 6;
    for (double f : { 25.0, 50.0, 100.0, 200.0, 400.0, 2000.0 })
    {
        const double w = 2.0 * pi * f / fs;
        const double got = 20.0 * std::log10 (std::abs (band.response (w)));
        const double want = analogButterworthLowCutDb (f, 200.0, order);
        INFO ("slope " << slope << " f " << f << " got " << got << " want " << want);
        REQUIRE (std::fabs (got - want) < 0.15);
    }
}

TEST_CASE ("Engine applies the designed gain to a sine and is a null when flat", "[shapeeq]")
{
    EqEngine e;
    e.prepare (fs, 512, 1);

    EqParams p;
    SECTION ("flat curve is bit-exact")
    {
        e.setParams (p, true);
        const auto in = whiteNoise (10000, 0.5);
        const auto out = run (e, in);
        for (size_t i = 0; i < in.size(); ++i)
            REQUIRE (out[i] == in[i]);
    }
    SECTION ("bell gain is reproduced on a sine")
    {
        p.bands[0].enabled = true;
        p.bands[0].freq = 1000.0;
        p.bands[0].gainDb = -9.0;
        p.bands[0].q = 2.0;
        e.setParams (p, true);
        REQUIRE (std::fabs (measuredGainDb (e, 1000.0) - (-9.0)) < 0.05);
        e.reset();
        REQUIRE (std::fabs (measuredGainDb (e, 100.0)) < 0.05);
    }
    SECTION ("output gain is applied")
    {
        p.outputDb = -6.0;
        e.setParams (p, true);
        REQUIRE (std::fabs (measuredGainDb (e, 1000.0) - (-6.0)) < 0.05);
    }
}

TEST_CASE ("Parameter changes ramp without clicks", "[shapeeq]")
{
    EqEngine e;
    e.prepare (fs, 512, 1);
    EqParams p;
    p.bands[0].enabled = true;
    p.bands[0].freq = 500.0;
    p.bands[0].gainDb = 0.0;
    p.bands[0].q = 1.0;
    e.setParams (p, true);

    auto x = sine (500.0, fs, 20000, 0.5);
    // Process the first half, then slam the gain to +12 dB mid-stream.
    float* ch[] = { x.data() };
    e.process (ch, 1, 10000);
    p.bands[0].gainDb = 12.0;
    e.setParams (p);
    float* ch2[] = { x.data() + 10000 };
    e.process (ch2, 1, 10000);

    // The largest sample-to-sample step must stay close to what a 500 Hz sine at +12 dB can do.
    const double maxSineStep = 0.5 * 4.0 * 2.0 * pi * 500.0 / fs;
    double maxStep = 0.0;
    for (size_t i = 10001; i < x.size(); ++i)
        maxStep = std::max (maxStep, (double) std::fabs (x[i] - x[i - 1]));
    REQUIRE (maxStep < maxSineStep * 1.3);
}

TEST_CASE ("Mid/side placement leaves the other component untouched", "[shapeeq]")
{
    EqEngine e;
    e.prepare (fs, 512, 2);
    EqParams p;
    p.bands[0].enabled = true;
    p.bands[0].type = FilterType::highCut;
    p.bands[0].freq = 200.0;
    p.bands[0].slopeDb = 48;
    p.bands[0].placement = Placement::side;
    e.setParams (p, true);

    // A mono (mid-only) signal has no side content, so a side-band filter must not change it.
    auto L = sine (3000.0, fs, 8192, 0.5);
    auto R = L;
    float* ch[] = { L.data(), R.data() };
    e.process (ch, 2, 8192);
    for (size_t i = 0; i < L.size(); ++i)
    {
        REQUIRE (std::fabs (L[i] - R[i]) < 1e-6f);
        REQUIRE (std::fabs ((double) L[i] - 0.5 * std::sin (2.0 * pi * 3000.0 * (double) i / fs)) < 1e-5);
    }
}

TEST_CASE ("Auto gain compensates a broad boost", "[shapeeq]")
{
    EqEngine e;
    e.prepare (fs, 512, 1);
    EqParams p;
    p.bands[0].enabled = true;
    p.bands[0].type = FilterType::bell;
    p.bands[0].freq = 1000.0;
    p.bands[0].gainDb = 6.0;
    p.bands[0].q = 0.3;
    p.autoGain = true;
    const double comp = e.autoGainDb (p);
    REQUIRE (comp < -2.0);
    REQUIRE (comp > -6.0);
}
