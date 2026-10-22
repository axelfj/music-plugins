// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "GateEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

namespace ParamIDs
{
    inline constexpr auto threshold = "threshold";
    inline constexpr auto ratio = "ratio";
    inline constexpr auto attack = "attack";
    inline constexpr auto hold = "hold";
    inline constexpr auto release = "release";
    inline constexpr auto range = "range";
    inline constexpr auto knee = "knee";
    inline constexpr auto hysteresis = "hysteresis";
    inline constexpr auto lookahead = "lookahead";
    inline constexpr auto scHpf = "schpf";
    inline constexpr auto scLpf = "sclpf";
    inline constexpr auto external = "external";
    inline constexpr auto listen = "listen";
}

namespace Choices
{
    inline const juce::StringArray lookahead { "Off", "1 ms", "2 ms", "5 ms", "10 ms" };
    inline constexpr float lookaheadValues[] = { 0.0f, 1.0f, 2.0f, 5.0f, 10.0f };
    inline const juce::StringArray scHpf { "Off", "60 Hz", "120 Hz", "250 Hz", "500 Hz", "1 kHz", "2 kHz" };
    inline constexpr float scHpfValues[] = { 0.0f, 60.0f, 120.0f, 250.0f, 500.0f, 1000.0f, 2000.0f };
    inline const juce::StringArray scLpf { "Off", "500 Hz", "1 kHz", "2 kHz", "4 kHz", "8 kHz" };
    inline constexpr float scLpfValues[] = { 0.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f };
}

// Starting points for the three buttons: threshold, attack, hold, release, range, SC HPF index, SC LPF index.
struct GatePreset
{
    const char* nameEs;
    const char* nameEn;
    float thresholdDb, attackMs, holdMs, releaseMs, rangeDb;
    int scHpf, scLpf;
};

inline constexpr GatePreset gatePresets[] = {
    { "LIMPIAR BOMBO", "CLEAN KICK", -30.0f, 0.5f, 60.0f, 80.0f, -80.0f, 0, 3 },
    { "LIMPIAR CAJA", "CLEAN SNARE", -28.0f, 0.3f, 40.0f, 60.0f, -80.0f, 3, 0 },
    { "RUIDO DE VOZ", "VOCAL NOISE", -45.0f, 5.0f, 150.0f, 300.0f, -30.0f, 1, 0 },
};

class GateReadyProcessor : public juce::AudioProcessor
{
public:
    GateReadyProcessor();

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

    rommuser::gateready::GateParams readParams() const;
    void applyPreset (const GatePreset& preset);

    juce::AudioProcessorValueTreeState state;

    // For the editor (max values reset on read, levelDb is the latest).
    std::atomic<float> inputPeak { 0.0f }, outputPeak { 0.0f }, gainReductionDb { 0.0f };
    std::atomic<float> levelDb { -120.0f };
    std::atomic<bool> gateOpen { false };

private:
    rommuser::gateready::GateEngine engine;
    int reportedLatency = -1;

    std::atomic<float>* pThreshold = nullptr;
    std::atomic<float>* pRatio = nullptr;
    std::atomic<float>* pAttack = nullptr;
    std::atomic<float>* pHold = nullptr;
    std::atomic<float>* pRelease = nullptr;
    std::atomic<float>* pRange = nullptr;
    std::atomic<float>* pKnee = nullptr;
    std::atomic<float>* pHysteresis = nullptr;
    std::atomic<float>* pLookahead = nullptr;
    std::atomic<float>* pScHpf = nullptr;
    std::atomic<float>* pScLpf = nullptr;
    std::atomic<float>* pExternal = nullptr;
    std::atomic<float>* pListen = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GateReadyProcessor)
};
