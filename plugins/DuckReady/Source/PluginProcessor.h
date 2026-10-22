// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "DuckEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

namespace ParamIDs
{
    inline constexpr auto depth = "depth";
    inline constexpr auto attack = "attack";
    inline constexpr auto hold = "hold";
    inline constexpr auto release = "release";
    inline constexpr auto curve = "curve";
    inline constexpr auto offset = "offset";
    inline constexpr auto rate = "rate";
    inline constexpr auto trigger = "trigger";
    inline constexpr auto scThreshold = "scthreshold";
    inline constexpr auto band = "band";
    inline constexpr auto crossover = "crossover";
}

namespace Choices
{
    inline const juce::StringArray rates { "1 bar", "1/2", "1/4", "1/4 T", "1/8 D", "1/8", "1/8 T", "1/16", "1/16 T", "1/32" };
    inline constexpr double rateBeats[] = { 4.0, 2.0, 1.0, 2.0 / 3.0, 0.75, 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0, 0.125 };
    inline const juce::StringArray triggers { "Tempo", "MIDI", "Sidechain" };
    inline const juce::StringArray bands { "Full", "Low", "High" };
}

// Preset curves for the three buttons in the editor (depth, attack, hold, release, curve).
struct CurvePreset
{
    const char* nameEs;
    const char* nameEn;
    float depth, attack, hold, release, curve;
};

inline constexpr CurvePreset curvePresets[] = {
    { "CLÁSICO", "CLASSIC", 0.85f, 0.02f, 0.05f, 0.55f, 0.3f },
    { "SUAVE", "SOFT", 0.5f, 0.10f, 0.00f, 0.80f, 0.0f },
    { "CORTO", "SHORT", 1.0f, 0.01f, 0.10f, 0.30f, 0.5f },
};

class DuckReadyProcessor : public juce::AudioProcessor
{
public:
    DuckReadyProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void reset() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    using AudioProcessor::processBlock;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
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

    // Safe from any thread (atomic reads only); the editor uses it to draw the curve.
    rommuser::duckready::DuckParams readParams() const;

    // Sets depth / attack / hold / release / curve from a preset (message thread).
    void applyPreset (const CurvePreset& preset);

    juce::AudioProcessorValueTreeState state;

    // For the editor: where in the cycle the audio thread is, and the gain right now.
    std::atomic<float> currentPhase { 0.0f }, currentGain { 1.0f };
    std::atomic<bool> hostPlaying { false };

private:
    rommuser::duckready::DuckEngine engine;

    std::atomic<float>* pDepth = nullptr;
    std::atomic<float>* pAttack = nullptr;
    std::atomic<float>* pHold = nullptr;
    std::atomic<float>* pRelease = nullptr;
    std::atomic<float>* pCurve = nullptr;
    std::atomic<float>* pOffset = nullptr;
    std::atomic<float>* pRate = nullptr;
    std::atomic<float>* pTrigger = nullptr;
    std::atomic<float>* pScThreshold = nullptr;
    std::atomic<float>* pBand = nullptr;
    std::atomic<float>* pCrossover = nullptr;

    static constexpr int maxMidiPerBlock = 64;
    int midiOffsets[maxMidiPerBlock] = {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DuckReadyProcessor)
};
