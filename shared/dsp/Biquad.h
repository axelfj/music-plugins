// Second-order sections for the EQ plugins.
//
// Coefficients follow the Audio EQ Cookbook (R. Bristow-Johnson), computed in double precision.
// State is transposed direct form II, also in double, so stacking 12 bands stays clean at 44.1 kHz
// with low-frequency, high-Q settings. `magnitudeDb()` evaluates the exact digital response, which
// the GUI draws and the tests compare against the analog target.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <cmath>
#include <complex>

namespace rommuser::dsp
{

enum class FilterType
{
    bell,
    lowShelf,
    highShelf,
    lowCut,   // high-pass
    highCut,  // low-pass
    notch,
    bandPass,
    tilt,
    allPass
};

struct BiquadCoeffs
{
    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0; // a0 normalised to 1

    static BiquadCoeffs identity() { return {}; }

    // Complex response at angular frequency w (radians/sample).
    std::complex<double> response (double w) const
    {
        const std::complex<double> z1 = std::polar (1.0, -w);
        const std::complex<double> z2 = z1 * z1;
        return (b0 + b1 * z1 + b2 * z2) / (1.0 + a1 * z1 + a2 * z2);
    }

    double magnitudeDb (double freqHz, double sampleRate) const
    {
        const double m = std::abs (response (2.0 * 3.14159265358979323846 * freqHz / sampleRate));
        return 20.0 * std::log10 (std::max (m, 1e-12));
    }
};

namespace biquad
{
    constexpr double pi = 3.14159265358979323846;

    inline double clampFreq (double f, double sampleRate) { return std::clamp (f, 10.0, sampleRate * 0.49); }

    inline BiquadCoeffs normalise (double b0, double b1, double b2, double a0, double a1, double a2)
    {
        BiquadCoeffs c;
        c.b0 = b0 / a0; c.b1 = b1 / a0; c.b2 = b2 / a0;
        c.a1 = a1 / a0; c.a2 = a2 / a0;
        return c;
    }

    inline BiquadCoeffs bell (double freq, double q, double gainDb, double sampleRate)
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w0 = 2.0 * pi * clampFreq (freq, sampleRate) / sampleRate;
        const double alpha = std::sin (w0) / (2.0 * std::max (q, 0.025));
        const double c = std::cos (w0);
        return normalise (1.0 + alpha * A, -2.0 * c, 1.0 - alpha * A,
                          1.0 + alpha / A, -2.0 * c, 1.0 - alpha / A);
    }


    // Peaking filter whose magnitude is matched to the analog prototype at DC, at the centre
    // frequency and at Nyquist (M. Vicanek, "Matched Second Order Digital Filters", 2016).
    // Unlike the cookbook bell it does not cramp towards Nyquist, so a 10 kHz bell at 44.1 kHz
    // still has the analog shape. Falls back to the cookbook bell for tiny gains, where the
    // matching equations lose precision and the two agree anyway.
    inline BiquadCoeffs bellMatched (double freq, double q, double gainDb, double sampleRate)
    {
        if (std::fabs (gainDb) < 0.01)
            return bell (freq, q, gainDb, sampleRate);

        const double w0 = 2.0 * pi * clampFreq (freq, sampleRate) / sampleRate;
        const double A = std::pow (10.0, gainDb / 40.0);   // sqrt of the linear gain at w0
        const double G = A * A;
        q = std::max (q, 0.025);

        // Poles of the analog prototype: s^2 + s / (A q) + 1, mapped by impulse invariance.
        const double poleQ = A * q;
        const double damping = 1.0 / (2.0 * poleQ);
        double a1, a2;
        a2 = std::exp (-2.0 * damping * w0);
        if (damping <= 1.0)
            a1 = -2.0 * std::exp (-damping * w0) * std::cos (std::sqrt (1.0 - damping * damping) * w0);
        else
            a1 = -2.0 * std::exp (-damping * w0) * std::cosh (std::sqrt (damping * damping - 1.0) * w0);

        const double A0 = (1.0 + a1 + a2) * (1.0 + a1 + a2);
        const double A1 = (1.0 - a1 + a2) * (1.0 - a1 + a2);
        const double A2 = -4.0 * a2;

        const double sn = std::sin (w0 * 0.5);
        const double phi1 = sn * sn;
        const double phi0 = 1.0 - phi1;
        const double phi2 = 4.0 * phi0 * phi1;

        const double R1 = (A0 * phi0 + A1 * phi1 + A2 * phi2) * G * G;
        const double R2 = (-A0 + A1 + 4.0 * (phi0 - phi1) * A2) * G * G;

        const double B0 = A0;
        const double B2 = (R1 - R2 * phi1 - B0) / (4.0 * phi1 * phi1);
        const double B1 = R2 + B0 + 4.0 * (phi1 - phi0) * B2;

        if (! (B0 >= 0.0 && B1 >= 0.0))
            return bell (freq, q, gainDb, sampleRate);

        const double W = 0.5 * (std::sqrt (B0) + std::sqrt (B1));
        const double b0 = 0.5 * (W + std::sqrt (std::max (W * W + B2, 0.0)));
        const double b1 = 0.5 * (std::sqrt (B0) - std::sqrt (B1));
        const double b2 = -B2 / (4.0 * b0);

        BiquadCoeffs c;
        c.b0 = b0; c.b1 = b1; c.b2 = b2; c.a1 = a1; c.a2 = a2;
        return c;
    }

