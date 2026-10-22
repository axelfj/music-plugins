// Vox Synth DSP tests: transparent with no voices, harmony intervals in scale degrees / semitones /
// MIDI with preserved formants, pan, robot mode, vocoder pitch and envelope following, silence.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/VoxSynth/Source/VoxEngine.h"
#include "../shared/dsp/Biquad.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cmath>
#include <random>
#include <vector>

using namespace rommuser::voxsynth;
using Catch::Approx;

namespace
{
constexpr double sr = 48000.0;
constexpr double pi = 3.14159265358979323846;

struct Stereo { std::vector<float> l, r; };

Stereo sine (double hz, int n, float amp = 0.5f)
{
    Stereo s { std::vector<float> ((size_t) n), std::vector<float> ((size_t) n) };
    for (int i = 0; i < n; ++i) s.l[(size_t) i] = s.r[(size_t) i] = amp * (float) std::sin (2.0 * pi * hz * i / sr);
    return s;
}

Stereo voiceLike (double f0, double formantHz, int n)
{
    std::vector<float> x ((size_t) n, 0.0f);
    double next = 0.0;
    for (int i = 0; i < n; ++i)
        if (i >= next) { x[(size_t) i] = 1.0f; next += sr / f0; }
    rommuser::dsp::Biquad res;
    res.setCoeffs (rommuser::dsp::biquad::bandPass (formantHz, 6.0, sr));
    float peak = 0.0f;
    for (auto& v : x) { v = res.process (v); peak = std::max (peak, std::fabs (v)); }
    for (auto& v : x) v *= 0.5f / std::max (peak, 1e-6f);   // normalise to -6 dBFS peak
    return { x, x };
}

Stereo noise (int n, unsigned seed = 1)
{
    std::mt19937 rng (seed);
    std::uniform_real_distribution<float> d (-0.5f, 0.5f);
    Stereo s { std::vector<float> ((size_t) n), std::vector<float> ((size_t) n) };
    for (int i = 0; i < n; ++i) { s.l[(size_t) i] = d (rng); s.r[(size_t) i] = d (rng); }
    return s;
}

void run (VoxEngine& e, Stereo& s, const Stereo* sidechain = nullptr, int block = 256)
{
    const int n = (int) s.l.size();
    for (int pos = 0; pos < n; pos += block)
    {
        const int len = std::min (block, n - pos);
        float* io[2] = { s.l.data() + pos, s.r.data() + pos };
        if (sidechain != nullptr)
        {
            const float* sc[2] = { sidechain->l.data() + pos, sidechain->r.data() + pos };
            e.process (io, 2, len, sc, 2);
        }
        else
            e.process (io, 2, len);
    }
}

double detectHz (const std::vector<float>& x, int from, int to, float minHz = 60.0f)
{
    rommuser::dsp::psola::PitchDetector det;
    det.prepare (sr, minHz);
    const int w = det.getWindowSize();
    std::vector<double> est;
    for (int pos = from; pos + w <= to; pos += w / 2)
    {
        const auto e = det.detect (x.data() + pos);
        if (e.voiced) est.push_back (e.hz);
    }
    if (est.empty()) return 0.0;
    std::sort (est.begin(), est.end());
    return est[est.size() / 2];
}

double cents (double a, double b) { return 1200.0 * std::log2 (a / b); }

double centroid (const std::vector<float>& x, int from, int to)
{
    const int n = to - from;
    double num = 0.0, den = 0.0;
    for (double f = 100.0; f < 6000.0; f += 25.0)
    {
        double re = 0.0, im = 0.0;
        for (int i = 0; i < n; ++i)
        {
            const double ph = 2.0 * pi * f * i / sr;
            re += x[(size_t) (from + i)] * std::cos (ph);
            im -= x[(size_t) (from + i)] * std::sin (ph);
        }
        const double p = re * re + im * im;
        num += f * p;
        den += p;
    }
    return den > 0.0 ? num / den : 0.0;
}

double rms (const std::vector<float>& x, int from, int to)
{
    double s = 0.0;
    for (int i = from; i < to; ++i) s += (double) x[(size_t) i] * x[(size_t) i];
    return std::sqrt (s / std::max (1, to - from));
}

// Energy of a tone at `hz` (projection over whole cycles), in dB.
double toneDb (const std::vector<float>& x, double hz, int from, int to)
{
    const int cycles = (int) std::floor ((to - from) * hz / sr);
    const int len = (int) std::floor (cycles * sr / hz);
    double s = 0.0, c = 0.0;
    for (int i = 0; i < len; ++i)
    {
        const double ph = 2.0 * pi * hz * (from + i) / sr;
        s += x[(size_t) (from + i)] * std::sin (ph);
        c += x[(size_t) (from + i)] * std::cos (ph);
    }
    return 20.0 * std::log10 (std::max (2.0 * std::sqrt (s * s + c * c) / len, 1e-9));
}

VoxParams base()
{
    VoxParams p;
    p.mode = Mode::harmony;
    p.minHz = 60.0f;
    p.key = 0;
    p.scale = Scale::major;
    p.intervalMode = IntervalMode::scale;
    p.glideMs = 0.0f;
    p.dryDb = 0.0f;
    p.mix = 1.0f;
    return p;
}
} // namespace

