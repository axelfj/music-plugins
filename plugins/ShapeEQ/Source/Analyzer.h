// Spectrum analyzer plumbing: the audio thread drops mono samples into a lock-free FIFO, the
// editor pulls them, windows + FFTs them and keeps a smoothed dB curve to draw.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <atomic>
#include <vector>

namespace rommuser::shapeeq
{

// Audio-thread side. Only pushes while an editor is open (`active`), so closed GUIs cost nothing.
class SpectrumSource
{
public:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;

    SpectrumSource() : fifo (fifoSize), buffer ((size_t) fifoSize, 0.0f) {}

    void push (const float* const* channels, int numChannels, int n)
    {
        if (! active.load (std::memory_order_relaxed) || numChannels <= 0)
            return;

        int start1, size1, start2, size2;
        fifo.prepareToWrite (n, start1, size1, start2, size2);
        auto writeRun = [&] (int start, int size, int offset)
        {
            for (int i = 0; i < size; ++i)
            {
                float m = 0.0f;
                for (int c = 0; c < numChannels; ++c)
                    m += channels[c][offset + i];
                buffer[(size_t) (start + i)] = m / (float) numChannels;
            }
        };
        writeRun (start1, size1, 0);
        writeRun (start2, size2, size1);
        fifo.finishedWrite (size1 + size2);
    }

    // UI side: copies up to `maxSamples` into dst, returns how many were read.
    int pull (float* dst, int maxSamples)
    {
        int start1, size1, start2, size2;
        fifo.prepareToRead (maxSamples, start1, size1, start2, size2);
        if (size1 > 0) std::copy (buffer.begin() + start1, buffer.begin() + start1 + size1, dst);
        if (size2 > 0) std::copy (buffer.begin() + start2, buffer.begin() + start2 + size2, dst + size1);
        fifo.finishedRead (size1 + size2);
        return size1 + size2;
    }

    std::atomic<bool> active { false };

private:
    static constexpr int fifoSize = fftSize * 4;
    juce::AbstractFifo fifo;
    std::vector<float> buffer;
};

// Editor side: turns the pulled samples into a dB-per-bin curve with a slow release.
class SpectrumAnalyzer
{
public:
    static constexpr int fftSize = SpectrumSource::fftSize;
    static constexpr int numBins = fftSize / 2;

    SpectrumAnalyzer()
        : fft (SpectrumSource::fftOrder),
          window ((size_t) fftSize, juce::dsp::WindowingFunction<float>::hann, false),
          ring ((size_t) fftSize, 0.0f),
          scratch ((size_t) fftSize * 2, 0.0f),
          levelDb ((size_t) numBins, floorDb)
    {}

    void setSampleRate (double sr) { sampleRate = sr; }
    double getSampleRate() const { return sampleRate; }

    // Pull everything the source has, run as many FFT frames as fit (hop = fftSize / 2).
    void update (SpectrumSource& source)
    {
        float tmp[1024];
        bool gotFrame = false;
        for (;;)
        {
            const int n = source.pull (tmp, 1024);
            if (n == 0) break;
            for (int i = 0; i < n; ++i)
            {
                ring[(size_t) ringPos] = tmp[i];
                if (++ringPos == fftSize) ringPos = 0;
                if (++sinceFrame >= fftSize / 2)
                {
                    sinceFrame = 0;
                    analyseFrame();
                    gotFrame = true;
                }
            }
        }
        if (! gotFrame)
            decay (0.6f);   // nothing new (transport stopped?): keep falling
    }

    void clear() { std::fill (levelDb.begin(), levelDb.end(), floorDb); }

    // Smoothed level (dB) at an arbitrary frequency, linearly interpolated between bins.
    float levelAt (double freq) const
    {
        const double bin = freq / sampleRate * fftSize;
        const int b0 = (int) bin;
        if (b0 < 1 || b0 >= numBins - 1) return floorDb;
        const float t = (float) (bin - b0);
        return levelDb[(size_t) b0] * (1.0f - t) + levelDb[(size_t) b0 + 1] * t;
    }

    static constexpr float floorDb = -100.0f;

private:
    void analyseFrame()
    {
        // Unroll the ring so the oldest sample comes first, then window + FFT.
        for (int i = 0; i < fftSize; ++i)
            scratch[(size_t) i] = ring[(size_t) ((ringPos + i) % fftSize)];
        std::fill (scratch.begin() + fftSize, scratch.end(), 0.0f);
        window.multiplyWithWindowingTable (scratch.data(), (size_t) fftSize);
        fft.performFrequencyOnlyForwardTransform (scratch.data(), true);

        // Scale so a full-scale sine reads 0 dB (Hann coherent gain 0.5, two-sided -> x2).
        const float scale = 4.0f / (float) fftSize;
        for (int b = 0; b < numBins; ++b)
        {
            const float db = juce::Decibels::gainToDecibels (scratch[(size_t) b] * scale, floorDb);
            float& cur = levelDb[(size_t) b];
            cur = db > cur ? db : juce::jmax (db, cur - releaseDbPerFrame);
        }
    }

    void decay (float db)
    {
        for (auto& v : levelDb)
            v = juce::jmax (floorDb, v - db);
    }

    static constexpr float releaseDbPerFrame = 2.5f;

    double sampleRate = 44100.0;
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    std::vector<float> ring, scratch, levelDb;
    int ringPos = 0, sinceFrame = 0;
};

} // namespace rommuser::shapeeq
