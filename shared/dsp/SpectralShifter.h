// High-quality real-time pitch shifter for polyphonic material (Pitch-R).
//
// STFT with a Hann window, 8x overlap. Every frame:
//   1. Analysis with three windows (w, dw/dn, n.w) gives each bin's reassigned frequency and group
//      delay, so a partial's exact frequency and a transient's exact time are known per bin.
//   2. Peaks of the magnitude spectrum split it into regions (valley to valley). Every clear partial
//      is moved rigidly to ratio x its exact frequency (Laroche & Dolson 1999): moving the whole
//      lobe, instead of resampling the spectrum, keeps the window shape of the partial, so a sine
//      stays a pure sine and close partials do not leave ghost tones between them. Regions that
//      are only ripples (noise, clicks) are frequency-scaled instead.
//   3. Optional formant handling: a spectral envelope through the harmonic peaks (its slope set by
//      the cepstral pitch) is divided out at the source position and multiplied back at the
//      target, so voices keep their character (or get a new one).
//   4. The output phase is integrated with Phase Gradient Heap Integration (Prusa & Holighaus,
//      "Phase vocoder done right", 2017, real-time variant): starting from the loudest bins, phase
//      is carried forwards in time with the shifted frequency and across frequency with the group
//      delay. Partials stay coherent (no phasiness) and clicks stay clicks (no pre-echo smear),
//      with no transient detector to tune.
//   5. Synthesis: each partial's region is copied with its own phase relations, anchored to the
//      integrated phase at its peak; everything else takes the integrated phase bin by bin.
//      Stereo: one phase field for the channel sum, every channel keeps its own magnitude and its
//      phase difference to that sum, so the image is not smeared and mono in gives mono out.
//
// Latency is exactly the FFT size. Pure C++ (no JUCE) so it is unit-tested headless in tests/.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "FFT.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstring>
#include <vector>

namespace rommuser::dsp
{

class SpectralShifter
{
public:
    using Complex = std::complex<float>;
    static constexpr int maxChannels = 2;
    static constexpr int overlap = 8;

    // fftOrder 12 = 4096 samples (85 ms at 48 kHz). Allocates; call off the audio thread.
    void prepare (int numChannels, int fftOrder)
    {
        channels = std::clamp (numChannels, 1, maxChannels);
        N = 1 << fftOrder;
        H = N / overlap;
        half = N / 2;
        fft.prepare (fftOrder);
        envBins = std::min (1024, half);
        envFft.prepare ((int) std::lround (std::log2 (2 * envBins)));
        envWork.assign ((size_t) (2 * envBins), {});

        window.resize ((size_t) N);
        tWindow.resize ((size_t) N);
        const double twoPi = 2.0 * pi;
        for (int n = 0; n < N; ++n)
        {
            const double a = twoPi * n / N;
            window[(size_t) n] = (float) (0.5 - 0.5 * std::cos (a));
            tWindow[(size_t) n] = (float) (n - N / 2) * window[(size_t) n];
        }
        double olaSum = 0.0;  // sum of w^2 over the overlapping frames (constant for Hann at 8x)
        for (int m = 0; m < overlap; ++m)
            olaSum += (double) window[(size_t) (m * H)] * window[(size_t) (m * H)];
        outputGain = (float) (1.0 / olaSum);

        for (int c = 0; c < maxChannels; ++c)
        {
            inBuf[c].assign ((size_t) N, 0.0f);
            outBuf[c].assign ((size_t) N, 0.0f);
            spec[c].assign ((size_t) half + 1, {});
            dSpec[c].assign ((size_t) half + 1, {});
            rawSpec[c].assign ((size_t) half + 1, {});
            tSpec[c].assign ((size_t) half + 1, {});
            rel[c].assign ((size_t) half + 1, {});
            cent[c].assign ((size_t) half + 1, {});
            centMag[c].assign ((size_t) half + 1, 0.0f);
            out[c].assign ((size_t) half + 1, {});
        }
        work.assign ((size_t) N, {});
        const auto bins = (size_t) half + 1;
        for (auto* v : { &srcMag, &srcOmega, &srcTau, &mag, &omega, &dPhiK, &magPrev, &omegaPrev, &envelope, &gapGain, &binFormantGain })
            v->assign (bins, 0.0f);
        phase.assign (bins, 0.0);
        phasePrev.assign (bins, 0.0);
        done.assign (bins, 0);
        covered.assign (bins, 0);
        nearest.assign (bins, -1);
        refCent.assign (bins, {});
        srcPos.assign (bins, 0.0f);
        peaks.reserve (bins);
        queueNext.assign (2 * bins, -1);
        bucketHead.assign ((size_t) numBuckets, -1);
        reset();
    }

