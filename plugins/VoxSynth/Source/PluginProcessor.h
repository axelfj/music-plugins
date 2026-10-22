// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "VoxEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>

namespace ParamIDs
{
    inline constexpr auto mode = "mode";
    inline constexpr auto range = "range";
    inline constexpr auto key = "key";
    inline constexpr auto scale = "scale";
    inline constexpr auto intervalMode = "intervalmode";
    inline constexpr auto glide = "glide";
    inline constexpr const char* voiceOn[4] = { "v1on", "v2on", "v3on", "v4on" };
    inline constexpr const char* voiceDegrees[4] = { "v1deg", "v2deg", "v3deg", "v4deg" };
    inline constexpr const char* voiceSemis[4] = { "v1semi", "v2semi", "v3semi", "v4semi" };
    inline constexpr const char* voiceDetune[4] = { "v1det", "v2det", "v3det", "v4det" };
    inline constexpr const char* voicePan[4] = { "v1pan", "v2pan", "v3pan", "v4pan" };
    inline constexpr const char* voiceLevel[4] = { "v1lvl", "v2lvl", "v3lvl", "v4lvl" };
    inline constexpr const char* voiceFormant[4] = { "v1frm", "v2frm", "v3frm", "v4frm" };
    inline constexpr auto carrier = "carrier";
    inline constexpr auto carrierFollow = "carrierfollow";
    inline constexpr auto carrierNote = "carriernote";
    inline constexpr auto bands = "bands";
    inline constexpr auto attack = "attack";
    inline constexpr auto release = "release";
    inline constexpr auto sibilance = "sibilance";
    inline constexpr auto carrierDetune = "carrierdetune";
    inline constexpr auto robotNote = "robotnote";
    inline constexpr auto robotFormant = "robotformant";
    inline constexpr auto dry = "dry";
    inline constexpr auto mix = "mix";
    inline constexpr auto output = "output";
}

namespace Choices
{
    inline const juce::StringArray modes { "Harmony", "Vocoder", "Robot" };
    inline const juce::StringArray ranges { "Low voice (50 Hz)", "Normal (60 Hz)", "High voice (80 Hz)", "Low latency (110 Hz)" };
    inline constexpr float rangeMinHz[] = { 50.0f, 60.0f, 80.0f, 110.0f };
    inline const juce::StringArray keys { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    inline const juce::StringArray scales { "Chromatic", "Major", "Minor" };
    inline const juce::StringArray intervalModes { "Scale degrees", "Semitones", "MIDI notes" };
    inline const juce::StringArray degrees { "-7", "-6", "-5", "-4", "-3", "-2", "-1", "0", "+1", "+2", "+3", "+4", "+5", "+6", "+7" };
    inline const juce::StringArray carriers { "Saw", "Pulse", "Noise", "External (sidechain)" };
    inline constexpr int firstNote = 24, numNotes = 49;   // C0 .. C4
    inline juce::StringArray noteNames()
    {
        static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        juce::StringArray out;
        for (int n = firstNote; n < firstNote + numNotes; ++n)
            out.add (juce::String (names[n % 12]) + juce::String (n / 12 - 1));
        return out;
    }
}

struct VoxPreset
{
    const char* nameEs;
    const char* nameEn;
    int mode, intervalMode;
    bool on[4]; int degrees[4]; float semis[4]; float pan[4]; float level[4];
    int carrier; bool follow; int robotNote; float robotFormant; float dry;
};

inline constexpr VoxPreset voxPresets[] = {
    { "3ª Y 5ª", "3RD & 5TH",         0, 0, { true, true, false, false }, { 2, 4, 0, 0 }, { 0, 0, 0, 0 }, { -0.5f, 0.5f, 0.0f, 0.0f }, { -6.0f, -8.0f, -6.0f, -6.0f }, 0, true, 48, 0.0f, 0.0f },
    { "CORO OCTAVAS", "OCTAVE CHOIR",  0, 1, { true, true, true, false }, { 0, 0, 0, 0 }, { -12.0f, 12.0f, 0.0f, 0.0f }, { -0.7f, 0.7f, 0.0f, 0.0f }, { -8.0f, -10.0f, -6.0f, -6.0f }, 0, true, 48, 0.0f, 0.0f },
    { "TALKBOX", "TALKBOX",           1, 0, { false, false, false, false }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { -6.0f, -6.0f, -6.0f, -6.0f }, 0, true, 48, 0.0f, -100.0f },
    { "VOCODER MIDI", "MIDI VOCODER", 1, 0, { false, false, false, false }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { -6.0f, -6.0f, -6.0f, -6.0f }, 0, false, 48, 0.0f, -100.0f },
    { "ROBOT", "ROBOT",               2, 0, { false, false, false, false }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { -6.0f, -6.0f, -6.0f, -6.0f }, 0, true, 48, 0.0f, -100.0f },
    { "MONSTRUO", "MONSTER",          2, 0, { false, false, false, false }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { -6.0f, -6.0f, -6.0f, -6.0f }, 0, true, 36, -7.0f, -100.0f },
};

class VoxSynthProcessor : public juce::AudioProcessor
{
public:
    static constexpr int historyLength = 600;

    VoxSynthProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void reset() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    using AudioProcessor::processBlock;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    rommuser::voxsynth::VoxParams readParams() const;
    void applyPreset (const VoxPreset& preset);

    juce::AudioProcessorValueTreeState state;

    // Display data.
    std::array<std::atomic<float>, historyLength> detectedHistory;
    std::array<std::array<std::atomic<float>, historyLength>, rommuser::voxsynth::maxVoices> voiceHistory;
    std::atomic<int> writeIndex { 0 };
    std::array<std::atomic<float>, rommuser::voxsynth::maxBands> bandLevels;
    std::atomic<float> inputHz { 0.0f }, carrierHz { 0.0f }, outputPeak { 0.0f };
    std::atomic<bool> voiced { false }, midiHeld { false };

private:
    rommuser::voxsynth::VoxEngine engine;
    int reportedLatency = -1;
    int samplesSinceEntry = 0, entryInterval = 800;

    std::atomic<float>* p (const char* id) const { return state.getRawParameterValue (id); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxSynthProcessor)
};
