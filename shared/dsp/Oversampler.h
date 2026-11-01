// Linear-phase polyphase oversampler built from cascaded 2x halfband FIR stages.
//
// Pure C++ (no JUCE) so the DSP can be unit-tested headless.
// Latency is always an integer number of base-rate samples, so a plain delay line
// keeps the dry signal sample-aligned with the processed one.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <vector>

namespace rommuser::dsp
{

namespace detail
{
    // Zeroth-order modified Bessel function of the first kind (series expansion).
    inline double besselI0 (double x)
    {
        double sum = 1.0, term = 1.0;
        const double q = x * x / 4.0;
        for (int k = 1; k < 64; ++k)
        {
            term *= q / (double (k) * double (k));
            sum += term;
            if (term < sum * 1e-17)
                break;
        }
        return sum;
    }

    // Designs a Kaiser-windowed halfband lowpass (cutoff = quarter of the stage's sample rate).
    // transitionWidth is in cycles/sample of the stage's (higher) rate, attenuationDb is the
    // stopband target. Length is rounded up to 4m + 3 so the centre tap index is odd, which is
    // what makes every other tap exactly zero and the polyphase split below valid.
    inline std::vector<double> designHalfband (double transitionWidth, double attenuationDb)
    {
        const double pi = 3.14159265358979323846;
        const double beta = attenuationDb > 50.0 ? 0.1102 * (attenuationDb - 8.7)
                                                 : 0.5842 * std::pow (attenuationDb - 21.0, 0.4) + 0.07886 * (attenuationDb - 21.0);
        auto n = (int) std::ceil ((attenuationDb - 7.95) / (14.36 * transitionWidth)) + 1;
        n = std::max (n, 7);
        while ((n - 3) % 4 != 0)
            ++n;

        std::vector<double> h ((size_t) n, 0.0);
        const int c = (n - 1) / 2;
        const double i0Beta = besselI0 (beta);

        for (int i = 0; i < n; ++i)
        {
            const int k = i - c;
            if (k == 0)
            {
                h[(size_t) i] = 0.5;
                continue;
            }
            if (k % 2 == 0)
                continue; // exact zeros of the halfband

            const double sinc = std::sin (pi * k / 2.0) / (pi * k);
            const double r = double (k) / double (c);
            const double w = besselI0 (beta * std::sqrt (std::max (0.0, 1.0 - r * r))) / i0Beta;
            h[(size_t) i] = sinc * w;
        }
        return h;
    }

    // Fixed-length FIR history with a mirrored buffer so a contiguous window is always available.
    class History
    {
    public:
        void resize (size_t length)
        {
            len = length;
            buf.assign (2 * length, 0.0f);
            pos = 0;
        }

        void clear() { std::fill (buf.begin(), buf.end(), 0.0f); pos = 0; }

        void push (float x)
        {
            pos = (pos == 0 ? len : pos) - 1;
            buf[pos] = x;
            buf[pos + len] = x;
        }

        // Newest sample at index 0, oldest at len - 1.
        const float* window() const { return buf.data() + pos; }

    private:
        std::vector<float> buf;
        size_t len = 0, pos = 0;
    };
} // namespace detail

// One 2x up/down stage. Up and down paths keep separate state.
class HalfbandStage
{
public:
    HalfbandStage (double transitionWidth, double attenuationDb)
    {
        const auto h = detail::designHalfband (transitionWidth, attenuationDb);
        length = (int) h.size();
        centre = (length - 1) / 2;

        // Even-index taps form the only non-trivial polyphase branch. The branch is symmetric
        // (tap k equals tap T - 1 - k, T even), so only the first half is stored and each
        // coefficient multiplies the sum of the two samples that share it: half the multiplies.
        std::vector<float> evenTaps;
        for (int i = 0; i < length; i += 2)
            evenTaps.push_back ((float) h[(size_t) i]);
        numEven = evenTaps.size();
        assert (numEven % 2 == 0);
        halfTaps.assign (evenTaps.begin(), evenTaps.begin() + (std::ptrdiff_t) (numEven / 2));

        upHistory.resize (numEven);
        downEvenHistory.resize (numEven);
        downOddHistory.resize ((size_t) (centre + 1) / 2 + 1);
    }

