// Delivery platforms: what each one does to loudness. Same values as the Master Check desktop app
// (master_check/profiles.py), so the two tools always agree.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

namespace rommuser::mastercheck
{

struct Platform
{
    const char* id;
    const char* name;
    float targetLufs;        // NaN = no normalisation (club)
    bool turnsUp;            // quiet masters are raised (only up to maxTruePeak)
    float maxTruePeakDb;     // what the platform asks for
    float maxTruePeakLoudDb; // stricter ceiling above the target (Spotify: -2 dBTP above -14 LUFS); NaN = none
    float clubLowLufs, clubHighLufs;   // club only: the usual range
    bool estimated;          // value not published by the platform itself
};

inline constexpr float none = std::numeric_limits<float>::quiet_NaN();

inline constexpr Platform platforms[] = {
    { "spotify", "Spotify",         -14.0f, true,  -1.0f, -2.0f, none,   none,  false },
    { "apple",   "Apple Music",     -16.0f, true,  -1.0f, none,  none,   none,  true },
    { "youtube", "YouTube",         -14.0f, false, -1.0f, none,  none,   none,  true },
    { "amazon",  "Amazon Music",    -14.0f, false, -1.0f, none,  none,   none,  true },
    { "tidal",   "TIDAL",           -14.0f, false, -1.0f, none,  none,   none,  true },
    { "deezer",  "Deezer",          -15.0f, false, -1.0f, none,  none,   none,  true },
    { "club",    "Beatport / club", none,   false, 0.0f,  none,  -10.0f, -6.0f, false },
};
inline constexpr int numPlatforms = (int) (sizeof (platforms) / sizeof (platforms[0]));

inline bool isClub (const Platform& p) { return std::isnan (p.targetLufs); }

// Gain the platform applies on playback, given the integrated loudness and the true peak (dB).
// Mirrors master_check/analysis.py: platforms that do not turn up never apply positive gain; the ones
// that do stop where the true peak would reach their ceiling.
inline float playbackGainDb (const Platform& p, float integratedLufs, float truePeakDb)
{
    if (isClub (p) || integratedLufs <= -99.0f)
        return 0.0f;
    float gain = p.targetLufs - integratedLufs;
    if (gain > 0.0f)
    {
        if (! p.turnsUp)
            return 0.0f;
        gain = std::min (gain, std::max (0.0f, p.maxTruePeakDb - truePeakDb));
    }
    return gain;
}

enum class Verdict { ok, warn, fail, info };

// The true-peak ceiling that applies to this master on this platform.
inline float applicableTruePeakCeiling (const Platform& p, float integratedLufs)
{
    if (! std::isnan (p.maxTruePeakLoudDb) && ! isClub (p) && integratedLufs > p.targetLufs)
        return p.maxTruePeakLoudDb;
    return p.maxTruePeakDb;
}

inline Verdict verdict (const Platform& p, float integratedLufs, float truePeakDb)
{
    if (integratedLufs <= -99.0f)
        return Verdict::info;
    if (truePeakDb > applicableTruePeakCeiling (p, integratedLufs) + 0.05f)
        return Verdict::fail;
    if (isClub (p))
        return (integratedLufs < p.clubLowLufs - 1.0f || integratedLufs > p.clubHighLufs + 1.0f) ? Verdict::warn : Verdict::ok;
    const float gain = playbackGainDb (p, integratedLufs, truePeakDb);
    if (gain < -6.0f)
        return Verdict::warn;    // much louder than needed: lost dynamics for nothing
    return Verdict::ok;
}

} // namespace rommuser::mastercheck
