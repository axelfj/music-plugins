// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "WideEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>

namespace ParamIDs
{
    inline constexpr auto mode = "mode";
    inline constexpr auto voices = "voices";
    inline constexpr auto ensemble = "ensemble";
    inline constexpr auto rate = "rate";
    inline constexpr auto depth = "depth";
    inline constexpr auto delay = "delay";
    inline constexpr auto feedback = "feedback";
    inline constexpr auto throughZero = "throughzero";
    inline constexpr auto invert = "invert";
    inline constexpr auto spread = "spread";
    inline constexpr auto cents = "cents";
    inline constexpr auto mix = "mix";
    inline constexpr auto output = "output";
}

namespace Choices
{
    inline const juce::StringArray modes { "Chorus", "Flanger", "Micro-pitch" };
    inline const juce::StringArray voices { "1", "2", "3" };
}

struct WidePreset
{
    const char* nameEs;
    const char* nameEn;
    int mode, voices;
    bool ensemble;
    float rateHz, depth, delayMs, feedback;
    bool throughZero, invert;
    float spread, cents, mix;
};

inline constexpr WidePreset widePresets[] = {
    { "SYNTH CHORUS", "SYNTH CHORUS", 0, 2, false, 0.4f, 0.5f, 15.0f, 0.0f, false, false, 0.5f, 12.0f, 0.5f },
    { "PAD ENSEMBLE", "PAD ENSEMBLE", 0, 3, true, 0.25f, 0.7f, 20.0f, 0.0f, false, false, 0.8f, 12.0f, 0.5f },
    { "VOZ ANCHA", "WIDE VOCAL", 2, 2, false, 0.4f, 0.5f, 15.0f, 0.0f, false, false, 0.5f, 9.0f, 0.5f },
    { "FLANGER JET", "JET FLANGER", 1, 1, false, 0.15f, 1.0f, 1.0f, 0.75f, true, true, 0.3f, 12.0f, 0.5f },
};

class WideReadyProcessor : public juce::AudioProcessor
{
public:
    WideReadyProcessor();

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
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    rommuser::wideready::WideParams readParams() const;
    void applyPreset (const WidePreset& preset);

    juce::AudioProcessorValueTreeState state;

    std::array<std::array<std::atomic<float>, (size_t) rommuser::wideready::maxVoices>, 2> voiceDelayMs {};
    std::atomic<int> activeVoices { 1 };
    std::atomic<float> outputPeak { 0.0f };

private:
    rommuser::wideready::WideEngine engine;

    std::atomic<float>* pMode = nullptr;
    std::atomic<float>* pVoices = nullptr;
    std::atomic<float>* pEnsemble = nullptr;
    std::atomic<float>* pRate = nullptr;
    std::atomic<float>* pDepth = nullptr;
    std::atomic<float>* pDelay = nullptr;
    std::atomic<float>* pFeedback = nullptr;
    std::atomic<float>* pThroughZero = nullptr;
    std::atomic<float>* pInvert = nullptr;
    std::atomic<float>* pSpread = nullptr;
    std::atomic<float>* pCents = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pOutput = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WideReadyProcessor)
};