    void reset()
    {
        upHistory.clear();
        downEvenHistory.clear();
        downOddHistory.clear();
    }

    // Group delay of the up+down pair, in samples of the stage's lower rate.
    // The pair delays by 2 * centre samples at the higher rate, i.e. centre samples here.
    int roundTripDelayLowRate() const { return centre; }
    int numTaps() const { return length; }

    // in: n samples at the low rate. out: 2n samples at the high rate.
    void upsample (const float* in, float* out, int n)
    {
        const auto oddOffset = (size_t) (centre - 1) / 2;

        for (int i = 0; i < n; ++i)
        {
            upHistory.push (in[i]);
            const float* w = upHistory.window();
            const float acc = symmetricDot (w);

            // Zero-stuffing halves the energy, hence the factor 2 (centre tap 0.5 * 2 = 1).
            out[2 * i] = 2.0f * acc;
            out[2 * i + 1] = w[oddOffset];
        }
    }

    // in: 2n samples at the high rate. out: n samples at the low rate.
    void downsample (const float* in, float* out, int n)
    {
        const auto oddOffset = (size_t) (centre + 1) / 2;

        for (int i = 0; i < n; ++i)
        {
            // z[m] = sum_j h[2j] y[2m - 2j] + 0.5 * y[2m - centre]
            downOddHistory.push (in[2 * i + 1]);
            downEvenHistory.push (in[2 * i]);

            const float acc = symmetricDot (downEvenHistory.window());

            // y[2m - centre] with centre odd lives in the odd-sample history:
            // odd sample index 2m - centre = 2(m - (centre + 1) / 2) + 1, pushed (centre + 1) / 2 calls ago.
            out[i] = acc + 0.5f * downOddHistory.window()[oddOffset];
        }
    }

private:
    // sum_k evenTap[k] * w[k] over the full symmetric branch. Four independent partial sums let
    // the compiler keep several multiply-adds in flight (and vectorise) without -ffast-math.
    inline float symmetricDot (const float* w) const
    {
        const size_t half = halfTaps.size();
        const float* c = halfTaps.data();
        const float* tail = w + numEven - 1;
        float a0 = 0.0f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
        size_t k = 0;
        for (; k + 4 <= half; k += 4)
        {
            a0 += c[k] * (w[k] + tail[-(std::ptrdiff_t) k]);
            a1 += c[k + 1] * (w[k + 1] + tail[-(std::ptrdiff_t) (k + 1)]);
            a2 += c[k + 2] * (w[k + 2] + tail[-(std::ptrdiff_t) (k + 2)]);
            a3 += c[k + 3] * (w[k + 3] + tail[-(std::ptrdiff_t) (k + 3)]);
        }
        for (; k < half; ++k)
            a0 += c[k] * (w[k] + tail[-(std::ptrdiff_t) k]);
        return (a0 + a1) + (a2 + a3);
    }

    int length = 0, centre = 0;
    size_t numEven = 0;
    std::vector<float> halfTaps;
    detail::History upHistory, downEvenHistory, downOddHistory;
};

// Integer delay line. Capacity is allocated up front so the delay can change on the audio thread.
class DelayLine
{
public:
    void setMaxDelay (int maxSamples)
    {
        buf.assign ((size_t) std::max (0, maxSamples) + 1, 0.0f);
        pos = 0;
        delay = std::min (delay, maxSamples);
    }

    // Changing the delay clears the history (callers switch delay only on discontinuities anyway).
    void setDelay (int samples)
    {
        if ((size_t) samples + 1 > buf.size())
            setMaxDelay (samples);
        delay = std::max (0, samples);
        reset();
    }

    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); pos = 0; }
    int getDelay() const { return delay; }

    float process (float x)
    {
        if (delay == 0)
            return x;
        buf[pos] = x;
        if (++pos > (size_t) delay) // wrap without a division (size is delay + 1)
            pos = 0;
        return buf[pos];
    }

