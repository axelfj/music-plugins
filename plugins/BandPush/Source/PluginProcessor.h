// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PushEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

namespace ParamIDs
{
    inline constexpr auto depth = "depth";
    inline constexpr auto time = "time";
    inline constexpr auto threshold = "threshold";
    inline constexpr auto input = "input";
    inline constexpr auto output = "output";
    inline constexpr auto mix = "mix";
    inline constexpr auto lowXover = "lowxover";
    inline constexpr auto highXover = "highxover";

    // "b1up", "b2down", "b3gain" ...
    inline juce::String band (int index, const char* suffix) { return "b" + juce::String (index + 1) + suffix; }
}

class BandPushProcessor : public juce::AudioProcessor
{
public:
    BandPushProcessor();

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

    // Safe from any thread (atomic reads only).
    rommuser::bandpush::PushParams readParams() const;

    juce::AudioProcessorValueTreeState state;

    // For the editor: signed gain per band (dB) and output peak (reset on read).
    std::atomic<float> bandGainDb[rommuser::bandpush::numBands] { 0.0f, 0.0f, 0.0f };
    std::atomic<float> outputPeak { 0.0f };

private:
    rommuser::bandpush::PushEngine engine;

    std::atomic<float>* pDepth = nullptr;
    std::atomic<float>* pTime = nullptr;
    std::atomic<float>* pThreshold = nullptr;
    std::atomic<float>* pInput = nullptr;
    std::atomic<float>* pOutput = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pLowXover = nullptr;
    std::atomic<float>* pHighXover = nullptr;
    std::atomic<float>* pUp[rommuser::bandpush::numBands] = {};
    std::atomic<float>* pDown[rommuser::bandpush::numBands] = {};
    std::atomic<float>* pGain[rommuser::bandpush::numBands] = {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BandPushProcessor)
};
