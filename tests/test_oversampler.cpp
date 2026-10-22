// SPDX-License-Identifier: GPL-3.0-or-later
#include "../shared/dsp/Oversampler.h"
#include "TestHelpers.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

using namespace rommuser::dsp;
using namespace testhelpers;

namespace
{
constexpr double fs = 44100.0;
constexpr int block = 256;

std::vector<float> roundTrip (Oversampler& os, const std::vector<float>& in)
{
    std::vector<float> out (in.size());
    for (size_t pos = 0; pos < in.size(); pos += block)
    {
        const int n = (int) std::min<size_t> (block, in.size() - pos);
        float* hi = os.upsample (in.data() + pos, n);
        os.downsample (hi, out.data() + pos, n);
    }
    return out;
}

// RMS error (dB relative to the signal) between out and in delayed by `latency`, skipping the warm-up.
double alignedErrorDb (const std::vector<float>& in, const std::vector<float>& out, int latency, int skip)
{
    double err = 0.0, sig = 0.0;
    for (size_t i = (size_t) (skip + latency); i < out.size(); ++i)
    {
        const double d = (double) out[i] - in[i - (size_t) latency];
        err += d * d;
        sig += (double) in[i - (size_t) latency] * in[i - (size_t) latency];
    }
    return 10.0 * std::log10 (err / sig);
}
} // namespace

TEST_CASE ("Halfband design has exact zeros and unit DC gain", "[oversampler]")
{
    const auto h = detail::designHalfband (0.05, 110.0);
    REQUIRE ((h.size() - 3) % 4 == 0);
    const int c = (int) (h.size() - 1) / 2;
    double sum = 0.0;
    for (size_t i = 0; i < h.size(); ++i)
    {
        sum += h[i];
        const int k = (int) i - c;
        if (k != 0 && k % 2 == 0)
            REQUIRE (h[i] == 0.0);
    }
    REQUIRE (std::fabs (sum - 1.0) < 1e-6);
}

TEST_CASE ("Oversampler latency is a whole number of samples and the round trip is transparent", "[oversampler]")
{
    const int factor = GENERATE (1, 2, 4, 8);
    Oversampler os (block);
    os.prepare (factor, block);

    const int latency = os.getLatencySamples();
    INFO ("factor " << factor << " latency " << latency);
    if (factor == 1)
        REQUIRE (latency == 0);

    // Low-mid, high and near the passband edge.
    for (double f : { 997.0, 9000.0, 18000.0 })
    {
        os.reset();
        const auto in = sine (f, fs, 1 << 15, 0.5);
        const auto out = roundTrip (os, in);
        const double e = alignedErrorDb (in, out, latency, 2048);
        INFO ("freq " << f << " error " << e << " dB");
        // Float processing plus passband ripple; -90 dB is far below audibility.
        REQUIRE (e < -90.0);
    }
}

TEST_CASE ("Upsampling rejects images by at least 100 dB", "[oversampler]")
{
    const int factor = GENERATE (2, 4, 8);
    Oversampler os (block);
    os.prepare (factor, block);

    const size_t n = 1 << 15;
    const auto in = sine (15000.0, fs, (int) (n / (size_t) factor) + 4096, 0.9);
    std::vector<float> hiAll;
    for (size_t pos = 0; pos + block <= in.size(); pos += block)
    {
        float* hi = os.upsample (in.data() + pos, block);
        hiAll.insert (hiAll.end(), hi, hi + block * factor);
    }
    // Skip the filter warm-up, analyse a steady-state window.
    std::vector<float> win (hiAll.end() - (long) n, hiAll.end());
    const double hiRate = fs * factor;
    const auto p = powerSpectrum (win.data(), n);

    const double tone = toneLevelDb (p, hiRate, n, 15000.0);
    const double image = toneLevelDb (p, hiRate, n, fs - 15000.0);
    INFO ("factor " << factor << " tone " << tone << " dB, first image " << image << " dB");
    REQUIRE (std::fabs (tone - toDb (0.9)) < 0.05);
    REQUIRE (image - tone < -100.0);
}

TEST_CASE ("Delay line delays by exactly the requested amount", "[delay]")
{
    DelayLine d;
    d.setMaxDelay (64);
    d.setDelay (13);
    std::vector<float> out;
    for (int i = 0; i < 40; ++i)
        out.push_back (d.process (i == 0 ? 1.0f : 0.0f));
    for (int i = 0; i < 40; ++i)
        REQUIRE (out[(size_t) i] == (i == 13 ? 1.0f : 0.0f));
}
