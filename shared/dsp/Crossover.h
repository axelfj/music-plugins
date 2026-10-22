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

// Four-way split for one channel, as a tree: the middle crossover splits first, then each half is
// split again. Each half is passed through the other half's crossover all-pass so the four bands sum
// to a flat magnitude.
class FourBandSplitter
{
public:
    void prepare (double sampleRate)
    {
        for (auto* x : { &lowXover, &midXover, &highXover, &lowCompensation, &highCompensation })
            x->prepare (sampleRate);
        setFrequencies (lowXover.getFrequency(), midXover.getFrequency(), highXover.getFrequency());
    }

    void setFrequencies (double lowHz, double midHz, double highHz)
    {
        if (midHz < lowHz * 1.05) midHz = lowHz * 1.05;
        if (highHz < midHz * 1.05) highHz = midHz * 1.05;
        lowXover.setFrequency (lowHz);
        midXover.setFrequency (midHz);
        highXover.setFrequency (highHz);
        lowCompensation.setFrequency (highHz);   // the low half gets the high crossover's all-pass
        highCompensation.setFrequency (lowHz);   // the high half gets the low crossover's all-pass
    }

    void reset()
    {
        for (auto* x : { &lowXover, &midXover, &highXover, &lowCompensation, &highCompensation })
            x->reset();
    }

    inline void process (float x, float* bands)
    {
        float lowHalf, highHalf;
        midXover.process (x, lowHalf, highHalf);
        lowHalf = lowCompensation.allPass (lowHalf);
        highHalf = highCompensation.allPass (highHalf);
        lowXover.process (lowHalf, bands[0], bands[1]);
        highXover.process (highHalf, bands[2], bands[3]);
    }

private:
    LR4Crossover lowXover, midXover, highXover, lowCompensation, highCompensation;
};

// 1 to 4 bands with one interface. Frequencies are the crossovers in ascending order; only the
// first (numBands - 1) are used.
class MultibandSplitter
{
public:
    static constexpr int maxBands = 4;

    void prepare (double sampleRate)
    {
        two.prepare (sampleRate);
        three.prepare (sampleRate);
        four.prepare (sampleRate);
    }

    void setNumBands (int n) { numBands = n < 1 ? 1 : n > maxBands ? maxBands : n; }
    int getNumBands() const { return numBands; }

    void setFrequencies (double f1, double f2, double f3)
    {
        two.setFrequency (f1);
        three.setFrequencies (f1, f2);
        four.setFrequencies (f1, f2, f3);
    }

    void reset()
    {
        two.reset();
        three.reset();
        four.reset();
    }

    // Writes numBands values into bands (the rest is left untouched).
    inline void process (float x, float* bands)
    {
        switch (numBands)
        {
            case 1: bands[0] = x; break;
            case 2: two.process (x, bands[0], bands[1]); break;
            case 3: three.process (x, bands[0], bands[1], bands[2]); break;
            default: four.process (x, bands); break;
        }
    }

private:
    int numBands = 1;
    LR4Crossover two;
    ThreeBandSplitter three;
    FourBandSplitter four;
};

} // namespace rommuser::dsp
