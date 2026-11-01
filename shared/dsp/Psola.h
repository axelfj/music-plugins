// Shared pitch detection (YIN) and TD-PSOLA building blocks, used by Tune Ready and Vox Synth.
//
// PsolaAnalyzer stores the input, runs YIN every hop and lays down analysis marks one period apart,
// snapped to the nearest waveform peak (a proxy for the glottal pulse). PsolaVoice turns those marks
// into output grains (Hann, two periods) overlap-added at the target period: the grain waveform is
// not resampled, so the formants stay; an independent formant ratio resamples the grains without
// moving the pitch. With ratio 1 and formant 1 the grains overlap-add back to the input exactly,
// delayed by the analyzer's latency. Several voices can share one analyzer.
//
// Pure C++ (no JUCE) so it is unit-tested headless in tests/.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "FFT.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace rommuser::dsp::psola
{

inline constexpr int maxChannels = 2;
inline constexpr float maxDetectHz = 1200.0f;

inline double hzToMidi (double hz) { return 69.0 + 12.0 * std::log2 (std::max (hz, 1.0) / 440.0); }
inline double midiToHz (double midi) { return 440.0 * std::pow (2.0, (midi - 69.0) / 12.0); }

enum class Scale { chromatic = 0, major, minor, custom };

inline uint16_t scaleMask (Scale s, uint16_t custom)
{
    switch (s)
    {
        case Scale::chromatic: return 0x0FFF;
        case Scale::major:     return (1 << 0) | (1 << 2) | (1 << 4) | (1 << 5) | (1 << 7) | (1 << 9) | (1 << 11);
        case Scale::minor:     return (1 << 0) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 8) | (1 << 10);
        case Scale::custom:    return custom == 0 ? (uint16_t) 0x0FFF : custom;
    }
    return 0x0FFF;
}

// Nearest MIDI note whose pitch class (relative to key) is allowed by the mask.
inline double snapToScale (double midi, int key, uint16_t mask)
{
    double best = std::round (midi);
    double bestDist = 1e9;
    const int centre = (int) std::round (midi);
    for (int n = centre - 6; n <= centre + 6; ++n)
    {
        const int pc = ((n - key) % 12 + 12) % 12;
        if ((mask >> pc) & 1)
        {
            const double d = std::fabs ((double) n - midi);
            if (d < bestDist) { bestDist = d; best = (double) n; }
        }
    }
    return best;
}

// The scale note `steps` scale degrees away from `note` (which should be a scale note); steps may be
// negative. With a chromatic mask a step is a semitone.
inline int scaleStep (int note, int steps, int key, uint16_t mask)
{
    if ((mask & 0x0FFF) == 0) return note;
    int n = note;
    const int dir = steps >= 0 ? 1 : -1;
    for (int k = 0; k < std::abs (steps); ++k)
    {
        int guard = 0;
        do { n += dir; ++guard; } while (! ((mask >> (((n - key) % 12 + 12) % 12)) & 1) && guard < 12);
    }
    return n;
}

struct PitchEstimate
{
    float hz = 0.0f;
    float confidence = 0.0f;        // 1 - normalised difference at the chosen lag
    bool voiced = false;
};

// YIN pitch detector (de Cheveigné & Kawahara 2002) through FFT correlation. detect() takes a frame
// of getWindowSize() samples: two periods of the lowest pitch.
class PitchDetector
{
public:
    void prepare (double sampleRate, float minHz)
    {
        sr = sampleRate;
        half = (int) std::ceil (sr / std::clamp ((double) minHz, 30.0, 200.0));   // one period of the lowest pitch
        window = 2 * half;
        // Circular correlation of length N >= window has no wrap-around for the lags we read:
        // j < half and tau < half, so j + tau < window. (window + half would only pad zeros.)
        int order = 1;
        while ((1 << order) < window) ++order;
        fft.prepare (order);
        fftSize = 1 << order;
        a.resize ((size_t) fftSize);
        b.resize ((size_t) fftSize);
        work.resize ((size_t) fftSize);
        realA.resize ((size_t) fftSize);
        realB.resize ((size_t) fftSize);
        d.resize ((size_t) half);
        prefix.resize ((size_t) window + 1);
        tauMin = std::max (2, (int) std::floor (sr / maxDetectHz));
    }

    int getWindowSize() const { return window; }

