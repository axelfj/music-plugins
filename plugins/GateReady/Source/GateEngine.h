// Gate Ready DSP engine: gate / expander with hysteresis, hold, lookahead and a filtered sidechain.
//
// Pure C++ (no JUCE) so it is unit-tested headless in tests/.
//
// Signal flow (stereo-linked):
//   key   = external ? sidechain : input  -> HPF -> LPF -> peak level (dB, 2 ms hold)
//   state = open  if level > threshold
//           closed if level < threshold - hysteresis            (in between: keep the last state)
//   target = gainComputer (level, effectiveThreshold, ratio, knee, range)   effective threshold is
//            threshold when closed and threshold - hysteresis when open, so an open gate does not chatter
//   gain  = ballistics (target): attack when opening, hold then release when closing
//   out   = delay (input, lookahead) * gain          (or the filtered key when "listen" is on)
//
// Below the threshold an expander turns the signal down by (threshold - level) * (ratio - 1) dB, never
// more than |range|; ratio >= infiniteRatio is a hard gate that jumps straight to range. Above the
// threshold the gain is exactly 0 dB (bit-exact pass-through once the attack has settled).
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../../../shared/dsp/Biquad.h"
#include "../../../shared/dsp/Oversampler.h" // DelayLine
#include "../../../shared/dsp/Smoothing.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace rommuser::gateready
{

constexpr float infiniteRatio = 20.0f;

struct GateParams
{
    float thresholdDb = -40.0f;   // -80 .. 0
    float ratio = infiniteRatio;  // 1 .. 20 (>= 20 = hard gate)
    float attackMs = 1.0f;        // 0.1 .. 100
    float holdMs = 50.0f;         // 0 .. 1000
    float releaseMs = 100.0f;     // 1 .. 3000
    float rangeDb = -80.0f;       // -100 .. 0 (maximum attenuation)
    float kneeDb = 0.0f;          // 0 .. 24
    float hysteresisDb = 3.0f;    // 0 .. 12
    float lookaheadMs = 0.0f;     // 0 .. 10
    float sidechainHpfHz = 0.0f;  // 0 = off, else 20 .. 2000
    float sidechainLpfHz = 0.0f;  // 0 = off, else 200 .. 20000
    bool externalSidechain = false;
    bool listen = false;          // output the filtered key instead of the gated audio
};

struct GateMeters
{
    float inputPeak = 0.0f, outputPeak = 0.0f;
    float gainReductionDb = 0.0f;  // positive, max over the block
    float levelDb = -120.0f;       // detector level at the end of the block
    bool open = false;
};

// Static gain (dB, <= 0) for a level, with a soft knee of kneeDb centred on the threshold and a floor at rangeDb.
inline float gainComputerDb (float levelDb, float thresholdDb, float ratio, float kneeDb, float rangeDb)
{
    const float under = thresholdDb - levelDb;   // > 0 below the threshold
    float g;
    if (ratio >= infiniteRatio)
    {
        if (kneeDb <= 0.0f || under <= -kneeDb * 0.5f) g = under > 0.0f ? rangeDb : 0.0f;
        else if (under >= kneeDb * 0.5f) g = rangeDb;
        else g = rangeDb * (under + kneeDb * 0.5f) / kneeDb;  // linear blend through the knee
    }
    else
    {
        const float slope = std::max (ratio, 1.0f) - 1.0f;
        if (kneeDb <= 0.0f || under <= -kneeDb * 0.5f) g = under > 0.0f ? -slope * under : 0.0f;
        else if (under >= kneeDb * 0.5f) g = -slope * under;
        else
        {
            const float t = under + kneeDb * 0.5f;
            g = -slope * t * t / (2.0f * kneeDb);
        }
    }
    return std::max (g, rangeDb);
}

class GateEngine
{
public:
    static constexpr int maxChannels = 2;
    static constexpr float maxLookaheadMs = 10.0f;
    static constexpr float peakHoldMs = 2.0f;

    void prepare (double newSampleRate, int numChannels)
    {
        sampleRate = newSampleRate;
        channels = std::clamp (numChannels, 1, maxChannels);
        const int maxLa = (int) std::ceil (sampleRate * maxLookaheadMs / 1000.0) + 1;
        for (auto& d : delays) d.setMaxDelay (maxLa);
        holdSamplesPeak = (int) std::lround (sampleRate * peakHoldMs / 1000.0);
        peakDecayCoef = coefFor (peakHoldMs);
        setParams (params, true);
        reset();
    }

    void reset()
    {
        gainDb = params.rangeDb;
        open = false;
        holdLeft = 0;
        peakLevel = 0.0f;
        peakHoldLeft = 0;
        for (auto& d : delays) d.reset();
        for (auto& f : hpf) f.reset();
        for (auto& f : lpf) f.reset();
    }

    void setParams (const GateParams& p, bool /*immediate*/ = false)
    {
        const bool filtersChanged = std::fabs (p.sidechainHpfHz - params.sidechainHpfHz) > 0.0f
                                 || std::fabs (p.sidechainLpfHz - params.sidechainLpfHz) > 0.0f
                                 || ! prepared;
        params = p;
        prepared = true;
        attackCoef = coefFor (std::max (p.attackMs, 0.01f));
        releaseCoef = coefFor (std::max (p.releaseMs, 0.1f));
        holdSamples = (int) std::lround (sampleRate * std::max (p.holdMs, 0.0f) / 1000.0);

        const int la = std::clamp ((int) std::lround (sampleRate * p.lookaheadMs / 1000.0), 0,
                                   (int) std::ceil (sampleRate * maxLookaheadMs / 1000.0));
        if (la != lookaheadSamples)
        {
            lookaheadSamples = la;
            for (auto& d : delays) d.setDelay (la);
        }
        if (filtersChanged)
        {
            if (p.sidechainHpfHz > 0.0f)
            {
                const auto c = dsp::biquad::highPass (p.sidechainHpfHz, 0.7071, sampleRate);
                for (auto& f : hpf) f.setCoeffs (c);
            }
            if (p.sidechainLpfHz > 0.0f)
            {
                const auto c = dsp::biquad::lowPass (p.sidechainLpfHz, 0.7071, sampleRate);
                for (auto& f : lpf) f.setCoeffs (c);
            }
        }
    }

    const GateParams& getParams() const { return params; }
    int getLatencySamples() const { return lookaheadSamples; }
    float getGainDb() const { return gainDb; }
    bool isOpen() const { return open; }

    GateMeters process (float* const* io, const float* const* sidechain, int numChannels, int n)
    {
        GateMeters meters;
        numChannels = std::clamp (numChannels, 1, channels);
        const bool useExt = params.externalSidechain && sidechain != nullptr;
        const bool useHpf = params.sidechainHpfHz > 0.0f, useLpf = params.sidechainLpfHz > 0.0f;

        for (int i = 0; i < n; ++i)
        {
            float key = 0.0f;
            float filtered[maxChannels] = {};
            for (int c = 0; c < numChannels; ++c)
            {
                float s = useExt ? sidechain[c][i] : io[c][i];
                if (useHpf) s = hpf[(size_t) c].process (s);
                if (useLpf) s = lpf[(size_t) c].process (s);
                filtered[c] = s;
                key = std::max (key, std::fabs (s));
                meters.inputPeak = std::max (meters.inputPeak, std::fabs (io[c][i]));
            }

            // Peak detector with a short hold.
            if (key >= peakLevel) { peakLevel = key; peakHoldLeft = holdSamplesPeak; }
            else if (peakHoldLeft > 0) --peakHoldLeft;
            else peakLevel += peakDecayCoef * (key - peakLevel);
            const float levelDb = 20.0f * std::log10 (std::max (peakLevel, 1e-7f));

            // Hysteresis state.
            if (levelDb > params.thresholdDb) open = true;
            else if (levelDb < params.thresholdDb - params.hysteresisDb) open = false;
            const float effThreshold = open ? params.thresholdDb - params.hysteresisDb : params.thresholdDb;
            const float target = gainComputerDb (levelDb, effThreshold, params.ratio, params.kneeDb, params.rangeDb);

            // Ballistics: attack when opening; when closing, hold first, then release.
            if (target >= gainDb)
            {
                gainDb += attackCoef * (target - gainDb);
                holdLeft = holdSamples;
            }
            else if (holdLeft > 0)
            {
                --holdLeft;
            }
            else
            {
                gainDb += releaseCoef * (target - gainDb);
            }
            if (gainDb > -1.0e-4f) gainDb = 0.0f;  // settle to exactly unity when open

            const float gain = gainDb >= 0.0f ? 1.0f : dsp::dbToGain (gainDb);
            meters.gainReductionDb = std::max (meters.gainReductionDb, -gainDb);
            meters.levelDb = levelDb;

            for (int c = 0; c < numChannels; ++c)
            {
                const float delayed = delays[(size_t) c].process (io[c][i]);
                const float y = params.listen ? filtered[c] : delayed * gain;
                io[c][i] = y;
                meters.outputPeak = std::max (meters.outputPeak, std::fabs (y));
            }
        }
        meters.open = open;
        return meters;
    }

private:
    float coefFor (float ms) const
    {
        if (ms <= 0.0f) return 1.0f;
        return 1.0f - std::exp (-1.0f / (float) (sampleRate * ms / 1000.0));
    }

    GateParams params;
    bool prepared = false;
    double sampleRate = 48000.0;
    int channels = 2, lookaheadSamples = 0;
    float attackCoef = 1.0f, releaseCoef = 1.0f, peakDecayCoef = 1.0f;
    int holdSamples = 0, holdLeft = 0, holdSamplesPeak = 0, peakHoldLeft = 0;
    float gainDb = -80.0f, peakLevel = 0.0f;
    bool open = false;
    std::array<dsp::DelayLine, maxChannels> delays;
    std::array<dsp::Biquad, maxChannels> hpf, lpf;
};

} // namespace rommuser::gateready
