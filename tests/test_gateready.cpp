// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/GateReady/Source/GateEngine.h"
#include "TestHelpers.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

using namespace rommuser::gateready;
using namespace testhelpers;

namespace
{
constexpr double fs = 48000.0;

std::vector<float> run (GateEngine& e, std::vector<float> x, const std::vector<float>* sc = nullptr, int block = 256)
{
    for (size_t pos = 0; pos < x.size(); pos += (size_t) block)
    {
        float* ch[] = { x.data() + pos };
        const float* scp[] = { sc ? sc->data() + pos : nullptr };
        e.process (ch, sc ? scp : nullptr, 1, (int) std::min<size_t> ((size_t) block, x.size() - pos));
    }
    return x;
}

double windowPeakDb (const std::vector<float>& x, size_t start, size_t len)
{
    double p = 0.0;
    for (size_t i = start; i < std::min (x.size(), start + len); ++i)
        p = std::max (p, (double) std::fabs (x[i]));
    return toDb (p);
}

std::vector<float> levelSteps (double loDb, double hiDb, int segmentSamples, int segments)
{
    std::vector<float> x;
    for (int s = 0; s < segments; ++s)
    {
        const double amp = std::pow (10.0, (s % 2 == 0 ? hiDb : loDb) / 20.0);
        for (int i = 0; i < segmentSamples; ++i)
            x.push_back ((float) (amp * std::sin (2.0 * pi * 1000.0 * (double) (s * segmentSamples + i) / fs)));
    }
    return x;
}
} // namespace

TEST_CASE ("Gain computer: expander slope, hard gate, knee and range floor")
{
    REQUIRE (gainComputerDb (-20.0f, -30.0f, 4.0f, 0.0f, -80.0f) == Catch::Approx (0.0f));
    REQUIRE (gainComputerDb (-40.0f, -30.0f, 4.0f, 0.0f, -80.0f) == Catch::Approx (-30.0f));   // 10 dB under x (4 - 1)
    REQUIRE (gainComputerDb (-70.0f, -30.0f, 4.0f, 0.0f, -80.0f) == Catch::Approx (-80.0f));   // floored at range
    REQUIRE (gainComputerDb (-30.1f, -30.0f, infiniteRatio, 0.0f, -60.0f) == Catch::Approx (-60.0f));
    REQUIRE (gainComputerDb (-30.0f, -30.0f, infiniteRatio, 6.0f, -60.0f) == Catch::Approx (-30.0f));  // middle of the knee
}

TEST_CASE ("Above the threshold the output is bit-exact once the gate has opened")
{
    GateEngine e;
    e.prepare (fs, 1);
    GateParams p;
    p.thresholdDb = -30.0f;
    p.attackMs = 1.0f;
    e.setParams (p, true);
    const auto in = sine (440.0, fs, 24000, 0.5);
    const auto out = run (e, in);
    for (size_t i = 4800; i < in.size(); ++i)  // 100 ms in: attack long settled
        REQUIRE (out[i] == in[i]);
}

TEST_CASE ("Below the threshold the steady-state attenuation is exactly Range")
{
    for (float range : { -20.0f, -40.0f, -80.0f })
    {
        GateEngine e;
        e.prepare (fs, 1);
        GateParams p;
        p.thresholdDb = -30.0f;
        p.rangeDb = range;
        p.releaseMs = 20.0f;
        e.setParams (p, true);
        const auto out = run (e, sine (1000.0, fs, 48000, std::pow (10.0, -50.0 / 20.0)));
        INFO ("range " << range);
        REQUIRE (windowPeakDb (out, 36000, 4800) == Catch::Approx (-50.0 + range).margin (0.1));
    }
}

TEST_CASE ("Expander ratio turns a quiet signal down by (threshold - level) x (ratio - 1)")
{
    GateEngine e;
    e.prepare (fs, 1);
    GateParams p;
    p.thresholdDb = -30.0f;
    p.ratio = 3.0f;
    p.hysteresisDb = 0.0f;
    p.rangeDb = -80.0f;
    e.setParams (p, true);
    const auto out = run (e, sine (1000.0, fs, 48000, std::pow (10.0, -40.0 / 20.0)));
    REQUIRE (windowPeakDb (out, 36000, 4800) == Catch::Approx (-40.0 - 20.0).margin (0.2));
}

TEST_CASE ("Hysteresis keeps the gate open while the level dips under the threshold but not under threshold - hysteresis")
{
    // Level alternates -29 / -31 dBFS every 50 ms, threshold -30.
    const auto in = levelSteps (-31.0, -29.0, 2400, 20);

    GateEngine withHyst;
    withHyst.prepare (fs, 1);
    GateParams p;
    p.thresholdDb = -30.0f;
    p.hysteresisDb = 3.0f;
    p.holdMs = 0.0f;
    p.releaseMs = 5.0f;
    withHyst.setParams (p, true);
    const auto out = run (withHyst, in);
    for (size_t i = 4800; i < in.size(); ++i)   // from the second segment on: never closes
        REQUIRE (out[i] == in[i]);

    GateEngine noHyst;
    noHyst.prepare (fs, 1);
    p.hysteresisDb = 0.0f;
    noHyst.setParams (p, true);
    const auto out2 = run (noHyst, in);
    // Without hysteresis the -31 dB segments get attenuated (checked at the end of the 4th segment).
    REQUIRE (windowPeakDb (out2, 4 * 2400 - 480, 480) < -31.0 - 20.0);
}