    PitchEstimate detect (const float* x)
    {
        PitchEstimate est;
        prefix[0] = 0.0;
        for (int i = 0; i < window; ++i) prefix[(size_t) i + 1] = prefix[(size_t) i] + (double) x[i] * x[i];
        const double energyY = prefix[(size_t) half];
        if (energyY < (double) half * 1e-8)   // below ~ -80 dBFS rms
            return est;

        // r(tau) = sum_{j<half} x[j] x[j+tau] via FFT correlation. Both inputs are real, so one
        // complex FFT transforms the two of them; the product is written out by hand.
        for (int i = 0; i < fftSize; ++i)
        {
            realA[(size_t) i] = i < window ? x[i] : 0.0f;
            realB[(size_t) i] = i < half ? x[i] : 0.0f;
        }
        fft.forwardTwoReal (realA.data(), realB.data(), work.data(), a.data(), b.data());
        for (int i = 0; i < fftSize; ++i)
        {
            const auto p = a[(size_t) i], q = b[(size_t) i]; // p * conj (q)
            a[(size_t) i] = { p.real() * q.real() + p.imag() * q.imag(), p.imag() * q.real() - p.real() * q.imag() };
        }
        fft.inverse (a.data());

        // Difference function and cumulative mean normalisation.
        double running = 0.0;
        for (int tau = 0; tau < half; ++tau)
        {
            const double energyX = prefix[(size_t) (tau + half)] - prefix[(size_t) tau];
            const double diff = std::max (0.0, energyY + energyX - 2.0 * (double) a[(size_t) tau].real());
            if (tau == 0) { d[0] = 1.0f; continue; }
            running += diff;
            d[(size_t) tau] = (float) (diff * tau / std::max (running, 1e-12));
        }

        // Absolute threshold: first dip below it, refined to its local minimum; else the global minimum.
        int tau = -1;
        for (int t = tauMin; t < half - 1; ++t)
        {
            if (d[(size_t) t] < threshold)
            {
                while (t + 1 < half - 1 && d[(size_t) t + 1] < d[(size_t) t]) ++t;
                tau = t;
                break;
            }
        }
        if (tau < 0)
        {
            float best = 1e9f;
            for (int t = tauMin; t < half - 1; ++t)
                if (d[(size_t) t] < best) { best = d[(size_t) t]; tau = t; }
            if (tau < 0) return est;
        }
        double refined = tau;
        if (tau > 0 && tau < half - 1)
        {
            const double s0 = d[(size_t) tau - 1], s1 = d[(size_t) tau], s2 = d[(size_t) tau + 1];
            const double denom = s0 - 2.0 * s1 + s2;
            if (std::fabs (denom) > 1e-12)
                refined = tau + 0.5 * (s0 - s2) / denom;
        }
        est.hz = (float) (sr / refined);
        est.confidence = 1.0f - std::clamp (d[(size_t) tau], 0.0f, 1.0f);
        est.voiced = d[(size_t) tau] < voicedThreshold && est.hz < maxDetectHz;
        return est;
    }

private:
    double sr = 48000.0;
    int window = 2048, half = 1024, fftSize = 4096, tauMin = 40;
    float threshold = 0.1f, voicedThreshold = 0.25f;
    FFT fft;
    std::vector<FFT::Complex> a, b, work;
    std::vector<float> realA, realB;
    std::vector<float> d;
    std::vector<double> prefix;
};

struct Mark { double pos; float period; };

// Input store + pitch tracking + analysis marks. Feed it one sample at a time with push(); every
// hop samples hopDue() turns true and analyse() runs the detector on the last window.
class PsolaAnalyzer
{
public:
    void prepare (double sampleRate, int maxBlockSize, int numChannels, float minHz)
    {
        sr = sampleRate;
        maxBlock = std::max (1, maxBlockSize);
        channels = std::clamp (numChannels, 1, maxChannels);
        detector.prepare (sr, minHz);
        window = detector.getWindowSize();
        maxPeriod = window / 2;
        // Voices lay grains down up to 2 * maxPeriod ahead of the output sample (formants down an
        // octave double the grain length) and read up to one period around their analysis mark,
        // which must already be in the input, with its successor mark known: latency > 3.25 maxPeriod.
        latency = window + maxPeriod + maxPeriod / 4 + 8;
        hop = std::max (64, window / 8);
        fallbackPeriod = std::min ((float) (sr / 100.0), (float) maxPeriod);   // 10 ms grains on unvoiced input
        int ringSize = 1;
        while (ringSize < 2 * latency + 4 * maxPeriod + 2 * maxBlock) ringSize <<= 1;
        ringMask = (uint64_t) ringSize - 1;
        for (auto& r : in) r.assign ((size_t) ringSize, 0.0f);
        mono.assign ((size_t) ringSize, 0.0f);
        frame.assign ((size_t) window, 0.0f);
        marks.reserve (512);
        reset();
    }

    void reset()
    {
        for (auto& r : in) std::fill (r.begin(), r.end(), 0.0f);
        std::fill (mono.begin(), mono.end(), 0.0f);
        inCount = 0;
        sinceHop = 0;
        nextAna = 0.0;
        marks.clear();
        curPeriod = fallbackPeriod;
        lastEstimate = {};
    }

