// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PhaseEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>

namespace ParamIDs
{
    inline constexpr auto stages = "stages";
    inline constexpr auto sync = "sync";
    inline constexpr auto division = "division";
    inline constexpr auto rate = "rate";
    inline constexpr auto depth = "depth";
    inline constexpr auto feedback = "feedback";
    inline constexpr auto center = "center";
    inline constexpr auto spread = "spread";
    inline constexpr auto shape = "shape";
    inline constexpr auto mix = "mix";
    inline constexpr auto output = "output";
}

namespace Choices
{
    inline const juce::StringArray stages { "4", "6", "8", "10", "12" };
    inline constexpr int stageCounts[] = { 4, 6, 8, 10, 12 };
    inline const juce::StringArray divisions { "4 bars", "2 bars", "1 bar", "1/2", "1/2 T", "1/4 D", "1/4", "1/4 T", "1/8 D", "1/8", "1/8 T", "1/16" };
    inline constexpr double divisionBeats[] = { 16.0, 8.0, 4.0, 2.0, 4.0 / 3.0, 1.5, 1.0, 2.0 / 3.0, 0.75, 0.5, 1.0 / 3.0, 0.25 };
    inline const juce::StringArray shapes { "Sine", "Triangle", "Random" };
}

struct PhasePreset
{
    const char* nameEs;
    const char* nameEn;
    int stagesIndex;
    bool sync;
    int division;
    float rateHz, depth, feedback, centerHz, spread;
    int shape;
    float mix;
};

inline constexpr PhasePreset phasePresets[] = {
    { "GUITARRA CLÁSICA", "CLASSIC GUITAR", 0, false, 6, 0.4f, 0.7f, 0.3f, 800.0f, 0.0f, 0, 0.5f },
    { "PAD LENTO", "SLOW PAD", 2, false, 2, 0.08f, 1.0f, 0.2f, 600.0f, 0.6f, 0, 0.5f },
    { "SYNTH 1/4", "SYNTH 1/4", 1, true, 6, 2.0f, 0.8f, 0.5f, 1200.0f, 0.3f, 1, 0.5f },
};

class PhaseReadyProcessor : public juce::AudioProcessor
{
public:
    PhaseReadyProcessor();

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

    rommuser::phaseready::PhaseParams readParams() const;
    void applyPreset (const PhasePreset& preset);

    juce::AudioProcessorValueTreeState state;

    // For the editor: current all-pass coefficients per channel, LFO values and output peak.
    std::array<std::array<std::atomic<float>, (size_t) rommuser::phaseready::maxStages>, 2> displayCoeffs {};
    std::atomic<int> displayStages { 6 };
    std::atomic<float> lfoL { 0.0f }, lfoR { 0.0f }, outputPeak { 0.0f }, hostBpm { 120.0f };

private:
    rommuser::phaseready::PhaseEngine engine;

    std::atomic<float>* pStages = nullptr;
    std::atomic<float>* pSync = nullptr;
    std::atomic<float>* pDivision = nullptr;
    std::atomic<float>* pRate = nullptr;
    std::atomic<float>* pDepth = nullptr;
    std::atomic<float>* pFeedback = nullptr;
    std::atomic<float>* pCenter = nullptr;
    std::atomic<float>* pSpread = nullptr;
    std::atomic<float>* pShape = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pOutput = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhaseReadyProcessor)
};
