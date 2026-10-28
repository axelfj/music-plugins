// Pitch-R DSP engine: clean pitch shifting for anything (mixes, chords, voices, bass, drums).
//
// Pure C++ (no JUCE) so it is unit-tested headless in tests/.
//
// The shifting itself is shared/dsp/SpectralShifter.h (phase vocoder with partial-locked regions
// and phase gradient heap integration). This file adds the musical part: three modes that trade
// latency for frequency resolution, formant handling, dry/wet mix with the dry path delayed by the
// same latency, an A/B "original" switch, and an exact pass-through at 0 semitones (the processed
// signal fades out and the delayed input plays, so "no shift" really means no change).
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../../../shared/dsp/Smoothing.h"
#include "../../../shared/dsp/SpectralShifter.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace rommuser::pitchr
{

inline constexpr int maxChannels = 2;

// Clean: 4096-point FFT at 44.1/48 kHz (~85 ms), for mixes, chords, pads, samples.
// Voice: 2048 (~43 ms), for one voice or instrument; formants are usually kept on.
// Fast: 1024 (~21 ms), for monitoring while playing or singing; chords get rougher.
enum class Mode { clean = 0, voice, fast };
inline constexpr int numModes = 3;

struct PitchParams
{
    Mode mode = Mode::clean;
    float semitones = 0.0f;         // -24 .. 24
    float cents = 0.0f;             // -100 .. 100
    bool keepFormants = false;      // the spectral envelope stays where it was (no chipmunk / monster)
    float formantSemis = 0.0f;      // -12 .. 12: moves the envelope on top of that
    float mix = 1.0f;               // 0 .. 1
    float outputDb = 0.0f;
    bool original = false;          // A/B: play the (latency-compensated) input
};

class PitchEngine
{
public:
    static int fftOrderFor (Mode mode, double sampleRate)
    {
        int order = mode == Mode::clean ? 12 : mode == Mode::voice ? 11 : 10;
        if (sampleRate > 66000.0) ++order;   // 88.2 / 96 kHz: same time span, same resolution in Hz
        if (sampleRate > 132000.0) ++order;  // 176.4 / 192 kHz
        return order;
    }

    // Allocates; call from prepareToPlay.
    void prepare (double sampleRate, int maxBlockSize, int numChannels)
    {
        sr = sampleRate;
        channels = std::clamp (numChannels, 1, maxChannels);
        maxBlock = std::max (1, maxBlockSize);
        int maxLatency = 0;
        for (int m = 0; m < numModes; ++m)
        {
            shifters[(size_t) m].prepare (channels, fftOrderFor ((Mode) m, sr));
            shifters[(size_t) m].setLifter ((int) std::lround (sr * 0.0025));
            maxLatency = std::max (maxLatency, shifters[(size_t) m].getLatencySamples());
        }
        delaySize = 1;
        while (delaySize < maxLatency + maxBlock + 1) delaySize <<= 1;
        for (auto& d : dryDelay) d.assign ((size_t) delaySize, 0.0f);
        for (auto& w : wet) w.assign ((size_t) maxBlock, 0.0f);
        wetGain.prepare (sr, 0.03);
        dryGain.prepare (sr, 0.03);
        outGain.prepare (sr, 0.02);
        modeFade.prepare (sr, 0.01);
        activeMode = params.mode;
        setParams (params, true);
        reset();
    }

    void reset()
    {
        for (auto& s : shifters) s.reset();
        for (auto& d : dryDelay) std::fill (d.begin(), d.end(), 0.0f);
        writePos = 0;
        modeFade.setImmediate (1.0f);
        switching = false;
    }

    void setParams (const PitchParams& p, bool immediate = false)
    {
        params = p;
        ratio = (float) std::pow (2.0, ((double) p.semitones + (double) p.cents * 0.01) / 12.0);
        const float formantShift = (float) std::pow (2.0, (double) p.formantSemis / 12.0);
        formantsOn = p.keepFormants || std::fabs (p.formantSemis) > 1.0e-3f;
        envRatio = (p.keepFormants ? 1.0f : ratio) * formantShift;
        for (auto& s : shifters)
        {
            s.setRatio (ratio);
            s.setFormants (formantsOn, envRatio);
        }

        // 0 st, no formant move: the processed signal is not needed, the delayed input is exact.
        passThrough = std::fabs (ratio - 1.0f) < 1.0e-5f && (! formantsOn || std::fabs (envRatio - 1.0f) < 1.0e-5f);
        const float wetTarget = (p.original || passThrough) ? 0.0f : std::clamp (p.mix, 0.0f, 1.0f);
        const float dryTarget = (p.original || passThrough) ? 1.0f : 1.0f - std::clamp (p.mix, 0.0f, 1.0f);
        const float outTarget = p.original ? 1.0f : dsp::dbToGain (p.outputDb);
        if (immediate)
        {
            wetGain.setImmediate (wetTarget);
            dryGain.setImmediate (dryTarget);
            outGain.setImmediate (outTarget);
            activeMode = p.mode;
        }
        else
        {
            wetGain.setTarget (wetTarget);
            dryGain.setTarget (dryTarget);
            outGain.setTarget (outTarget);
            if (p.mode != activeMode && ! switching)
            {
                // fade out, swap the shifter (and the dry delay that matches it), fade back in
                switching = true;
                modeFade.setTarget (0.0f);
            }
        }
    }

    // Latency of the active mode, for setLatencySamples.
    int getLatencySamples() const { return shifters[(size_t) activeMode].getLatencySamples(); }
    Mode getActiveMode() const { return activeMode; }
    bool isPassThrough() const { return passThrough; }
    float getRatio() const { return ratio; }

    void process (float* const* io, int numChannels, int numSamples)
    {
        const int chs = std::min (numChannels, channels);
        int done = 0;
        while (done < numSamples)
        {
            const int n = std::min (numSamples - done, maxBlock);
            processChunk (io, chs, done, n);
            done += n;
        }
    }

private:
    void processChunk (float* const* io, int chs, int offset, int n)
    {
        auto& shifter = shifters[(size_t) activeMode];
        float* wetPtrs[maxChannels] = {};
        for (int c = 0; c < chs; ++c)
        {
            std::copy (io[c] + offset, io[c] + offset + n, wet[(size_t) c].begin());
            wetPtrs[c] = wet[(size_t) c].data();
        }
        shifter.process (wetPtrs, chs, n);

        const int latency = shifter.getLatencySamples();
        const int mask = delaySize - 1;
        for (int i = 0; i < n; ++i)
        {
            const float wg = wetGain.next(), dg = dryGain.next(), og = outGain.next() * modeFade.next();
            const int w = (writePos + i) & mask, r = (writePos + i - latency) & mask;
            for (int c = 0; c < chs; ++c)
            {
                auto& d = dryDelay[(size_t) c];
                d[(size_t) w] = io[c][offset + i];
                io[c][offset + i] = (wet[(size_t) c][(size_t) i] * wg + d[(size_t) r] * dg) * og;
            }
        }
        writePos = (writePos + n) & mask;

        if (switching && ! modeFade.isSmoothing() && modeFade.getTarget() < 0.5f)
        {
            activeMode = params.mode;
            shifters[(size_t) activeMode].reset();
            for (auto& d : dryDelay) std::fill (d.begin(), d.end(), 0.0f);
            modeFade.setTarget (1.0f);
            switching = false;
        }
    }

    double sr = 48000.0;
    int channels = 2, maxBlock = 512;
    PitchParams params;
    Mode activeMode = Mode::clean;
    std::array<dsp::SpectralShifter, numModes> shifters;
    std::array<std::vector<float>, maxChannels> dryDelay, wet;
    int delaySize = 1, writePos = 0;
    dsp::LinearSmoother wetGain, dryGain, outGain, modeFade;
    float ratio = 1.0f, envRatio = 1.0f;
    bool formantsOn = false, passThrough = true, switching = false;
};

} // namespace rommuser::pitchr
