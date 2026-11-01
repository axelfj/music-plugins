// Lookahead peak guard: a smooth gain that keeps a signal under a ceiling without clipping it.
//
// Used where a hard clamp would add aliasing back, e.g. after Clipper-R's downsampling filter, which
// rings a little above the ceiling. Per sample (all channels share one gain):
//   gReq = min (1, ceiling / peak)              peak = max over channels of |x|
//   m    = sliding minimum of gReq over lookahead + 1 samples
//   a    = two cascaded moving averages of m (lengths summing to lookahead + 1): an S-shaped ramp
//          that reaches gReq exactly when the delayed peak arrives (same core as Limiter-R)
//   env  = a if a < env, else env + release * (a - env)
//   gain = min (env, gReq of the sample leaving now)       (rounding guard)
// The caller delays the audio by `lookahead` samples and multiplies it by the returned gain.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace rommuser::dsp
{

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

class PeakGuard
{
public:
    void prepare (double sampleRate, int lookaheadSamples, float releaseMs)
    {
        lookahead = std::max (1, lookaheadSamples);
        slidingMin.prepare (lookahead + 1);
        len1 = (lookahead + 2) / 2;
        len2 = std::max (1, lookahead + 1 - len1);
        buf1.assign ((size_t) len1, 1.0f);
        buf2.assign ((size_t) len2, 1.0f);
        hist.assign ((size_t) lookahead + 1, 1.0f);
        release = (float) (1.0 - std::exp (-1.0 / (std::max (1.0f, releaseMs) * 0.001 * sampleRate)));
        reset();
    }

    void reset()
    {
        slidingMin.reset();
        std::fill (buf1.begin(), buf1.end(), 1.0f);
        std::fill (buf2.begin(), buf2.end(), 1.0f);
        std::fill (hist.begin(), hist.end(), 1.0f);
        sum1 = (double) len1;
        sum2 = (double) len2;
        pos1 = pos2 = histPos = 0;
        env = 1.0f;
    }

    int getLatencySamples() const { return lookahead; }

    // Feed the newest sample's peak (max |x| over channels) and its ceiling; returns the gain for
    // the sample `lookahead` samples ago.
    inline float process (float peak, float ceiling)
    {
        const float gReq = peak > ceiling ? ceiling / peak : 1.0f;
        const float mn = slidingMin.push (gReq);
        sum1 += (double) mn - (double) buf1[(size_t) pos1];
        buf1[(size_t) pos1] = mn;
        if (++pos1 == len1) pos1 = 0;
        const float a1 = (float) (sum1 / (double) len1);
        sum2 += (double) a1 - (double) buf2[(size_t) pos2];
        buf2[(size_t) pos2] = a1;
        if (++pos2 == len2) pos2 = 0;
        hist[(size_t) histPos] = gReq;
        if (++histPos == lookahead + 1) histPos = 0;
        const float due = hist[(size_t) histPos]; // gReq of the sample leaving now
        const float a = (float) (sum2 / (double) len2);
        env = a < env ? a : env + release * (a - env);
        return std::min (env, due);
    }

private:
    SlidingMin slidingMin;
    std::vector<float> buf1, buf2, hist;
    double sum1 = 0.0, sum2 = 0.0;
    int lookahead = 1, len1 = 1, len2 = 1, pos1 = 0, pos2 = 0, histPos = 0;
    float release = 0.01f, env = 1.0f;
};

} // namespace rommuser::dsp
