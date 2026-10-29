// Peak Ready DSP engine: true-peak lookahead limiter with adaptive release and a loudness meter.
//
// Pure C++ (no JUCE) so it is unit-tested headless in tests/.
//
// Per sample (all channels share one gain so the stereo image never moves):
//   x  = in * inputGain                       (Aggressive style: soft-clipped at the ceiling first)
//   tp = true peak of x (4x oversampled, max over channels) or the sample peak when True Peak is off
//   gReq = min (1, ceiling / tp)
//   m    = sliding minimum of gReq over lookahead + 1 samples
//   a    = two cascaded moving averages of m (lengths summing to lookahead + 1): a smooth S-shaped
//          ramp that reaches gReq exactly when the delayed peak arrives
//   env  = a if a < env, else env + aRel * (a - env)      (release; never above a, so never above ceiling)
//   y    = delay (x, latency) * env
//
// The release time adapts to the average reduction (Auto): isolated peaks recover fast, sustained
// limiting releases slower so the low end does not distort.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../../../shared/dsp/FractionalDelay.h"
#include "../../../shared/dsp/Loudness.h"
#include "../../../shared/dsp/Oversampler.h"
#include "../../../shared/dsp/Smoothing.h"
#include "../../ClipReady/Source/ClipEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <vector>

namespace rommuser::peakready
{

enum class Style { transparent = 0, punchy, aggressive };

struct PeakParams
{
    float inputDb = 0.0f;        // -12 .. +24
    float ceilingDb = -1.0f;     // -12 .. 0
    float releaseMs = 200.0f;    // 10 .. 1000
    bool autoRelease = true;     // program-dependent release
    Style style = Style::transparent;
    bool truePeak = true;        // 4x oversampled detection
};

struct PeakMeters
{
    float gainReductionDb = 0.0f;   // largest reduction in the block (positive dB)
    float inputPeak = 0.0f;         // linear, after input gain (sample peak)
    float outputPeak = 0.0f;        // linear sample peak
    float momentaryLufs = -120.0f, shortTermLufs = -120.0f, integratedLufs = -120.0f;
};

inline constexpr int maxChannels = 2;
inline constexpr int oversampling = 4;
inline constexpr float maxLookaheadMs = 5.0f;

inline float lookaheadMsFor (Style s) { return s == Style::transparent ? 4.0f : s == Style::punchy ? 1.5f : 0.5f; }

// Sliding minimum over a fixed window using a monotonic queue (O(1) amortised per sample).
class SlidingMin
{
public:
    void prepare (int windowLength)
    {
        window = std::max (1, windowLength);
        values.assign ((size_t) window + 1, 1.0f);
        indices.assign ((size_t) window + 1, 0);
        reset();
    }

    void reset()
    {
        front = back = 0;
        count = 0;
        n = 0;
    }

