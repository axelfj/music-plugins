// SPDX-License-Identifier: GPL-3.0-or-later
#include "../plugins/MasterCheckLive/Source/MeterEngine.h"
#include "../plugins/MasterCheckLive/Source/Platforms.h"
#include "TestHelpers.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

using namespace rommuser::mastercheck;
using namespace testhelpers;

namespace
{
constexpr double fs = 48000.0;
constexpr int block = 512;

// Stereo 997 Hz tone at the given level (dBFS) for `seconds`, appended to l / r.
void appendTone (std::vector<float>& l, std::vector<float>& r, double levelDb, double seconds)
{
    const double amp = std::pow (10.0, levelDb / 20.0);
    const size_t start = l.size();
    const int n = (int) std::lround (seconds * fs);
    for (int i = 0; i < n; ++i)
    {
        const float v = (float) (amp * std::sin (2.0 * pi * 997.0 * (double) (start + (size_t) i) / fs));
        l.push_back (v);
        r.push_back (v);
    }
}

MeterReadings measure (MeterEngine& e, const std::vector<float>& l, const std::vector<float>& r)
{
    MeterReadings m;
    for (size_t pos = 0; pos < l.size(); pos += (size_t) block)
    {
        const float* ch[] = { l.data() + pos, r.data() + pos };
        m = e.process (ch, 2, (int) std::min<size_t> ((size_t) block, l.size() - pos));
    }
    return m;
}
} // namespace

TEST_CASE ("Master Check Live: the audio passes through bit-identical")
{
    MeterEngine e;
    e.prepare (fs, block, 2);
    auto l = whiteNoise (20000, 0.8, 1), r = whiteNoise (20000, 0.8, 2);
    const auto lIn = l, rIn = r;
    for (size_t pos = 0; pos < l.size(); pos += (size_t) block)
    {
        float* ch[] = { l.data() + pos, r.data() + pos };
        e.process (ch, 2, (int) std::min<size_t> ((size_t) block, l.size() - pos));
    }
    REQUIRE (l == lIn);
    REQUIRE (r == rIn);
}

TEST_CASE ("Master Check Live: Tech 3341 case 1 - stereo 997 Hz at -23 dBFS reads -23.0 LUFS (M, S, I)")
{
    MeterEngine e;
    e.prepare (fs, block, 2);
    std::vector<float> l, r;
    appendTone (l, r, -23.0, 20.0);
    const auto m = measure (e, l, r);
    REQUIRE (m.momentaryLufs == Catch::Approx (-23.0).margin (0.1));
    REQUIRE (m.shortTermLufs == Catch::Approx (-23.0).margin (0.1));
    REQUIRE (m.integratedLufs == Catch::Approx (-23.0).margin (0.1));
    REQUIRE (m.maxShortTermLufs == Catch::Approx (-23.0).margin (0.1));
    REQUIRE (m.loudnessRangeLu < 0.5f);
}

TEST_CASE ("Master Check Live: Tech 3341 case 3 - quiet / loud / quiet integrates to the loud part")
{
    MeterEngine e;
    e.prepare (fs, block, 2);
    std::vector<float> l, r;
    appendTone (l, r, -36.0, 10.0);
    appendTone (l, r, -23.0, 60.0);
    appendTone (l, r, -36.0, 10.0);
    const auto m = measure (e, l, r);
    REQUIRE (m.integratedLufs == Catch::Approx (-23.0).margin (0.1));   // -36 is below the relative gate
}

TEST_CASE ("Master Check Live: Tech 3341 case 4 - -70 LUFS content is excluded by the absolute gate")
{
    MeterEngine e;
    e.prepare (fs, block, 2);
    std::vector<float> l, r;
    appendTone (l, r, -72.0, 10.0);
    appendTone (l, r, -23.0, 20.0);
    appendTone (l, r, -72.0, 10.0);
    REQUIRE (measure (e, l, r).integratedLufs == Catch::Approx (-23.0).margin (0.1));
}

TEST_CASE ("Master Check Live: Tech 3342 - -20 / -30 steps give a loudness range of 10 LU")
{
    MeterEngine e;
    e.prepare (fs, block, 2);
    std::vector<float> l, r;
    appendTone (l, r, -20.0, 20.0);
    appendTone (l, r, -30.0, 20.0);
    const auto m = measure (e, l, r);
    REQUIRE (m.loudnessRangeLu == Catch::Approx (10.0).margin (1.0));
    REQUIRE (m.maxShortTermLufs == Catch::Approx (-20.0).margin (0.1));
}

TEST_CASE ("Master Check Live: true peak catches the peaks between samples")
{
    MeterEngine e;
    e.prepare (fs, block, 2);
    // fs / 8 with a phase of pi / 8: every sample sits at cos(pi/8) = -0.69 dB below the real peak.
    std::vector<float> l, r;
    const double amp = std::pow (10.0, -6.0 / 20.0);
    for (int i = 0; i < 48000; ++i)
    {
        const float v = (float) (amp * std::sin (2.0 * pi * (fs / 8.0) * i / fs + pi / 8.0));
        l.push_back (v);
        r.push_back (v);
    }
    const auto m = measure (e, l, r);
    REQUIRE (m.samplePeakDb == Catch::Approx (-6.69).margin (0.05));
    REQUIRE (m.truePeakDb == Catch::Approx (-6.0).margin (0.1));
}

TEST_CASE ("Master Check Live: platform playback gain follows the Master Check rules")
{
    const auto& spotify = platforms[0];
    const auto& youtube = platforms[2];
    const auto& club = platforms[6];

    REQUIRE (playbackGainDb (spotify, -10.0f, -0.5f) == Catch::Approx (-4.0f));        // too loud: turned down
    REQUIRE (playbackGainDb (spotify, -18.0f, -1.0f) == Catch::Approx (0.0f));         // quiet but the peak already at -1 dBTP: no room
    REQUIRE (playbackGainDb (spotify, -18.0f, -4.0f) == Catch::Approx (3.0f));         // quiet with headroom: up to -1 dBTP
    REQUIRE (playbackGainDb (youtube, -18.0f, -6.0f) == Catch::Approx (0.0f));         // YouTube never turns up
    REQUIRE (playbackGainDb (youtube, -8.0f, -1.0f) == Catch::Approx (-6.0f));
    REQUIRE (playbackGainDb (club, -7.0f, -0.2f) == Catch::Approx (0.0f));             // no normalisation

    REQUIRE (verdict (spotify, -10.0f, -1.5f) == Verdict::fail);     // above -14 LUFS Spotify asks for -2 dBTP
    REQUIRE (verdict (spotify, -10.0f, -2.5f) == Verdict::ok);
    REQUIRE (verdict (spotify, -5.0f, -2.5f) == Verdict::warn);      // 9 dB louder than needed
    REQUIRE (verdict (youtube, -14.0f, -0.5f) == Verdict::fail);     // over -1 dBTP
    REQUIRE (verdict (club, -8.0f, -0.3f) == Verdict::ok);
    REQUIRE (verdict (club, -14.0f, -1.0f) == Verdict::warn);
    REQUIRE (verdict (club, -8.0f, 0.3f) == Verdict::fail);
    REQUIRE (verdict (spotify, -120.0f, -120.0f) == Verdict::info);
}
