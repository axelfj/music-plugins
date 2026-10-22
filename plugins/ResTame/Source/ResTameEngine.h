// Res Tame: dynamic resonance suppressor. STFT (Hann, 75 % overlap); per bin, the level that sticks
// out above a smoothed spectral envelope (selectivity = envelope width) is treated as a resonance
// and attenuated with attack / release, up to Depth dB. Optional external sidechain ("unmask":
// cut where the other track has its peaks), Mid/Side placement, frequency range and Delta.
//
// Pure C++ (no JUCE) so the DSP is tested headless.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "dsp/FFT.h"
#include "dsp/Smoothing.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <vector>

namespace rommuser::restame
{

enum class Placement { stereo = 0, mid = 1, side = 2 };

struct TameParams
{
    float depthDb = 8.0f;        // 0..24 maximum reduction
    float sharpness = 0.7f;      // 0..1: how narrow the cuts stay (1 = one bin each side)
    float selectivity = 0.5f;    // 0..1: envelope width 1.0 .. 0.15 octaves (narrow = only sharp peaks)
    float thresholdDb = 6.0f;    // a bin must exceed its envelope by this much (2..12)
    float attackMs = 20.0f;
    float releaseMs = 200.0f;
    float lowHz = 80.0f;
    float highHz = 16000.0f;
    Placement placement = Placement::stereo;
    bool externalSidechain = false;
    bool delta = false;
};

struct TameMeters
{
    float maxReductionDb = 0.0f;
    float meanReductionDb = 0.0f;
};

class TameEngine
{
public:
    static constexpr int maxChannels = 2;
    static constexpr int order = 11; // 2048 at 44.1 / 48 kHz
    static constexpr int kneeDb = 4;  // excess above the threshold that reaches the full depth

    void prepare (double newSampleRate, int numChannels)
    {
        sampleRate = newSampleRate;
        channels = std::clamp (numChannels, 1, maxChannels);
        fftOrder = sampleRate > 60000.0 ? order + 1 : order;
        fftSize = 1 << fftOrder;
        hop = fftSize / 4;
        bins = fftSize / 2 + 1;
        fft.prepare (fftOrder);

        window.resize ((size_t) fftSize);
        for (int i = 0; i < fftSize; ++i)
            window[(size_t) i] = 0.5f - 0.5f * std::cos (6.283185307179586f * (float) i / (float) fftSize);

        for (auto& c : chans)
        {
            c.input.assign ((size_t) fftSize, 0.0f);
            c.output.assign ((size_t) fftSize * 2, 0.0f);
            c.dryDelay.assign ((size_t) fftSize, 0.0f);
            c.spectrum.assign ((size_t) fftSize, {});
            c.scInput.assign ((size_t) fftSize, 0.0f);
            c.scSpectrum.assign ((size_t) fftSize, {});
        }
        detectorPower.assign ((size_t) bins, 0.0f);
        gainDb.assign ((size_t) bins, 0.0f);
        envelopeDb.assign ((size_t) bins, 0.0f);
        magDb.assign ((size_t) bins, 0.0f);
        targetDb.assign ((size_t) bins, 0.0f);
        prefix.assign ((size_t) bins + 1, 0.0);
        reset();
        setParams (params);
    }

    void reset()
    {
        for (auto& c : chans)
        {
            std::fill (c.input.begin(), c.input.end(), 0.0f);
            std::fill (c.output.begin(), c.output.end(), 0.0f);
            std::fill (c.dryDelay.begin(), c.dryDelay.end(), 0.0f);
            std::fill (c.scInput.begin(), c.scInput.end(), 0.0f);
        }
        std::fill (detectorPower.begin(), detectorPower.end(), 0.0f);
        std::fill (gainDb.begin(), gainDb.end(), 0.0f);
        writePos = 0;
        hopCounter = 0;
        outReadPos = 0;
        lastMeters = {};
    }

    void setParams (const TameParams& p)
    {
        params = p;
        const double frameSeconds = (double) hop / sampleRate;
        attackCoef = 1.0f - (float) std::exp (-frameSeconds / std::max (0.001, p.attackMs * 0.001));
        releaseCoef = 1.0f - (float) std::exp (-frameSeconds / std::max (0.001, p.releaseMs * 0.001));
        detectorCoef = 1.0f - (float) std::exp (-frameSeconds / 0.05);
        const float octaves = 1.0f - 0.85f * std::clamp (p.selectivity, 0.0f, 1.0f); // 1.0 .. 0.15
        halfWidthRatio = std::exp2 (octaves * 0.5f) - 1.0f;
        dilation = std::clamp ((int) std::lround (1.0f + 3.0f * (1.0f - std::clamp (p.sharpness, 0.0f, 1.0f))), 1, 4);
        lowBin = (int) std::floor (p.lowHz * fftSize / sampleRate);
        highBin = (int) std::ceil (p.highHz * fftSize / sampleRate);
    }

