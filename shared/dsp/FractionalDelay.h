// Fractional delay line with 4-point cubic (Hermite) interpolation, for modulated delays, chorus,
// flangers and tempo-synced echoes whose time is not a whole number of samples.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace rommuser::dsp
{

class FractionalDelay
{
public:
    // The buffer is rounded up to a power of two so every index wraps with a mask (no division);
    // the usable delay stays exactly the one asked for.
    void setMaxDelay (int maxSamples)
    {
        limit = (size_t) std::max (8, maxSamples + 4);
        size = 8;
        while (size < limit) size <<= 1;
        mask = size - 1;
        buf.assign (size, 0.0f);
        writePos = 0;
    }

    void reset()
    {
        std::fill (buf.begin(), buf.end(), 0.0f);
        writePos = 0;
    }

    int getMaxDelay() const { return (int) limit - 4; }

    inline void push (float x)
    {
        buf[writePos] = x;
        writePos = (writePos + 1) & mask;
    }

    // Sample `delay` samples before the most recently pushed one (delay >= 1 recommended; 0 reads the
    // sample just written). Cubic Hermite between the two nearest samples.
    inline float read (float delay) const
    {
        delay = std::clamp (delay, 1.0f, (float) (limit - 4));
        const float ip = std::floor (delay);
        const float frac = delay - ip;
        const int d = (int) ip;
        // index of x[n - d]
        const size_t i1 = (writePos + size - 1 - (size_t) d) & mask;
        const size_t i0 = (i1 + 1) & mask;             // one sample newer
        const size_t i2 = (i1 + size - 1) & mask;      // one older
        const size_t i3 = (i1 + size - 2) & mask;      // two older
        const float y0 = buf[i0], y1 = buf[i1], y2 = buf[i2], y3 = buf[i3];
        // Hermite: interpolate between y1 (frac 0) and y2 (frac 1)
        const float c0 = y1;
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

    inline float readInteger (int delay) const
    {
        delay = std::clamp (delay, 0, (int) limit - 4);
        return buf[(writePos + size - 1 - (size_t) delay) & mask];
    }

private:
    std::vector<float> buf { 8, 0.0f };
    size_t size = 8, mask = 7, limit = 8, writePos = 0;
};

} // namespace rommuser::dsp
