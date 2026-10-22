// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "EchoEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

namespace ParamIDs
{
    inline constexpr auto sync = "sync";
    inline constexpr auto division = "division";
    inline constexpr auto time = "time";
    inline constexpr auto mode = "mode";
    inline constexpr auto style = "style";
    inline constexpr auto timeChange = "timechange";
    inline constexpr auto feedback = "feedback";
    inline constexpr auto lowCut = "lowcut";
    inline constexpr auto highCut = "highcut";
    inline constexpr auto diffusion = "diffusion";
    inline constexpr auto wow = "wow";
    inline constexpr auto duck = "duck";
    inline constexpr auto mix = "mix";
    inline constexpr auto output = "output";
}

namespace Choices
{
    inline const juce::StringArray divisions { "1/1", "1/2", "1/2 T", "1/4 D", "1/4", "1/4 T", "1/8 D", "1/8", "1/8 T", "1/16 D", "1/16", "1/16 T", "1/32" };
    inline constexpr double divisionBeats[] = { 4.0, 2.0, 4.0 / 3.0, 1.5, 1.0, 2.0 / 3.0, 0.75, 0.5, 1.0 / 3.0, 0.375, 0.25, 1.0 / 6.0, 0.125 };
    inline const juce::StringArray modes { "Stereo", "Ping-Pong" };
    inline const juce::StringArray styles { "Digital", "Tape", "Analog" };
    inline const juce::StringArray timeChanges { "Crossfade", "Glide" };
}

struct EchoPreset
{
    const char* nameEs;
    const char* nameEn;
    bool sync;
    int division;
    float timeMs;
    int mode, style;
    float feedback, lowCutHz, highCutHz, diffusion, wow, duck, mix;
};

inline constexpr EchoPreset echoPresets[] = {
    { "SLAPBACK", "SLAPBACK", false, 7, 95.0f, 0, 1, 0.08f, 120.0f, 6000.0f, 0.0f, 0.1f, 0.0f, 0.3f },
    { "1/8 PUNTEADO VOZ", "DOTTED 1/8 VOCAL", true, 6, 375.0f, 1, 0, 0.35f, 250.0f, 5000.0f, 0.3f, 0.0f, 0.6f, 0.3f },
    { "DUB", "DUB", true, 4, 500.0f, 0, 1, 0.65f, 150.0f, 2500.0f, 0.5f, 0.35f, 0.0f, 0.45f },
};

class EchoReadyProcessor : public juce::AudioProcessor
{
public:
    EchoReadyProcessor();

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
    double getTailLengthSeconds() const override { return 10.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    rommuser::echoready::EchoParams readParams() const;
    void applyPreset (const EchoPreset& preset);

    juce::AudioProcessorValueTreeState state;

    std::atomic<float> wetPeak { 0.0f }, duckGain { 1.0f }, currentDelayMs { 0.0f };
    std::atomic<float> hostBpm { 120.0f };

private:
    rommuser::echoready::EchoEngine engine;

    std::atomic<float>* pSync = nullptr;
    std::atomic<float>* pDivision = nullptr;
    std::atomic<float>* pTime = nullptr;
    std::atomic<float>* pMode = nullptr;
    std::atomic<float>* pStyle = nullptr;
    std::atomic<float>* pTimeChange = nullptr;
    std::atomic<float>* pFeedback = nullptr;
    std::atomic<float>* pLowCut = nullptr;
    std::atomic<float>* pHighCut = nullptr;
    std::atomic<float>* pDiffusion = nullptr;
    std::atomic<float>* pWow = nullptr;
    std::atomic<float>* pDuck = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pOutput = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EchoReadyProcessor)
};
