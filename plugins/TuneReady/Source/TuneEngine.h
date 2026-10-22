// Tune Ready DSP engine: real-time pitch correction for monophonic voices.
//
// Pure C++ (no JUCE) so it is unit-tested headless in tests/.
//
// Detection: YIN (de Cheveigné & Kawahara 2002) on a window of two periods of the lowest pitch,
// computed through FFT correlation every hop, with cumulative-mean normalisation, absolute threshold
// and parabolic interpolation. Correction: the detected note is snapped to the nearest note of the
// chosen key / scale (or to a held MIDI note), and the shift glides towards it at the chosen speed
// (0 ms = hard tune). Shifting: TD-PSOLA. Grains of two periods (Hann) are taken around analysis
// marks spaced one period apart and overlap-added at the target period; the grain waveform itself is
// not resampled, so the formants stay where they were. An independent formant ratio resamples the
// grains (voice bigger / smaller) without touching the pitch. When no shift is applied the grains
// overlap-add back to the input exactly, so the plugin is transparent on unvoiced sounds.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../../../shared/dsp/FFT.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace rommuser::tuneready
{

enum class Mode { correct = 0, fixed };
enum class Scale { chromatic = 0, major, minor, custom };

inline constexpr int maxChannels = 2;
inline constexpr float maxDetectHz = 1200.0f;

inline double hzToMidi (double hz) { return 69.0 + 12.0 * std::log2 (std::max (hz, 1.0) / 440.0); }
inline double midiToHz (double midi) { return 440.0 * std::pow (2.0, (midi - 69.0) / 12.0); }

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

struct TuneParams
{
    Mode mode = Mode::correct;
    int key = 0;                    // 0 = C .. 11 = B
    Scale scale = Scale::chromatic;
    uint16_t customMask = 0x0FFF;   // bit n = pitch class n (relative to key) allowed
    float speedMs = 20.0f;          // 0 .. 400: time constant of the correction (0 = hard)
    float amount = 1.0f;            // 0 .. 1: how much of the correction to apply
    float humanizeCents = 0.0f;     // 0 .. 50: slow random drift of the target
    float minHz = 60.0f;            // 40 .. 120: lowest pitch to track (sets window and latency)
    float fixedSemis = 0.0f;        // -24 .. 24: shift in Fixed mode
    float formantSemis = 0.0f;      // -12 .. 12: formant shift (both modes)
    bool midiPriority = true;       // held MIDI notes override the scale
    float mix = 1.0f;
    float outputDb = 0.0f;
};

struct TuneMeters
{
    bool voiced = false;
    float inputHz = 0.0f;           // detected
    float detectedMidi = 0.0f;
    float targetMidi = 0.0f;
    float shiftSemis = 0.0f;        // applied right now
    float confidence = 0.0f;
};

struct PitchEstimate
{
    float hz = 0.0f;
    float confidence = 0.0f;        // 1 - normalised difference at the chosen lag
    bool voiced = false;
};

// YIN pitch detector. detect() takes a frame of getWindowSize() samples.
class PitchDetector
{
public:
    void prepare (double sampleRate, float minHz)
    {
        sr = sampleRate;
        half = (int) std::ceil (sr / std::clamp ((double) minHz, 30.0, 200.0));   // one period of the lowest pitch
        window = 2 * half;
        int order = 1;
        while ((1 << order) < window + half) ++order;
        fft.prepare (order);
        fftSize = 1 << order;
        a.resize ((size_t) fftSize);
        b.resize ((size_t) fftSize);
        d.resize ((size_t) half);
        prefix.resize ((size_t) window + 1);
        tauMin = std::max (2, (int) std::floor (sr / maxDetectHz));
    }

    int getWindowSize() const { return window; }

    PitchEstimate detect (const float* x)
    {
        PitchEstimate est;
        // Energy gate.
        prefix[0] = 0.0;
        for (int i = 0; i < window; ++i) prefix[(size_t) i + 1] = prefix[(size_t) i] + (double) x[i] * x[i];
        const double energyY = prefix[(size_t) half];
        if (energyY < (double) half * 1e-8)   // below ~ -80 dBFS rms
            return est;

        // r(tau) = sum_{j<half} x[j] x[j+tau] via FFT correlation.
        for (int i = 0; i < fftSize; ++i)
        {
            a[(size_t) i] = i < window ? dsp::FFT::Complex (x[i], 0.0f) : dsp::FFT::Complex (0.0f, 0.0f);
            b[(size_t) i] = i < half ? dsp::FFT::Complex (x[i], 0.0f) : dsp::FFT::Complex (0.0f, 0.0f);
        }
        fft.forward (a.data());
        fft.forward (b.data());
        for (int i = 0; i < fftSize; ++i) a[(size_t) i] *= std::conj (b[(size_t) i]);
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
        // Parabolic interpolation.
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
    dsp::FFT fft;
    std::vector<dsp::FFT::Complex> a, b;
    std::vector<float> d;
    std::vector<double> prefix;
};

class TuneEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize, int numChannels)
    {
        sr = sampleRate;
        maxBlock = std::max (1, maxBlockSize);
        channels = std::clamp (numChannels, 1, maxChannels);
        configure (params.minHz);
        setParams (params, true);
        reset();
    }

    void reset()
    {
        for (auto& r : in) std::fill (r.begin(), r.end(), 0.0f);
        std::fill (mono.begin(), mono.end(), 0.0f);
        for (auto& r : acc) std::fill (r.begin(), r.end(), 0.0f);
        std::fill (wsum.begin(), wsum.end(), 0.0f);
        inCount = 0;
        sinceHop = 0;
        nextAna = 0.0;
        nextOut = (double) latency;
        outStarted = false;
        marks.clear();
        curPeriod = fallbackPeriod;
        shift = shiftTarget = 0.0f;
        humanOffset = 0.0f;
        meters = {};
        heldNotes.clear();
    }

    void setParams (const TuneParams& p, bool immediate = false)
    {
        (void) immediate;
        if (std::fabs (p.minHz - params.minHz) > 0.01f)
        {
            params = p;
            configure (p.minHz);
            reset();
            return;
        }
        params = p;
    }

    const TuneParams& getParams() const { return params; }
    int getLatencySamples() const { return latency; }
    const TuneMeters& getMeters() const { return meters; }

    void noteOn (int note) { if (std::find (heldNotes.begin(), heldNotes.end(), note) == heldNotes.end()) heldNotes.push_back (note); }
    void noteOff (int note) { heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end()); }
    void allNotesOff() { heldNotes.clear(); }
    bool hasHeldNotes() const { return ! heldNotes.empty(); }

    void process (float* const* io, int numChannels, int n)
    {
        const int nch = std::clamp (numChannels, 1, channels);
        const float formantRatio = std::pow (2.0f, std::clamp (params.formantSemis, -12.0f, 12.0f) / 12.0f);
        const float outGain = std::pow (10.0f, std::clamp (params.outputDb, -24.0f, 12.0f) / 20.0f);

        for (int i = 0; i < n; ++i)
        {
            // 1. Store input.
            const int w = (int) ((uint64_t) inCount & ringMask);
            float m = 0.0f;
            for (int c = 0; c < nch; ++c)
            {
                in[(size_t) c][(size_t) w] = io[c][i];
                m += io[c][i];
            }
            mono[(size_t) w] = m / (float) nch;
            ++inCount;
            // Analysis marks, snapped to the nearest waveform peak (a proxy for the glottal pulse),
            // as soon as the input around the candidate position is available.
            while (nextAna + (double) (maxPeriod / 4 + 2) < (double) inCount)
                pushMark();

            // 2. Every hop: detect pitch on the last window, update the shift and the analysis marks.
            if (++sinceHop >= hop)
            {
                sinceHop = 0;
                analyse();
            }

            // 3. Lay down output grains ahead of the output position. The first output grain is
            //    anchored on the first analysis mark so that an unshifted signal comes out delayed
            //    by exactly the reported latency.
            const int64_t outTime = inCount - 1;   // output sample index being produced now
            if (! outStarted && ! marks.empty())
            {
                nextOut = marks.front().pos + (double) latency;
                outStarted = true;
            }
            while (nextOut < (double) (outTime + 2 * maxPeriod + 1))
                emitGrain (nch, formantRatio);

            // 4. Output the finished sample, mix with the delayed dry signal.
            const int o = (int) ((uint64_t) outTime & accMask);
            const float norm = wsum[(size_t) o] > 1e-3f ? 1.0f / wsum[(size_t) o] : 1.0f;
            const int dryIdx = (int) ((uint64_t) (outTime - latency) & ringMask);
            for (int c = 0; c < nch; ++c)
            {
                const float wet = acc[(size_t) c][(size_t) o] * norm;
                const float dry = in[(size_t) c][(size_t) dryIdx];
                float y = params.mix >= 1.0f ? wet : params.mix <= 0.0f ? dry : dry + (wet - dry) * params.mix;
                io[c][i] = y * outGain;
                acc[(size_t) c][(size_t) o] = 0.0f;
            }
            wsum[(size_t) o] = 0.0f;
        }
    }

    PitchDetector& getDetector() { return detector; }

