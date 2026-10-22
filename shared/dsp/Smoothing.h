// Small helpers shared by the plugins: dB conversion and click-free parameter ramps.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <cmath>

namespace rommuser::dsp
{

inline float dbToGain (float db) { return std::pow (10.0f, db / 20.0f); }
inline float gainToDb (float g, float floorDb = -120.0f)
{
    return g > 0.0f ? std::fmax (20.0f * std::log10 (g), floorDb) : floorDb;
}

// Linear ramp towards a target over a fixed time. Linear (not exponential) so it settles exactly.
class LinearSmoother
{
public:
    void prepare (double sampleRate, double rampSeconds)
    {
        rampLength = std::max (1, (int) std::lround (sampleRate * rampSeconds));
        setImmediate (target);
    }

    void setImmediate (float v)
    {
        current = target = v;
        remaining = 0;
    }

    void setTarget (float v)
    {
        if (std::fabs (v - target) <= 0.0f)
            return;
        target = v;
        remaining = rampLength;
        step = (target - current) / (float) rampLength;
    }

    float next()
    {
        if (remaining > 0)
        {
            current += step;
            if (--remaining == 0)
                current = target;
        }
        return current;
    }

    bool isSmoothing() const { return remaining > 0; }
    float getTarget() const { return target; }

private:
    float current = 0.0f, target = 0.0f, step = 0.0f;
    int rampLength = 1, remaining = 0;
};

} // namespace rommuser::dsp
