// SPDX-License-Identifier: GPL-3.0-or-later
#include "../shared/dsp/FFT.h"
#include "TestHelpers.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

using namespace rommuser::dsp;

namespace
{
// Reference DFT in double precision.
std::vector<std::complex<double>> dft (const std::vector<FFT::Complex>& x, bool inverse)
{
    const size_t n = x.size();
    std::vector<std::complex<double>> y (n);
    const double sgn = inverse ? 1.0 : -1.0;
    for (size_t k = 0; k < n; ++k)
    {
        std::complex<double> acc = 0.0;
        for (size_t t = 0; t < n; ++t)
            acc += std::complex<double> (x[t]) * std::polar (1.0, sgn * 2.0 * testhelpers::pi * (double) (k * t % n) / (double) n);
        y[k] = inverse ? acc / (double) n : acc;
    }
    return y;
}

double maxError (const std::vector<FFT::Complex>& a, const std::vector<std::complex<double>>& b)
{
    double e = 0.0;
    for (size_t i = 0; i < a.size(); ++i) e = std::max (e, std::abs (std::complex<double> (a[i]) - b[i]));
    return e;
}
} // namespace

TEST_CASE ("FFT matches a direct DFT in both directions", "[fft]")
{
    const int order = GENERATE (1, 2, 3, 5, 8, 11);
    FFT fft;
    fft.prepare (order);
    const int n = 1 << order;
    const auto re = testhelpers::whiteNoise (n, 1.0, 11), im = testhelpers::whiteNoise (n, 1.0, 12);
    std::vector<FFT::Complex> x ((size_t) n);
    for (int i = 0; i < n; ++i) x[(size_t) i] = { re[(size_t) i], im[(size_t) i] };

    auto y = x;
    fft.forward (y.data());
    INFO ("order " << order);
    REQUIRE (maxError (y, dft (x, false)) < 2e-5 * n);

    auto z = x;
    fft.inverse (z.data());
    REQUIRE (maxError (z, dft (x, true)) < 2e-6);

    fft.inverse (y.data()); // round trip
    REQUIRE (maxError (y, std::vector<std::complex<double>> (x.begin(), x.end())) < 1e-5);
}

TEST_CASE ("Two real signals share one complex FFT", "[fft]")
{
    FFT fft;
    fft.prepare (10);
    const int n = 1024;
    const auto x = testhelpers::whiteNoise (n, 1.0, 3), y = testhelpers::whiteNoise (n, 1.0, 4);
    std::vector<FFT::Complex> work ((size_t) n), X ((size_t) n), Y ((size_t) n), rx ((size_t) n), ry ((size_t) n);
    fft.forwardTwoReal (x.data(), y.data(), work.data(), X.data(), Y.data());
    for (int i = 0; i < n; ++i) { rx[(size_t) i] = { x[(size_t) i], 0.0f }; ry[(size_t) i] = { y[(size_t) i], 0.0f }; }
    fft.forward (rx.data());
    fft.forward (ry.data());
    double e = 0.0;
    for (int k = 0; k < n; ++k)
        e = std::max ({ e, (double) std::abs (X[(size_t) k] - rx[(size_t) k]), (double) std::abs (Y[(size_t) k] - ry[(size_t) k]) });
    REQUIRE (e < 1e-3);

    fft.inverseTwoReal (X.data(), Y.data(), work.data());
    double r = 0.0;
    for (int i = 0; i < n; ++i)
        r = std::max ({ r, (double) std::fabs (work[(size_t) i].real() - x[(size_t) i]), (double) std::fabs (work[(size_t) i].imag() - y[(size_t) i]) });
    REQUIRE (r < 1e-5);
}