private:
    std::vector<float> buf { 0.0f };
    size_t pos = 0;
    int delay = 0;
};

// Mono oversampler, factor 1, 2, 4 or 8.
class Oversampler
{
public:
    static constexpr int maxStages = 3;

    // Stopband target for every stage. -110 dB keeps images and aliases far below 24-bit dither
    // for typical program material while staying cheap enough for 8x on a laptop.
    static constexpr double attenuationDb = 110.0;

    explicit Oversampler (int maxBlockSize = 512) { prepare (1, maxBlockSize); }

    void prepare (int newFactor, int maxBlockSize)
    {
        assert (newFactor == 1 || newFactor == 2 || newFactor == 4 || newFactor == 8);
        factor = newFactor;
        stages.clear();

        // Passband edge is 0.45 of the base rate (19.8 kHz at 44.1 kHz).
        // Stage k (1-based) runs at 2^k x base. Images of the stage below start at
        // 2^(k-1) - 0.45 (in base-rate units); normalise both edges to the stage rate.
        const double passEdge = 0.45;
        int numStages = factor == 1 ? 0 : factor == 2 ? 1 : factor == 4 ? 2 : 3;
        for (int k = 1; k <= numStages; ++k)
        {
            const double rate = std::pow (2.0, k);
            const double stopEdge = rate / 2.0 - passEdge;
            stages.emplace_back ((stopEdge - passEdge) / rate, attenuationDb);
        }

        // Total round-trip delay expressed at the top rate; pad so it is a whole base-rate sample.
        int topDelay = 0;
        for (int k = 0; k < numStages; ++k)
            topDelay += stages[(size_t) k].roundTripDelayLowRate() * 2 * (factor >> (k + 1));
        const int pad = (factor - topDelay % factor) % factor;
        topPad.setDelay (pad);
        latency = (topDelay + pad) / factor;

        scratchA.assign ((size_t) (maxBlockSize * factor), 0.0f);
        scratchB.assign ((size_t) (maxBlockSize * factor), 0.0f);
        maxBlock = maxBlockSize;
    }

    void reset()
    {
        for (auto& s : stages)
            s.reset();
        topPad.reset();
    }

    int getFactor() const { return factor; }
    int getLatencySamples() const { return latency; }
    int getMaxBlockSize() const { return maxBlock; }

    // Upsamples n base-rate samples. Returns a pointer to n * factor samples, valid until the next call.
    float* upsample (const float* in, int n)
    {
        assert (n <= maxBlock);
        if (stages.empty())
        {
            std::copy (in, in + n, scratchA.begin());
            return scratchA.data();
        }

        const float* src = in;
        float* dst = scratchA.data();
        int len = n;
        for (size_t k = 0; k < stages.size(); ++k)
        {
            stages[k].upsample (src, dst, len);
            len *= 2;
            src = dst;
            dst = (dst == scratchA.data() ? scratchB.data() : scratchA.data());
        }
        return const_cast<float*> (src);
    }

    // Downsamples n * factor samples (normally the buffer returned by upsample) into n base-rate samples.
    void downsample (float* highRate, float* out, int n)
    {
        if (stages.empty())
        {
            std::copy (highRate, highRate + n, out);
            return;
        }

        const int topLen = n * factor;
        for (int i = 0; i < topLen; ++i)
            highRate[i] = topPad.process (highRate[i]);

        float* src = highRate;
        float* other = (highRate == scratchA.data() ? scratchB.data() : scratchA.data());
        int len = topLen;
        for (int k = (int) stages.size() - 1; k >= 0; --k)
        {
            len /= 2;
            float* dst = (k == 0) ? out : other;
            stages[(size_t) k].downsample (src, dst, len);
            other = src;
            src = dst;
        }
    }

private:
    int factor = 1, latency = 0, maxBlock = 0;
    std::vector<HalfbandStage> stages;
    DelayLine topPad;
    std::vector<float> scratchA, scratchB;
};

} // namespace rommuser::dsp
