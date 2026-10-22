// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/EchoReady/Source/EchoEngine.h"
#include "../shared/dsp/FractionalDelay.h"
#include "TestHelpers.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

using namespace rommuser::echoready;
using namespace testhelpers;

namespace
{
constexpr double fs = 48000.0;

struct Stereo { std::vector<float> l, r; };

Stereo run (EchoEngine& e, Stereo x, int block = 256)
{
    for (size_t pos = 0; pos < x.l.size(); pos += (size_t) block)
    {
        float* ch[] = { x.l.data() + pos, x.r.data() + pos };
        e.process (ch, 2, (int) std::min<size_t> ((size_t) block, x.l.size() - pos));
    }
    return x;
}

Stereo impulse (int n, int at = 100, float amp = 1.0f)
{
    Stereo s { std::vector<float> ((size_t) n, 0.0f), std::vector<float> ((size_t) n, 0.0f) };
    s.l[(size_t) at] = s.r[(size_t) at] = amp;
    return s;
}

size_t argmaxAbs (const std::vector<float>& v, size_t from, size_t to)
{
    size_t best = from;
    for (size_t i = from; i < std::min (to, v.size()); ++i)
        if (std::fabs (v[i]) > std::fabs (v[best])) best = i;
    return best;
}

EchoParams cleanParams()
{
    EchoParams p;
    p.sync = true;
    p.beats = 1.0;   // 1/4 at 120 bpm = 500 ms = 24000 samples
    p.bpm = 120.0;
    p.style = Style::digital;
    p.feedback = 0.0f;
    p.mix = 1.0f;
    p.duck = 0.0f;
    p.wow = 0.0f;
    return p;
}
} // namespace

TEST_CASE ("FractionalDelay: integer delays are exact and a half-sample delay of a slow sine is accurate")
{
    rommuser::dsp::FractionalDelay d;
    d.setMaxDelay (1000);
    for (int i = 0; i < 2000; ++i)
    {
        d.push ((float) i);
        if (i >= 100)
            REQUIRE (d.read (37.0f) == Catch::Approx ((float) (i - 37)));
    }
    rommuser::dsp::FractionalDelay s;
    s.setMaxDelay (1000);
    const auto x = sine (1000.0, fs, 4000);
    double err = 0.0;
    for (size_t i = 0; i < x.size(); ++i)
    {
        s.push (x[i]);
        if (i > 200)
        {
            const double expected = std::sin (2.0 * pi * 1000.0 * ((double) i - 10.5) / fs);
            err = std::max (err, std::fabs (s.read (10.5f) - expected));
        }
    }
    REQUIRE (err < 0.002);
}

TEST_CASE ("Tempo sync: the first repeat lands exactly beats x 60 / bpm x sampleRate samples later")
{
    for (double beats : { 1.0, 0.5, 0.75, 1.0 / 3.0 })
    {
        EchoEngine e;
        e.prepare (fs, 2);
        auto p = cleanParams();
        p.beats = beats;
        e.setParams (p);
        const auto out = run (e, impulse (72000));
        const double expected = 100.0 + beats * 60.0 / 120.0 * fs;
        const size_t peak = argmaxAbs (out.l, 200, 72000);
        INFO ("beats " << beats << " expected " << expected << " got " << peak);
        REQUIRE (std::fabs ((double) peak - expected) <= 0.5);
    }
}

TEST_CASE ("Free time in ms and mix 0 passes the dry signal bit-exact")
{
    EchoEngine e;
    e.prepare (fs, 2);
    auto p = cleanParams();
    p.sync = false;
    p.timeMs = 250.0f;
    e.setParams (p);
    const auto out = run (e, impulse (48000));
    REQUIRE (argmaxAbs (out.l, 200, 48000) == 100 + 12000);

    EchoEngine dry;
    dry.prepare (fs, 2);
    p.mix = 0.0f;
    p.feedback = 0.8f;
    dry.setParams (p);
    const auto in = Stereo { whiteNoise (8192, 0.5, 1), whiteNoise (8192, 0.5, 2) };
    const auto out2 = run (dry, in);
    REQUIRE (out2.l == in.l);
    REQUIRE (out2.r == in.r);
}

TEST_CASE ("Feedback scales each repeat by the feedback amount")
{
    EchoEngine e;
    e.prepare (fs, 2);
    auto p = cleanParams();
    p.feedback = 0.5f;
    e.setParams (p);
    const auto out = run (e, impulse (100000));
    const float first = out.l[100 + 24000], second = out.l[100 + 48000], third = out.l[100 + 72000];
    REQUIRE (first == Catch::Approx (1.0f).margin (0.01f));
    REQUIRE (second == Catch::Approx (0.5f).margin (0.01f));
    REQUIRE (third == Catch::Approx (0.25f).margin (0.01f));
}

