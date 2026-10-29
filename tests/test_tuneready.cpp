// Tune Ready DSP tests: YIN accuracy and octave safety, 30-cent correction to within 3 cents,
// transparent pass-through when nothing is shifted, scale snapping, MIDI priority, correction speed,
// formant preservation under a pitch shift, formant shift at constant pitch, reported latency.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/TuneReady/Source/TuneEngine.h"
#include "../shared/dsp/Biquad.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cmath>
#include <random>
#include <vector>

using namespace rommuser::tuneready;
using Catch::Approx;

namespace
{
constexpr double sr = 48000.0;
constexpr double pi = 3.14159265358979323846;

std::vector<float> sine (double hz, int n, float amp = 0.5f)
{
    std::vector<float> x ((size_t) n);
    for (int i = 0; i < n; ++i) x[(size_t) i] = amp * (float) std::sin (2.0 * pi * hz * i / sr);
    return x;
}

// Voice-like signal: pulse train at f0 through a resonator at `formantHz` (one "formant").
std::vector<float> voiceLike (double f0, double formantHz, int n)
{
    std::vector<float> x ((size_t) n, 0.0f);
    double next = 0.0;
    for (int i = 0; i < n; ++i)
    {
        if (i >= next) { x[(size_t) i] = 1.0f; next += sr / f0; }
    }
    rommuser::dsp::Biquad res;
    res.setCoeffs (rommuser::dsp::biquad::bandPass (formantHz, 6.0, sr));
    for (auto& v : x) v = res.process (v) * 0.5f;
    return x;
}

std::vector<float> noise (int n, unsigned seed = 1)
{
    std::mt19937 rng (seed);
    std::uniform_real_distribution<float> d (-0.5f, 0.5f);
    std::vector<float> x ((size_t) n);
    for (auto& v : x) v = d (rng);
    return x;
}

void run (TuneEngine& e, std::vector<float>& x, int block = 256)
{
    const int n = (int) x.size();
    for (int pos = 0; pos < n; pos += block)
    {
        float* io[1] = { x.data() + pos };
        e.process (io, 1, std::min (block, n - pos));
    }
}

double zeroCrossingHz (const std::vector<float>& x, int from, int to)
{
    double first = -1.0, last = -1.0;
    int count = 0;
    for (int i = from + 1; i < to; ++i)
        if (x[(size_t) i - 1] < 0.0f && x[(size_t) i] >= 0.0f)
        {
            const double t = (i - 1) - x[(size_t) i - 1] / (x[(size_t) i] - x[(size_t) i - 1]);
            if (first < 0.0) first = t; else { last = t; ++count; }
        }
    return count > 0 ? sr * count / (last - first) : 0.0;
}

double cents (double a, double b) { return 1200.0 * std::log2 (a / b); }

// Pitch of a segment with the engine's own detector (median of frames).
double detectHz (const std::vector<float>& x, int from, int to, float minHz = 60.0f)
{
    PitchDetector det;
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

// Spectral centroid (Hz) by DFT over [from, to).
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

TuneParams base()
{
    TuneParams p;
    p.mode = Mode::correct;
    p.scale = Scale::chromatic;
    p.speedMs = 0.0f;
    p.amount = 1.0f;
    p.humanizeCents = 0.0f;
    p.minHz = 60.0f;
    p.mix = 1.0f;
    return p;
}
} // namespace

TEST_CASE ("Tune Ready: YIN finds sines from 60 Hz to 1 kHz within 0.3 % and never picks an octave on a voice-like signal", "[tuneready]")
{
    for (double hz : { 60.0, 110.0, 220.0, 440.0, 1000.0 })
    {
        auto x = sine (hz, 24000);
        const double got = detectHz (x, 0, 24000);
        INFO ("hz " << hz << " got " << got);
        REQUIRE (std::fabs (got / hz - 1.0) < 0.003);
    }
    for (double f0 : { 90.0, 150.0, 250.0 })
    {
        auto x = voiceLike (f0, 1200.0, 24000);
        const double got = detectHz (x, 2000, 24000);
        INFO ("f0 " << f0 << " got " << got);
        REQUIRE (std::fabs (got / f0 - 1.0) < 0.01);
    }
    // Silence and noise are unvoiced.
    PitchDetector det;
    det.prepare (sr, 60.0f);
    std::vector<float> zeros ((size_t) det.getWindowSize(), 0.0f);
    REQUIRE_FALSE (det.detect (zeros.data()).voiced);
    auto nz = noise (det.getWindowSize() + 10);
    REQUIRE_FALSE (det.detect (nz.data()).voiced);
}

TEST_CASE ("Tune Ready: a sine 30 cents sharp lands within 3 cents of the note at full speed", "[tuneready]")
{
    for (double offset : { 30.0, -30.0 })
        for (double note : { 220.0, 329.63 })
        {
            TuneEngine e;
            e.prepare (sr, 256, 1);
            e.setParams (base());
            auto x = sine (note * std::pow (2.0, offset / 1200.0), 48000);
            run (e, x);
            const double got = zeroCrossingHz (x, 24000, 46000);
            INFO ("note " << note << " offset " << offset << " got " << got << " latency " << e.getLatencySamples());
            REQUIRE (std::fabs (cents (got, note)) < 3.0);
            REQUIRE (e.getMeters().voiced);
        }
}

TEST_CASE ("Tune Ready: an in-tune signal and unvoiced noise pass through unchanged (delayed by the latency)", "[tuneready]")
{
    for (int which : { 0, 1 })
    {
        TuneEngine e;
        e.prepare (sr, 256, 1);
        e.setParams (base());
        auto in = which == 0 ? sine (midiToHz (55), 24000) : noise (24000);   // G3 exactly in tune
        auto x = in;
        run (e, x);
        const int L = e.getLatencySamples();
        REQUIRE (L > 0);
        double err = 0.0, ref = 0.0;
        for (int i = L + 4000; i < 24000; ++i)
        {
            const double d = x[(size_t) i] - in[(size_t) (i - L)];
            err += d * d;
            ref += (double) in[(size_t) (i - L)] * in[(size_t) (i - L)];
        }
        INFO ("signal " << which);
        // Noise is unvoiced: exact overlap-add. The sine is re-tuned by the sub-cent detection error
        // (a fraction of a cent), which shows up as a slow phase drift against the input.
        REQUIRE (10.0 * std::log10 (err / ref) < (which == 0 ? -20.0 : -40.0));
        if (which == 0)
            REQUIRE (std::fabs (cents (zeroCrossingHz (x, L + 4000, 24000), midiToHz (55))) < 1.0);
    }
}

TEST_CASE ("Tune Ready: amount 0 is bit-transparent apart from the delay", "[tuneready]")
{
    TuneEngine e;
    e.prepare (sr, 256, 1);
    auto p = base();
    p.amount = 0.0f;
    e.setParams (p);
    auto in = sine (233.0, 24000);   // out of tune on purpose
    auto x = in;
    run (e, x);
    const int L = e.getLatencySamples();
    for (int i = L + 2000; i < 24000; ++i)
        REQUIRE (x[(size_t) i] == Approx (in[(size_t) (i - L)]).margin (1e-3));
}

TEST_CASE ("Tune Ready: scale snapping moves F#4 to the nearest C-major note and a held MIDI note wins", "[tuneready]")
{
    {
        TuneEngine e;
        e.prepare (sr, 256, 1);
        auto p = base();
        p.scale = Scale::major;
        p.key = 0;
        e.setParams (p);
        auto x = sine (midiToHz (66) * std::pow (2.0, 20.0 / 1200.0), 48000);   // F#4 + 20 cents -> G4
        run (e, x);
        const double got = zeroCrossingHz (x, 24000, 46000);
        REQUIRE (std::fabs (cents (got, midiToHz (67))) < 3.0);
    }
    {
        TuneEngine e;
        e.prepare (sr, 256, 1);
        e.setParams (base());
        e.noteOn (55);   // G3
        auto x = sine (midiToHz (57), 48000);   // A3 in tune, forced down to G3
        run (e, x);
        const double got = zeroCrossingHz (x, 24000, 46000);
        REQUIRE (std::fabs (cents (got, midiToHz (55))) < 3.0);
        e.noteOff (55);
    }
}

TEST_CASE ("Tune Ready: correction speed glides the pitch instead of jumping", "[tuneready]")
{
    TuneEngine e;
    e.prepare (sr, 256, 1);
    auto p = base();
    p.speedMs = 300.0f;
    e.setParams (p);
    auto x = sine (220.0 * std::pow (2.0, 40.0 / 1200.0), 96000);   // 40 cents sharp
    run (e, x);
    const int L = e.getLatencySamples();
    // Shortly after the first detection: still mostly sharp. After 1.5 s: in tune.
    const double early = cents (zeroCrossingHz (x, L + 2400, L + 7200), 220.0);
    const double late = cents (zeroCrossingHz (x, 72000, 94000), 220.0);
    INFO ("early " << early << " late " << late);
    REQUIRE (early > 10.0);
    REQUIRE (early < 40.5);
    REQUIRE (std::fabs (late) < 3.0);
}

TEST_CASE ("Tune Ready: a fixed +7 st shift keeps the formant where it was", "[tuneready]")
{
    TuneEngine e;
    e.prepare (sr, 256, 1);
    auto p = base();
    p.mode = Mode::fixed;
    p.fixedSemis = 7.0f;
    e.setParams (p);
    auto in = voiceLike (130.0, 1500.0, 72000);
    auto x = in;
    run (e, x);
    const double f0 = detectHz (x, 24000, 70000);
    INFO ("f0 " << f0);
    REQUIRE (std::fabs (cents (f0, 130.0 * std::pow (2.0, 7.0 / 12.0))) < 15.0);
    const double cIn = centroid (in, 24000, 24000 + 8192), cOut = centroid (x, 24000, 24000 + 8192);
    INFO ("centroid in " << cIn << " out " << cOut);
    REQUIRE (std::fabs (cOut / cIn - 1.0) < 0.2);      // a plain resample would move it by 50 %
}

TEST_CASE ("Tune Ready: a shift of a pitch with a fractional period stays on the target harmonics", "[tuneready]")
{
    // 220 Hz is 218.18 samples per period: grains placed on whole samples, or read with the
    // fractional offset the wrong way round, jitter the output pitch (sidebands -20 dB around each
    // harmonic, about -8 dB of noise in total before the fix).
    TuneEngine e;
    e.prepare (sr, 256, 1);
    auto p = base();
    p.mode = Mode::fixed;
    p.fixedSemis = 7.0f;
    e.setParams (p);
    std::vector<float> x (96000);
    for (size_t i = 0; i < x.size(); ++i)
    {
        double v = 0.0;
        for (int h = 1; h <= 20; ++h) v += std::sin (2.0 * pi * h * 220.0 * (double) i / sr) / h;
        x[i] = (float) (0.3 * v);
    }
    run (e, x);
    // Least-squares fit of the target's harmonics over a Hann-weighted segment; what is left is noise.
    const double f1 = 220.0 * std::pow (2.0, 7.0 / 12.0);
    const int from = 48000, n = 32768;
    std::vector<double> w ((size_t) n), resid ((size_t) n);
    double total = 0.0;
    for (int i = 0; i < n; ++i)
    {
        w[(size_t) i] = 0.5 - 0.5 * std::cos (2.0 * pi * i / n);
        resid[(size_t) i] = x[(size_t) (from + i)];
        total += w[(size_t) i] * resid[(size_t) i] * resid[(size_t) i];
    }
    for (int h = 1; h * f1 < 20000.0; ++h)
    {
        double sc = 0.0, ss = 0.0, cc = 0.0, sn = 0.0;
        for (int i = 0; i < n; ++i)
        {
            const double ph = 2.0 * pi * h * f1 * (from + i) / sr;
            sc += w[(size_t) i] * resid[(size_t) i] * std::cos (ph);
            sn += w[(size_t) i] * resid[(size_t) i] * std::sin (ph);
            cc += w[(size_t) i] * std::cos (ph) * std::cos (ph);
            ss += w[(size_t) i] * std::sin (ph) * std::sin (ph);
        }
        const double a = sc / cc, b = sn / ss;
        for (int i = 0; i < n; ++i)
        {
            const double ph = 2.0 * pi * h * f1 * (from + i) / sr;
            resid[(size_t) i] -= a * std::cos (ph) + b * std::sin (ph);
        }
    }
    double left = 0.0;
    for (int i = 0; i < n; ++i) left += w[(size_t) i] * resid[(size_t) i] * resid[(size_t) i];
    const double noiseDb = 10.0 * std::log10 (left / total);
    INFO ("noise " << noiseDb << " dB");
    REQUIRE (noiseDb < -30.0);
}

TEST_CASE ("Tune Ready: a +12 st formant shift doubles the formant and leaves the pitch alone", "[tuneready]")
{
    TuneEngine e;
    e.prepare (sr, 256, 1);
    auto p = base();
    p.mode = Mode::fixed;
    p.fixedSemis = 0.0f;
    p.formantSemis = 12.0f;
    e.setParams (p);
    auto in = voiceLike (120.0, 800.0, 72000);
    auto x = in;
    run (e, x);
    const double f0 = detectHz (x, 24000, 70000);
    REQUIRE (std::fabs (cents (f0, 120.0)) < 15.0);
    const double cIn = centroid (in, 24000, 24000 + 8192), cOut = centroid (x, 24000, 24000 + 8192);
    INFO ("centroid in " << cIn << " out " << cOut);
    REQUIRE (cOut / cIn > 1.6);
    REQUIRE (cOut / cIn < 2.4);
}

TEST_CASE ("Tune Ready: the reported latency matches the measured delay and shrinks with a higher minimum pitch", "[tuneready]")
{
    int latencies[2] = {};
    int k = 0;
    for (float minHz : { 60.0f, 110.0f })
    {
        TuneEngine e;
        e.prepare (sr, 256, 1);
        auto p = base();
        p.minHz = minHz;
        p.amount = 0.0f;
        e.setParams (p);
        auto in = noise (24000, 5);
        auto x = in;
        run (e, x);
        const int L = e.getLatencySamples();
        // Cross-correlation argmax must be exactly L.
        int bestLag = -1;
        double best = -1.0;
        for (int lag = 0; lag < 6000; ++lag)
        {
            double s = 0.0;
            for (int i = 8000; i < 16000; ++i) s += (double) x[(size_t) i] * in[(size_t) (i - lag)];
            if (s > best) { best = s; bestLag = lag; }
        }
        REQUIRE (bestLag == L);
        latencies[k++] = L;
    }
    REQUIRE (latencies[1] < latencies[0]);
    REQUIRE (latencies[1] < (int) (0.04 * sr));
}
