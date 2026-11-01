// First-order antiderivative antialiasing (ADAA) for static waveshapers.
//
// Instead of y[n] = f (x[n]), ADAA outputs the average of f over the segment between the last two
// input samples:  y[n] = (F (x[n]) - F (x[n-1])) / (x[n] - x[n-1]),  F' = f
// (Parker, Zavalishin & Le Bihan, "Reducing the aliasing of nonlinear waveshaping using
// continuous-time convolution", DAFx 2016; Bilbao, Esqueda, Parker & Välimäki, IEEE SPL 2017).
// Harmonics that would fold back above Nyquist are attenuated before they alias, so a given
// aliasing level needs a much lower oversampling factor. The price: half a sample of delay at the
// rate it runs at, and a gentle low-pass (the average of two samples), which is why the plugins
// offer it as an opt-in "HQ" mode on top of oversampling, where both effects sit far above 20 kHz.
// When the two samples are (almost) equal the quotient is ill-conditioned; f at the midpoint is
// the same limit. The antiderivatives are evaluated in double, because F grows like |x|.
//
// Because y is an average of f, it never leaves the range of f: a clipper stays below its ceiling.
//
// AdaaCompensator undoes the two-sample average in the audio band and rounds the delay up to
// whole base-rate samples, so HQ mode stays flat (within 0.015 dB at 2x, 0.0001 dB at 4x and 8x,
// up to 0.45 of the base rate) and the dry path can be aligned with a plain integer delay.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace rommuser::dsp
{

// log (cosh (x)), stable for large |x| (the antiderivative of tanh).
inline double logCosh (double x)
{
    const double a = std::fabs (x);
    return a + std::log1p (std::exp (-2.0 * a)) - 0.69314718055994530942;
}

// One ADAA state (one per channel and per shaper).
struct Adaa1
{
    double xPrev = 0.0, fPrev = 0.0; // last input and F (last input)

    void reset (double x0 = 0.0) { xPrev = x0; fPrev = 0.0; primed = false; }

    // f: the curve, antiF: its antiderivative (any constant). Both take and return double.
    template <typename Curve, typename Anti>
    inline float process (double x, Curve&& f, Anti&& antiF)
    {
        const double fx = antiF (x);
        if (! primed) { primed = true; xPrev = x; fPrev = fx; return (float) f (x); }
        const double dx = x - xPrev;
        const double y = std::fabs (dx) > 1.0e-7 ? (fx - fPrev) / dx : f (0.5 * (x + xPrev));
        xPrev = x;
        fPrev = fx;
        return (float) y;
    }

private:
    bool primed = false;
};

// Linear-phase FIR at the oversampled rate, least-squares designed so that (two-sample average) *
// FIR = a pure delay of whole base-rate samples (2 at 2x, 1 at 4x and 8x) up to 0.45 of the base
// rate. With factor 1 there is no room above the audio band, so it passes the signal
// through (HQ then keeps ADAA's gentle top-octave roll-off and half-sample delay).
class AdaaCompensator
{
public:
    void prepare (int newFactor)
    {
        factor = std::max (1, newFactor);
        taps.clear();
        history.clear();
        if (factor < 2)
            return;

        // m symmetric pairs: length 2m, delay m - 1/2, plus ADAA's 1/2 = m oversampled samples.
        // m is a multiple of the factor (so the delay is whole base samples) and at least 4.
        const int m = std::max (factor, 4);
        delayBase = m / factor;
        const double pi = 3.14159265358979323846, wp = 0.9 * pi / factor; // 0.45 of the base rate
        constexpr int grid = 256;
        std::vector<double> g ((size_t) (m * m), 0.0), b ((size_t) m, 0.0), a ((size_t) m);
        for (int i = 0; i < grid; ++i)
        {
            const double w = wp * i / (grid - 1), avg = std::cos (0.5 * w);
            for (int k = 0; k < m; ++k) a[(size_t) k] = 2.0 * std::cos ((k + 0.5) * w) * avg;
            for (int r = 0; r < m; ++r)
            {
                b[(size_t) r] += a[(size_t) r];
                for (int c = 0; c < m; ++c) g[(size_t) (r * m + c)] += a[(size_t) r] * a[(size_t) c];
            }
        }
        // Gaussian elimination with partial pivoting (m <= 8).
        for (int col = 0; col < m; ++col)
        {
            int piv = col;
            for (int r = col + 1; r < m; ++r)
                if (std::fabs (g[(size_t) (r * m + col)]) > std::fabs (g[(size_t) (piv * m + col)])) piv = r;
            for (int c = 0; c < m; ++c) std::swap (g[(size_t) (col * m + c)], g[(size_t) (piv * m + c)]);
            std::swap (b[(size_t) col], b[(size_t) piv]);
            for (int r = col + 1; r < m; ++r)
            {
                const double f = g[(size_t) (r * m + col)] / g[(size_t) (col * m + col)];
                for (int c = col; c < m; ++c) g[(size_t) (r * m + c)] -= f * g[(size_t) (col * m + c)];
                b[(size_t) r] -= f * b[(size_t) col];
            }
        }
        std::vector<double> h ((size_t) m);
        for (int r = m - 1; r >= 0; --r)
        {
            double acc = b[(size_t) r];
            for (int c = r + 1; c < m; ++c) acc -= g[(size_t) (r * m + c)] * h[(size_t) c];
            h[(size_t) r] = acc / g[(size_t) (r * m + r)];
        }
        taps.assign ((size_t) (2 * m), 0.0f);
        for (int k = 0; k < m; ++k)
            taps[(size_t) (m - 1 - k)] = taps[(size_t) (m + k)] = (float) h[(size_t) k];
        history.assign (taps.size() * 2, 0.0f);
        pos = 0;
    }

    void reset() { std::fill (history.begin(), history.end(), 0.0f); pos = 0; }

    // Extra latency (base-rate samples) of ADAA plus this filter, beyond the oversampler's own.
    int getLatencyBaseSamples() const { return factor >= 2 ? delayBase : 0; }

    inline float process (float x)
    {
        if (taps.empty())
            return x;
        const size_t len = taps.size();
        pos = (pos == 0 ? len : pos) - 1;
        history[pos] = x;
        history[pos + len] = x; // mirrored, so the window is contiguous
        const float* w = history.data() + pos;
        float acc = 0.0f;
        for (size_t k = 0; k < len; ++k) acc += taps[k] * w[k];
        return acc;
    }

private:
    int factor = 1, delayBase = 0;
    std::vector<float> taps, history;
    size_t pos = 0;
};

} // namespace rommuser::dsp
