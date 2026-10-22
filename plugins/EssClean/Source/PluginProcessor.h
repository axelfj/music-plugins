// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "EssEngine.h"
#include "../../../shared/dsp/FFT.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>

namespace ParamIDs
{
    inline constexpr auto low = "low";
    inline constexpr auto high = "high";
    inline constexpr auto threshold = "threshold";
    inline constexpr auto range = "range";
    inline constexpr auto attack = "attack";
    inline constexpr auto release = "release";
    inline constexpr auto mode = "mode";
    inline constexpr auto lookahead = "lookahead";
    inline constexpr auto listen = "listen";
}

namespace Choices
{
    inline const juce::StringArray mode { "Band", "Broadband" };
    inline const juce::StringArray lookahead { "Off", "1 ms", "2 ms", "5 ms" };
    inline constexpr float lookaheadValues[] = { 0.0f, 1.0f, 2.0f, 5.0f };
}

struct EssPreset
{
    const char* nameEs;
    const char* nameEn;
    float lowHz, highHz, thresholdDb, rangeDb;
};

inline constexpr EssPreset essPresets[] = {
    { "VOZ GRAVE", "LOW VOICE", 3500.0f, 7500.0f, -24.0f, 10.0f },
    { "VOZ MEDIA", "MID VOICE", 4500.0f, 9000.0f, -24.0f, 10.0f },
    { "VOZ AGUDA", "HIGH VOICE", 6000.0f, 12000.0f, -24.0f, 10.0f },
};

class EssCleanProcessor : public juce::AudioProcessor
{
public:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int numBins = fftSize / 2;

    EssCleanProcessor();

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

    rommuser::essclean::EssParams readParams() const;
    void applyPreset (const EssPreset& preset);
    double getSampleRateForDisplay() const { return currentSampleRate; }

    juce::AudioProcessorValueTreeState state;

    // For the editor.
    std::atomic<float> reductionDb { 0.0f }, levelDb { -120.0f }, inputPeak { 0.0f }, outputPeak { 0.0f };
    std::array<std::atomic<float>, numBins> spectrumDb {};   // input spectrum, dBFS per bin

private:
    void pushSpectrumSample (float x);

    rommuser::essclean::EssEngine engine;
    int reportedLatency = -1;
    double currentSampleRate = 48000.0;

    rommuser::dsp::FFT fft;
    std::array<float, fftSize> fifo {};
    std::array<float, fftSize> window {};
    std::array<rommuser::dsp::FFT::Complex, fftSize> fftData {};
    int fifoPos = 0;

    std::atomic<float>* pLow = nullptr;
    std::atomic<float>* pHigh = nullptr;
    std::atomic<float>* pThreshold = nullptr;
    std::atomic<float>* pRange = nullptr;
    std::atomic<float>* pAttack = nullptr;
    std::atomic<float>* pRelease = nullptr;
    std::atomic<float>* pMode = nullptr;
    std::atomic<float>* pLookahead = nullptr;
    std::atomic<float>* pListen = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EssCleanProcessor)
};
