// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "MonoEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>

namespace ParamIDs
{
    inline constexpr auto width = "width";
    inline constexpr auto monoBelow = "monobelow";
    inline constexpr auto steep = "steep";
    inline constexpr auto lowWidth = "lowwidth";
}

struct MonoPreset
{
    const char* nameEs;
    const char* nameEn;
    float width, monoBelowHz;
    bool steep;
    float lowWidth;
};

inline constexpr MonoPreset monoPresets[] = {
    { "MASTER SEGURO", "SAFE MASTER", 1.0f, 120.0f, false, 0.0f },
    { "CLUB", "CLUB", 1.1f, 150.0f, true, 0.0f },
    { "ANCHO", "WIDE", 1.5f, 100.0f, false, 0.3f },
    { "SOLO MEDIR", "METER ONLY", 1.0f, 0.0f, false, 0.0f },
};

class LowMonoProcessor : public juce::AudioProcessor
{
public:
    LowMonoProcessor();

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

    rommuser::lowmono::MonoParams readParams() const;
    void applyPreset (const MonoPreset& preset);

    juce::AudioProcessorValueTreeState state;

    // For the editor: goniometer samples (copied per block), correlation and peaks.
    static constexpr int scopePoints = rommuser::lowmono::MonoEngine::scopePoints;
    std::array<std::atomic<float>, (size_t) scopePoints> scopeL {}, scopeR {};
    std::atomic<float> correlation { 1.0f }, midPeak { 0.0f }, sidePeak { 0.0f };

private:
    rommuser::lowmono::MonoEngine engine;

    std::atomic<float>* pWidth = nullptr;
    std::atomic<float>* pMonoBelow = nullptr;
    std::atomic<float>* pSteep = nullptr;
    std::atomic<float>* pLowWidth = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LowMonoProcessor)
};
