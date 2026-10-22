// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "SpaceEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

namespace ParamIDs
{
    inline constexpr auto mode = "mode";
    inline constexpr auto size = "size";
    inline constexpr auto decay = "decay";
    inline constexpr auto preDelay = "predelay";
    inline constexpr auto damping = "damping";
    inline constexpr auto diffusion = "diffusion";
    inline constexpr auto modulation = "modulation";
    inline constexpr auto width = "width";
    inline constexpr auto lowCut = "lowcut";
    inline constexpr auto highCut = "highcut";
    inline constexpr auto duck = "duck";
    inline constexpr auto mix = "mix";
    inline constexpr auto output = "output";
    inline constexpr auto freeze = "freeze";
}

namespace Choices
{
    inline const juce::StringArray modes { "Room", "Plate", "Hall", "Ambience" };
}

struct SpacePreset
{
    const char* nameEs;
    const char* nameEn;
    int mode;
    float size, decay, preDelayMs, dampingHz, diffusion, modulation, width, lowCutHz, highCutHz, duck, mix;
};

inline constexpr SpacePreset spacePresets[] = {
    { "SALA", "ROOM", 0, 1.0f, 0.6f, 5.0f, 5000.0f, 0.7f, 0.3f, 1.0f, 100.0f, 20000.0f, 0.0f, 0.25f },
    { "PLATE VOZ", "VOCAL PLATE", 1, 1.0f, 1.8f, 20.0f, 8000.0f, 0.8f, 0.5f, 1.0f, 200.0f, 12000.0f, 0.4f, 0.3f },
    { "HALL", "HALL", 2, 1.2f, 2.8f, 15.0f, 5000.0f, 0.6f, 0.5f, 1.0f, 120.0f, 20000.0f, 0.0f, 0.3f },
    { "ESPACIO", "SUPERMASSIVE", 3, 1.6f, 14.0f, 0.0f, 6000.0f, 1.0f, 1.0f, 1.0f, 150.0f, 9000.0f, 0.0f, 0.5f },
};

class SpaceReadyProcessor : public juce::AudioProcessor
{
public:
    SpaceReadyProcessor();

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

    rommuser::spaceready::SpaceParams readParams() const;
    void applyPreset (const SpacePreset& preset);

    juce::AudioProcessorValueTreeState state;

    std::atomic<float> wetRms { 0.0f }, duckGain { 1.0f }, outputPeak { 0.0f };

private:
    rommuser::spaceready::SpaceEngine engine;

    std::atomic<float>* pMode = nullptr;
    std::atomic<float>* pSize = nullptr;
    std::atomic<float>* pDecay = nullptr;
    std::atomic<float>* pPreDelay = nullptr;
    std::atomic<float>* pDamping = nullptr;
    std::atomic<float>* pDiffusion = nullptr;
    std::atomic<float>* pModulation = nullptr;
    std::atomic<float>* pWidth = nullptr;
    std::atomic<float>* pLowCut = nullptr;
    std::atomic<float>* pHighCut = nullptr;
    std::atomic<float>* pDuck = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pOutput = nullptr;
    std::atomic<float>* pFreeze = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpaceReadyProcessor)
};
