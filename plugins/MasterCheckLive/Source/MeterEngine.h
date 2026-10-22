// Master Check Live DSP engine: loudness (BS.1770), loudness range (Tech 3342), true peak (4x) and
// sample peak, measured on the signal passing through untouched.
//
// Pure C++ (no JUCE) so it is unit-tested headless in tests/.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../../../shared/dsp/Loudness.h"

#include <algorithm>
#include <cmath>

namespace rommuser::mastercheck
{

struct MeterReadings
{
    float momentaryLufs = -120.0f, shortTermLufs = -120.0f, integratedLufs = -120.0f;
    float maxMomentaryLufs = -120.0f, maxShortTermLufs = -120.0f;
    float loudnessRangeLu = 0.0f;
    float truePeakDb = -120.0f;        // held maximum
    float blockTruePeakDb = -120.0f;   // this block
    float samplePeakDb = -120.0f;      // held maximum
    float plrDb = 0.0f;                // true peak - integrated
    bool newBlock = false;             // a new 100 ms block finished in this call
};

inline constexpr int maxChannels = 2;

inline float toDb (float linear) { return linear > 1.0e-6f ? 20.0f * std::log10 (linear) : -120.0f; }

class MeterEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize, int numChannels)
    {
        channels = std::clamp (numChannels, 1, maxChannels);
        loudness.prepare (sampleRate, channels);
        truePeak.prepare (sampleRate, maxBlockSize);
    }

    void reset()
    {
        loudness.reset();
        truePeak.reset();
    }

    // Audio is read only: the output is bit-identical to the input.
    MeterReadings process (const float* const* in, int numChannels, int n)
    {
        const int nch = std::clamp (numChannels, 1, maxChannels);
        loudness.process (in, nch, n);
        const float blockTp = truePeak.process (in, nch, n);

        MeterReadings r;
        r.momentaryLufs = loudness.getMomentary();
        r.shortTermLufs = loudness.getShortTerm();
        r.integratedLufs = loudness.getIntegrated();
        r.maxMomentaryLufs = loudness.getMaxMomentary();
        r.maxShortTermLufs = loudness.getMaxShortTerm();
        r.loudnessRangeLu = loudness.getLoudnessRange();
        r.truePeakDb = toDb (truePeak.getHeld());
        r.blockTruePeakDb = toDb (blockTp);
        r.samplePeakDb = toDb (truePeak.getSamplePeakHeld());
        r.plrDb = r.integratedLufs > -99.0f ? r.truePeakDb - r.integratedLufs : 0.0f;
        r.newBlock = loudness.hasNewBlock();
        return r;
    }

private:
    int channels = 2;
    dsp::LoudnessMeter loudness;
    dsp::TruePeakMeter truePeak;
};

} // namespace rommuser::mastercheck