TEST_CASE ("Hold keeps the gate open for Hold ms after the signal stops, then it releases")
{
    GateEngine e;
    e.prepare (fs, 1);
    GateParams p;
    p.thresholdDb = -30.0f;
    p.holdMs = 200.0f;
    p.releaseMs = 10.0f;
    p.rangeDb = -80.0f;
    e.setParams (p, true);

    // 100 ms of tone at -10 dBFS, then a -60 dBFS tone (under the threshold, so we can see the gain).
    std::vector<float> in = sine (1000.0, fs, 4800, std::pow (10.0, -10.0 / 20.0));
    const auto tail = sine (1000.0, fs, 48000, std::pow (10.0, -60.0 / 20.0));
    in.insert (in.end(), tail.begin(), tail.end());
    const auto out = run (e, in);

    // 150 ms after the loud part stopped: still fully open (hold is 200 ms).
    REQUIRE (windowPeakDb (out, 4800 + 7200, 480) == Catch::Approx (-60.0).margin (0.1));
    // 200 ms + 10 time constants of release later: closed, i.e. at -60 + range.
    REQUIRE (windowPeakDb (out, 4800 + 9600 + 4800, 480) == Catch::Approx (-60.0 - 80.0).margin (0.5));
}

TEST_CASE ("The fastest attack still ramps the gain over several samples (no single-sample step)")
{
    GateEngine e;
    e.prepare (fs, 1);
    GateParams p;
    p.thresholdDb = -30.0f;
    p.attackMs = 0.1f;
    p.rangeDb = -80.0f;
    e.setParams (p, true);

    // Silence, then a full-scale DC-free tone: the output gain goes from -80 dB to 0 dB.
    std::vector<float> in (2400, 0.0f);
    const auto tone = sine (100.0, fs, 4800, 1.0);
    in.insert (in.end(), tone.begin(), tone.end());
    const auto out = run (e, in);
    float maxStep = 0.0f;
    for (size_t i = 2400; i < in.size(); ++i)
    {
        const float gIn = std::fabs (in[i]) > 1e-3f ? out[i] / in[i] : 1.0f;
        const float gPrev = std::fabs (in[i - 1]) > 1e-3f ? out[i - 1] / in[i - 1] : gIn;
        maxStep = std::max (maxStep, std::fabs (gIn - gPrev));
    }
    REQUIRE (maxStep < 0.3f);
}

TEST_CASE ("Lookahead opens the gate before the transient arrives")
{
    auto makeInput = []
    {
        std::vector<float> in (4800, 0.0f);
        const auto burst = sine (2000.0, fs, 4800, 0.5);
        in.insert (in.end(), burst.begin(), burst.end());
        return in;
    };

    GateParams p;
    p.thresholdDb = -30.0f;
    p.attackMs = 1.0f;   // 5 ms of lookahead = 5 time constants: the 80 dB ramp is done to within 0.6 dB
    p.rangeDb = -80.0f;

    GateEngine noLa;
    noLa.prepare (fs, 1);
    noLa.setParams (p, true);
    const auto outNoLa = run (noLa, makeInput());
    // Without lookahead the first 0.5 ms of the burst are heavily attenuated.
    REQUIRE (windowPeakDb (outNoLa, 4800, 24) < toDb (0.5) - 20.0);

    GateEngine la;
    la.prepare (fs, 1);
    p.lookaheadMs = 5.0f;
    la.setParams (p, true);
    REQUIRE (la.getLatencySamples() == 240);
    const auto outLa = run (la, makeInput());
    // The burst now comes out 240 samples later, and its first 0.5 ms are already within 1 dB of full level.
    REQUIRE (windowPeakDb (outLa, 4800 + 240, 24) > toDb (0.5) - 1.0);
}

TEST_CASE ("External sidechain and Listen")
{
    GateEngine e;
    e.prepare (fs, 1);
    GateParams p;
    p.thresholdDb = -30.0f;
    p.externalSidechain = true;
    p.rangeDb = -60.0f;
    p.releaseMs = 10.0f;
    e.setParams (p, true);

    // Audio is loud, sidechain is silent: the gate stays closed.
    const auto audio = sine (500.0, fs, 24000, 0.5);
    const std::vector<float> silentKey (24000, 0.0f);
    auto out = run (e, audio, &silentKey);
    REQUIRE (windowPeakDb (out, 20000, 2400) == Catch::Approx (toDb (0.5) - 60.0).margin (0.5));

    // Listen outputs the (filtered) key, not the audio.
    GateEngine l;
    l.prepare (fs, 1);
    p.listen = true;
    p.sidechainHpfHz = 100.0f;
    l.setParams (p, true);
    const auto key = sine (1000.0, fs, 24000, 0.25);
    out = run (l, audio, &key);
    REQUIRE (windowPeakDb (out, 20000, 2400) == Catch::Approx (toDb (0.25)).margin (0.2));
}