    void reset()
    {
        for (int c = 0; c < maxChannels; ++c)
        {
            std::fill (inBuf[c].begin(), inBuf[c].end(), 0.0f);
            std::fill (outBuf[c].begin(), outBuf[c].end(), 0.0f);
        }
        std::fill (magPrev.begin(), magPrev.end(), 0.0f);
        std::fill (omegaPrev.begin(), omegaPrev.end(), 0.0f);
        std::fill (phasePrev.begin(), phasePrev.end(), 0.0);
        pos = 0;
        hopCounter = 0;
    }

    int getLatencySamples() const { return N; }
    int getFftSize() const { return N; }

    // ratio = output / input frequency. formantRatio: where the spectral envelope goes, as a ratio of
    // its input position (1 = stays put, i.e. formants preserved; = ratio follows the pitch).
    // formantsOn = false skips the envelope stage entirely (envelope moves with the pitch).
    void setRatio (float r) { ratio = std::clamp (r, 0.125f, 8.0f); }
    void setFormants (bool on, float formantRatio)
    {
        formantsOn = on;
        envRatio = std::clamp (formantRatio, 0.125f, 8.0f);
    }

    // Fallback harmonic spacing for the formant envelope when no clear pitch is found, as a period
    // in samples (2.5 ms is a good value); it also bounds the pitch search (0.4 x .. 6 x).
    void setLifter (int samples) { lifter = std::max (4, samples); }

    // In-place, sample by sample through internal FIFOs, so any block size works.
    void process (float* const* io, int numChannels, int numSamples)
    {
        const int chs = std::min (numChannels, channels);
        for (int i = 0; i < numSamples; ++i)
        {
            for (int c = 0; c < chs; ++c)
            {
                inBuf[c][(size_t) pos] = io[c][i];
                io[c][i] = outBuf[c][(size_t) pos];
                outBuf[c][(size_t) pos] = 0.0f;
            }
            for (int c = chs; c < channels; ++c)  // a mono host feeding a stereo-prepared shifter
            {
                inBuf[c][(size_t) pos] = io[0][i];
                outBuf[c][(size_t) pos] = 0.0f;
            }
            pos = (pos + 1) & (N - 1);
            if (++hopCounter == H)
            {
                hopCounter = 0;
                processFrame();
            }
        }
    }

private:
    static constexpr double pi = 3.14159265358979323846;
    static constexpr double twoPiD = 2.0 * pi;

    void processFrame()
    {
        analyse();
        mapRegions();
        if (formantsOn)
            applyFormants();
        integratePhase();
        synthesise();
    }

