// Linkwitz-Riley crossovers (4th order: two cascaded Butterworth sections) whose bands sum back to
// a flat magnitude. Used by the multiband plugins (Duck Ready, Band Push, Warm Drive).
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "Biquad.h"

#include <array>
#include <cmath>

namespace rommuser::dsp
{

// Two-way LR4 split for one channel: low + high = all-pass (flat magnitude).
class LR4Crossover
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        setFrequency (frequency);
        reset();
    }

    void setFrequency (double hz)
    {
        frequency = hz;
        const double q = 1.0 / std::sqrt (2.0);
        const auto lp = biquad::lowPass (hz, q, sampleRate);
        const auto hp = biquad::highPass (hz, q, sampleRate);
        for (auto& f : low) f.setCoeffs (lp);
        for (auto& f : high) f.setCoeffs (hp);
    }

    double getFrequency() const { return frequency; }

    void reset()
    {
        for (auto& f : low) f.reset();
        for (auto& f : high) f.reset();
    }

    inline void process (float x, float& lo, float& hi)
    {
        lo = low[1].process (low[0].process (x));
        hi = high[1].process (high[0].process (x));
    }

    // The all-pass response of lo + hi, applied to another signal (phase matching of a third band).
    inline float allPass (float x)
    {
        return low[1].process (low[0].process (x)) + high[1].process (high[0].process (x));
    }

private:
    double sampleRate = 48000.0, frequency = 1000.0;
    std::array<Biquad, 2> low, high;
};

// Three-way split for one channel. The low band is passed through the high crossover's all-pass so
// that low + mid + high has a flat magnitude (the classic LR4 phase-compensation trick).
class ThreeBandSplitter
{
public:
    void prepare (double sampleRate)
    {
        lowXover.prepare (sampleRate);
        highXover.prepare (sampleRate);
        lowCompensation.prepare (sampleRate);
        setFrequencies (lowXover.getFrequency(), highXover.getFrequency());
    }

    void setFrequencies (double lowHz, double highHz)
    {
        if (highHz < lowHz * 1.05)
            highHz = lowHz * 1.05;
        lowXover.setFrequency (lowHz);
        highXover.setFrequency (highHz);
        lowCompensation.setFrequency (highHz);
    }

    void reset()
    {
        lowXover.reset();
        highXover.reset();
        lowCompensation.reset();
    }

    inline void process (float x, float& lo, float& mid, float& hi)
    {
        float rest;
        lowXover.process (x, lo, rest);
        lo = lowCompensation.allPass (lo);
        highXover.process (rest, mid, hi);
    }

private:
    LR4Crossover lowXover, highXover, lowCompensation;
};

} // namespace rommuser::dsp
