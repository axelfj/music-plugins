// dsp_lab: sound-quality measurements of every audio engine, headless.
//
// Where dsp_bench answers "how much CPU", this answers "how clean": aliasing, harmonic distortion,
// transparency at neutral settings, filter accuracy near Nyquist, delay interpolation loss, pitch
// shifter purity and reverb decay / density / colour. Every number is measured on the real engine
// code at 48 kHz, so it can be compared between versions and against the results in docs/QUALITY.md.
//
//   cmake --build build --target dsp_lab && ./build/tools/dsp-lab/dsp_lab [filter]
//
// Conventions: spectra use a 4-term Blackman-Harris window (-92 dB side lobes) over 65536 samples
// taken after the engine settled; "alias" is all energy that is not the input tone, one of its
// harmonics or DC, relative to the tone (dBc). Lower is better everywhere unless said otherwise.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../../plugins/BandPush/Source/PushEngine.h"
#include "../../plugins/ClipReady/Source/ClipEngine.h"
#include "../../plugins/EchoReady/Source/EchoEngine.h"
#include "../../plugins/FilterReady/Source/FilterEngine.h"
#include "../../plugins/GateReady/Source/GateEngine.h"
#include "../../plugins/KickReady/Source/KickEngine.h"
#include "../../plugins/LowMono/Source/MonoEngine.h"
#include "../../plugins/PeakReady/Source/PeakEngine.h"
#include "../../plugins/PhaseReady/Source/PhaseEngine.h"
#include "../../plugins/PressReady/Source/CompEngine.h"
#include "../../plugins/PunchShape/Source/PunchEngine.h"
#include "../../plugins/ResTame/Source/ResTameEngine.h"
#include "../../plugins/ShapeEQ/Source/EqEngine.h"
#include "../../plugins/ShimmerReady/Source/ShimmerEngine.h"
#include "../../plugins/SpaceReady/Source/SpaceEngine.h"
#include "../../plugins/TuneReady/Source/TuneEngine.h"
#include "../../plugins/VoxSynth/Source/VoxEngine.h"
#include "../../plugins/WarmDrive/Source/DriveEngine.h"
#include "../../plugins/WaveReady/Source/WaveEngine.h"
#include "../../plugins/WideReady/Source/WideEngine.h"

#include <cmath>
#include <complex>
#include <cstdio>
#include <functional>
#include <random>
#include <string>
#include <vector>

#if defined(__SSE__) || defined(_M_X64)
 #include <xmmintrin.h>
 #define DSP_LAB_FTZ() _mm_setcsr (_mm_getcsr() | 0x8040)
#else
 #define DSP_LAB_FTZ() ((void) 0)
#endif