TEST_CASE ("Vox Synth: no voices and mix 0 give back the input, delayed by the latency, bit-exact", "[voxsynth]")
{
    VoxEngine e;
    e.prepare (sr, 256, 2);
    auto p = base();
    p.mix = 0.0f;
    e.setParams (p);
    auto in = noise (24000);
    auto s = in;
    run (e, s);
    const int L = e.getLatencySamples();
    REQUIRE (L > 0);
    for (int i = L; i < 24000; ++i)
    {
        REQUIRE (s.l[(size_t) i] == in.l[(size_t) (i - L)]);
        REQUIRE (s.r[(size_t) i] == in.r[(size_t) (i - L)]);
    }
    // Harmony mode with voices disabled and mix 1 is also the input (no voices contribute).
    p.mix = 1.0f;
    e.setParams (p);
    e.reset();
    s = in;
    run (e, s);
    for (int i = L; i < 24000; ++i)
        REQUIRE (s.l[(size_t) i] == in.l[(size_t) (i - L)]);
}

TEST_CASE ("Vox Synth: a scale-degree harmony on E4 in C major lands on G4 (+2 degrees) and C4 (-2 degrees)", "[voxsynth]")
{
    for (int degrees : { 2, -2 })
    {
        VoxEngine e;
        e.prepare (sr, 256, 2);
        auto p = base();
        p.dryDb = -100.0f;   // harmony only
        p.voices[0].enabled = true;
        p.voices[0].degrees = degrees;
        p.voices[0].levelDb = 0.0f;
        e.setParams (p);
        auto s = voiceLike (midiToHz (64), 1200.0, 72000);   // E4
        run (e, s);
        const int expected = degrees > 0 ? 67 : 60;   // G4 or C4
        const double got = detectHz (s.l, 30000, 70000);
        INFO ("degrees " << degrees << " got " << got);
        REQUIRE (std::fabs (cents (got, midiToHz (expected))) < 10.0);
    }
}

TEST_CASE ("Vox Synth: a +12 st harmony keeps the voice's formant (centroid) while doubling the pitch", "[voxsynth]")
{
    VoxEngine e;
    e.prepare (sr, 256, 2);
    auto p = base();
    p.intervalMode = IntervalMode::semitones;
    p.dryDb = -100.0f;
    p.voices[0].enabled = true;
    p.voices[0].semitones = 12.0f;
    p.voices[0].levelDb = 0.0f;
    e.setParams (p);
    auto in = voiceLike (120.0, 1500.0, 72000);
    auto s = in;
    run (e, s);
    REQUIRE (std::fabs (cents (detectHz (s.l, 30000, 70000), 240.0)) < 10.0);
    const double cIn = centroid (in.l, 30000, 30000 + 8192), cOut = centroid (s.l, 30000, 30000 + 8192);
    INFO ("centroid in " << cIn << " out " << cOut);
    REQUIRE (std::fabs (cOut / cIn - 1.0) < 0.2);
}

