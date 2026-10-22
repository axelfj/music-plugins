// Small radix-2 complex FFT (iterative Cooley-Tukey, float) for the STFT-based plugins.
// No external dependencies so the DSP stays testable without JUCE.
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
        twiddles.resize ((size_t) size / 2);
        for (int k = 0; k < size / 2; ++k)
            twiddles[(size_t) k] = std::polar (1.0f, -2.0f * 3.14159265358979323846f * (float) k / (float) size);
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
        for (int i = 0; i < size; ++i) data[i] *= s;
    }

private:
    void transform (Complex* data, bool inverseDirection) const
    {
        for (int i = 0; i < size; ++i)
        {
            const int j = bitReverse[(size_t) i];
            if (j > i) std::swap (data[i], data[j]);
        }
        for (int len = 2; len <= size; len <<= 1)
        {
            const int half = len / 2, step = size / len;
            for (int start = 0; start < size; start += len)
                for (int k = 0; k < half; ++k)
                {
                    Complex w = twiddles[(size_t) (k * step)];
                    if (inverseDirection) w = std::conj (w);
                    const Complex t = data[start + k + half] * w;
                    data[start + k + half] = data[start + k] - t;
                    data[start + k] += t;
                }
        }
    }

    int size = 0;
    std::vector<Complex> twiddles;
    std::vector<int> bitReverse;
};

} // namespace rommuser::dsp