TEST_CASE ("Ping-pong alternates the repeats between left and right")
{
    EchoEngine e;
    e.prepare (fs, 2);
    auto p = cleanParams();
    p.mode = Mode::pingPong;
    p.feedback = 0.5f;
    e.setParams (p);
    const auto out = run (e, impulse (100000));
    // First repeat: left only. Second: right only. Third: left again (x feedback).
    REQUIRE (std::fabs (out.l[100 + 24000]) > 0.9f);
    REQUIRE (std::fabs (out.r[100 + 24000]) < 1e-3f);
    REQUIRE (std::fabs (out.r[100 + 48000]) > 0.9f);
    REQUIRE (std::fabs (out.l[100 + 48000]) < 1e-3f);
    REQUIRE (std::fabs (out.l[100 + 72000]) == Catch::Approx (0.5f).margin (0.02f));
}

TEST_CASE ("Changing the time while a tone plays does not click (crossfade)")
{
    EchoEngine e;
    e.prepare (fs, 2);
    auto p = cleanParams();
    p.sync = false;
    p.timeMs = 100.0f;
    p.mix = 1.0f;
    e.setParams (p);

    const auto tone = sine (440.0, fs, 48000, 0.5);
    Stereo in { tone, tone };
    Stereo out { std::vector<float> (48000), std::vector<float> (48000) };
    for (size_t pos = 0; pos + 256 <= 48000; pos += 256)
    {
        if (pos == 24064) { p.timeMs = 333.0f; e.setParams (p); }  // jump mid-way
        std::vector<float> l (tone.begin() + (long) pos, tone.begin() + (long) pos + 256), r = l;
        float* ch[] = { l.data(), r.data() };
        e.process (ch, 2, 256);
        std::copy (l.begin(), l.end(), out.l.begin() + (long) pos);
    }
    // The wet signal is a 440 Hz tone (possibly a blend of two phases): its slope can never exceed the
    // slope of one full-amplitude 440 Hz sine.
    const float maxSlope = (float) (0.5 * 2.0 * pi * 440.0 / fs);
    float maxStep = 0.0f;
    for (size_t i = 24064 + 1; i < 47872; ++i)
        maxStep = std::max (maxStep, std::fabs (out.l[i] - out.l[i - 1]));
    REQUIRE (maxStep <= maxSlope * 1.1f);
}

TEST_CASE ("Changing the time in glide mode bends smoothly to the new time")
{
    EchoEngine e;
    e.prepare (fs, 2);
    auto p = cleanParams();
    p.sync = false;
    p.timeMs = 100.0f;
    p.timeChange = TimeChange::glide;
    e.setParams (p);
    float* dummy[] = { nullptr, nullptr };
    std::vector<float> l (256, 0.0f), r (256, 0.0f);
    dummy[0] = l.data(); dummy[1] = r.data();
    e.process (dummy, 2, 256);
    p.timeMs = 400.0f;
    e.setParams (p);
    float last = e.getCurrentDelaySamples();
    bool monotonic = true;
    for (int b = 0; b < 200; ++b)
    {
        e.process (dummy, 2, 256);
        const float now = e.getCurrentDelaySamples();
        if (now < last - 1e-3f) monotonic = false;
        last = now;
    }
    REQUIRE (monotonic);
    REQUIRE (last == Catch::Approx (400.0f / 1000.0f * fs).margin (1.0f));
}

TEST_CASE ("High cut darkens the repeats")
{
    auto measure = [] (double f, float highCut)
    {
        EchoEngine e;
        e.prepare (fs, 2);
        auto p = cleanParams();
        p.highCutHz = highCut;
        p.sync = false;
        p.timeMs = 200.0f;
        e.setParams (p);
        // 100 ms burst, repeat measured 200 ms later.
        const auto burst = sine (f, fs, 4800, 0.5);
        Stereo in { std::vector<float> (48000, 0.0f), std::vector<float> (48000, 0.0f) };
        std::copy (burst.begin(), burst.end(), in.l.begin() + 100);
        std::copy (burst.begin(), burst.end(), in.r.begin() + 100);
        const auto out = run (e, in);
        double pk = 0.0;
        for (size_t i = 100 + 9600 + 1000; i < 100 + 9600 + 4800; ++i) pk = std::max (pk, (double) std::fabs (out.l[i]));
        return toDb (pk);
    };
    REQUIRE (measure (100.0, 1000.0f) > toDb (0.5) - 1.0);
    REQUIRE (measure (10000.0, 1000.0f) < toDb (0.5) - 30.0);
}

TEST_CASE ("Ducking turns the repeats down while the input is loud")
{
    auto wetLevelDuring = [] (float duck)
    {
        EchoEngine e;
        e.prepare (fs, 2);
        auto p = cleanParams();
        p.sync = false;
        p.timeMs = 100.0f;
        p.feedback = 0.0f;
        p.duck = duck;
        p.mix = 1.0f;
        e.setParams (p);
        const auto tone = sine (1000.0, fs, 48000, 0.5);
        const auto out = run (e, Stereo { tone, tone });
        return toDb (rms (out.l.data() + 24000, 16384) * std::sqrt (2.0));
    };
    REQUIRE (wetLevelDuring (0.0f) == Catch::Approx (toDb (0.5)).margin (0.2));
    REQUIRE (wetLevelDuring (1.0f) < toDb (0.5) - 30.0);
}