    int getLatencySamples() const { return latency; }
    int getMaxPeriod() const { return maxPeriod; }
    int getWindowSize() const { return window; }
    int getHop() const { return hop; }
    double getSampleRate() const { return sr; }
    int64_t getInputCount() const { return inCount; }
    const std::vector<Mark>& getMarks() const { return marks; }
    const PitchEstimate& getLastEstimate() const { return lastEstimate; }
    float getCurrentPeriod() const { return curPeriod; }

    // Stores one sample per channel and places analysis marks as soon as their peak search window is
    // available.
    void push (const float* const* io, int numChannels, int i)
    {
        const int nch = std::clamp (numChannels, 1, channels);
        const int w = (int) ((uint64_t) inCount & ringMask);
        float m = 0.0f;
        for (int c = 0; c < nch; ++c)
        {
            in[(size_t) c][(size_t) w] = io[c][i];
            m += io[c][i];
        }
        mono[(size_t) w] = m / (float) nch;
        ++inCount;
        while (nextAna + (double) (maxPeriod / 4 + 2) < (double) inCount)
            pushMark();
        ++sinceHop;
    }

    bool hopDue() const { return sinceHop >= hop; }

    // Runs YIN on the last window; updates the period used for the marks. Returns the estimate.
    const PitchEstimate& analyse()
    {
        sinceHop = 0;
        for (int i = 0; i < window; ++i)
            frame[(size_t) i] = mono[(size_t) ((uint64_t) (inCount - window + i) & ringMask)];
        lastEstimate = detector.detect (frame.data());
        curPeriod = lastEstimate.voiced ? std::clamp ((float) (sr / lastEstimate.hz), 2.0f, (float) maxPeriod) : fallbackPeriod;
        // Drop marks no output grain can reach any more.
        const double oldest = (double) inCount - (double) latency - 2.0 * maxPeriod;
        size_t drop = 0;
        while (drop < marks.size() && marks[drop].pos < oldest) ++drop;
        if (drop > 0) marks.erase (marks.begin(), marks.begin() + (long) drop);
        return lastEstimate;
    }

    // Analysis mark nearest to an absolute input position.
    const Mark& nearestMark (double want)
    {
        if (marks.empty())
            pushMark();
        size_t best = 0;
        double bestDist = 1e18;
        for (size_t k = 0; k < marks.size(); ++k)
        {
            const double dist = std::fabs (marks[k].pos - want);
            if (dist < bestDist) { bestDist = dist; best = k; }
            else if (marks[k].pos > want) break;
        }
        return marks[best];
    }

    // Cubic Hermite read of channel c at a fractional absolute position.
    inline float readInterp (int c, double pos) const
    {
        const auto& buf = in[(size_t) c];
        const double fl = std::floor (pos);
        const float t = (float) (pos - fl);
        const int64_t i = (int64_t) fl;
        const float y0 = buf[(size_t) ((uint64_t) (i - 1) & ringMask)];
        const float y1 = buf[(size_t) ((uint64_t) i & ringMask)];
        const float y2 = buf[(size_t) ((uint64_t) (i + 1) & ringMask)];
        const float y3 = buf[(size_t) ((uint64_t) (i + 2) & ringMask)];
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * t + c2) * t + c1) * t + y1;
    }

    // Integer read (e.g. the dry signal delayed by the latency).
    inline float read (int c, int64_t pos) const { return in[(size_t) c][(size_t) ((uint64_t) pos & ringMask)]; }
    inline float readMono (int64_t pos) const { return mono[(size_t) ((uint64_t) pos & ringMask)]; }

