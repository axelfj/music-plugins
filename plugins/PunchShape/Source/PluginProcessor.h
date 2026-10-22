// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PunchEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

namespace ParamIDs
{
    inline constexpr auto attack = "attack";
    inline constexpr auto sustain = "sustain";
    inline constexpr auto speed = "speed";
    inline constexpr auto tail = "tail";
    inline constexpr auto detector = "detector";
    inline constexpr auto clip = "clip";
    inline constexpr auto output = "output";
}

namespace Choices
{
    inline const juce::StringArray detector { "All", "Lows", "Highs" };
}

struct PunchPreset
{
    const char* nameEs;
    const char* nameEn;
    float attackDb, sustainDb, speedMs, tailMs;
    int detector;
};

inline constexpr PunchPreset punchPresets[] = {
    { "MÁS PEGADA", "MORE PUNCH", 8.0f, -4.0f, 20.0f, 300.0f, 0 },
    { "SUAVIZAR", "SOFTEN", -8.0f, 0.0f, 30.0f, 300.0f, 0 },
    { "MÁS COLA", "MORE TAIL", 0.0f, 8.0f, 20.0f, 500.0f, 0 },
    { "BOMBO SECO", "TIGHT KICK", 4.0f, -10.0f, 15.0f, 200.0f, 1 },
};

class PunchShapeProcessor : public juce::AudioProcessor
{
public:
    PunchShapeProcessor();

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

    rommuser::punchshape::PunchParams readParams() const;
    void applyPreset (const PunchPreset& preset);

    juce::AudioProcessorValueTreeState state;

    // For the editor: peaks are max-held until read, gains carry the largest magnitude seen.
    std::atomic<float> inputPeak { 0.0f }, outputPeak { 0.0f };
    std::atomic<float> attackGainDb { 0.0f }, sustainGainDb { 0.0f }, levelDb { -120.0f };
    std::atomic<bool> clipped { false };

private:
    rommuser::punchshape::PunchEngine engine;

    std::atomic<float>* pAttack = nullptr;
    std::atomic<float>* pSustain = nullptr;
    std::atomic<float>* pSpeed = nullptr;
    std::atomic<float>* pTail = nullptr;
    std::atomic<float>* pDetector = nullptr;
    std::atomic<float>* pClip = nullptr;
    std::atomic<float>* pOutput = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PunchShapeProcessor)
};