    // --- 1. analysis ----------------------------------------------------------------------------
    void analyse()
    {
        // Frame starts at the oldest sample of the circular buffer (pos). One transform of the
        // unwindowed frame (both channels packed as L + iR) gives the Hann spectrum and the
        // derivative-window spectrum by 3-tap combinations of neighbouring bins:
        //   x.w  = x (1 - cos)/2  ->  X  = Xr(k)/2 - (Xr(k-1) + Xr(k+1))/4
        //   x.w' = x (pi/N) sin   ->  Xd = (pi/N) (Xr(k-1) - Xr(k+1)) / 2i
        for (int n = 0; n < N; ++n)
        {
            const size_t i = (size_t) ((pos + n) & (N - 1));
            work[(size_t) n] = { inBuf[0][i], channels > 1 ? inBuf[1][i] : 0.0f };
        }
        fft.forward (work.data());
        for (int k = 0; k <= half; ++k)
        {
            const Complex z = work[(size_t) k], zc = std::conj (work[(size_t) ((N - k) & (N - 1))]);
            rawSpec[0][(size_t) k] = 0.5f * (z + zc);
            rawSpec[1][(size_t) k] = divI (z - zc);
        }
        const float dScale = (float) (pi / N);
        for (int c = 0; c < channels; ++c)
        {
            const auto& xr = rawSpec[c];
            for (int k = 0; k <= half; ++k)
            {
                const Complex below = k == 0 ? std::conj (xr[1]) : xr[(size_t) k - 1];
                const Complex above = k == half ? std::conj (xr[(size_t) half - 1]) : xr[(size_t) k + 1];
                spec[c][(size_t) k] = 0.5f * xr[(size_t) k] - 0.25f * (below + above);
                const Complex d = below - above;
                dSpec[c][(size_t) k] = dScale * divI (d);
            }
        }
        // time-weighted window for both channels in one transform
        for (int n = 0; n < N; ++n)
        {
            const float l = inBuf[0][(size_t) ((pos + n) & (N - 1))];
            const float r = channels > 1 ? inBuf[1][(size_t) ((pos + n) & (N - 1))] : 0.0f;
            work[(size_t) n] = { l * tWindow[(size_t) n], r * tWindow[(size_t) n] };
        }
        fft.forward (work.data());
        for (int k = 0; k <= half; ++k)
        {
            const Complex z = work[(size_t) k], zc = std::conj (work[(size_t) ((N - k) & (N - 1))]);
            tSpec[0][(size_t) k] = 0.5f * (z + zc);
            tSpec[1][(size_t) k] = divI (z - zc);
        }

        float maxPow = 0.0f;
        for (int k = 0; k <= half; ++k)
        {
            float p = 0.0f;
            for (int c = 0; c < channels; ++c) p += std::norm (spec[c][(size_t) k]);
            srcMag[(size_t) k] = p;
            maxPow = std::max (maxPow, p);
        }
        const float eps = maxPow * 1e-12f + 1e-30f;
        const float binW = (float) (twoPiD / N);
        for (int k = 0; k <= half; ++k)
        {
            // Reference spectrum: the channel sum (falls back to the left channel if L and R cancel).
            Complex ref = spec[0][(size_t) k];
            if (channels > 1)
            {
                const Complex sum = spec[0][(size_t) k] + spec[1][(size_t) k];
                if (std::norm (sum) > 0.01f * srcMag[(size_t) k]) ref = sum;
            }
            const float sign = (k & 1) ? -1.0f : 1.0f;
            refCent[(size_t) k] = ref * sign;
            for (int c = 0; c < channels; ++c)
            {
                cent[c][(size_t) k] = spec[c][(size_t) k] * sign;
                centMag[c][(size_t) k] = absOf (spec[c][(size_t) k]);
            }
            const float refMag = absOf (ref);
            const Complex refUnit = refMag > 1e-20f ? std::conj (ref) / refMag : Complex (1.0f, 0.0f);
            float num = 0.0f, numT = 0.0f;
            for (int c = 0; c < channels; ++c)
            {
                const Complex X = spec[c][(size_t) k];
                const Complex d = dSpec[c][(size_t) k], t = tSpec[c][(size_t) k];
                rel[c][(size_t) k] = mul (X, refUnit);
                num += d.imag() * X.real() - d.real() * X.imag();    // Im (Xd conj X)
                numT += t.real() * X.real() + t.imag() * X.imag();   // Re (Xt conj X)
            }
            const float den = srcMag[(size_t) k] + eps;
            srcOmega[(size_t) k] = binW * (float) k - num / den;
            srcTau[(size_t) k] = numT / den;
            srcMag[(size_t) k] = std::sqrt (srcMag[(size_t) k]);
        }
    }

