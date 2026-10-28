// dsp_bench: CPU cost of every audio engine, measured headless.
//
// Each engine processes 20 s of stereo noise at 48 kHz in 512-sample blocks (after one second of
// warm-up), with its default parameters plus the settings named in the label. The result is the
// percentage of one core needed to run it in real time, the number to compare between versions.
//
//   cmake --build build --target dsp_bench && ./build/tools/dsp-bench/dsp_bench [filter] [seconds]
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../../plugins/BandPush/Source/PushEngine.h"
#include "../../plugins/ClipReady/Source/ClipEngine.h"
#include "../../plugins/DuckReady/Source/DuckEngine.h"
#include "../../plugins/EchoReady/Source/EchoEngine.h"
#include "../../plugins/EssClean/Source/EssEngine.h"
#include "../../plugins/FilterReady/Source/FilterEngine.h"
#include "../../plugins/GateReady/Source/GateEngine.h"
#include "../../plugins/LowMono/Source/MonoEngine.h"
#include "../../plugins/MasterCheckLive/Source/MeterEngine.h"
#include "../../plugins/PeakReady/Source/PeakEngine.h"
#include "../../plugins/PhaseReady/Source/PhaseEngine.h"
#include "../../plugins/PressReady/Source/CompEngine.h"
#include "../../plugins/PunchShape/Source/PunchEngine.h"
#include "../../plugins/ResTame/Source/ResTameEngine.h"
#include "../../plugins/ShapeEQ/Source/EqEngine.h"
#include "../../plugins/ShimmerReady/Source/ShimmerEngine.h"
#include "../../plugins/SpaceReady/Source/SpaceEngine.h"
#include "../../plugins/StutterReady/Source/StutterEngine.h"
#include "../../plugins/TuneReady/Source/TuneEngine.h"
#include "../../plugins/VoxSynth/Source/VoxEngine.h"
#include "../../plugins/WarmDrive/Source/DriveEngine.h"
#include "../../plugins/WideReady/Source/WideEngine.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <random>
#include <string>
#include <vector>

#if defined(__SSE__) || defined(_M_X64)
 #include <xmmintrin.h>
 #define DSP_BENCH_FTZ() _mm_setcsr (_mm_getcsr() | 0x8040)
#else
 #define DSP_BENCH_FTZ() ((void) 0)
#endif

namespace
{
constexpr double sr = 48000.0;
constexpr int block = 512;
int seconds = 20;

struct Bench
{
    std::string name;
    std::function<void (float* const*, int)> run; // processes one stereo block in place
};

double measure (const Bench& b)
{
    std::mt19937 rng (7);
    std::uniform_real_distribution<float> dist (-0.5f, 0.5f);
    std::vector<float> l ((size_t) block), r ((size_t) block), srcL ((size_t) sr), srcR ((size_t) sr);
    for (size_t i = 0; i < srcL.size(); ++i) { srcL[i] = dist (rng); srcR[i] = dist (rng); }
    float* io[2] { l.data(), r.data() };

    const int warm = (int) sr / block, blocks = seconds * (int) sr / block;
    double best = 1e30;
    for (int pass = 0; pass < 3; ++pass)
    {
        size_t pos = 0;
        auto fill = [&]
        {
            if (pos + block > srcL.size()) pos = 0;
            std::memcpy (l.data(), srcL.data() + pos, sizeof (float) * block);
            std::memcpy (r.data(), srcR.data() + pos, sizeof (float) * block);
            pos += block;
        };
        for (int i = 0; i < warm; ++i) { fill(); b.run (io, block); }
        double total = 0.0;
        for (int i = 0; i < blocks; ++i)
        {
            fill(); // copying is outside the timed region
            const auto t0 = std::chrono::steady_clock::now();
            b.run (io, block);
            total += std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        }
        best = std::min (best, total);
    }
    return 100.0 * best / (double) seconds;
}
} // namespace

