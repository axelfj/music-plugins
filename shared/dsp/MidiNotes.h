// Note names, scales and a few helpers shared by the MIDI tools (Chord Ready, Arp Ready).
// Pure C++ so the engines stay testable without JUCE.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>

namespace rommuser::midi
{

inline constexpr const char* noteNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

// Name with octave, C4 = 60.
inline std::string noteName (int midi)
{
    const int pc = ((midi % 12) + 12) % 12;
    return std::string (noteNames[pc]) + std::to_string (midi / 12 - 1);
}

// A scale is a set of semitone offsets from the root, ascending, within one octave.
struct Scale
{
    const char* name;
    int size;
    std::array<int, 12> steps;
};

inline constexpr int numScales = 10;
inline constexpr Scale scales[numScales] = {
    { "Major",          7, { 0, 2, 4, 5, 7, 9, 11 } },
    { "Minor",          7, { 0, 2, 3, 5, 7, 8, 10 } },
    { "Harmonic minor", 7, { 0, 2, 3, 5, 7, 8, 11 } },
    { "Dorian",         7, { 0, 2, 3, 5, 7, 9, 10 } },
    { "Mixolydian",     7, { 0, 2, 4, 5, 7, 9, 10 } },
    { "Lydian",         7, { 0, 2, 4, 6, 7, 9, 11 } },
    { "Phrygian",       7, { 0, 1, 3, 5, 7, 8, 10 } },
    { "Major pent.",    5, { 0, 2, 4, 7, 9 } },
    { "Minor pent.",    5, { 0, 3, 5, 7, 10 } },
    { "Chromatic",     12, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 } },
};

inline int pitchClass (int midi) { return ((midi % 12) + 12) % 12; }

// Degree (0-based) of `midi` in the scale, or -1 when the note is not in the scale.
inline int scaleDegree (const Scale& s, int root, int midi)
{
    const int rel = pitchClass (midi - root);
    for (int i = 0; i < s.size; ++i)
        if (s.steps[(size_t) i] == rel) return i;
    return -1;
}

// Nearest scale note (ties go down), so a wrong key still lands on the scale.
inline int snapToScale (const Scale& s, int root, int midi)
{
    if (scaleDegree (s, root, midi) >= 0) return midi;
    for (int d = 1; d < 12; ++d)
    {
        if (scaleDegree (s, root, midi - d) >= 0) return midi - d;
        if (scaleDegree (s, root, midi + d) >= 0) return midi + d;
    }
    return midi;
}

// Note that sits `offset` scale degrees above the scale note `midi` (which must be in the scale).
inline int stepInScale (const Scale& s, int root, int midi, int offset)
{
    const int degree = scaleDegree (s, root, midi);
    if (degree < 0) return midi + offset;   // not in scale: fall back to semitones
    const int total = degree + offset;
    const int octaves = total >= 0 ? total / s.size : -((-total + s.size - 1) / s.size);
    const int wrapped = total - octaves * s.size;
    return midi - s.steps[(size_t) degree] + s.steps[(size_t) wrapped] + 12 * octaves;
}

// Tiny deterministic random source for humanize / random modes (no <random> on the audio thread).
struct Rng
{
    uint32_t state = 0x9E3779B9u;
    void seed (uint32_t s) { state = s | 1u; }
    uint32_t next() { state = state * 1664525u + 1013904223u; return state; }
    float unit() { return (float) (next() >> 8) / 16777216.0f; }           // 0..1
    int below (int n) { return n <= 1 ? 0 : (int) (next() % (uint32_t) n); }
};

} // namespace rommuser::midi
