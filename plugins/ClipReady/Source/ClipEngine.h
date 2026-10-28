// Clip Ready DSP engine: oversampled soft clipper with a guaranteed sample-peak ceiling.
//
// Pure C++ (no JUCE) so it is unit-tested headless in tests/.
//
// Signal flow per channel:
//   x -> * (inGain / ceiling) -> upsample -> curve -> downsample -> [strict: clip at 1] -> * ceiling = wet
//   x -> delay(latency) = dry
//   out = outGain * (mix * wet * linkComp + (1 - mix) * dry)          (normal)
//         -> with strictCeiling the final sample is clamped to +-ceiling as well
//   out = outGain * (dry * inGain - wet) * linkComp                   (delta: only what gets clipped, unlimited)
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../../../shared/dsp/Oversampler.h"
#include "../../../shared/dsp/Smoothing.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <vector>

namespace rommuser::clipready
{

struct ClipParams
{
    float inputDb = 0.0f;     // -12 .. +24
    float ceilingDb = -0.3f;  // -24 .. 0
    float softness = 0.35f;   // 0 = hard clip, 1 = tanh-like saturation curve
    float outputDb = 0.0f;    // -24 .. +12
    float mix = 1.0f;         // 0 .. 1
    int oversampling = 4;     // 1, 2, 4, 8
    bool delta = false;
    bool gainLink = false;    // output follows -input so loudness stays comparable
    bool strictCeiling = true; // hard-limit wet after downsampling AND the final output (incl. Mix/Output), so the sample peak never passes the ceiling
};

struct ClipMeters
{
    float inputPeak = 0.0f;   // linear, after input gain
    float outputPeak = 0.0f;  // linear, final output
    float clipDb = 0.0f;      // how far the peak went above the ceiling, in dB (0 = no clipping)
};

// The clipping curve, normalised so the ceiling is 1.
// Below the knee start t = 1 - softness the signal passes untouched. Above it a tanh segment
// joins with matching slope and approaches 1 asymptotically, so the curve never exceeds 1.
inline float clipCurve (float x, float softness)
{
    const float t = 1.0f - softness;
    const float a = std::fabs (x);
    if (a <= t)
        return x;
    if (softness <= 1.0e-6f)
        return x > 0.0f ? 1.0f : -1.0f;
    const float w = 1.0f - t;
    const float y = t + w * std::tanh ((a - t) / w);
    return x > 0.0f ? y : -y;
}

class ClipEngine
{
public:
    static constexpr int maxChannels = 2;
    static constexpr std::array<int, 4> factors { 1, 2, 4, 8 };

    void prepare (double sampleRate, int maxBlockSize, int numChannels)
    {
        channels = std::min (numChannels, maxChannels);
        blockSize = maxBlockSize;

        int maxLatency = 0;
        for (int c = 0; c < maxChannels; ++c)
        {
            for (size_t f = 0; f < factors.size(); ++f)
            {
                oversamplers[(size_t) c][f] = std::make_unique<dsp::Oversampler> (maxBlockSize);
                oversamplers[(size_t) c][f]->prepare (factors[f], maxBlockSize);
                maxLatency = std::max (maxLatency, oversamplers[(size_t) c][f]->getLatencySamples());
            }
            dryDelay[(size_t) c].setMaxDelay (maxLatency);
        }

        pre.assign ((size_t) maxBlockSize, 0.0f);
        wet.assign ((size_t) maxBlockSize, 0.0f);

        for (auto* s : { &inGain, &ceiling, &outGain, &mix, &softness, &linkComp })
            s->prepare (sampleRate, 0.02);

        currentFactorIndex = -1;
        setParams (params, true);
    }

    void reset()
    {
        for (int c = 0; c < maxChannels; ++c)
        {
            for (auto& o : oversamplers[(size_t) c])
                if (o)
                    o->reset();
            dryDelay[(size_t) c].reset();
        }
    }

    void setParams (const ClipParams& p, bool immediate = false)
    {
        params = p;

        int idx = 2;
        for (size_t f = 0; f < factors.size(); ++f)
            if (factors[f] == p.oversampling)
                idx = (int) f;

        if (idx != currentFactorIndex)
        {
            currentFactorIndex = idx;
            const int lat = oversamplers[0u][(size_t) idx] ? oversamplers[0u][(size_t) idx]->getLatencySamples() : 0;
            for (int c = 0; c < maxChannels; ++c)
            {
                dryDelay[(size_t) c].setDelay (lat);
                if (oversamplers[(size_t) c][(size_t) idx])
                    oversamplers[(size_t) c][(size_t) idx]->reset();
            }
        }

        const float in = dsp::dbToGain (p.inputDb);
        const float targets[] = { in,
                                  dsp::dbToGain (std::min (p.ceilingDb, 0.0f)),
                                  dsp::dbToGain (p.outputDb),
                                  std::clamp (p.mix, 0.0f, 1.0f),
                                  std::clamp (p.softness, 0.0f, 1.0f),
                                  p.gainLink ? 1.0f / in : 1.0f };
        dsp::LinearSmoother* smoothers[] = { &inGain, &ceiling, &outGain, &mix, &softness, &linkComp };
        for (int i = 0; i < 6; ++i)
            immediate ? smoothers[i]->setImmediate (targets[i]) : smoothers[i]->setTarget (targets[i]);
    }