    // --- 2. rigid region shift ------------------------------------------------------------------
    void mapRegions()
    {
        peaks.clear();
        float maxMag = 0.0f;
        for (int k = 0; k <= half; ++k) maxMag = std::max (maxMag, srcMag[(size_t) k]);
        const float floorMag = maxMag * 1e-6f;
        const float binsPerRad = (float) (N / twoPiD);
        for (int k = 1; k < half; ++k)
        {
            const float m = srcMag[(size_t) k];
            if (m > floorMag && m >= srcMag[(size_t) k - 1] && m > srcMag[(size_t) k + 1])
            {
                // precise peak position from the reassigned frequency (stays inside the bin's lobe)
                float pf = srcOmega[(size_t) k] * binsPerRad;
                if (! (std::fabs (pf - (float) k) < 1.5f)) pf = (float) k;
                peaks.push_back ({ k, pf, pf * (ratio - 1.0f) });
            }
        }
        // each region's source bins run from valley to valley: [lo, hi), the last one up to Nyquist
        for (size_t i = 0; i < peaks.size(); ++i)
        {
            peaks[i].lo = i == 0 ? 0 : peaks[i - 1].hi;
            int v = half + 1;
            if (i + 1 < peaks.size())
            {
                v = peaks[i].bin;
                for (int b = peaks[i].bin; b <= peaks[i + 1].bin; ++b)
                    if (srcMag[(size_t) b] < srcMag[(size_t) v]) v = b;
            }
            peaks[i].hi = v;
        }
        // How clearly each peak is a partial: peak over its higher valley. Clear partials (> 18 dB)
        // are copied with their own phase relations; ripples of noise and clicks (< 6 dB) use the
        // integrated phase bin by bin, which is what keeps clicks sharp. In between: a blend.
        for (auto& pk : peaks)
        {
            const float vl = srcMag[(size_t) pk.lo];
            const float vr = srcMag[(size_t) std::min (pk.hi, half)];
            const float prom = srcMag[(size_t) pk.bin] / std::max (std::max (vl, vr), 1e-30f);
            pk.tonal = std::clamp ((20.0f * std::log10 (std::max (prom, 1e-6f)) - 6.0f) / 12.0f, 0.0f, 1.0f);
        }

        // Fields the phase integration works on, one per output bin, taken from the region whose
        // shifted peak is nearest. Bins in the gap between two regions (shifting up) read the
        // valley, so they carry valley level: nearly nothing between partials, flat for clicks.
        const float binW = (float) (twoPiD / N);
        size_t region = 0;
        for (int k = 0; k <= half; ++k)
        {
            float s, shiftBins;
            if (peaks.empty())
            {
                s = (float) k / ratio;
                shiftBins = (float) k - s;
            }
            else
            {
                while (region + 1 < peaks.size()
                       && (float) k > 0.5f * (peaks[region].pf + peaks[region].shift + peaks[region + 1].pf + peaks[region + 1].shift))
                    ++region;
                shiftBins = peaks[region].shift;
                s = std::clamp ((float) k - shiftBins, (float) peaks[region].lo, (float) std::min (peaks[region].hi, half));
                if (peaks[region].tonal < 0.5f)
                {
                    // noise / click ripple: no partial to keep whole, scale the spectrum instead
                    s = (float) k / ratio;
                    shiftBins = (float) k - s;
                }
            }
            srcPos[(size_t) k] = s;
            nearest[(size_t) k] = peaks.empty() ? -1 : (int) region;
            if (s < 0.0f || s > (float) half)
            {
                mag[(size_t) k] = 0.0f;
                gapGain[(size_t) k] = 0.0f;
                omega[(size_t) k] = binW * (float) k;
                dPhiK[(size_t) k] = (float) -pi;
                continue;
            }
            const int i0 = std::min ((int) s, half - 1);
            const float f = s - (float) i0;
            omega[(size_t) k] = lerp (srcOmega, i0, f) + shiftBins * binW;
            dPhiK[(size_t) k] = (float) (-twoPiD / N) * (lerp (srcTau, i0, f) + 0.5f * (float) N);
            mag[(size_t) k] = lerp (srcMag, i0, f);
            gapGain[(size_t) k] = 1.0f;
            if (! peaks.empty() && ! sameValue ((float) k - shiftBins, s))
            {
                // A gap bin reads the valley. Between two clear partials the true spectrum keeps
                // falling past the valley, so scale it by the valley's depth (valley / peak): a
                // deep valley (tones) leaves the gap nearly empty, a shallow one (clicks, noise)
                // fills it at the same level.
                const auto& pk = peaks[region];
                const float depth = srcMag[(size_t) std::min (pk.bin, half)] > 0.0f ? mag[(size_t) k] / srcMag[(size_t) pk.bin] : 0.0f;
                gapGain[(size_t) k] = std::min (1.0f, depth);
                mag[(size_t) k] *= gapGain[(size_t) k];
            }
        }
    }

