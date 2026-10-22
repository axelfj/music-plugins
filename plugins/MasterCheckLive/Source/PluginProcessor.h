// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "MeterEngine.h"
#include "Platforms.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>

namespace ParamIDs
{
    inline constexpr auto platform = "platform";
}

namespace Choices
{
    inline const juce::StringArray platform { "Spotify", "Apple Music", "YouTube", "Amazon Music", "TIDAL", "Deezer", "Beatport / club" };
}

class MasterCheckLiveProcessor : public juce::AudioProcessor
{
public:
    static constexpr int historyLength = 600;   // 60 s of short-term values at 100 ms

    MasterCheckLiveProcessor();

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

    const rommuser::mastercheck::Platform& currentPlatform() const;
    void requestReset() { resetRequested.store (true); }

    juce::AudioProcessorValueTreeState state;

    // Latest readings for the editor.
    std::atomic<float> momentary { -120.0f }, shortTerm { -120.0f }, integrated { -120.0f };
    std::atomic<float> maxMomentary { -120.0f }, maxShortTerm { -120.0f }, loudnessRange { 0.0f };
    std::atomic<float> truePeak { -120.0f }, samplePeak { -120.0f }, plr { 0.0f };
    std::atomic<float> blockTruePeak { -120.0f };   // max-held until the editor reads it
    std::array<std::atomic<float>, historyLength> shortTermHistory {};
    std::atomic<int> historyHead { 0 };

private:
    rommuser::mastercheck::MeterEngine engine;
    std::atomic<bool> resetRequested { false };
    std::atomic<float>* pPlatform = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasterCheckLiveProcessor)
};
