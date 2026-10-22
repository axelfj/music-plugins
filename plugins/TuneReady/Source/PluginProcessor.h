// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "TuneEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>

namespace ParamIDs
{
    inline constexpr auto mode = "mode";
    inline constexpr auto key = "key";
    inline constexpr auto scale = "scale";
    inline constexpr const char* pitchClass[12] = { "pc0", "pc1", "pc2", "pc3", "pc4", "pc5", "pc6", "pc7", "pc8", "pc9", "pc10", "pc11" };
    inline constexpr auto speed = "speed";
    inline constexpr auto amount = "amount";
    inline constexpr auto humanize = "humanize";
    inline constexpr auto range = "range";
    inline constexpr auto fixedPitch = "fixedpitch";
    inline constexpr auto formant = "formant";
    inline constexpr auto midiPriority = "midi";
    inline constexpr auto mix = "mix";
    inline constexpr auto output = "output";
}

namespace Choices
{
    inline const juce::StringArray modes { "Correct", "Fixed" };
    inline const juce::StringArray keys { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    inline const juce::StringArray scales { "Chromatic", "Major", "Minor", "Custom" };
    inline const juce::StringArray ranges { "Low voice (50 Hz)", "Normal (60 Hz)", "High voice (80 Hz)", "Low latency (110 Hz)" };
    inline constexpr float rangeMinHz[] = { 50.0f, 60.0f, 80.0f, 110.0f };
}

struct TunePreset
{
    const char* nameEs;
    const char* nameEn;
    int mode; float speed, amount, humanize; float fixedPitch, formant;
};

inline constexpr TunePreset tunePresets[] = {
    { "NATURAL", "NATURAL",     0, 80.0f, 100.0f, 10.0f, 0.0f, 0.0f },
    { "DURO (T-PAIN)", "HARD",  0, 0.0f, 100.0f, 0.0f, 0.0f, 0.0f },
    { "VOZ GRAVE", "DEEP VOICE", 1, 0.0f, 100.0f, 0.0f, -4.0f, -4.0f },
    { "VOZ AGUDA", "HIGH VOICE", 1, 0.0f, 100.0f, 0.0f, 4.0f, 4.0f },
};

class TuneReadyProcessor : public juce::AudioProcessor
{
public:
    static constexpr int historyLength = 600;   // ~10 s at 60 entries / s

    TuneReadyProcessor();

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

    rommuser::tuneready::TuneParams readParams() const;
    void applyPreset (const TunePreset& preset);

    juce::AudioProcessorValueTreeState state;

    // Pitch history for the display: detected and corrected MIDI (0 = unvoiced), newest at writeIndex - 1.
    std::array<std::atomic<float>, historyLength> detectedHistory, correctedHistory;
    std::atomic<int> writeIndex { 0 };
    std::atomic<float> inputHz { 0.0f }, shiftSemis { 0.0f };
    std::atomic<bool> voiced { false }, midiHeld { false };

private:
    rommuser::tuneready::TuneEngine engine;
    int reportedLatency = -1;
    int samplesSinceEntry = 0, entryInterval = 800;

    std::atomic<float>* p (const char* id) const { return state.getRawParameterValue (id); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TuneReadyProcessor)
};
