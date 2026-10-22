// Band Push DSP engine: three-band upward + downward compressor ("OTT style").
//
// Pure C++ (no JUCE) so it is unit-tested headless in tests/.
//
// Signal flow (per channel, stereo-linked detection):
//   x * inGain -> LR4 three-band split (low / mid / high, flat sum)
//   per band: level (peak, 2 ms hold, dB)
//             target = depth * ( down * min (0, threshold - level)      downward: push loud parts down
//                              + up   * max (0, threshold - level) )    upward:   pull quiet parts up (capped)
//             gain   = ballistics (target) + band gain
//   wet = sum (band * gain) * outGain
//   out = mix * wet + (1 - mix) * sum (band)                             (dry is the split sum, so the
//                                                                        mix never phases against itself)
//
// With depth 0 every band's gain is exactly its band gain, and with band gains 0 the output is the
// all-pass sum of the bands: same magnitude as the input.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../../../shared/dsp/Crossover.h"
#include "../../../shared/dsp/Smoothing.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace rommuser::bandpush
{

constexpr int numBands = 3;
constexpr float maxUpwardDb = 24.0f;   // never pull silence up by more than this

struct BandSettings
{
    float up = 1.0f;      // 0 .. 1  (0 = no upward compression, 1 = fully pulled to the threshold)
    float down = 1.0f;    // 0 .. 1
    float gainDb = 0.0f;  // -24 .. 24
};

struct PushParams
{
    float depth = 0.5f;          // 0 .. 1, scales the whole dynamic action
    float time = 1.0f;           // 0.1 .. 10, scales attack (15 ms) and release (150 ms)
    float thresholdDb = -24.0f;  // -48 .. 0, the level every band is pushed towards
    float inDb = 0.0f;           // -24 .. 24
    float outDb = 0.0f;          // -24 .. 24
    float mix = 1.0f;            // 0 .. 1
    float lowXoverHz = 120.0f;   // 40 .. 1000
    float highXoverHz = 2500.0f; // 500 .. 12000
    std::array<BandSettings, numBands> bands {};
};

struct PushMeters
{
    std::array<float, numBands> bandGainDb { 0.0f, 0.0f, 0.0f };  // signed, last sample of the block
    float outputPeak = 0.0f;
};

// Static gain (dB) a band gets at a steady level, before ballistics. Shared with the editor.
inline float staticGainDb (float levelDb, const PushParams& p, int band)
{
    const auto& b = p.bands[(size_t) band];
    const float diff = p.thresholdDb - levelDb;   // > 0: quiet (pull up), < 0: loud (push down)
    const float dyn = diff > 0.0f ? std::min (b.up * diff, maxUpwardDb) : b.down * diff;
    return p.depth * dyn;
}

class PushEngine
{
public:
    static constexpr int maxChannels = 2;
    static constexpr float attackBaseMs = 15.0f, releaseBaseMs = 150.0f;
    // Peak hold per band: long enough to bridge the gaps between the peaks of the lowest tone in the band
    // (a 50 Hz kick has 20 ms between peaks), short in the highs so transients are tracked closely.
    static constexpr float peakHoldMs[numBands] = { 20.0f, 5.0f, 2.0f };

    void prepare (double newSampleRate, int numChannels)
    {
        sampleRate = newSampleRate;
        channels = std::clamp (numChannels, 1, maxChannels);
        for (auto& s : splitters) s.prepare (sampleRate);
        inGain.prepare (sampleRate, 0.02);
        outGain.prepare (sampleRate, 0.02);
        mixSmooth.prepare (sampleRate, 0.02);
        for (auto& g : bandGain) g.prepare (sampleRate, 0.02);
        for (int b = 0; b < numBands; ++b)
        {
            holdSamples[b] = (int) std::lround (sampleRate * peakHoldMs[b] / 1000.0);
            peakDecayCoef[b] = coefFor (peakHoldMs[b]);
        }
        setParams (params, true);
        reset();
    }

    void reset()
    {
        for (auto& s : splitters) s.reset();
        for (auto& b : bandState) b = {};
    }

    void setParams (const PushParams& p, bool immediate = false)
    {
        const bool xoverChanged = immediate || std::fabs (p.lowXoverHz - params.lowXoverHz) > 0.0f
                                            || std::fabs (p.highXoverHz - params.highXoverHz) > 0.0f;
        params = p;
        const float t = std::clamp (p.time, 0.05f, 20.0f);
        attackCoef = coefFor (attackBaseMs * t);
        releaseCoef = coefFor (releaseBaseMs * t);
        if (xoverChanged)
            for (auto& s : splitters) s.setFrequencies (p.lowXoverHz, p.highXoverHz);

        auto set = [immediate] (dsp::LinearSmoother& s, float v) { immediate ? s.setImmediate (v) : s.setTarget (v); };
        set (inGain, dsp::dbToGain (p.inDb));
        set (outGain, dsp::dbToGain (p.outDb));
        set (mixSmooth, p.mix);
        for (int b = 0; b < numBands; ++b)
            set (bandGain[(size_t) b], dsp::dbToGain (p.bands[(size_t) b].gainDb));
    }

    const PushParams& getParams() const { return params; }

    PushMeters process (float* const* io, int numChannels, int n)
    {
        PushMeters meters;
        numChannels = std::clamp (numChannels, 1, channels);

        for (int i = 0; i < n; ++i)
        {
            const float gIn = inGain.next(), gOut = outGain.next(), m = mixSmooth.next();
            float bands[maxChannels][numBands];
            float key[numBands] = { 0.0f, 0.0f, 0.0f };

            for (int c = 0; c < numChannels; ++c)
            {
                auto& bb = bands[c];
                splitters[(size_t) c].process (io[c][i] * gIn, bb[0], bb[1], bb[2]);
                for (int b = 0; b < numBands; ++b)
                    key[b] = std::max (key[b], std::fabs (bb[b]));
            }

            float gain[numBands];
            for (int b = 0; b < numBands; ++b)
            {
                auto& st = bandState[(size_t) b];
                // Peak detector with a short hold (see Press Ready for why).
                if (key[b] >= st.peak) { st.peak = key[b]; st.holdLeft = holdSamples[b]; }
                else if (st.holdLeft > 0) --st.holdLeft;
                else st.peak += peakDecayCoef[b] * (key[b] - st.peak);
                const float levelDb = 20.0f * std::log10 (std::max (st.peak, 1e-6f));

                const float target = staticGainDb (levelDb, params, b);
                // Attack when the gain is going down (loud transient, or a quiet part ending), release when it comes back up.
                st.gainDb += (target < st.gainDb ? attackCoef : releaseCoef) * (target - st.gainDb);
                gain[b] = dsp::dbToGain (st.gainDb) * bandGain[(size_t) b].next();
                meters.bandGainDb[(size_t) b] = st.gainDb;
            }

            for (int c = 0; c < numChannels; ++c)
            {
                const auto& bb = bands[c];
                const float dry = bb[0] + bb[1] + bb[2];
                const float wet = (bb[0] * gain[0] + bb[1] * gain[1] + bb[2] * gain[2]) * gOut;
                const float y = m * wet + (1.0f - m) * dry;
                io[c][i] = y;
                meters.outputPeak = std::max (meters.outputPeak, std::fabs (y));
            }
        }
        return meters;
    }

private:
    float coefFor (float ms) const
    {
        if (ms <= 0.0f) return 1.0f;
        return 1.0f - std::exp (-1.0f / (float) (sampleRate * ms / 1000.0));
    }

    struct BandState
    {
        float peak = 0.0f, gainDb = 0.0f;
        int holdLeft = 0;
    };

    PushParams params;
    double sampleRate = 48000.0;
    int channels = 2;
    int holdSamples[numBands] = { 0, 0, 0 };
    float attackCoef = 1.0f, releaseCoef = 1.0f;
    float peakDecayCoef[numBands] = { 1.0f, 1.0f, 1.0f };
    std::array<dsp::ThreeBandSplitter, maxChannels> splitters;
    std::array<BandState, numBands> bandState;
    dsp::LinearSmoother inGain, outGain, mixSmooth;
    std::array<dsp::LinearSmoother, numBands> bandGain;
};

} // namespace rommuser::bandpush