private:
    struct Mark { double pos; float period; };

    void configure (float minHz)
    {
        detector.prepare (sr, minHz);
        window = detector.getWindowSize();
        maxPeriod = window / 2;
        // Grains are laid down up to 2 * maxPeriod ahead of the output sample (formants down an
        // octave double the grain length) and read up to one period around their analysis mark,
        // which must already be in the input: latency >= 3 * maxPeriod. Marks are placed once the
        // input reaches a quarter period past the candidate, so add that look-ahead too.
        latency = window + maxPeriod + maxPeriod / 4 + 8;   // + the peak-search look-ahead of the marks
        hop = std::max (64, window / 8);
        fallbackPeriod = std::min ((float) (sr / 100.0), (float) maxPeriod);   // 10 ms grains on unvoiced / silent input
        int ringSize = 1;
        while (ringSize < 2 * latency + 4 * maxPeriod + 2 * maxBlock) ringSize <<= 1;
        ringMask = (uint64_t) ringSize - 1;
        for (auto& r : in) r.assign ((size_t) ringSize, 0.0f);
        mono.assign ((size_t) ringSize, 0.0f);
        int accSize = 1;
        while (accSize < 8 * maxPeriod + 2 * maxBlock) accSize <<= 1;
        accMask = (uint64_t) accSize - 1;
        for (auto& r : acc) r.assign ((size_t) accSize, 0.0f);
        wsum.assign ((size_t) accSize, 0.0f);
        frame.assign ((size_t) window, 0.0f);
        marks.reserve (512);
        hopCoeffBase = (float) hop / (float) sr;
    }

    void analyse()
    {
        // Frame = the last `window` input samples; its centre is at inCount - window / 2.
        for (int i = 0; i < window; ++i)
            frame[(size_t) i] = mono[(size_t) ((uint64_t) (inCount - window + i) & ringMask)];
        const auto est = detector.detect (frame.data());

        meters.voiced = est.voiced;
        meters.confidence = est.confidence;
        if (est.voiced)
        {
            curPeriod = std::clamp ((float) (sr / est.hz), 2.0f, (float) maxPeriod);
            meters.inputHz = est.hz;
            meters.detectedMidi = (float) hzToMidi (est.hz);
        }
        else
        {
            curPeriod = fallbackPeriod;
        }

        // Target shift.
        float target = 0.0f;
        if (params.mode == Mode::fixed)
        {
            target = std::clamp (params.fixedSemis, -24.0f, 24.0f);
            meters.targetMidi = meters.detectedMidi + target;
        }
        else if (est.voiced)
        {
            const double detected = hzToMidi (est.hz);
            double targetMidi;
            if (params.midiPriority && ! heldNotes.empty())
            {
                // Nearest held note (any octave of it is not implied: the player chose the octave).
                targetMidi = heldNotes.front();
                for (int note : heldNotes)
                    if (std::fabs (note - detected) < std::fabs (targetMidi - detected)) targetMidi = note;
            }
            else
            {
                targetMidi = snapToScale (detected, params.key, scaleMask (params.scale, params.customMask));
            }
            // Humanize: slow random walk of the target, bounded.
            if (params.humanizeCents > 0.0f)
            {
                rng = rng * 1664525u + 1013904223u;
                const float r = (float) (rng >> 8) / 8388608.0f - 1.0f;
                humanOffset += 0.1f * r * params.humanizeCents / 100.0f;
                humanOffset = std::clamp (humanOffset, -params.humanizeCents / 100.0f, params.humanizeCents / 100.0f);
                humanOffset *= 0.98f;
                targetMidi += humanOffset;
            }
            target = (float) (targetMidi - detected) * std::clamp (params.amount, 0.0f, 1.0f);
            meters.targetMidi = (float) targetMidi;
        }
        else
        {
            target = 0.0f;   // unvoiced: relax towards no shift
        }
        shiftTarget = target;
        if (params.speedMs <= 0.0f || params.mode == Mode::fixed)
            shift = shiftTarget;
        else
        {
            const float coeff = 1.0f - std::exp (-hopCoeffBase / (params.speedMs * 0.001f));
            shift += (shiftTarget - shift) * coeff;
        }
        meters.shiftSemis = shift;

        // Drop marks that can no longer be reached by any output grain.
        const double oldest = (double) inCount - (double) latency - 2.0 * maxPeriod;
        size_t drop = 0;
        while (drop < marks.size() && marks[drop].pos < oldest) ++drop;
        if (drop > 0) marks.erase (marks.begin(), marks.begin() + (long) drop);
    }

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
        if (! marks.empty())
            marks.back().period = (float) ((double) best - marks.back().pos);   // actual spacing
        marks.push_back ({ (double) best, (float) period });
        nextAna = (double) best + period;
    }

    void emitGrain (int nch, float formantRatio)
    {
        if (marks.empty())
            pushMark();
        // Analysis mark nearest to the output position mapped back by the latency.
        const double want = nextOut - (double) latency;
        size_t best = 0;
        double bestDist = 1e18;
        for (size_t k = 0; k < marks.size(); ++k)
        {
            const double dist = std::fabs (marks[k].pos - want);
            if (dist < bestDist) { bestDist = dist; best = k; }
            else if (marks[k].pos > want) break;
        }
        const Mark mark = marks[best];
        const float halfIn = mark.period;                  // grain half length in input samples
        const float halfOut = halfIn / formantRatio;       // in output samples
        const int span = std::max (1, (int) std::ceil (halfOut) - 1);
        const double centre = std::round (nextOut);         // integer placement of the grain centre
        const double frac = nextOut - centre;               // sub-sample offset read from the input instead
        for (int j = -span; j <= span; ++j)
        {
            const float wgt = 0.5f * (1.0f + std::cos (3.14159265f * (float) j / halfOut));
            const size_t o = (size_t) ((uint64_t) ((int64_t) centre + j) & accMask);
            const double readPos = mark.pos + ((double) j + frac) * formantRatio;
            for (int c = 0; c < nch; ++c)
                acc[(size_t) c][o] += wgt * readInterp (in[(size_t) c], readPos);
            wsum[o] += wgt;
        }
        const float ratio = std::pow (2.0f, shift / 12.0f);
        nextOut += mark.period / ratio;
    }

    inline float readInterp (const std::vector<float>& buf, double pos) const
    {
        const double fl = std::floor (pos);
        const float t = (float) (pos - fl);
        const int64_t i = (int64_t) fl;
        const float y0 = buf[(size_t) ((uint64_t) (i - 1) & ringMask)];
        const float y1 = buf[(size_t) ((uint64_t) i & ringMask)];
        const float y2 = buf[(size_t) ((uint64_t) (i + 1) & ringMask)];
        const float y3 = buf[(size_t) ((uint64_t) (i + 2) & ringMask)];
        // Cubic Hermite (Catmull-Rom).
        const float c0 = y1;
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * t + c2) * t + c1) * t + c0;
    }

    double sr = 48000.0;
    int maxBlock = 512, channels = 2;
    TuneParams params;
    TuneMeters meters;
    PitchDetector detector;

    int window = 2048, hop = 256, maxPeriod = 1024, latency = 2048;
    float fallbackPeriod = 480.0f, curPeriod = 480.0f;
    float hopCoeffBase = 0.005f;
    uint64_t ringMask = 0, accMask = 0;
    std::array<std::vector<float>, maxChannels> in, acc;
    std::vector<float> mono, wsum, frame;
    int64_t inCount = 0;
    int sinceHop = 0;
    double nextAna = 0.0, nextOut = 0.0;
    bool outStarted = false;
    std::vector<Mark> marks;
    float shift = 0.0f, shiftTarget = 0.0f, humanOffset = 0.0f;
    uint32_t rng = 777u;
    std::vector<int> heldNotes;
};

} // namespace rommuser::tuneready