    // Complex products written out: std::complex's operator* checks for inf/NaN (C99 Annex G)
    // unless built with -ffast-math, which is slow in these per-bin loops.
    static Complex mul (Complex a, Complex b)
    {
        return { a.real() * b.real() - a.imag() * b.imag(), a.real() * b.imag() + a.imag() * b.real() };
    }
    static Complex divI (Complex d) { return { 0.5f * d.imag(), -0.5f * d.real() }; }  // d / 2i

    // |z| without std::abs's overflow-safe hypot, which is far slower and not needed at audio levels
    static float absOf (Complex z) { return std::sqrt (z.real() * z.real() + z.imag() * z.imag()); }
    static bool sameValue (float a, float b) { return ! (a < b || a > b); }   // exact, without -Wfloat-equal

    static float lerp (const std::vector<float>& v, int i0, float f)
    {
        return v[(size_t) i0] * (1.0f - f) + v[(size_t) i0 + 1] * f;
    }

    static Complex lerp (const std::vector<Complex>& v, int i0, float f)
    {
        return v[(size_t) i0] * (1.0f - f) + v[(size_t) i0 + 1] * f;
    }

    float logEnvAt (float s) const
    {
        if (s <= 0.0f) return envelope[0];
        if (s >= (float) half) return envelope[(size_t) half];
        const int i0 = (int) s;
        return lerp (envelope, i0, s - (float) i0);
    }

    // Envelope correction for a bin moved from source position s to output bin k.
    float formantGain (int k, float s) const
    {
        if (! formantsOn) return 1.0f;
        return std::min (std::exp (logEnvAt ((float) k / envRatio) - logEnvAt (s)), 100.0f);  // +40 dB cap
    }

    // --- 3. formants ----------------------------------------------------------------------------
    void applyFormants()
    {
        // Spectral envelope that runs through the harmonic peaks: an upper hull of the log spectrum
        // whose slope is limited to what the harmonic spacing allows (so it bridges from one
        // harmonic to the next instead of dipping between them), then a lower hull with the same
        // slope to round it off. The spacing comes from the cepstral pitch peak; without a clear
        // pitch it falls back to the lifter setting.
        for (int k = 0; k <= half; ++k)
        {
            const float m = srcMag[(size_t) k];
            envelope[(size_t) k] = std::log (m * m + 1e-18f);
        }
        float spacing = (float) N / (float) lifter;
        {
            // decimated real cepstrum (peak-held), one small transform
            const int dec = half / envBins;
            for (int e = 0; e <= envBins; ++e)
            {
                float m = -60.0f;
                for (int k = std::max (0, e * dec - dec / 2); k <= std::min (half, e * dec + dec / 2); ++k) m = std::max (m, envelope[(size_t) k]);
                envWork[(size_t) e] = { m, 0.0f };
                if (e > 0 && e < envBins) envWork[(size_t) (2 * envBins - e)] = { m, 0.0f };
            }
            envFft.inverse (envWork.data());
            // Pitch periods from 1 kHz down to 60 Hz (lifter = 2.5 ms). Quefrency q of this decimated
            // cepstrum is the period in samples, since 2 envBins x dec = N.
            const int qLo = std::max (2, (2 * lifter) / 5), qHi = std::min (envBins - 1, 6 * lifter);
            int best = 0;
            float bestV = 0.0f;
            for (int q = qLo; q <= qHi; ++q)
                if (envWork[(size_t) q].real() > bestV) { bestV = envWork[(size_t) q].real(); best = q; }
            if (best > 0 && bestV > 0.2f)
                spacing = (float) N / (float) best;
        }
        const float slope = -std::log (1.0f - 1.0f / (0.5f * std::max (spacing, 1.0f) + 1.0f));  // per bin, log energy
        auto hull = [this] (float step, bool upper) {
            for (int pass = 0; pass < 2; ++pass)
            {
                float e = envelope[(size_t) half];
                for (int k = half; k >= 0; --k)
                {
                    e = upper ? std::max (envelope[(size_t) k], e - step) : std::min (envelope[(size_t) k], e + step);
                    envelope[(size_t) k] = e;
                }
                for (int k = 0; k <= half; ++k)
                {
                    e = upper ? std::max (envelope[(size_t) k], e - step) : std::min (envelope[(size_t) k], e + step);
                    envelope[(size_t) k] = e;
                }
            }
        };
        hull (slope, true);
        hull (slope, false);
        for (int k = 0; k <= half; ++k) envelope[(size_t) k] *= 0.5f;  // log amplitude

        for (int k = 0; k <= half; ++k)
        {
            binFormantGain[(size_t) k] = formantGain (k, srcPos[(size_t) k]);
            mag[(size_t) k] *= binFormantGain[(size_t) k];
        }
    }