    inline float push (float v)
    {
        // Drop larger values from the back: they can never be the minimum while v is in the window.
        while (count > 0 && values[(size_t) ((back + (int) values.size() - 1) % (int) values.size())] >= v)
        {
            back = (back + (int) values.size() - 1) % (int) values.size();
            --count;
        }
        values[(size_t) back] = v;
        indices[(size_t) back] = n;
        back = (back + 1) % (int) values.size();
        ++count;
        // Drop the front once it leaves the window.
        if (indices[(size_t) front] <= n - window)
        {
            front = (front + 1) % (int) values.size();
            --count;
        }
        ++n;
        return values[(size_t) front];
    }

private:
    std::vector<float> values;
    std::vector<long long> indices;
    int window = 1, front = 0, back = 0, count = 0;
    long long n = 0;
};

class PeakEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize, int numChannels)
    {
        sr = sampleRate;
        channels = std::clamp (numChannels, 1, maxChannels);
        maxBlock = std::max (1, maxBlockSize);
        maxLookahead = std::max (1, (int) std::lround (maxLookaheadMs * 0.001 * sr));

        for (int c = 0; c < maxChannels; ++c)
        {
            oversamplers[(size_t) c] = std::make_unique<dsp::Oversampler> (maxBlock);
            oversamplers[(size_t) c]->prepare (oversampling, maxBlock);
        }
        measureUpsamplerDelay();

        const int maxDelay = maxLookahead + detectorDelay + 2;
        for (auto& d : dryDelay) d.setMaxDelay (maxDelay);
        for (auto& r : hiRing) r.assign ((size_t) ringSize, 0.0f);
        gReqBuf.assign ((size_t) maxBlock, 1.0f);
        inGain.prepare (sr, 0.02);
        loudness.prepare (sr, channels);

        setParams (params, true);
        firstSet = true;
        reset();
    }

    void reset()
    {
        for (auto& o : oversamplers) if (o) o->reset();
        for (auto& d : dryDelay) d.reset();
        for (auto& r : hiRing) std::fill (r.begin(), r.end(), 0.0f);
        ringPos = 0;
        slidingMin.reset();
        avgBuf.assign ((size_t) std::max (1, avgLen1), 1.0f);
        avgSum = (double) avgBuf.size();
        avgPos = 0;
        avg2Buf.assign ((size_t) std::max (1, avgLen2), 1.0f);
        avg2Sum = (double) avg2Buf.size();
        avg2Pos = 0;
        std::fill (gReqHist.begin(), gReqHist.end(), 1.0f);
        histPos = 0;
        env = 1.0f;
        grAvgDb = 0.0f;
        loudness.reset();
    }

    void setParams (const PeakParams& p, bool immediate = false)
    {
        immediate = immediate || firstSet;
        firstSet = false;

        const int newLookahead = std::clamp ((int) std::lround (lookaheadMsFor (p.style) * 0.001 * sr), 1, maxLookahead);
        if (immediate || newLookahead != lookahead || p.truePeak != params.truePeak)
        {
            lookahead = newLookahead;
            slidingMin.prepare (lookahead + 1);
            // Two moving averages whose lengths add up to lookahead + 1 turn the attack into a
            // smooth S-shaped (trapezoid-window) ramp that still reaches the required gain exactly
            // when the delayed peak arrives: the second average only sees minima of windows that
            // contain the peak. A single average gives a linear ramp with a corner at its start,
            // and that corner is heard as a click / intermodulation on dense material.
            avgLen1 = (lookahead + 2) / 2;
            avgLen2 = lookahead + 1 - avgLen1;
            avgBuf.assign ((size_t) avgLen1, 1.0f);
            avgSum = (double) avgLen1;
            avgPos = 0;
            avg2Buf.assign ((size_t) avgLen2, 1.0f);
            avg2Sum = (double) avgLen2;
            avg2Pos = 0;
            gReqHist.assign ((size_t) lookahead + 1, 1.0f);
            histPos = 0;
            latency = lookahead + (p.truePeak ? detectorDelay : 0);
        }

        ceiling = dsp::dbToGain (p.ceilingDb);
        const float in = dsp::dbToGain (p.inputDb);
        immediate ? inGain.setImmediate (in) : inGain.setTarget (in);
        releaseBase = (float) p.releaseMs;
        params = p;
    }

    const PeakParams& getParams() const { return params; }
    int getLatencySamples() const { return latency; }
    void resetLoudness() { loudness.reset(); }

    PeakMeters process (float* const* io, int numChannels, int n)
    {
        PeakMeters m;
        const int nch = std::clamp (numChannels, 1, maxChannels);
        const bool tpMode = params.truePeak;
        const float preClipInv = 1.0f / ceiling;

        // Input gain (and the Aggressive pre-clip) in place, then the required gain per sample.
        for (int i = 0; i < n; ++i)
        {
            const float g = inGain.next();
            float samplePeak = 0.0f;
            for (int c = 0; c < nch; ++c)
            {
                float x = io[c][i] * g;
                if (params.style == Style::aggressive)
                    x = clipready::clipCurve (x * preClipInv, 0.3f) * ceiling;
                io[c][i] = x;
                samplePeak = std::max (samplePeak, std::fabs (x));
            }
            m.inputPeak = std::max (m.inputPeak, samplePeak);
            gReqBuf[(size_t) i] = samplePeak;   // sample peak for now; true-peak pass below replaces it
        }

        if (tpMode)
        {
            // Upsample every channel; keep the high-rate samples in a ring so the window around the
            // (delayed) base sample is available.
            std::array<float*, maxChannels> hi {};
            for (int c = 0; c < nch; ++c)
                hi[(size_t) c] = oversamplers[(size_t) c]->upsample (io[c], n);

            for (int i = 0; i < n; ++i)
            {
                for (int k = 0; k < oversampling; ++k)
                {
                    float v = 0.0f;
                    for (int c = 0; c < nch; ++c)
                        v = std::max (v, std::fabs (hi[(size_t) c][i * oversampling + k]));
                    hiRing[0][(size_t) ringPos] = v;
                    ringPos = (ringPos + 1) % ringSize;
                }
                // The base sample we can now evaluate is detectorDelay samples ago; its high-rate
                // window ends windowOffset samples before the one just written.
                float tp = 0.0f;
                for (int k = 0; k < windowLength; ++k)
                    tp = std::max (tp, hiRing[0][(size_t) ((ringPos - 1 - windowOffset - k + ringSize) % ringSize)]);
                gReqBuf[(size_t) i] = tp;
            }
        }

        // Gain path and output.
        const bool autoRel = params.autoRelease;
        for (int i = 0; i < n; ++i)
        {
            // Dry audio delayed by the full latency; its own sample peak joins the detector so the
            // sample peak can never pass the ceiling either.
            float delayedPeak = 0.0f;
            std::array<float, maxChannels> dry {};
            for (int c = 0; c < nch; ++c)
            {
                auto& d = dryDelay[(size_t) c];
                d.push (io[c][i]);
                dry[(size_t) c] = d.readInteger (latency);
            }
            if (tpMode)
            {
                // gReqBuf holds the peak for the sample `detectorDelay` ago; the dry signal is a further
                // `lookahead` behind, which is exactly what the lookahead stage needs.
                for (int c = 0; c < nch; ++c)
                    delayedPeak = std::max (delayedPeak, std::fabs (dryDelay[(size_t) c].readInteger (detectorDelay)));
            }
            const float peak = std::max (gReqBuf[(size_t) i], delayedPeak);
            const float gReq = peak > ceiling ? ceiling / peak : 1.0f;

            const float mn = slidingMin.push (gReq);
            avgSum += (double) mn - (double) avgBuf[(size_t) avgPos];
            avgBuf[(size_t) avgPos] = mn;
            if (++avgPos == avgLen1) avgPos = 0;
            const float a1 = (float) (avgSum / (double) avgLen1);
            avg2Sum += (double) a1 - (double) avg2Buf[(size_t) avg2Pos];
            avg2Buf[(size_t) avg2Pos] = a1;
            if (++avg2Pos == avgLen2) avg2Pos = 0;
            // Rounding guard: never above the gain the sample leaving now needs. (This used to be the
            // sliding minimum itself, which already contains the newest peak, so the gain stepped
            // down to the full reduction `lookahead` samples early instead of ramping.)
            gReqHist[(size_t) histPos] = gReq;
            if (++histPos == lookahead + 1) histPos = 0;
            const float due = gReqHist[(size_t) histPos]; // gReq of lookahead samples ago
            const float a = std::min (due, (float) (avg2Sum / (double) avgLen2));

            if ((i & 15) == 0)
            {
                const float scale = autoRel ? std::clamp (0.5f + grAvgDb / 4.0f, 0.5f, 4.0f) : 1.0f;
                aRelease = (float) (1.0 - std::exp (-1.0 / (std::max (1.0f, releaseBase * scale) * 0.001 * sr)));
            }
            env = a < env ? a : env + aRelease * (a - env);
            const float grDb = env < 1.0f ? -20.0f * std::log10 (env) : 0.0f;
            grAvgDb += (grDb - grAvgDb) * grAvgCoeff;
            m.gainReductionDb = std::max (m.gainReductionDb, grDb);

            for (int c = 0; c < nch; ++c)
            {
                float y = dry[(size_t) c] * env;
                y = std::clamp (y, -ceiling, ceiling);   // sample-domain safety net
                io[c][i] = y;
                m.outputPeak = std::max (m.outputPeak, std::fabs (y));
            }
        }

        loudness.process (io, nch, n);
        m.momentaryLufs = loudness.getMomentary();
        m.shortTermLufs = loudness.getShortTerm();
        m.integratedLufs = loudness.getIntegrated();
        return m;
    }