TEST_CASE ("Vox Synth: MIDI interval mode puts voice k on the k-th held note, and pan sends a voice to one side", "[voxsynth]")
{
    VoxEngine e;
    e.prepare (sr, 256, 2);
    auto p = base();
    p.intervalMode = IntervalMode::midi;
    p.dryDb = -100.0f;
    p.voices[0].enabled = true;
    p.voices[0].levelDb = 0.0f;
    p.voices[0].pan = -1.0f;   // hard left
    p.voices[1].enabled = true;
    p.voices[1].levelDb = 0.0f;
    p.voices[1].pan = 1.0f;    // hard right
    e.setParams (p);
    e.noteOn (67);   // G4 -> voice 0 (sorted ascending)
    e.noteOn (72);   // C5 -> voice 1
    auto s = voiceLike (midiToHz (64), 1200.0, 72000);   // E4 in
    run (e, s);
    REQUIRE (std::fabs (cents (detectHz (s.l, 30000, 70000), midiToHz (67))) < 10.0);
    REQUIRE (std::fabs (cents (detectHz (s.r, 30000, 70000), midiToHz (72))) < 10.0);
}

TEST_CASE ("Vox Synth: robot mode forces any input pitch to the chosen note within 5 cents", "[voxsynth]")
{
    for (double inHz : { 150.0, 233.0 })
    {
        VoxEngine e;
        e.prepare (sr, 256, 2);
        auto p = base();
        p.mode = Mode::robot;
        p.robotNote = 48;   // C3
        p.dryDb = -100.0f;
        e.setParams (p);
        auto s = sine (inHz, 72000);
        run (e, s);
        const double got = detectHz (s.l, 30000, 70000);
        INFO ("in " << inHz << " got " << got);
        REQUIRE (std::fabs (cents (got, midiToHz (48))) < 5.0);
    }
}

TEST_CASE ("Vox Synth: the vocoder output has the carrier's pitch and the voice's spectral envelope, and is silent without a voice", "[voxsynth]")
{
    VoxEngine e;
    e.prepare (sr, 256, 2);
    auto p = base();
    p.mode = Mode::vocoder;
    p.carrier = Carrier::saw;
    p.carrierFollowsPitch = false;
    p.carrierNote = 45;   // A2 = 110 Hz
    p.sibilance = 0.0f;
    p.dryDb = -100.0f;
    e.setParams (p);
    auto in = voiceLike (180.0, 1500.0, 72000);
    auto s = in;
    run (e, s);
    const double f0 = detectHz (s.l, 30000, 70000);
    INFO ("f0 " << f0);
    REQUIRE (std::fabs (cents (f0, 110.0)) < 10.0);
    // Envelope: a 1.5 kHz formant on the modulator gives a 1.5 kHz hump on the output. Compare the
    // output level at harmonics near 1.5 kHz vs near 400 Hz (a bare saw would be louder at 400 Hz).
    const double near = toneDb (s.l, 110.0 * 14.0, 30000, 70000);   // 1540 Hz
    const double far = toneDb (s.l, 110.0 * 4.0, 30000, 70000);     // 440 Hz
    INFO ("1540 Hz " << near << " dB, 440 Hz " << far << " dB");
    REQUIRE (near > far + 6.0);

    // Silence in: silence out (the carrier is gated by the envelopes).
    Stereo z { std::vector<float> (48000, 0.0f), std::vector<float> (48000, 0.0f) };
    run (e, z);
    REQUIRE (rms (z.l, 24000, 48000) < 1e-4);
}

TEST_CASE ("Vox Synth: the external carrier is shaped by the voice and follows the sidechain's pitch", "[voxsynth]")
{
    VoxEngine e;
    e.prepare (sr, 256, 2);
    auto p = base();
    p.mode = Mode::vocoder;
    p.carrier = Carrier::external;
    p.sibilance = 0.0f;
    p.dryDb = -100.0f;
    e.setParams (p);
    auto s = voiceLike (200.0, 1000.0, 72000);
    auto sc = voiceLike (98.0, 3000.0, 72000);   // G2 synth as carrier
    run (e, s, &sc);
    REQUIRE (std::fabs (cents (detectHz (s.l, 30000, 70000), 98.0)) < 10.0);
    REQUIRE (rms (s.l, 30000, 70000) > 1e-3);
}