    // --- 4. phase gradient heap integration -----------------------------------------------------
    struct Peak { int bin; float pf; float shift; int lo = 0, hi = 0; float tonal = 1.0f; };  // source bins [lo, hi)
    // "Heap" of bins to propagate from, loudest first. A bucket queue (0.75 dB steps, LIFO inside a
    // bucket) instead of a binary heap: O(1) per bin, and PGHI does not need a finer order.
    static constexpr int bucketsPerOctave = 8, numBuckets = 18 * bucketsPerOctave;

    int bucketOf (float m) const
    {
        float r = m * invMaxMag;
        if (! (r > 1.0e-6f)) return 0;
        if (r >= 1.0f) return numBuckets - 1;
        uint32_t bits;
        std::memcpy (&bits, &r, sizeof bits);
        const float log2r = (float) ((int) (bits >> 23) - 127) + (float) (bits & 0x7fffff) * (1.0f / 8388608.0f);
        return std::clamp ((int) ((log2r + 18.0f) * (float) bucketsPerOctave), 0, numBuckets - 1);
    }

    void queuePush (int item, float m)
    {
        const int b = bucketOf (m);
        queueNext[(size_t) item] = bucketHead[(size_t) b];
        bucketHead[(size_t) b] = item;
        topBucket = std::max (topBucket, b);
        ++queued;
    }

    int queuePop()  // item id: bin for the previous frame, half + 1 + bin for the current one
    {
        while (bucketHead[(size_t) topBucket] < 0) --topBucket;
        const int item = bucketHead[(size_t) topBucket];
        bucketHead[(size_t) topBucket] = queueNext[(size_t) item];
        --queued;
        return item;
    }

