// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ClipEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

namespace ParamIDs
{
    inline constexpr auto input = "input";
    inline constexpr auto ceiling = "ceiling";
    inline constexpr auto softness = "softness";
    inline constexpr auto output = "output";
    inline constexpr auto mix = "mix";
    inline constexpr auto oversampling = "oversampling";
    inline constexpr auto delta = "delta";
    inline constexpr auto link = "link";
    inline constexpr auto strict = "strict";
}

class ClipReadyProcessor : public juce::AudioProcessor
{
public:
    ClipReadyProcessor();

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

    juce::AudioProcessorValueTreeState state;

    // Meter values for the editor (linear peaks, decayed in the editor).
    std::atomic<float> inputPeak { 0.0f }, outputPeak { 0.0f }, clipDb { 0.0f };

private:
    rommuser::clipready::ClipParams readParams() const;

    rommuser::clipready::ClipEngine engine;
    int reportedLatency = -1;

    std::atomic<float>* pInput = nullptr;
    std::atomic<float>* pCeiling = nullptr;
    std::atomic<float>* pSoftness = nullptr;
    std::atomic<float>* pOutput = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pOversampling = nullptr;
    std::atomic<float>* pDelta = nullptr;
    std::atomic<float>* pLink = nullptr;
    std::atomic<float>* pStrict = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipReadyProcessor)
};