private:
    // Group delay of the upsampling path, found by sending an impulse through it.
    void measureUpsamplerDelay()
    {
        auto& os = *oversamplers[0];
        os.reset();
        std::vector<float> impulse ((size_t) std::min (maxBlock, 256), 0.0f);
        impulse[0] = 1.0f;
        int bestIndex = 0;
        float best = 0.0f;
        int offset = 0;
        for (int pass = 0; pass < 4; ++pass)
        {
            const float* hi = os.upsample (impulse.data(), (int) impulse.size());
            for (int j = 0; j < (int) impulse.size() * oversampling; ++j)
                if (std::fabs (hi[j]) > best) { best = std::fabs (hi[j]); bestIndex = offset + j; }
            std::fill (impulse.begin(), impulse.end(), 0.0f);
            offset += (int) impulse.size() * oversampling;
        }
        os.reset();
        // High-rate samples belonging to base sample t sit at 4t + p .. 4t + p + 3. We look at a window
        // of windowLength high-rate samples [4t + p - 2, 4t + p + 5] (half a base sample of margin on
        // each side). After processing base sample t_i the newest high-rate index is 4 t_i + 3, so the
        // window of t = t_i - D is complete once 4t + p + 5 <= 4 t_i + 3, i.e. D = ceil ((p + 2) / 4),
        // and it ends 4D - p - 2 samples before the newest one.
        const int p = bestIndex;
        detectorDelay = (p + 2 + oversampling - 1) / oversampling;
        windowOffset = oversampling * detectorDelay - p - 2;
        windowLength = oversampling + 4;
        grAvgCoeff = (float) (1.0 - std::exp (-1.0 / (1.0 * sr)));
    }

    static constexpr int ringSize = 1024;

    double sr = 48000.0;
    int channels = 2, maxBlock = 512, maxLookahead = 240, lookahead = 192, latency = 0;
    int detectorDelay = 0, windowLength = 8, windowOffset = 0;
    bool firstSet = true;
    PeakParams params;
    float ceiling = 0.891f, releaseBase = 200.0f, aRelease = 0.001f, env = 1.0f, grAvgDb = 0.0f, grAvgCoeff = 0.0001f;

    std::array<std::unique_ptr<dsp::Oversampler>, maxChannels> oversamplers;
    std::array<dsp::FractionalDelay, maxChannels> dryDelay;
    std::array<std::vector<float>, 1> hiRing;
    int ringPos = 0;
    std::vector<float> gReqBuf;
    SlidingMin slidingMin;
    std::vector<float> avgBuf, avg2Buf;
    double avgSum = 0.0, avg2Sum = 0.0;
    int avgPos = 0, avg2Pos = 0, avgLen1 = 1, avgLen2 = 1;
    std::vector<float> gReqHist { 1.0f };
    int histPos = 0;
    dsp::LinearSmoother inGain;
    dsp::LoudnessMeter loudness;
};

} // namespace rommuser::peakready