    void integratePhase()
    {
        float maxMag = 0.0f;
        for (int k = 0; k <= half; ++k) maxMag = std::max (maxMag, mag[(size_t) k]);
        const float tol = maxMag * 1e-5f;
        const double hop = (double) H;

        std::fill (done.begin(), done.end(), 0);
        int remaining = 0;
        for (int k = 0; k <= half; ++k)
        {
            if (mag[(size_t) k] <= tol)
            {
                // too quiet to matter: just carry the phase forwards
                phase[(size_t) k] = phasePrev[(size_t) k] + hop * 0.5 * ((double) omegaPrev[(size_t) k] + omega[(size_t) k]);
                done[(size_t) k] = 1;
            }
            else
                ++remaining;
        }

        std::fill (bucketHead.begin(), bucketHead.end(), -1);
        topBucket = 0;
        queued = 0;
        invMaxMag = maxMag > 0.0f ? 1.0f / maxMag : 0.0f;
        const int cur = half + 1;
        for (int k = 0; k <= half; ++k)
            if (magPrev[(size_t) k] > tol && ! done[(size_t) k])
                queuePush (k, magPrev[(size_t) k]);

        while (remaining > 0)
        {
            if (queued == 0)
            {
                // nothing left to propagate from (start, or after silence): seed the loudest bin
                int best = -1;
                for (int k = 0; k <= half; ++k)
                    if (! done[(size_t) k] && (best < 0 || mag[(size_t) k] > mag[(size_t) best])) best = k;
                phase[(size_t) best] = phasePrev[(size_t) best] + hop * 0.5 * ((double) omegaPrev[(size_t) best] + omega[(size_t) best]);
                done[(size_t) best] = 1;
                --remaining;
                queuePush (cur + best, mag[(size_t) best]);
            }
            const int item = queuePop();
            if (item < cur)
            {
                const int k = item;
                if (done[(size_t) k]) continue;
                phase[(size_t) k] = phasePrev[(size_t) k] + hop * 0.5 * ((double) omegaPrev[(size_t) k] + omega[(size_t) k]);
                done[(size_t) k] = 1;
                --remaining;
                queuePush (cur + k, mag[(size_t) k]);
            }
            else
            {
                const int k = item - cur;
                for (int nb : { k - 1, k + 1 })
                {
                    if (nb < 0 || nb > half || done[(size_t) nb]) continue;
                    const double step = 0.5 * ((double) dPhiK[(size_t) k] + dPhiK[(size_t) nb]);
                    phase[(size_t) nb] = phase[(size_t) k] + (nb > k ? step : -step);
                    done[(size_t) nb] = 1;
                    --remaining;
                    queuePush (cur + nb, mag[(size_t) nb]);
                }
            }
        }

        for (int k = 0; k <= half; ++k)
        {
            phase[(size_t) k] -= twoPiD * std::floor (phase[(size_t) k] * (1.0 / twoPiD) + 0.5);
            phasePrev[(size_t) k] = phase[(size_t) k];
            magPrev[(size_t) k] = mag[(size_t) k];
            omegaPrev[(size_t) k] = omega[(size_t) k];
        }
    }