    const TameParams& getParams() const { return params; }
    int getLatencySamples() const { return fftSize; }
    int getNumBins() const { return bins; }
    float getBinGainDb (int k) const { return -gainDb[(size_t) std::clamp (k, 0, bins - 1)]; }
    double getBinFrequency (int k) const { return k * sampleRate / fftSize; }

    // io: main channels (in place). sidechain: may be nullptr (then the main signal is the detector).
    TameMeters process (float* const* io, const float* const* sidechain, int numChannels, int n)
    {
        const int nch = std::min (numChannels, channels);
        for (int i = 0; i < n; ++i)
        {
            for (int ch = 0; ch < nch; ++ch)
            {
                auto& c = chans[(size_t) ch];
                const float x = io[ch][i];
                const float dry = c.dryDelay[(size_t) writePos]; // the sample written fftSize samples ago
                c.input[(size_t) writePos] = x;
                c.scInput[(size_t) writePos] = sidechain != nullptr && sidechain[ch] != nullptr ? sidechain[ch][i] : x;
                c.dryDelay[(size_t) writePos] = x;
                // Output: the overlap-added sample that is now complete (fftSize samples behind).
                const size_t outIdx = (size_t) outReadPos;
                const float wet = c.output[outIdx] * overlapNorm;
                c.output[outIdx] = 0.0f;
                io[ch][i] = params.delta ? dry - wet : wet;
            }
            writePos = (writePos + 1) % fftSize;
            outReadPos = (outReadPos + 1) % (fftSize * 2);
            if (++hopCounter >= hop)
            {
                hopCounter = 0;
                processFrame (nch);
            }
        }
        return lastMeters;
    }

private:
    struct Channel
    {
        std::vector<float> input, output, dryDelay, scInput;
        std::vector<std::complex<float>> spectrum, scSpectrum;
    };

    void analyse (const std::vector<float>& ring, std::vector<std::complex<float>>& spec) const
    {
        // Oldest sample first: the ring's write position is the oldest after the increment.
        for (int i = 0; i < fftSize; ++i)
            spec[(size_t) i] = { ring[(size_t) ((writePos + i) % fftSize)] * window[(size_t) i], 0.0f };
        fft.forward (spec.data());
    }

