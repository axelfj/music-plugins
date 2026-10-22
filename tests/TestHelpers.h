// Measurement helpers for the DSP tests: signal generators, a radix-2 FFT and level measurements.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <cmath>
#include <complex>
#include <random>
#include <vector>

namespace testhelpers
{

constexpr double pi = 3.14159265358979323846;

inline std::vector<float> sine (double freq, double sampleRate, int n, double amplitude = 1.0)
{
    std::vector<float> v ((size_t) n);
    for (int i = 0; i < n; ++i)
        v[(size_t) i] = (float) (amplitude * std::sin (2.0 * pi * freq * i / sampleRate));
    return v;
}

inline std::vector<float> whiteNoise (int n, double amplitude, unsigned seed = 1234)
{
    std::mt19937 rng (seed);
    std::uniform_real_distribution<float> dist ((float) -amplitude, (float) amplitude);
    std::vector<float> v ((size_t) n);
    for (auto& x : v)
        x = dist (rng);
    return v;
}

inline double toDb (double linear) { return 20.0 * std::log10 (std::max (linear, 1e-30)); }

inline double rms (const float* x, int n)
{
    double s = 0.0;
    for (int i = 0; i < n; ++i)
        s += (double) x[i] * x[i];
    return std::sqrt (s / std::max (1, n));
}

inline double peak (const std::vector<float>& x)
{
    double p = 0.0;
    for (auto v : x)
        p = std::max (p, (double) std::fabs (v));
    return p;
}

// In-place iterative radix-2 FFT. n must be a power of two.
inline void fft (std::vector<std::complex<double>>& a)
{
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i)
    {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j)
            std::swap (a[i], a[j]);
    }
    for (size_t len = 2; len <= n; len <<= 1)
    {
        const double ang = -2.0 * pi / (double) len;
        const std::complex<double> wl (std::cos (ang), std::sin (ang));
        for (size_t i = 0; i < n; i += len)
        {
            std::complex<double> w (1.0);
            for (size_t k = 0; k < len / 2; ++k)
            {
                const auto u = a[i + k];
                const auto v = a[i + k + len / 2] * w;
                a[i + k] = u + v;
                a[i + k + len / 2] = u - v;
                w *= wl;
            }
        }
    }
}

// Power spectrum (per bin, Blackman-Harris windowed) normalised so a full-scale sine reads 0 dB
// when its power is summed over its main lobe.
inline std::vector<double> powerSpectrum (const float* x, size_t n)
{
    std::vector<std::complex<double>> a (n);
    double wsum = 0.0;
    for (size_t i = 0; i < n; ++i)
    {
        const double t = 2.0 * pi * (double) i / (double) (n - 1);
        const double w = 0.35875 - 0.48829 * std::cos (t) + 0.14128 * std::cos (2 * t) - 0.01168 * std::cos (3 * t);
        wsum += w * w;
        a[i] = x[i] * w;
    }
    fft (a);
    std::vector<double> p (n / 2);
    for (size_t k = 0; k < n / 2; ++k)
        p[k] = 4.0 * std::norm (a[k]) / (wsum * (double) n);  // Parseval: a sine of amplitude A sums to A^2
    return p;
}

// Level (dBFS, sine-referenced) of everything in [loHz, hiHz] that is NOT within +-guardBins
// of a harmonic of f0 (folded harmonics are counted as aliasing because they are not at k * f0).
inline double inharmonicLevelDb (const std::vector<double>& p, double sampleRate, size_t n, double f0,
                                 double loHz, double hiHz, int guardBins = 8)
{
    const double binHz = sampleRate / (double) n;
    std::vector<bool> excluded (p.size(), false);
    for (int h = 1; h * f0 < sampleRate / 2.0; ++h)
    {
        const long centre = std::lround (h * f0 / binHz);
        for (long b = centre - guardBins; b <= centre + guardBins; ++b)
            if (b >= 0 && b < (long) p.size())
                excluded[(size_t) b] = true;
    }
    double sum = 0.0;
    const auto lo = (size_t) std::max (1.0, std::floor (loHz / binHz));
    const auto hi = std::min (p.size() - 1, (size_t) std::ceil (hiHz / binHz));
    for (size_t k = lo; k <= hi; ++k)
        if (! excluded[k])
            sum += p[k];
    return 10.0 * std::log10 (std::max (sum, 1e-30));
}

// Level (dBFS) summed over +-guardBins around a frequency.
inline double toneLevelDb (const std::vector<double>& p, double sampleRate, size_t n, double f, int guardBins = 6)
{
    const double binHz = sampleRate / (double) n;
    const long centre = std::lround (f / binHz);
    double sum = 0.0;
    for (long b = centre - guardBins; b <= centre + guardBins; ++b)
        if (b >= 0 && b < (long) p.size())
            sum += p[(size_t) b];
    return 10.0 * std::log10 (std::max (sum, 1e-30));
}

} // namespace testhelpers