    // --- 5. synthesis ---------------------------------------------------------------------------
    void synthesise()
    {
        for (int k = 0; k <= half; ++k)
        {
            out[0][(size_t) k] = {};
            out[1][(size_t) k] = {};
            covered[(size_t) k] = 0;
        }
        // Every region is copied whole, with its own bins' phase relations (identity phase locking)
        // anchored to the integrated phase at its shifted peak. Overlapping regions (shifting down)
        // add up, as the lobes of two close partials do in a real spectrum. Phases are handled in
        // frame-centre convention here ((-1)^k), where a steady partial's lobe has a flat phase and
        // can be interpolated between bins.
        for (const auto& pk : peaks)
        {
            const int kFrom = std::max (0, (int) std::ceil ((float) pk.lo + pk.shift));
            const int kTo = std::min (half, (int) std::ceil ((float) pk.hi + pk.shift) - 1);
            if (pk.tonal <= 0.0f)
            {
                for (int k = kFrom; k <= kTo; ++k) covered[(size_t) k] = 1;
                continue;
            }
            const int kStar = std::clamp ((int) std::lround (pk.pf + pk.shift), 0, half);
            const float sStar = std::clamp ((float) kStar - pk.shift, 0.0f, (float) half);
            const int a0 = std::min ((int) sStar, half - 1);
            const Complex refAtPeak = lerp (refCent, a0, sStar - (float) a0);
            const float anchor = (float) phase[(size_t) kStar] + (float) pi * (float) (kStar & 1) - std::arg (refAtPeak);
            const Complex rot = std::polar (pk.tonal, anchor);
            for (int k = kFrom; k <= kTo; ++k)
            {
                const float s = (float) k - pk.shift;
                if (s < 0.0f || s > (float) half) continue;
                const int i0 = std::min ((int) s, half - 1);
                const float f = s - (float) i0;
                const float fg = ! formantsOn ? 1.0f : (sameValue (s, srcPos[(size_t) k]) ? binFormantGain[(size_t) k] : formantGain (k, s));
                const Complex r = rot * fg * ((k & 1) ? -1.0f : 1.0f);
                for (int c = 0; c < channels; ++c)
                {
                    // A plain complex lerp dips where neighbouring bins turn apart (a click away
                    // from the frame centre), so there the magnitude is interpolated separately.
                    // Not across a sign flip (> 90 degrees): that is a lobe edge and must pass zero.
                    const Complex a = cent[c][(size_t) i0], b = cent[c][(size_t) i0 + 1];
                    Complex z = a * (1.0f - f) + b * f;
                    if (a.real() * b.real() + a.imag() * b.imag() > 0.0f)
                    {
                        const float zm = absOf (z);
                        if (zm > 1e-30f) z *= lerp (centMag[c], i0, f) / zm;
                    }
                    out[c][(size_t) k] += mul (z, r);
                }
                covered[(size_t) k] = 1;
            }
        }
        // Integrated phase bin by bin: the non-tonal share of every region, and the gaps no region
        // reaches (shifting up, or no partial at all).
        for (int k = 0; k <= half; ++k)
        {
            const int j = nearest[(size_t) k];
            const float w = (! covered[(size_t) k] || j < 0) ? gapGain[(size_t) k] : 1.0f - peaks[(size_t) j].tonal;
            if (w <= 0.0f) continue;
            const float s = srcPos[(size_t) k];
            if (s < 0.0f || s > (float) half) continue;
            const int i0 = std::min ((int) s, half - 1);
            const float f = s - (float) i0;
            const Complex r = std::polar ((formantsOn ? binFormantGain[(size_t) k] : 1.0f) * w, (float) phase[(size_t) k]);
            for (int c = 0; c < channels; ++c)
                out[c][(size_t) k] += mul (lerp (rel[c], i0, f), r);
        }

        // Both channels in one inverse transform: y = IFFT(Y_L + i Y_R) = y_L + i y_R.
        for (int k = 0; k <= half; ++k)
        {
            Complex yl = out[0][(size_t) k], yr = out[1][(size_t) k];
            if (k == 0 || k == half) { yl = { yl.real(), 0.0f }; yr = { yr.real(), 0.0f }; }
            work[(size_t) k] = { yl.real() - yr.imag(), yl.imag() + yr.real() };  // yl + i yr
            if (k > 0 && k < half)
                work[(size_t) (N - k)] = { yl.real() + yr.imag(), -yl.imag() + yr.real() };  // conj yl + i conj yr
        }
        fft.inverse (work.data());
        // The frame covers the last N input samples; its output lands N samples later (latency N),
        // i.e. at the same circular positions, which the FIFO plays back after one full turn.
        for (int n = 0; n < N; ++n)
        {
            const size_t idx = (size_t) ((pos + n) & (N - 1));
            const float w = window[(size_t) n] * outputGain;
            outBuf[0][idx] += work[(size_t) n].real() * w;
            if (channels > 1) outBuf[1][idx] += work[(size_t) n].imag() * w;
        }
    }

    int channels = 2, N = 4096, H = 512, half = 2048;
    FFT fft, envFft;
    int envBins = 1024;
    std::vector<Complex> envWork;
    std::vector<float> window, tWindow;
    float outputGain = 1.0f;
    std::vector<float> inBuf[maxChannels], outBuf[maxChannels];
    std::vector<Complex> spec[maxChannels], dSpec[maxChannels], rawSpec[maxChannels], tSpec[maxChannels], rel[maxChannels];
    std::vector<Complex> work, refCent, cent[maxChannels], out[maxChannels];
    std::vector<float> centMag[maxChannels];
    std::vector<float> srcMag, srcOmega, srcTau, mag, omega, dPhiK, magPrev, omegaPrev, envelope, gapGain, binFormantGain, srcPos;
    std::vector<double> phase, phasePrev;
    std::vector<uint8_t> done, covered;
    std::vector<int> nearest;
    std::vector<Peak> peaks;
    std::vector<int> queueNext, bucketHead;
    int topBucket = 0, queued = 0;
    float invMaxMag = 0.0f;
    int pos = 0, hopCounter = 0;
    float ratio = 1.0f, envRatio = 1.0f;
    bool formantsOn = false;
    int lifter = 120;
};

} // namespace rommuser::dsp