namespace
{
constexpr double sr = 48000.0;
constexpr int block = 512;
constexpr int fftOrder = 16;
constexpr int N = 1 << fftOrder;
constexpr double binHz = sr / N;
constexpr double pi = 3.14159265358979323846;

using Vec = std::vector<float>;
using Proc = std::function<void (float* const*, int)>; // processes one stereo block in place

std::string filterArg;
bool wanted (const std::string& name) { return filterArg.empty() || name.find (filterArg) != std::string::npos; }

double db (double powerRatio) { return 10.0 * std::log10 (std::max (powerRatio, 1e-30)); }

// Runs `in` through the engine as L = R, block by block; returns L and R.
void run (const Proc& p, const Vec& in, Vec& outL, Vec& outR)
{
    outL = in; outR = in;
    for (size_t pos = 0; pos < in.size(); pos += block)
    {
        const int n = (int) std::min ((size_t) block, in.size() - pos);
        float* io[2] { outL.data() + pos, outR.data() + pos };
        p (io, n);
    }
}

Vec run (const Proc& p, const Vec& in)
{
    Vec l, r;
    run (p, in, l, r);
    return l;
}

Vec sine (double hz, double amp, size_t len)
{
    Vec v (len);
    for (size_t i = 0; i < len; ++i) v[i] = (float) (amp * std::sin (2.0 * pi * hz * (double) i / sr));
    return v;
}

// Power spectrum (one side, bins 0..N/2) of x[start .. start + N), Blackman-Harris windowed,
// normalised so a full-scale sine reads 1.0 summed over its main lobe.
std::vector<double> spectrum (const Vec& x, size_t start)
{
    static rommuser::dsp::FFT fft;
    static bool ready = false;
    if (! ready) { fft.prepare (fftOrder); ready = true; }
    std::vector<std::complex<float>> buf ((size_t) N);
    double wsum2 = 0.0;
    for (int i = 0; i < N; ++i)
    {
        const double t = 2.0 * pi * i / N;
        const double w = 0.35875 - 0.48829 * std::cos (t) + 0.14128 * std::cos (2 * t) - 0.01168 * std::cos (3 * t);
        wsum2 += w * w;
        buf[(size_t) i] = { (float) (w * x[start + (size_t) i]), 0.0f };
    }
    fft.forward (buf.data());
    std::vector<double> p ((size_t) N / 2 + 1);
    for (size_t k = 0; k < p.size(); ++k)
        p[k] = 2.0 * std::norm (std::complex<double> (buf[k])) / (wsum2 * N);
    return p;
}

double bandPower (const std::vector<double>& p, double loHz, double hiHz)
{
    const int a = std::max (0, (int) std::floor (loHz / binHz)), b = std::min ((int) p.size() - 1, (int) std::ceil (hiHz / binHz));
    double s = 0.0;
    for (int k = a; k <= b; ++k) s += p[(size_t) k];
    return s;
}

constexpr double lobeHz = 8 * binHz; // main lobe half-width we attribute to a tone

struct ToneReport { double fund = 0, harmonics = 0, other = 0; };

// Splits the spectrum into: the tone at f0, its harmonics below Nyquist, and everything else
// above 20 Hz (aliases, noise, intermodulation). `extraTones` are treated like f0 (IMD tests).
ToneReport analyse (const std::vector<double>& p, double f0, double maskHz = lobeHz)
{
    ToneReport r;
    std::vector<char> used (p.size(), 0);
    auto take = [&] (double f, double& acc)
    {
        const int a = std::max (0, (int) std::floor ((f - maskHz) / binHz)), b = std::min ((int) p.size() - 1, (int) std::ceil ((f + maskHz) / binHz));
        for (int k = a; k <= b; ++k) if (! used[(size_t) k]) { acc += p[(size_t) k]; used[(size_t) k] = 1; }
    };
    take (f0, r.fund);
    for (int m = 2; m * f0 < sr / 2 - maskHz; ++m) take (m * f0, r.harmonics);
    for (size_t k = (size_t) std::ceil (20.0 / binHz); k < p.size(); ++k) if (! used[k]) r.other += p[k];
    return r;
}

// Steady-state analysis of a sine through the engine.
ToneReport toneTest (const Proc& p, double hz, double amp, double settleSeconds = 0.5, double maskHz = lobeHz)
{
    const size_t settle = (size_t) (settleSeconds * sr);
    const auto out = run (p, sine (hz, amp, settle + N));
    return analyse (spectrum (out, settle), hz, maskHz);
}

double aliasDbc (const ToneReport& r) { return db (r.other / r.fund); }
double thdDb (const ToneReport& r) { return db (r.harmonics / r.fund); }

// Gain at one frequency (dB), from the main lobe of a steady sine.
double gainAt (const Proc& p, double hz, double amp = 0.1)
{
    const auto r = toneTest (p, hz, amp, 0.3);
    return db (r.fund / (amp * amp * 0.5 * 1.0));
}

// Residual of out against the input delayed by `latency`, relative to the input (dB).
double nullDb (const Proc& p, int latency, double seconds = 2.0)
{
    std::mt19937 rng (3);
    std::normal_distribution<float> g (0.0f, 0.1f);
    Vec in ((size_t) (seconds * sr));
    for (auto& v : in) v = g (rng);
    const auto out = run (p, in);
    double e = 0.0, s = 0.0;
    for (size_t i = (size_t) sr / 2; i < in.size(); ++i)
    {
        const double d = out[i] - in[i - (size_t) latency];
        e += d * d; s += (double) in[i] * in[i];
    }
    return db (e / s);
}

void print (const char* group, const std::string& name, const char* metric, double value, const char* unit, const char* note = "")
{
    std::printf ("%-12s %-36s %-26s %9.1f %-4s %s\n", group, name.c_str(), metric, value, unit, note);
}

// --------------------------------------------------------------------------- engines

template <typename E> Proc procOf (E& e) { return [&e] (float* const* io, int n) { e.process (io, 2, n); }; }

void saturation()
{
    using namespace rommuser;
    // "strict" is the default guaranteed sample-peak ceiling (a clamp after downsampling).
    struct C { std::string name; int os; bool hq; float soft; bool strict; };
    for (const C& c : { C { "Clipper-R hard 4x strict", 4, false, 0.0f, true }, C { "Clipper-R hard 2x HQ strict", 2, true, 0.0f, true }, C { "Clipper-R hard 4x HQ strict", 4, true, 0.0f, true },
                        C { "Clipper-R hard 8x strict", 8, false, 0.0f, true }, C { "Clipper-R soft 4x strict", 4, false, 0.35f, true },
                        C { "Clipper-R hard 2x", 2, false, 0.0f, false }, C { "Clipper-R hard 4x", 4, false, 0.0f, false },
                        C { "Clipper-R hard 2x HQ", 2, true, 0.0f, false }, C { "Clipper-R hard 4x HQ", 4, true, 0.0f, false },
                        C { "Clipper-R soft 4x", 4, false, 0.35f, false } })
    {
        if (! wanted (c.name)) continue;
        clipready::ClipEngine e; e.prepare (sr, block, 2);
        clipready::ClipParams p; p.inputDb = 12.0f; p.oversampling = c.os; p.hq = c.hq; p.softness = c.soft; p.strictCeiling = c.strict;
        e.setParams (p, true);
        print ("saturation", c.name, "alias, 7 kHz +12 dB", aliasDbc (toneTest (procOf (e), 6997.0, 0.5)), "dBc");
    }
    using S = warmdrive::Saturation;
    struct D { std::string name; S s; int os; bool hq; };
    for (const D& d : { D { "Saturator-R tape 2x", S::tape, 2, false }, D { "Saturator-R tape 2x HQ", S::tape, 2, true },
                        D { "Saturator-R tube 2x", S::tube, 2, false }, D { "Saturator-R rectify 4x", S::rectify, 4, false },
                        D { "Saturator-R transistor 1x", S::transistor, 1, false } })
    {
        if (! wanted (d.name)) continue;
        warmdrive::DriveEngine e; e.prepare (sr, block, 2);
        warmdrive::DriveParams p; p.bands[0].type = d.s; p.bands[0].driveDb = 18.0f; p.oversampling = d.os; p.hq = d.hq;
        e.setParams (p, true);
        print ("saturation", d.name, "alias, 7 kHz drive 18", aliasDbc (toneTest (procOf (e), 6997.0, 0.5)), "dBc");
    }
}

void dynamics()
{
    using namespace rommuser;
    for (auto det : { pressready::Detector::peak, pressready::Detector::rms })
    {
        const std::string name = std::string ("Compressor-R ") + (det == pressready::Detector::peak ? "peak" : "rms") + " 1/30 ms";
        if (! wanted (name)) continue;
        pressready::CompEngine e; e.prepare (sr, block, 2);
        pressready::CompParams p; p.thresholdDb = -24.0f; p.ratio = 4.0f; p.attackMs = 1.0f; p.releaseMs = 30.0f; p.detector = det;
        e.setParams (p, true);
        Proc pr = [&] (float* const* io, int n) { e.process (io, nullptr, 2, n); };
        print ("dynamics", name, "THD, 50 Hz -6 dBFS", thdDb (toneTest (pr, 50.0, 0.5)), "dB", "peak vs rms detector");
    }
    for (auto ch : { pressready::Character::punch, pressready::Character::glue })
    {
        const std::string name = std::string ("Compressor-R ") + (ch == pressready::Character::punch ? "punch" : "glue");
        if (! wanted (name)) continue;
        pressready::CompEngine e; e.prepare (sr, block, 2);
        pressready::CompParams p; p.thresholdDb = -24.0f; p.character = ch;
        e.setParams (p, true);
        Proc pr = [&] (float* const* io, int n) { e.process (io, nullptr, 2, n); };
        print ("dynamics", name, "alias, 7 kHz -6 dBFS", aliasDbc (toneTest (pr, 6997.0, 0.5)), "dBc", "saturation at 1x");
    }
    if (wanted ("Multiband-R"))
    {
        bandpush::PushEngine e; e.prepare (sr, 2);
        bandpush::PushParams p; p.depth = 1.0f;
        e.setParams (p, true);
        print ("dynamics", "Multiband-R depth 100 %", "THD, 50 Hz -12 dBFS", thdDb (toneTest (procOf (e), 50.0, 0.25)), "dB");
        print ("dynamics", "Multiband-R depth 100 %", "alias, 7 kHz -12 dBFS", aliasDbc (toneTest (procOf (e), 6997.0, 0.25)), "dBc");
    }
    for (auto st : { peakready::Style::transparent, peakready::Style::punchy, peakready::Style::aggressive })
    {
        const char* sn[] = { "transparent", "punchy", "aggressive" };
        const std::string name = std::string ("Limiter-R ") + sn[(int) st];
        if (! wanted (name)) continue;
        peakready::PeakEngine e; e.prepare (sr, block, 2);
        peakready::PeakParams p; p.inputDb = 6.0f; p.style = st;
        e.setParams (p, true);
        print ("dynamics", name, "THD, 100 Hz +6 dB push", thdDb (toneTest (procOf (e), 100.0, 0.89)), "dB", "bass under limiting");
        // True-peak overshoot of noise pushed +12 dB, measured with 8x band-limited interpolation.
        peakready::PeakEngine f; f.prepare (sr, block, 2);
        p.inputDb = 12.0f; f.setParams (p, true);
        std::mt19937 rng (5); std::normal_distribution<float> g (0.0f, 0.25f);
        Vec in ((size_t) (4 * sr)); for (auto& v : in) v = g (rng);
        const auto out = run (procOf (f), in);
        const int taps = 32;
        double tp = 0.0;
        for (size_t i = (size_t) sr; i + taps < out.size(); ++i)
            for (int ph = 0; ph < 8; ++ph)
            {
                const double frac = ph / 8.0;
                double acc = 0.0;
                for (int k = -taps / 2 + 1; k <= taps / 2; ++k)
                {
                    const double t = k - frac;
                    const double sinc = std::fabs (t) < 1e-9 ? 1.0 : std::sin (pi * t) / (pi * t);
                    const double w = 0.5 + 0.5 * std::cos (pi * t / (taps / 2 + 1));
                    acc += out[i + (size_t) k] * sinc * w;
                }
                tp = std::max (tp, std::fabs (acc));
            }
        print ("dynamics", name, "true peak vs -1 dB ceiling", 20.0 * std::log10 (tp) + 1.0, "dB", "> 0 = overshoot");
    }
}

void transparency()
{
    using namespace rommuser;
    if (wanted ("EQ-R"))
    {
        shapeeq::EqEngine e; e.prepare (sr, block, 2);
        shapeeq::EqParams p;
        for (int b = 0; b < 8; ++b) { p.bands[(size_t) b].enabled = true; p.bands[(size_t) b].freq = 60.0 * std::pow (2.0, b); p.bands[(size_t) b].gainDb = 0.0; }
        e.setParams (p, true);
        print ("neutral", "EQ-R 8 bells at 0 dB", "null vs input", nullDb (procOf (e), 0), "dB");
    }
    if (wanted ("Multiband-R"))
    {
        bandpush::PushEngine e; e.prepare (sr, 2);
        bandpush::PushParams p; p.depth = 0.0f; e.setParams (p, true);
        // The LR4 sum is an all-pass: compare magnitudes, not samples.
        print ("neutral", "Multiband-R depth 0", "gain at 120 Hz (xover)", gainAt (procOf (e), 120.0), "dB");
        print ("neutral", "Multiband-R depth 0", "gain at 2.5 kHz (xover)", gainAt (procOf (e), 2500.0), "dB");
    }
    if (wanted ("Transient-R"))
    {
        punchshape::PunchEngine e; e.prepare (sr, 2);
        e.setParams ({}, true);
        print ("neutral", "Transient-R at 0 / 0", "null vs input", nullDb (procOf (e), 0), "dB");
    }
    if (wanted ("Resonance-R"))
    {
        restame::TameEngine e; e.prepare (sr, 2);
        restame::TameParams p; p.depthDb = 0.0f; e.setParams (p);
        Proc pr = [&] (float* const* io, int n) { e.process (io, nullptr, 2, n); };
        print ("neutral", "Resonance-R depth 0", "null vs input", nullDb (pr, e.getLatencySamples()), "dB", "STFT reconstruction");
    }
    if (wanted ("Tune-R"))
    {
        tuneready::TuneEngine e; e.prepare (sr, block, 2);
        tuneready::TuneParams p; p.mode = tuneready::Mode::fixed; p.fixedSemis = 0.0f; e.setParams (p, true);
        const auto r = toneTest (procOf (e), 220.0, 0.5, 1.0);
        print ("neutral", "Tune-R fixed 0 st", "residual, 220 Hz sine", db ((r.other + r.harmonics) / r.fund), "dBc");
    }
}

// Analog prototypes for the EQ accuracy check (s normalised to the band frequency).
double analogDb (rommuser::dsp::FilterType t, double f, double f0, double q, double g)
{
    const std::complex<double> s (0.0, f / f0);
    const double A = std::pow (10.0, g / 40.0);
    std::complex<double> h;
    using T = rommuser::dsp::FilterType;
    switch (t)
    {
        case T::bell:      h = (s * s + s * (A / q) + 1.0) / (s * s + s / (A * q) + 1.0); break;
        case T::highShelf: h = A * (A * s * s + std::sqrt (A) / q * s + 1.0) / (s * s + std::sqrt (A) / q * s + A); break;
        case T::lowShelf:  h = A * (s * s + std::sqrt (A) / q * s + A) / (A * s * s + std::sqrt (A) / q * s + 1.0); break;
        case T::highCut:   h = 1.0 / (s * s + s / q + 1.0); break;
        default:           h = 1.0; break;
    }
    return 20.0 * std::log10 (std::abs (h));
}

void filters()
{
    using namespace rommuser;
    using T = dsp::FilterType;
    if (wanted ("EQ-R"))
    {
        struct B { const char* name; T type; double f0, q, g, at; };
        for (const B& b : { B { "EQ-R bell 15k +6 Q2", T::bell, 15000, 2.0, 6.0, 18000 },
                            B { "EQ-R high shelf 10k +6", T::highShelf, 10000, 0.707, 6.0, 20000 },
                            B { "EQ-R high shelf 4k +6", T::highShelf, 4000, 0.707, 6.0, 16000 },
                            B { "EQ-R low shelf 80 +6", T::lowShelf, 80, 0.707, 6.0, 40 },
                            B { "EQ-R high cut 12k Q0.7", T::highCut, 12000, 0.707, 0.0, 18000 } })
        {
            shapeeq::EqEngine e; e.prepare (sr, block, 2);
            shapeeq::EqParams p; p.bands[0] = { true, b.type, b.f0, b.g, b.q, 12, {} };
            e.setParams (p, true);
            const double meas = gainAt (procOf (e), b.at);
            char m[64]; std::snprintf (m, sizeof m, "error vs analog at %.0f Hz", b.at);
            print ("filter", b.name, m, meas - analogDb (b.type, b.at, b.f0, b.q, b.g), "dB");
        }
    }
    if (wanted ("Filter-R"))
    {
        for (auto slope : { filterready::Slope::db12, filterready::Slope::db24 })
        {
            filterready::FilterEngine e; e.prepare (sr, block, 2);
            filterready::FilterParams p; p.a.slope = slope; p.a.resonance = 0.0f; p.a.cutoffHz = 1000.0f;
            e.setParams (p, true);
            Proc pr = [&] (float* const* io, int n) { e.process (io, 2, n); };
            const std::string name = std::string ("Filter-R LP") + (slope == filterready::Slope::db12 ? "12" : "24") + " res 0";
            print ("filter", name, "gain at cutoff (1 kHz)", gainAt (pr, 1000.0), "dB", "Butterworth = -3");
        }
        {
            filterready::FilterEngine e; e.prepare (sr, block, 2);
            filterready::FilterParams p; p.a.slope = filterready::Slope::db12; p.a.resonance = 0.0f; p.a.cutoffHz = 16000.0f;
            e.setParams (p, true);
            Proc pr = [&] (float* const* io, int n) { e.process (io, 2, n); };
            print ("filter", "Filter-R LP12 at 16 kHz", "gain at 19 kHz vs analog", gainAt (pr, 19000.0) - analogDb (T::highCut, 19000, 16000, 0.5, 0), "dB");
        }
        {
            filterready::FilterEngine e; e.prepare (sr, block, 2);
            filterready::FilterParams p; p.driveDb = 24.0f; p.character = filterready::Character::analog; p.a.cutoffHz = 20000.0f;
            e.setParams (p, true);
            Proc pr = [&] (float* const* io, int n) { e.process (io, 2, n); };
            print ("filter", "Filter-R drive 24 analog", "alias, 7 kHz -6 dBFS", aliasDbc (toneTest (pr, 6997.0, 0.5)), "dBc", "2x oversampling");
        }
    }
}

void delays()
{
    using namespace rommuser;
    if (wanted ("Delay-R"))
    {
        // Echo at 18000.5 samples (half-sample fraction = worst case of the interpolator).
        echoready::EchoEngine e; e.prepare (sr, 2);
        echoready::EchoParams p; p.sync = false; p.timeMs = (float) (18000.5 / sr * 1000.0); p.feedback = 0.0f; p.mix = 1.0f;
        e.setParams (p);
        Vec in ((size_t) (sr)); in[0] = 1.0f;
        const auto out = run (procOf (e), in);
        // Spectrum of the echo only: a window of 4096 samples around it.
        dsp::FFT fft; fft.prepare (12);
        std::vector<std::complex<float>> buf (4096);
        for (int i = 0; i < 4096; ++i) buf[(size_t) i] = { out[(size_t) (18000 - 2048 + i)], 0.0f };
        fft.forward (buf.data());
        auto mag = [&] (double hz) { return 20.0 * std::log10 (std::abs (buf[(size_t) std::lround (hz / sr * 4096)])); };
        print ("delay", "Delay-R digital, frac 0.5", "echo HF loss at 16 kHz", mag (16000) - mag (1000), "dB", "Hermite interpolation");
        print ("delay", "Delay-R digital, frac 0.5", "echo HF loss at 19 kHz", mag (19000) - mag (1000), "dB");
    }
    if (wanted ("FractionalDelay"))
    {
        // The shared interpolator (Chorus-R, Flanger, Delay-R wow, Pitch Delay-R, Reverb-R modulation)
        // read with a delay that grows steadily: the output is a pure tone at a Doppler-shifted
        // frequency, so anything else is interpolation error.
        for (double hz : { 1000.0, 8000.0, 12000.0, 16000.0 })
        {
            dsp::FractionalDelay d; d.setMaxDelay (8192);
            const double rate = 0.0007; // samples of delay per sample: a slow chorus-like sweep
            const size_t len = (size_t) sr / 4 + N;
            Vec out (len);
            for (size_t i = 0; i < len; ++i)
            {
                d.push ((float) (0.5 * std::sin (2.0 * pi * hz * (double) i / sr)));
                out[i] = d.read ((float) (10.0 + rate * (double) i));
            }
            const auto r = analyse (spectrum (out, (size_t) sr / 4), hz * (1.0 - rate), lobeHz);
            char m[48]; std::snprintf (m, sizeof m, "error, %.0f kHz swept tone", hz / 1000.0);
            print ("delay", "FractionalDelay (Hermite)", m, db ((r.other + r.harmonics) / r.fund), "dBc");
        }
    }
    if (wanted ("Phaser-R"))
    {
        for (float fb : { 0.5f, 0.8f })
        {
            phaseready::PhaseEngine e; e.prepare (sr, 2);
            phaseready::PhaseParams p; p.stages = 12; p.feedback = fb; e.setParams (p);
            Proc pr = [&] (float* const* io, int n) { e.process (io, 2, n, {}); };
            char nm[48]; std::snprintf (nm, sizeof nm, "Phaser-R 12 st fb %.1f", fb);
            print ("delay", nm, "THD, 1 kHz -6 dBFS", thdDb (toneTest (pr, 1000.0, 0.5, 0.5, 40.0)), "dB", "feedback safety clip");
        }
    }
}

// Shift purity: energy at the harmonics of the target pitch versus everything else.
void pitch()
{
    using namespace rommuser;
    const double f0 = 220.0, f1 = 220.0 * std::pow (2.0, 7.0 / 12.0);
    auto saw = [&] (size_t len)
    {
        Vec v (len);
        for (size_t i = 0; i < len; ++i)
        {
            double s = 0.0;
            for (int h = 1; h <= 20; ++h) s += std::sin (2.0 * pi * h * f0 * (double) i / sr) / h;
            v[i] = (float) (0.3 * s);
        }
        return v;
    };
    auto purity = [&] (const Vec& out, size_t start)
    {
        const auto p = spectrum (out, start);
        const auto r = analyse (p, f1, 6.0);
        return db (r.other / (r.fund + r.harmonics));
    };
    const size_t settle = (size_t) sr;
    if (wanted ("Tune-R"))
    {
        tuneready::TuneEngine e; e.prepare (sr, block, 2);
        tuneready::TuneParams p; p.mode = tuneready::Mode::fixed; p.fixedSemis = 7.0f; e.setParams (p, true);
        print ("pitch", "Tune-R fixed +7 st", "noise vs target, saw 220", purity (run (procOf (e), saw (settle + N)), settle), "dB");
        print ("pitch", "Tune-R fixed +7 st", "noise vs target, sine 220", purity (run (procOf (e), sine (f0, 0.5, settle + N)), settle), "dB");
    }
    if (wanted ("Vocoder-R"))
    {
        voxsynth::VoxEngine e; e.prepare (sr, block, 2);
        voxsynth::VoxParams p; p.intervalMode = voxsynth::IntervalMode::semitones; p.voices[0].enabled = true; p.voices[0].semitones = 7.0f;
        p.voices[0].levelDb = 0.0f; p.dryDb = -60.0f; e.setParams (p, true);
        Proc pr = [&] (float* const* io, int n) { e.process (io, 2, n); };
        print ("pitch", "Vocoder-R harmony +7 st", "noise vs target, saw 220", purity (run (pr, saw (settle + N)), settle), "dB");
    }
}

// Reverb impulse response: RT60 per octave (Schroeder, T30), echo density and colour.
void reverb()
{
    using namespace rommuser;
    for (float mod : { 0.0f, 0.5f })
    for (auto mode : { spaceready::Mode::hall, spaceready::Mode::plate, spaceready::Mode::room })
    {
        const char* mn[] = { "room", "plate", "hall", "ambience" };
        const std::string name = std::string ("Reverb-R ") + mn[(int) mode] + " 2 s" + (mod > 0 ? " mod 0.5" : " mod 0");
        if (! wanted (name)) continue;
        spaceready::SpaceEngine e; e.prepare (sr, 2);
        spaceready::SpaceParams p; p.mode = mode; p.decaySeconds = 2.0f; p.mix = 1.0f; p.preDelayMs = 0.0f; p.modulation = mod;
        p.dampingHz = 20000.0f;
        e.setParams (p);
        Vec in ((size_t) (5 * sr)); in[0] = 1.0f;
        Vec l, r; run (procOf (e), in, l, r);

        for (double fc : { 125.0, 500.0, 2000.0, 8000.0 })
        {
            dsp::Biquad bp1, bp2;
            const auto c = dsp::biquad::bandPass (fc, 1.414, sr);
            bp1.setCoeffs (c); bp2.setCoeffs (c);
            std::vector<double> e2 (l.size());
            for (size_t i = 0; i < l.size(); ++i) { const double v = bp2.process (bp1.process (l[i])); e2[i] = v * v; }
            for (size_t i = e2.size() - 1; i-- > 0;) e2[i] += e2[i + 1]; // Schroeder backward integral
            auto t = [&] (double dbDown) { for (size_t i = 0; i < e2.size(); ++i) if (db (e2[i] / e2[0]) <= -dbDown) return (double) i / sr; return 99.0; };
            char m[64]; std::snprintf (m, sizeof m, "RT60 at %.0f Hz (target 2)", fc);
            print ("reverb", name, m, 2.0 * (t (35.0) - t (5.0)), "s");
        }
        // Normalised echo density (Abel & Huang): 1.0 = Gaussian, i.e. fully diffuse.
        const int w = (int) (0.02 * sr);
        double mixing = -1.0;
        for (int c = w; c + w < (int) (0.5 * sr); c += 48)
        {
            double s2 = 0.0; for (int i = c - w / 2; i < c + w / 2; ++i) s2 += (double) l[(size_t) i] * l[(size_t) i];
            const double sd = std::sqrt (s2 / w);
            int out = 0; for (int i = c - w / 2; i < c + w / 2; ++i) out += std::fabs (l[(size_t) i]) > sd;
            if ((double) out / w / 0.3173 >= 0.95) { mixing = (double) c / sr * 1000.0; break; }
        }
        print ("reverb", name, "time to diffuse (NED 0.95)", mixing, "ms", "-1 = never in 500 ms");
        // Colour: spread of the late tail's spectrum, 200 Hz .. 8 kHz, after removing a 1/3-octave
        // trend. White noise gives 5.6 dB; more means audible ringing modes.
        const auto sp = spectrum (l, (size_t) (0.3 * sr));
        std::vector<double> d;
        for (size_t k = (size_t) (200 / binHz); k < (size_t) (8000 / binHz); ++k)
        {
            const double f = k * binHz;
            const double trend = bandPower (sp, f / 1.122, f * 1.122) / (((f * 1.122) - (f / 1.122)) / binHz + 1);
            d.push_back (10.0 * std::log10 (sp[k] / trend));
        }
        double m = 0.0, v = 0.0; for (double x : d) m += x; m /= d.size(); for (double x : d) v += (x - m) * (x - m);
        print ("reverb", name, "tail spectrum spread", std::sqrt (v / d.size()), "dB", "noise = 5.6");
        double lr = 0.0, ll = 0.0, rr = 0.0;
        for (size_t i = (size_t) (0.1 * sr); i < (size_t) (1.0 * sr); ++i) { lr += (double) l[i] * r[i]; ll += (double) l[i] * l[i]; rr += (double) r[i] * r[i]; }
        print ("reverb", name, "L/R correlation of tail", lr / std::sqrt (ll * rr), "", "0 = widest");
    }
}

void synths()
{
    using namespace rommuser;
    struct C { std::string name; int note; int unison; float drive; float filterDrive; };
    for (const C& c : { C { "Synth-R saw C7", 96, 1, 0.0f, 0.0f }, C { "Synth-R saw G6", 91, 1, 0.0f, 0.0f },
                        C { "Synth-R saw C5 drive 18", 72, 1, 18.0f, 0.0f }, C { "Synth-R saw C5 filter drive 1", 72, 1, 0.0f, 1.0f } })
    {
        if (! wanted (c.name)) continue;
        waveready::WaveEngine e; e.prepare (sr, block);
        waveready::WaveParams p; p.osc1.unison = c.unison; p.osc1.retrig = true; p.osc2.on = false; p.cutoffHz = 20000.0f;
        p.filterMode = waveready::FilterMode::lp12; p.resonance = 0.0f; p.driveDb = c.drive; p.filterDrive = c.filterDrive;
        p.ampEnv.attackMs = 1.0f; p.ampEnv.sustain = 1.0f;
        e.setParams (p, true);
        e.noteOn (c.note, 1.0f);
        const size_t len = (size_t) sr + N;
        Vec l (len), r (len);
        for (size_t pos = 0; pos < len; pos += block) e.render (l.data() + pos, r.data() + pos, (int) std::min ((size_t) block, len - pos));
        const double f0 = 440.0 * std::pow (2.0, (c.note - 69) / 12.0);
        const auto sp = spectrum (l, (size_t) sr);
        const auto rep = analyse (sp, f0, 6.0);
        print ("synth", c.name, "alias", db (rep.other / (rep.fund + rep.harmonics)), "dBc");
        if (c.drive == 0.0f && c.filterDrive == 0.0f)
        {
            // Brightness: harmonics an ideal band-limited saw has below 20 kHz versus what is there.
            double ideal = 0.0, got = 0.0;
            for (int h = 1; h * f0 < 20000.0; ++h)
            {
                ideal += 1.0 / (h * h);
                got += bandPower (sp, h * f0 - 6, h * f0 + 6);
            }
            const double fundIdeal = 1.0;
            print ("synth", c.name, "missing HF (harmonics < 20k)", db ((got / rep.fund) / (ideal / fundIdeal)), "dB", "0 = complete");
        }
    }
}
} // namespace

int main (int argc, char** argv)
{
    DSP_LAB_FTZ();
    filterArg = argc > 1 ? argv[1] : "";
    std::printf ("%-12s %-36s %-26s %9s\n", "group", "setup", "metric", "value");
    saturation();
    dynamics();
    transparency();
    filters();
    delays();
    pitch();
    reverb();
    synths();
    return 0;
}
