// Small radix-2 complex FFT (iterative Cooley-Tukey, float) for the STFT-based plugins.
// No external dependencies so the DSP stays testable without JUCE.
//
// Speed notes: the butterflies are written on the real and imaginary parts directly, because
// std::complex<float> multiplication goes through a NaN/Inf-safe library call (__mulsc3) unless the
// whole program is built with -ffast-math. Each stage reads its own contiguous twiddle table, and
// the first two stages (multiplications by 1 and -i only) are done without any multiply.
// forwardTwoReal() / inverseTwoReal() transform two real signals with one complex FFT.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cmath>
#include <complex>
#include <vector>

namespace rommuser::dsp
{

class FFT
{
public:
    using Complex = std::complex<float>;

    void prepare (int order)
    {
        size = 1 << order;
        // Stage with butterfly span `half` uses twiddles exp(-2 pi i k / (2 half)), k < half,
        // stored back to back from offset half - 1 (spans 1, 2, 4, ...: offsets 0, 1, 3, ...).
        twRe.assign ((size_t) std::max (1, size), 1.0f);
        twIm.assign ((size_t) std::max (1, size), 0.0f);
        for (int half = 1; half < size; half <<= 1)
            for (int k = 0; k < half; ++k)
            {
                const double a = -3.14159265358979323846 * (double) k / (double) half;
                twRe[(size_t) (half - 1 + k)] = (float) std::cos (a);
                twIm[(size_t) (half - 1 + k)] = (float) std::sin (a);
            }
        bitReverse.resize ((size_t) size);
        for (int i = 0; i < size; ++i)
        {
            int r = 0;
            for (int b = 0; b < order; ++b) r |= ((i >> b) & 1) << (order - 1 - b);
            bitReverse[(size_t) i] = r;
        }
    }

    int getSize() const { return size; }

    // In-place forward transform (no scaling).
    void forward (Complex* data) const { transform (data, false); }

    // In-place inverse transform, scaled by 1/N.
    void inverse (Complex* data) const
    {
        transform (data, true);
        const float s = 1.0f / (float) size;
        float* d = reinterpret_cast<float*> (data);
        for (int i = 0; i < 2 * size; ++i) d[i] *= s;
    }

    // Spectra of two real signals x and y (each `size` samples) with one complex FFT:
    // z = x + i y, X[k] = (Z[k] + conj Z[N-k]) / 2, Y[k] = (Z[k] - conj Z[N-k]) / 2i.
    // `work` must hold `size` values; X and Y receive all `size` bins (Hermitian).
    void forwardTwoReal (const float* x, const float* y, Complex* work, Complex* X, Complex* Y) const
    {
        for (int i = 0; i < size; ++i) work[i] = { x[i], y[i] };
        forward (work);
        for (int k = 0; k < size; ++k)
        {
            const Complex zk = work[k], zn = work[(size - k) & (size - 1)];
            X[k] = { 0.5f * (zk.real() + zn.real()), 0.5f * (zk.imag() - zn.imag()) };
            Y[k] = { 0.5f * (zk.imag() + zn.imag()), 0.5f * (zn.real() - zk.real()) };
        }
    }

    // Inverse of two Hermitian spectra at once: the real part of the result is x, the imaginary y.
    // `work` (size values) receives x + i y.
    void inverseTwoReal (const Complex* X, const Complex* Y, Complex* work) const
    {
        for (int k = 0; k < size; ++k)
            work[k] = { X[k].real() - Y[k].imag(), X[k].imag() + Y[k].real() }; // X + i Y
        inverse (work);
    }

private:
    void transform (Complex* data, bool inverseDirection) const
    {
        float* d = reinterpret_cast<float*> (data);
        for (int i = 0; i < size; ++i)
        {
            const int j = bitReverse[(size_t) i];
            if (j > i) std::swap (data[i], data[j]);
        }
        if (size < 2)
            return;

        // Span 1: (a, b) -> (a + b, a - b).
        for (int s = 0; s < size; s += 2)
        {
            float* p = d + 2 * s;
            const float ar = p[0], ai = p[1], br = p[2], bi = p[3];
            p[0] = ar + br; p[1] = ai + bi;
            p[2] = ar - br; p[3] = ai - bi;
        }
        int firstGeneral = 2;
        if (size >= 4)
        {
            // Span 2: the second twiddle is -i (forward) or +i (inverse).
            const float sgn = inverseDirection ? -1.0f : 1.0f;
            for (int s = 0; s < size; s += 4)
            {
                float* p = d + 2 * s;
                const float r0 = p[0], i0 = p[1], r1 = p[2], i1 = p[3];
                const float r2 = p[4], i2 = p[5], r3 = p[6], i3 = p[7];
                // t = w * x3 with w = -i (forward): (i3, -r3)
                const float tr = sgn * i3, ti = -sgn * r3;
                p[0] = r0 + r2; p[1] = i0 + i2;
                p[4] = r0 - r2; p[5] = i0 - i2;
                p[2] = r1 + tr; p[3] = i1 + ti;
                p[6] = r1 - tr; p[7] = i1 - ti;
            }
            firstGeneral = 4;
        }

        const float conj = inverseDirection ? -1.0f : 1.0f;
        for (int half = firstGeneral; half < size; half <<= 1)
        {
            const float* wr = twRe.data() + (half - 1);
            const float* wi = twIm.data() + (half - 1);
            for (int start = 0; start < size; start += 2 * half)
            {
                float* lo = d + 2 * start;
                float* hi = d + 2 * (start + half);
                for (int k = 0; k < half; ++k)
                {
                    const float cr = wr[k], ci = conj * wi[k];
                    const float xr = hi[2 * k], xi = hi[2 * k + 1];
                    const float tr = xr * cr - xi * ci;
                    const float ti = xr * ci + xi * cr;
                    const float ur = lo[2 * k], ui = lo[2 * k + 1];
                    hi[2 * k] = ur - tr; hi[2 * k + 1] = ui - ti;
                    lo[2 * k] = ur + tr; lo[2 * k + 1] = ui + ti;
                }
            }
        }
    }

    int size = 0;
    std::vector<float> twRe, twIm;
    std::vector<int> bitReverse;
};

} // namespace rommuser::dsp