    int getLatencySamples() const
    {
        const auto& o = oversamplers[0u][(size_t) std::max (0, currentFactorIndex)];
        return o ? o->getLatencySamples() : 0;
    }

    // Processes numChannels buffers of n samples in place. n must not exceed the prepared block size;
    // longer host buffers are split.
    ClipMeters process (float* const* io, int numChannels, int n)
    {
        ClipMeters meters;
        int offset = 0;
        while (offset < n)
        {
            const int len = std::min (blockSize, n - offset);
            processChunk (io, std::min (numChannels, channels), offset, len, meters);
            offset += len;
        }
        return meters;
    }

private:
    void processChunk (float* const* io, int numChannels, int offset, int n, ClipMeters& meters)
    {
        // Snapshot smoother state so every channel sees the same ramp.
        std::array<dsp::LinearSmoother, 6> start { inGain, ceiling, outGain, mix, softness, linkComp };

        for (int c = 0; c < numChannels; ++c)
        {
            const auto ci = (size_t) c;
            auto sIn = start[0], sCeil = start[1], sOut = start[2], sMix = start[3], sSoft = start[4], sLink = start[5];
            float* x = io[c] + offset;
            auto& os = *oversamplers[ci][(size_t) currentFactorIndex];
            const int factor = os.getFactor();

            // Pre-gain at base rate (linear, so it commutes with the oversampling filters).
            for (int i = 0; i < n; ++i)
            {
                const float g = sIn.next();
                const float ce = sCeil.next();
                const float v = x[i] * g;
                meters.inputPeak = std::max (meters.inputPeak, std::fabs (v));
                pre[(size_t) i] = v / ce;
                wet[(size_t) i] = ce; // stash the ceiling ramp, reused after downsampling
            }

            float* hi = os.upsample (pre.data(), n);
            {
                float maxOver = 1.0f; // largest |v| above the ceiling; one log10 per block, not per sample
                for (int i = 0; i < n; ++i)
                {
                    const float soft = sSoft.next();
                    for (int k = 0; k < factor; ++k)
                    {
                        float& v = hi[i * factor + k];
                        maxOver = std::max (maxOver, std::fabs (v));
                        v = clipCurve (v, soft);
                    }
                }
                if (maxOver > 1.0f)
                    meters.clipDb = std::max (meters.clipDb, 20.0f * std::log10 (maxOver));
            }

            // Reuse 'pre' as the downsampled wet buffer; the ceiling ramp lives in 'wet'.
            os.downsample (hi, pre.data(), n);

            sIn = start[0];
            for (int i = 0; i < n; ++i)
            {
                const float g = sIn.next();
                const float ce = wet[(size_t) i];
                // Downsampling filters ring slightly above the ceiling. Strict mode hard-limits that
                // overshoot so the sample peak never crosses the ceiling, at the cost of some aliasing
                // when driven hard; off keeps the oversampled result untouched (cleaner, may peak a bit over).
                const float v = pre[(size_t) i];
                const float w = (params.strictCeiling ? std::clamp (v, -1.0f, 1.0f) : v) * ce;
                const float d = dryDelay[ci].process (x[i]);
                const float m = sMix.next();
                const float o = sOut.next();
                const float lc = sLink.next();

                float y;
                if (params.delta)
                {
                    y = o * (d * g - w) * lc;   // monitoring mode: not limited on purpose
                }
                else
                {
                    y = o * (m * w * lc + (1.0f - m) * d);
                    // The ceiling is a promise about what leaves the plugin, so it also covers the dry
                    // share of Mix and the Output gain. Dry material below the ceiling is untouched.
                    if (params.strictCeiling)
                        y = std::clamp (y, -ce, ce);
                }

                x[i] = y;
                meters.outputPeak = std::max (meters.outputPeak, std::fabs (y));
            }

            if (c == numChannels - 1)
            {
                inGain = sIn; ceiling = sCeil; outGain = sOut; mix = sMix; softness = sSoft; linkComp = sLink;
            }
        }

        if (numChannels == 0)
            for (auto* s : { &inGain, &ceiling, &outGain, &mix, &softness, &linkComp })
                for (int i = 0; i < n; ++i)
                    s->next();
    }

    ClipParams params;
    int channels = 2, blockSize = 512, currentFactorIndex = -1;
    std::array<std::array<std::unique_ptr<dsp::Oversampler>, 4>, (size_t) maxChannels> oversamplers;
    std::array<dsp::DelayLine, (size_t) maxChannels> dryDelay;
    std::vector<float> pre, wet;
    dsp::LinearSmoother inGain, ceiling, outGain, mix, softness, linkComp;
};

} // namespace rommuser::clipready