int main (int argc, char** argv)
{
    DSP_BENCH_FTZ();
    const std::string filter = argc > 1 ? argv[1] : "";
    if (argc > 2) seconds = std::max (1, std::atoi (argv[2]));
    std::vector<Bench> benches;

    {
        static rommuser::clipready::ClipEngine e;
        e.prepare (sr, block, 2);
        auto p = rommuser::clipready::ClipParams {}; p.inputDb = 12.0f; p.oversampling = 4;
        e.setParams (p, true);
        benches.push_back ({ "Clipper-R (+12 dB, 4x)", [] (float* const* io, int n) { e.process (io, 2, n); } });
    }
    {
        static rommuser::clipready::ClipEngine e;
        e.prepare (sr, block, 2);
        auto p = rommuser::clipready::ClipParams {}; p.inputDb = 12.0f; p.oversampling = 8;
        e.setParams (p, true);
        benches.push_back ({ "Clipper-R (+12 dB, 8x)", [] (float* const* io, int n) { e.process (io, 2, n); } });
    }
    {
        static rommuser::warmdrive::DriveEngine e;
        e.prepare (sr, block, 2);
        benches.push_back ({ "Saturator-R (default)", [] (float* const* io, int n) { e.process (io, 2, n); } });
    }
    {
        static rommuser::shapeeq::EqEngine e;
        e.prepare (sr, block, 2);
        auto p = rommuser::shapeeq::EqParams {};
        for (int b = 0; b < 8; ++b) { p.bands[(size_t) b].enabled = true; p.bands[(size_t) b].freq = 60.0 * std::pow (2.0, b); p.bands[(size_t) b].gainDb = 3.0; }
        e.setParams (p, true);
        benches.push_back ({ "EQ-R (8 bells)", [] (float* const* io, int n) { e.process (io, 2, n); } });
    }
    {
        static rommuser::pressready::CompEngine e;
        e.prepare (sr, block, 2);
        benches.push_back ({ "Compressor-R (default)", [] (float* const* io, int n) { e.process (io, nullptr, 2, n); } });
    }
    {
        static rommuser::bandpush::PushEngine e;
        e.prepare (sr, 2);
        benches.push_back ({ "Multiband-R (default)", [] (float* const* io, int n) { e.process (io, 2, n); } });
    }
    {
        static rommuser::duckready::DuckEngine e;
        e.prepare (sr, 2);
        benches.push_back ({ "Sidechain-R (default)", [] (float* const* io, int n) { e.process (io, nullptr, 2, n, {}); } });
    }
    {
        static rommuser::gateready::GateEngine e;
        e.prepare (sr, 2);
        benches.push_back ({ "Gate-R (default)", [] (float* const* io, int n) { e.process (io, nullptr, 2, n); } });
    }
    {
        static rommuser::echoready::EchoEngine e;
        e.prepare (sr, 2);
        benches.push_back ({ "Delay-R (default)", [] (float* const* io, int n) { e.process (io, 2, n); } });
    }
    {
        static rommuser::phaseready::PhaseEngine e;
        e.prepare (sr, 2);
        auto p = rommuser::phaseready::PhaseParams {}; p.stages = 12;
        e.setParams (p);
        benches.push_back ({ "Phaser-R (12 stages)", [] (float* const* io, int n) { e.process (io, 2, n, {}); } });
    }
    {
        static rommuser::wideready::WideEngine e;
        e.prepare (sr, 2);
        benches.push_back ({ "Chorus-R (default)", [] (float* const* io, int n) { e.process (io, 2, n); } });
    }
    {
        static rommuser::spaceready::SpaceEngine e;
        e.prepare (sr, 2);
        benches.push_back ({ "Reverb-R (default)", [] (float* const* io, int n) { e.process (io, 2, n); } });
    }
    {
        static rommuser::restame::TameEngine e;
        e.prepare (sr, 2);
        benches.push_back ({ "Resonance-R (default)", [] (float* const* io, int n) { e.process (io, nullptr, 2, n); } });
    }
    {
        static rommuser::lowmono::MonoEngine e;
        e.prepare (sr);
        benches.push_back ({ "Stereo-R (default)", [] (float* const* io, int n) { e.process (io, 2, n); } });
    }
    {
        static rommuser::punchshape::PunchEngine e;
        e.prepare (sr, 2);
        benches.push_back ({ "Transient-R (default)", [] (float* const* io, int n) { e.process (io, 2, n); } });
    }
    {
        static rommuser::essclean::EssEngine e;
        e.prepare (sr, 2);
        benches.push_back ({ "De-Esser-R (default)", [] (float* const* io, int n) { e.process (io, 2, n); } });
    }
    {
        static rommuser::peakready::PeakEngine e;
        e.prepare (sr, block, 2);
        auto p = rommuser::peakready::PeakParams {}; p.inputDb = 6.0f;
        e.setParams (p, true);
        benches.push_back ({ "Limiter-R (+6 dB)", [] (float* const* io, int n) { e.process (io, 2, n); } });
    }
    {
        static rommuser::mastercheck::MeterEngine e;
        e.prepare (sr, block, 2);
        benches.push_back ({ "Meter-R", [] (float* const* io, int n) { e.process (io, 2, n); } });
    }
    {
        static rommuser::filterready::FilterEngine e;
        e.prepare (sr, block, 2);
        benches.push_back ({ "Filter-R (default)", [] (float* const* io, int n) { e.process (io, 2, n); } });
    }
    {
        static rommuser::tuneready::TuneEngine e;
        e.prepare (sr, block, 2);
        benches.push_back ({ "Tune-R (default)", [] (float* const* io, int n) { e.process (io, 2, n); } });
    }
    {
        static rommuser::voxsynth::VoxEngine e;
        e.prepare (sr, block, 2);
        benches.push_back ({ "Vocoder-R (default)", [] (float* const* io, int n) { e.process (io, 2, n); } });
    }
    {
        static rommuser::shimmerready::ShimmerEngine e;
        e.prepare (sr, block, 2);
        benches.push_back ({ "Pitch Delay-R (default)", [] (float* const* io, int n) { e.process (io, 2, n); } });
    }
    {
        static rommuser::stutterready::StutterEngine e;
        e.prepare (sr, block, 2);
        benches.push_back ({ "Stutter-R (idle)", [] (float* const* io, int n) { e.process (io, 2, n, {}); } });
    }

    std::printf ("%-28s %10s\n", "engine", "% of core");
    for (const auto& b : benches)
        if (filter.empty() || b.name.find (filter) != std::string::npos)
            std::printf ("%-28s %9.3f%%\n", b.name.c_str(), measure (b));
    return 0;
}
