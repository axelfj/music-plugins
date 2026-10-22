// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PeakEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

namespace ParamIDs
{
    inline constexpr auto input = "input";
    inline constexpr auto ceiling = "ceiling";
    inline constexpr auto release = "release";
    inline constexpr auto autoRelease = "auto";
    inline constexpr auto style = "style";
    inline constexpr auto truePeak = "truepeak";
    inline constexpr auto target = "target";
}

namespace Choices
{
    inline const juce::StringArray style { "Transparent", "Punchy", "Aggressive" };
}

struct PeakPreset
{
    const char* nameEs;
    const char* nameEn;
    float ceilingDb, targetLufs, releaseMs;
    int style;
};

inline constexpr PeakPreset peakPresets[] = {
    { "STREAMING −14", "STREAMING −14", -1.0f, -14.0f, 300.0f, 0 },
    { "CLUB −8", "CLUB −8", -0.5f, -8.0f, 150.0f, 1 },
    { "EDM −6", "EDM −6", -0.3f, -6.0f, 80.0f, 2 },
};

class PeakReadyProcessor : public juce::AudioProcessor
{
public:
    PeakReadyProcessor();

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

    rommuser::peakready::PeakParams readParams() const;
    void applyPreset (const PeakPreset& preset);
    void requestLoudnessReset() { loudnessResetRequested.store (true); }

    juce::AudioProcessorValueTreeState state;

    // For the editor: peaks and reduction are max-held until read; loudness is the latest.
    std::atomic<float> inputPeak { 0.0f }, outputPeak { 0.0f }, gainReductionDb { 0.0f };
    std::atomic<float> momentaryLufs { -120.0f }, shortTermLufs { -120.0f }, integratedLufs { -120.0f };

private:
    rommuser::peakready::PeakEngine engine;
    int reportedLatency = -1;
    std::atomic<bool> loudnessResetRequested { false };

    std::atomic<float>* pInput = nullptr;
    std::atomic<float>* pCeiling = nullptr;
    std::atomic<float>* pRelease = nullptr;
    std::atomic<float>* pAuto = nullptr;
    std::atomic<float>* pStyle = nullptr;
    std::atomic<float>* pTruePeak = nullptr;
    std::atomic<float>* pTarget = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PeakReadyProcessor)
};
