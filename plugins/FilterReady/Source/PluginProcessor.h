// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "FilterEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

namespace ParamIDs
{
    inline constexpr auto aOn = "aon";
    inline constexpr auto aType = "atype";
    inline constexpr auto aSlope = "aslope";
    inline constexpr auto aCutoff = "acutoff";
    inline constexpr auto aRes = "ares";
    inline constexpr auto aLfo = "alfo";
    inline constexpr auto aEnv = "aenv";
    inline constexpr auto bOn = "bon";
    inline constexpr auto bType = "btype";
    inline constexpr auto bSlope = "bslope";
    inline constexpr auto bCutoff = "bcutoff";
    inline constexpr auto bRes = "bres";
    inline constexpr auto bLfo = "blfo";
    inline constexpr auto bEnv = "benv";
    inline constexpr auto routing = "routing";
    inline constexpr auto character = "character";
    inline constexpr auto drive = "drive";
    inline constexpr auto lfoSync = "lfosync";
    inline constexpr auto lfoRate = "lforate";
    inline constexpr auto lfoDivision = "lfodivision";
    inline constexpr auto lfoShape = "lfoshape";
    inline constexpr auto envAttack = "envattack";
    inline constexpr auto envRelease = "envrelease";
    inline constexpr auto envSens = "envsens";
    inline constexpr auto mix = "mix";
    inline constexpr auto output = "output";
}

namespace Choices
{
    inline const juce::StringArray types { "Low-pass", "High-pass", "Band-pass", "Notch" };
    inline const juce::StringArray slopes { "12 dB", "24 dB" };
    inline const juce::StringArray routing { "Series", "Parallel" };
    inline const juce::StringArray character { "Clean", "Analog" };
    inline const juce::StringArray divisions { "4 bars", "2 bars", "1 bar", "1/2", "1/2 T", "1/4 D", "1/4", "1/4 T", "1/8 D", "1/8", "1/8 T", "1/16" };
    inline constexpr double divisionBeats[] = { 16.0, 8.0, 4.0, 2.0, 4.0 / 3.0, 1.5, 1.0, 2.0 / 3.0, 0.75, 0.5, 1.0 / 3.0, 0.25 };
    inline const juce::StringArray shapes { "Sine", "Triangle", "Saw", "Square", "Random" };
}

struct FilterPreset
{
    const char* nameEs;
    const char* nameEn;
    int aType, aSlope; float aCutoff, aRes, aLfo, aEnv;
    bool bOn; int bType, bSlope; float bCutoff, bRes;
    int routing, character; float drive;
    bool lfoSync; int lfoDivision; float lfoRate; int lfoShape;
};

inline constexpr FilterPreset filterPresets[] = {
    { "SUBIDA (BUILD)", "SWEEP BUILD", 0, 1, 300.0f, 0.55f, 4.0f, 0.0f, false, 1, 0, 150.0f, 0.2f, 0, 0, 0.0f, true, 0, 0.1f, 2 },
    { "WOBBLE", "WOBBLE",             0, 1, 400.0f, 0.65f, 3.0f, 0.0f, false, 1, 0, 150.0f, 0.2f, 0, 1, 6.0f, true, 9, 2.0f, 0 },
    { "ÁCIDO", "ACID",                0, 1, 500.0f, 0.8f, 0.0f, 3.0f, false, 1, 0, 150.0f, 0.2f, 0, 1, 12.0f, false, 6, 0.5f, 0 },
    { "TELÉFONO", "TELEPHONE",        1, 1, 600.0f, 0.3f, 0.0f, 0.0f, true, 0, 1, 3000.0f, 0.3f, 0, 0, 3.0f, false, 6, 0.5f, 0 },
};

class FilterReadyProcessor : public juce::AudioProcessor
{
public:
    FilterReadyProcessor();

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
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    rommuser::filterready::FilterParams readParams() const;
    void applyPreset (const FilterPreset& preset);
    double getSampleRateForDisplay() const { return currentSampleRate; }

    juce::AudioProcessorValueTreeState state;

    std::atomic<float> cutoffA { 1000.0f }, cutoffB { 150.0f }, lfo { 0.0f }, env { 0.0f }, outputPeak { 0.0f }, hostBpm { 120.0f };

private:
    rommuser::filterready::FilterEngine engine;
    int reportedLatency = -1;
    double currentSampleRate = 48000.0;

    std::atomic<float>* p (const char* id) const { return state.getRawParameterValue (id); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterReadyProcessor)
};
