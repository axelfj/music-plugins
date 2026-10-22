// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ShimmerEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

namespace ParamIDs
{
    inline constexpr auto sync = "sync";
    inline constexpr auto division = "division";
    inline constexpr auto time = "time";
    inline constexpr auto pitch = "pitch";
    inline constexpr auto grain = "grain";
    inline constexpr auto reverse = "reverse";
    inline constexpr auto feedback = "feedback";
    inline constexpr auto lowCut = "lowcut";
    inline constexpr auto highCut = "highcut";
    inline constexpr auto freeze = "freeze";
    inline constexpr auto shimmer = "shimmer";
    inline constexpr auto decay = "decay";
    inline constexpr auto width = "width";
    inline constexpr auto mix = "mix";
    inline constexpr auto output = "output";
}

namespace Choices
{
    inline const juce::StringArray divisions { "1/32", "1/16T", "1/16", "1/8T", "1/16D", "1/8", "1/4T", "1/8D", "1/4", "1/4D", "1/2", "1 bar" };
}

struct ShimmerPreset
{
    const char* nameEs;
    const char* nameEn;
    int division; float pitch, feedback, shimmer, decay, highCut; bool reverse; float mix;
};

inline constexpr ShimmerPreset shimmerPresets[] = {
    { "PAD OCTAVA", "OCTAVE PAD",        8, 12.0f, 70.0f, 60.0f, 6.0f, 8000.0f, false, 45.0f },
    { "QUINTAS", "FIFTHS",               5, 7.0f, 60.0f, 40.0f, 4.0f, 10000.0f, false, 40.0f },
    { "VOZ ETÉREA", "ETHEREAL VOICE",    8, 12.0f, 50.0f, 85.0f, 8.0f, 6000.0f, false, 35.0f },
    { "REVERSA", "REVERSE",              8, 0.0f, 55.0f, 30.0f, 3.0f, 12000.0f, true, 50.0f },
    { "TRANSICIÓN", "TRANSITION",        2, 12.0f, 85.0f, 40.0f, 5.0f, 20000.0f, false, 60.0f },
    { "DELAY LIMPIO", "CLEAN DELAY",     5, 0.0f, 40.0f, 0.0f, 2.0f, 20000.0f, false, 35.0f },
};

class ShimmerReadyProcessor : public juce::AudioProcessor
{
public:
    ShimmerReadyProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void reset() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    using AudioProcessor::processBlock;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 30.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    rommuser::shimmerready::ShimmerParams readParams() const;
    void applyPreset (const ShimmerPreset& preset);

    juce::AudioProcessorValueTreeState state;

    std::atomic<float> hostBpm { 120.0f }, delayMs { 0.0f }, wetPeak { 0.0f }, feedbackPeak { 0.0f }, inputHz { 0.0f };

private:
    rommuser::shimmerready::ShimmerEngine engine;

    std::atomic<float>* p (const char* id) const { return state.getRawParameterValue (id); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShimmerReadyProcessor)
};