    void processFrame (int nch)
    {
        auto& L = chans[0];
        auto& R = chans[(size_t) std::min (1, nch - 1)];
        const bool stereo = nch > 1;

        analyse (L.input, L.spectrum);
        if (stereo) analyse (R.input, R.spectrum);
        if (params.externalSidechain)
        {
            analyse (L.scInput, L.scSpectrum);
            if (stereo) analyse (R.scInput, R.scSpectrum);
        }

        // Mid/Side placement works on transformed spectra (linear, so it commutes with the FFT).
        if (stereo && params.placement != Placement::stereo)
            for (int k = 0; k < fftSize; ++k)
            {
                const auto m = 0.5f * (L.spectrum[(size_t) k] + R.spectrum[(size_t) k]);
                const auto s = 0.5f * (L.spectrum[(size_t) k] - R.spectrum[(size_t) k]);
                L.spectrum[(size_t) k] = m; R.spectrum[(size_t) k] = s;
                if (params.externalSidechain)
                {
                    const auto sm = 0.5f * (L.scSpectrum[(size_t) k] + R.scSpectrum[(size_t) k]);
                    const auto ss = 0.5f * (L.scSpectrum[(size_t) k] - R.scSpectrum[(size_t) k]);
                    L.scSpectrum[(size_t) k] = sm; R.scSpectrum[(size_t) k] = ss;
                }
            }

        // Detector power per bin: the processed component(s), or the sidechain's.
        const auto& detL = params.externalSidechain ? L.scSpectrum : L.spectrum;
        const auto& detR = params.externalSidechain ? R.scSpectrum : R.spectrum;
        for (int k = 0; k < bins; ++k)
        {
            float p;
            if (! stereo || params.placement == Placement::mid) p = std::norm (detL[(size_t) k]);
            else if (params.placement == Placement::side) p = std::norm (detR[(size_t) k]);
            else p = 0.5f * (std::norm (detL[(size_t) k]) + std::norm (detR[(size_t) k]));
            auto& d = detectorPower[(size_t) k];
            d += detectorCoef * (p - d);
            magDb[(size_t) k] = 10.0f * std::log10 (d + 1e-12f);
        }

        // Spectral envelope: mean of the dB magnitude over a window that widens with frequency, twice
        // (the second pass clamps peaks so they do not lift the envelope they are compared against).
        smoothedMean (magDb, envelopeDb);
        for (int k = 0; k < bins; ++k) targetDb[(size_t) k] = std::min (magDb[(size_t) k], envelopeDb[(size_t) k] + 3.0f);
        smoothedMean (targetDb, envelopeDb);

        // Target reduction per bin.
        const float depth = std::clamp (params.depthDb, 0.0f, 24.0f);
        for (int k = 0; k < bins; ++k)
        {
            float t = 0.0f;
            if (k >= lowBin && k <= highBin && k > 1)
            {
                const float excess = magDb[(size_t) k] - envelopeDb[(size_t) k] - params.thresholdDb;
                t = depth * std::clamp (excess / (float) kneeDb, 0.0f, 1.0f);
            }
            targetDb[(size_t) k] = t;
        }
        // Widen each cut a little (dilation keeps the depth, avoids one-bin "needles").
        for (int k = 0; k < bins; ++k)
        {
            float m = targetDb[(size_t) k];
            for (int j = 1; j <= dilation; ++j)
            {
                if (k - j >= 0) m = std::max (m, targetDb[(size_t) (k - j)]);
                if (k + j < bins) m = std::max (m, targetDb[(size_t) (k + j)]);
            }
            magDb[(size_t) k] = m; // reuse as scratch
        }
        // Attack / release on the reduction, then apply.
        float maxRed = 0.0f;
        double sumRed = 0.0;
        for (int k = 0; k < bins; ++k)
        {
            auto& g = gainDb[(size_t) k];
            const float t = magDb[(size_t) k];
            g += (t > g ? attackCoef : releaseCoef) * (t - g);
            maxRed = std::max (maxRed, g);
            sumRed += g;
        }
        lastMeters.maxReductionDb = maxRed;
        lastMeters.meanReductionDb = (float) (sumRed / bins);

        auto applyGain = [&] (std::vector<std::complex<float>>& spec)
        {
            for (int k = 0; k < bins; ++k)
            {
                const float g = std::pow (10.0f, -gainDb[(size_t) k] / 20.0f);
                spec[(size_t) k] *= g;
                if (k > 0 && k < fftSize / 2) spec[(size_t) (fftSize - k)] *= g;
            }
        };
        if (! stereo) applyGain (L.spectrum);
        else if (params.placement == Placement::stereo) { applyGain (L.spectrum); applyGain (R.spectrum); }
        else if (params.placement == Placement::mid) applyGain (L.spectrum);
        else applyGain (R.spectrum);

        if (stereo && params.placement != Placement::stereo)
            for (int k = 0; k < fftSize; ++k)
            {
                const auto m = L.spectrum[(size_t) k], s = R.spectrum[(size_t) k];
                L.spectrum[(size_t) k] = m + s;
                R.spectrum[(size_t) k] = m - s;
            }

        // Back to time, synthesis window, overlap-add. The frame's first sample is the one that was
        // written fftSize samples ago, i.e. the output sample read next.
        for (int ch = 0; ch < nch; ++ch)
        {
            auto& c = chans[(size_t) ch];
            fft.inverse (c.spectrum.data());
            for (int i = 0; i < fftSize; ++i)
                c.output[(size_t) ((outReadPos + i) % (fftSize * 2))] += c.spectrum[(size_t) i].real() * window[(size_t) i];
        }
    }

    void smoothedMean (const std::vector<float>& in, std::vector<float>& out)
    {
        prefix[0] = 0.0;
        for (int k = 0; k < bins; ++k) prefix[(size_t) k + 1] = prefix[(size_t) k] + in[(size_t) k];
        for (int k = 0; k < bins; ++k)
        {
            const int w = std::max (2, (int) std::lround (k * halfWidthRatio));
            const int a = std::max (0, k - w), b = std::min (bins - 1, k + w);
            out[(size_t) k] = (float) ((prefix[(size_t) b + 1] - prefix[(size_t) a]) / (b - a + 1));
        }
    }

    double sampleRate = 48000.0;
    int channels = 2, fftOrder = order, fftSize = 1 << order, hop = (1 << order) / 4, bins = (1 << order) / 2 + 1;
    TameParams params;
    dsp::FFT fft;
    std::vector<float> window;
    std::array<Channel, (size_t) maxChannels> chans;
    std::vector<float> detectorPower, gainDb, envelopeDb, magDb, targetDb;
    std::vector<double> prefix;
    int writePos = 0, hopCounter = 0, outReadPos = 0;
    float attackCoef = 0.5f, releaseCoef = 0.1f, detectorCoef = 0.2f, halfWidthRatio = 0.2f;
    int dilation = 2, lowBin = 0, highBin = 1 << 20;
    static constexpr float overlapNorm = 1.0f / 1.5f; // Hann^2 at 75 % overlap sums to 1.5
    TameMeters lastMeters;
};

} // namespace rommuser::restame
