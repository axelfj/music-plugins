// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ResTameEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>

namespace ParamIDs
{
    inline constexpr auto depth = "depth";
    inline constexpr auto sharpness = "sharpness";
    inline constexpr auto selectivity = "selectivity";
    inline constexpr auto threshold = "threshold";
    inline constexpr auto attack = "attack";
    inline constexpr auto release = "release";
    inline constexpr auto low = "low";
    inline constexpr auto high = "high";
    inline constexpr auto placement = "placement";
    inline constexpr auto external = "external";
    inline constexpr auto delta = "delta";
}

namespace Choices
{
    inline const juce::StringArray placements { "Stereo", "Mid", "Side" };
}

struct TamePreset
{
    const char* nameEs;
    const char* nameEn;
    float depthDb, sharpness, selectivity, thresholdDb, attackMs, releaseMs, lowHz, highHz;
    int placement;
    bool external;
};

inline constexpr TamePreset tamePresets[] = {
    { "VOZ", "VOCAL", 8.0f, 0.7f, 0.5f, 6.0f, 20.0f, 200.0f, 150.0f, 12000.0f, 0, false },
    { "SYNTH / GUITARRA", "SYNTH / GUITAR", 10.0f, 0.9f, 0.6f, 5.0f, 10.0f, 150.0f, 200.0f, 16000.0f, 0, false },
    { "SIBILANCIA", "HARSHNESS", 9.0f, 0.5f, 0.3f, 5.0f, 5.0f, 120.0f, 2000.0f, 12000.0f, 0, false },
    { "UNMASK (SIDECHAIN)", "UNMASK (SIDECHAIN)", 10.0f, 0.5f, 0.4f, 6.0f, 15.0f, 250.0f, 100.0f, 10000.0f, 0, true },
};

class ResTameProcessor : public juce::AudioProcessor
{
public:
    static constexpr int displayPoints = 256;

    ResTameProcessor();

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
    double getTailLengthSeconds() const override { return 0.1; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    rommuser::restame::TameParams readParams() const;
    void applyPreset (const TamePreset& preset);

    juce::AudioProcessorValueTreeState state;

    // Reduction curve for the editor: displayPoints values on a log axis 20 Hz .. 20 kHz, in dB (>= 0).
    std::array<std::atomic<float>, (size_t) displayPoints> displayReduction {};
    std::atomic<float> maxReductionDb { 0.0f }, meanReductionDb { 0.0f };

private:
    rommuser::restame::TameEngine engine;

    std::atomic<float>* pDepth = nullptr;
    std::atomic<float>* pSharpness = nullptr;
    std::atomic<float>* pSelectivity = nullptr;
    std::atomic<float>* pThreshold = nullptr;
    std::atomic<float>* pAttack = nullptr;
    std::atomic<float>* pRelease = nullptr;
    std::atomic<float>* pLow = nullptr;
    std::atomic<float>* pHigh = nullptr;
    std::atomic<float>* pPlacement = nullptr;
    std::atomic<float>* pExternal = nullptr;
    std::atomic<float>* pDelta = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResTameProcessor)
};