    inline BiquadCoeffs lowShelf (double freq, double q, double gainDb, double sampleRate)
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w0 = 2.0 * pi * clampFreq (freq, sampleRate) / sampleRate;
        const double alpha = std::sin (w0) / (2.0 * std::max (q, 0.025));
        const double c = std::cos (w0);
        const double s = 2.0 * std::sqrt (A) * alpha;
        return normalise (A * ((A + 1.0) - (A - 1.0) * c + s),
                          2.0 * A * ((A - 1.0) - (A + 1.0) * c),
                          A * ((A + 1.0) - (A - 1.0) * c - s),
                          (A + 1.0) + (A - 1.0) * c + s,
                          -2.0 * ((A - 1.0) + (A + 1.0) * c),
                          (A + 1.0) + (A - 1.0) * c - s);
    }

    inline BiquadCoeffs highShelf (double freq, double q, double gainDb, double sampleRate)
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w0 = 2.0 * pi * clampFreq (freq, sampleRate) / sampleRate;
        const double alpha = std::sin (w0) / (2.0 * std::max (q, 0.025));
        const double c = std::cos (w0);
        const double s = 2.0 * std::sqrt (A) * alpha;
        return normalise (A * ((A + 1.0) + (A - 1.0) * c + s),
                          -2.0 * A * ((A - 1.0) + (A + 1.0) * c),
                          A * ((A + 1.0) + (A - 1.0) * c - s),
                          (A + 1.0) - (A - 1.0) * c + s,
                          2.0 * ((A - 1.0) - (A + 1.0) * c),
                          (A + 1.0) - (A - 1.0) * c - s);
    }

    inline BiquadCoeffs lowPass (double freq, double q, double sampleRate)
    {
        const double w0 = 2.0 * pi * clampFreq (freq, sampleRate) / sampleRate;
        const double alpha = std::sin (w0) / (2.0 * std::max (q, 0.025));
        const double c = std::cos (w0);
        return normalise ((1.0 - c) / 2.0, 1.0 - c, (1.0 - c) / 2.0, 1.0 + alpha, -2.0 * c, 1.0 - alpha);
    }

    inline BiquadCoeffs highPass (double freq, double q, double sampleRate)
    {
        const double w0 = 2.0 * pi * clampFreq (freq, sampleRate) / sampleRate;
        const double alpha = std::sin (w0) / (2.0 * std::max (q, 0.025));
        const double c = std::cos (w0);
        return normalise ((1.0 + c) / 2.0, -(1.0 + c), (1.0 + c) / 2.0, 1.0 + alpha, -2.0 * c, 1.0 - alpha);
    }

    // First-order sections for 6 dB/oct slopes, expressed as a biquad with b2 = a2 = 0.
    inline BiquadCoeffs lowPass1 (double freq, double sampleRate)
    {
        const double k = std::tan (pi * clampFreq (freq, sampleRate) / sampleRate);
        return normalise (k, k, 0.0, 1.0 + k, k - 1.0, 0.0);
    }

    inline BiquadCoeffs highPass1 (double freq, double sampleRate)
    {
        const double k = std::tan (pi * clampFreq (freq, sampleRate) / sampleRate);
        return normalise (1.0, -1.0, 0.0, 1.0 + k, k - 1.0, 0.0);
    }

    inline BiquadCoeffs notch (double freq, double q, double sampleRate)
    {
        const double w0 = 2.0 * pi * clampFreq (freq, sampleRate) / sampleRate;
        const double alpha = std::sin (w0) / (2.0 * std::max (q, 0.025));
        const double c = std::cos (w0);
        return normalise (1.0, -2.0 * c, 1.0, 1.0 + alpha, -2.0 * c, 1.0 - alpha);
    }

    // Constant 0 dB peak gain band-pass.
    inline BiquadCoeffs bandPass (double freq, double q, double sampleRate)
    {
        const double w0 = 2.0 * pi * clampFreq (freq, sampleRate) / sampleRate;
        const double alpha = std::sin (w0) / (2.0 * std::max (q, 0.025));
        const double c = std::cos (w0);
        return normalise (alpha, 0.0, -alpha, 1.0 + alpha, -2.0 * c, 1.0 - alpha);
    }

    inline BiquadCoeffs allPass (double freq, double q, double sampleRate)
    {
        const double w0 = 2.0 * pi * clampFreq (freq, sampleRate) / sampleRate;
        const double alpha = std::sin (w0) / (2.0 * std::max (q, 0.025));
        const double c = std::cos (w0);
        return normalise (1.0 - alpha, -2.0 * c, 1.0 + alpha, 1.0 + alpha, -2.0 * c, 1.0 - alpha);
    }

    // Q of each second-order section of a Butterworth filter of the given order (2, 4, 6, 8).
    inline double butterworthSectionQ (int order, int section)
    {
        // Damping ratio of each conjugate pole pair; sin() gives the right set for odd orders too
        // (order 3 -> Q = 1, order 5 -> 0.618 / 1.618), the real pole is handled as a first-order section.
        const double zeta = std::sin (pi * (2.0 * section + 1.0) / (2.0 * order));
        return 1.0 / (2.0 * zeta);
    }
} // namespace biquad

// One second-order section with double-precision TDF-II state.
class Biquad
{
public:
    void setCoeffs (const BiquadCoeffs& c) { coeffs = c; }
    const BiquadCoeffs& getCoeffs() const { return coeffs; }
    void reset() { s1 = s2 = 0.0; }

    inline float process (float x)
    {
        const double in = x;
        const double out = coeffs.b0 * in + s1;
        s1 = coeffs.b1 * in - coeffs.a1 * out + s2;
        s2 = coeffs.b2 * in - coeffs.a2 * out;
        return (float) out;
    }

private:
    BiquadCoeffs coeffs;
    double s1 = 0.0, s2 = 0.0;
};

} // namespace rommuser::dsp