private:
    void pushMark()
    {
        const double period = curPeriod;
        const int64_t centre = (int64_t) std::llround (nextAna);
        int64_t lo = centre - (int64_t) (period * 0.25), hi = centre + (int64_t) (period * 0.25);
        if (! marks.empty())
            lo = std::max (lo, (int64_t) marks.back().pos + (int64_t) (period * 0.5));
        lo = std::max<int64_t> (lo, 0);
        hi = std::min<int64_t> (hi, inCount - 1);
        int64_t best = centre;
        float bestVal = -1.0f;
        for (int64_t k = lo; k <= hi; ++k)
        {
            const float v = std::fabs (mono[(size_t) ((uint64_t) k & ringMask)]);
            if (v > bestVal) { bestVal = v; best = k; }
        }
        // Sub-sample peak (parabola through the three samples around it), then lock softly: the mark
        // sits on the predicted fractional position and moves only part of the way towards the
        // peak, so the spacing stays the detected (fractional) period instead of jumping between
        // whole samples, which would jitter the output pitch.
        const float ym = std::fabs (mono[(size_t) ((uint64_t) (best - 1) & ringMask)]);
        const float y0 = std::fabs (mono[(size_t) ((uint64_t) best & ringMask)]);
        const float yp = std::fabs (mono[(size_t) ((uint64_t) (best + 1) & ringMask)]);
        const float den = ym - 2.0f * y0 + yp;
        const double peak = (double) best + (std::fabs (den) > 1e-12f ? std::clamp (0.5f * (ym - yp) / den, -0.5f, 0.5f) : 0.0f);
        double pos = peak;
        if (! marks.empty() && std::fabs (peak - nextAna) < period * 0.25)
            pos = nextAna + markLock * (peak - nextAna);
        if (! marks.empty())
            marks.back().period = (float) (pos - marks.back().pos);   // actual spacing
        marks.push_back ({ pos, (float) period });
        nextAna = pos + period;
    }

    // How far each mark moves from its predicted position towards the waveform peak: enough to follow
    // a real voice's pulses, small enough that the spacing stays the detected fractional period.
    static constexpr double markLock = 0.05;

    double sr = 48000.0;
    int maxBlock = 512, channels = 2;
    PitchDetector detector;
    PitchEstimate lastEstimate;
    int window = 2048, hop = 256, maxPeriod = 1024, latency = 2048;
    float fallbackPeriod = 480.0f, curPeriod = 480.0f;
    uint64_t ringMask = 0;
    std::array<std::vector<float>, maxChannels> in;
    std::vector<float> mono, frame;
    int64_t inCount = 0;
    int sinceHop = 0;
    double nextAna = 0.0;
    std::vector<Mark> marks;
};

// One pitch-shifted rendering of the analyzer's input. Call renderSample() once per input sample,
// after the analyzer has pushed that sample.
class PsolaVoice
{
public:
    void prepare (const PsolaAnalyzer& analyzer, int maxBlockSize, int numChannels)
    {
        channels = std::clamp (numChannels, 1, maxChannels);
        latency = analyzer.getLatencySamples();
        maxPeriod = analyzer.getMaxPeriod();
        int accSize = 1;
        while (accSize < 8 * maxPeriod + 2 * std::max (1, maxBlockSize)) accSize <<= 1;
        accMask = (uint64_t) accSize - 1;
        for (auto& r : acc) r.assign ((size_t) accSize, 0.0f);
        wsum.assign ((size_t) accSize, 0.0f);
        reset();
    }

    void reset()
    {
        for (auto& r : acc) std::fill (r.begin(), r.end(), 0.0f);
        std::fill (wsum.begin(), wsum.end(), 0.0f);
        nextOut = (double) latency;
        outStarted = false;
    }

    // Produces the output sample for the input sample the analyzer just pushed. ratio: pitch ratio
    // (2 = up an octave), formantRatio: formant ratio (2 = formants up an octave).
    void renderSample (PsolaAnalyzer& an, int numChannels, float ratio, float formantRatio, float* out)
    {
        const int nch = std::clamp (numChannels, 1, channels);
        const int64_t outTime = an.getInputCount() - 1;
        if (! outStarted && ! an.getMarks().empty())
        {
            nextOut = an.getMarks().front().pos + (double) latency;
            outStarted = true;
        }
        while (nextOut < (double) (outTime + 2 * maxPeriod + 1))
            emitGrain (an, nch, ratio, formantRatio);

        const size_t o = (size_t) ((uint64_t) outTime & accMask);
        const float norm = wsum[o] > 1e-3f ? 1.0f / wsum[o] : 1.0f;
        for (int c = 0; c < nch; ++c)
        {
            out[c] = acc[(size_t) c][o] * norm;
            acc[(size_t) c][o] = 0.0f;
        }
        wsum[o] = 0.0f;
    }

private:
    void emitGrain (PsolaAnalyzer& an, int nch, float ratio, float formantRatio)
    {
        const Mark mark = an.nearestMark (nextOut - (double) latency);
        const float halfIn = mark.period;                  // grain half length in input samples
        const float halfOut = halfIn / formantRatio;       // in output samples
        const int span = std::max (1, (int) std::ceil (halfOut) - 1);
        const double centre = std::round (nextOut);
        const double frac = nextOut - centre;
        for (int j = -span; j <= span; ++j)
        {
            const float wgt = 0.5f * (1.0f + std::cos (3.14159265f * (float) j / halfOut));
            const size_t o = (size_t) ((uint64_t) ((int64_t) centre + j) & accMask);
            const double readPos = mark.pos + ((double) j - frac) * formantRatio;
            for (int c = 0; c < nch; ++c)
                acc[(size_t) c][o] += wgt * an.readInterp (c, readPos);
            wsum[o] += wgt;
        }
        nextOut += mark.period / std::max (ratio, 0.05f);
    }

    int channels = 2, latency = 0, maxPeriod = 0;
    uint64_t accMask = 0;
    std::array<std::vector<float>, maxChannels> acc;
    std::vector<float> wsum;
    double nextOut = 0.0;
    bool outStarted = false;
};

} // namespace rommuser::dsp::psola
