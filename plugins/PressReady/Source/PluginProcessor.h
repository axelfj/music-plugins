// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "CompEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

namespace ParamIDs
{
    inline constexpr auto threshold = "threshold";
    inline constexpr auto ratio = "ratio";
    inline constexpr auto attack = "attack";
    inline constexpr auto release = "release";
    inline constexpr auto autoRelease = "autorelease";
    inline constexpr auto knee = "knee";
    inline constexpr auto makeup = "makeup";
    inline constexpr auto autoMakeup = "automakeup";
    inline constexpr auto mix = "mix";
    inline constexpr auto detector = "detector";
    inline constexpr auto scHpf = "schpf";
    inline constexpr auto external = "external";
    inline constexpr auto character = "character";
    inline constexpr auto lookahead = "lookahead";
}

namespace Choices
{
    inline const juce::StringArray detectors { "Peak", "RMS" };
    inline const juce::StringArray characters { "Clean", "Punch", "Glue" };
    inline const juce::StringArray scHpf { "Off", "60 Hz", "100 Hz", "150 Hz", "250 Hz", "400 Hz" };
    inline constexpr float scHpfValues[] = { 0.0f, 60.0f, 100.0f, 150.0f, 250.0f, 400.0f };
    inline const juce::StringArray lookahead { "Off", "1 ms", "2 ms", "5 ms", "10 ms" };
    inline constexpr float lookaheadValues[] = { 0.0f, 1.0f, 2.0f, 5.0f, 10.0f };
}

class PressReadyProcessor : public juce::AudioProcessor
{
public:
    PressReadyProcessor();

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

    // Safe from any thread (atomic reads only); the editor uses it for the transfer curve.
    rommuser::pressready::CompParams readParams() const;

    juce::AudioProcessorValueTreeState state;

    // Meter values for the editor (reset on read).
    std::atomic<float> inputPeak { 0.0f }, outputPeak { 0.0f }, gainReductionDb { 0.0f };
    // Current detector level estimate for the live dot on the curve (dB, decayed in the editor).
    std::atomic<float> levelDb { -100.0f };

private:
    rommuser::pressready::CompEngine engine;
    int reportedLatency = -1;

    std::atomic<float>* pThreshold = nullptr;
    std::atomic<float>* pRatio = nullptr;
    std::atomic<float>* pAttack = nullptr;
    std::atomic<float>* pRelease = nullptr;
    std::atomic<float>* pAutoRelease = nullptr;
    std::atomic<float>* pKnee = nullptr;
    std::atomic<float>* pMakeup = nullptr;
    std::atomic<float>* pAutoMakeup = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pDetector = nullptr;
    std::atomic<float>* pScHpf = nullptr;
    std::atomic<float>* pExternal = nullptr;
    std::atomic<float>* pCharacter = nullptr;
    std::atomic<float>* pLookahead = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PressReadyProcessor)
};
