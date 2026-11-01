// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PitchEngine.h"
#include "Spectrum.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

namespace ParamIDs
{
    inline constexpr auto mode = "mode";
    inline constexpr auto semitones = "semitones";
    inline constexpr auto cents = "cents";
    inline constexpr auto keepFormants = "keepformants";
    inline constexpr auto formant = "formant";
    inline constexpr auto mix = "mix";
    inline constexpr auto output = "output";
    inline constexpr auto original = "original";
}

namespace Choices
{
    inline const juce::StringArray modes { "Clean", "Voice", "Fast" };
}

class PitchRProcessor : public juce::AudioProcessor
{
public:
    PitchRProcessor();

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

    rommuser::pitchr::PitchParams readParams() const;
    void setParameter (const char* id, float value);

    // Latency (ms) each mode reports at the current sample rate, for the mode buttons.
    float latencyMsFor (rommuser::pitchr::Mode mode) const;

    juce::AudioProcessorValueTreeState state;

    // Display: the input before and the output after the shift (fed only while an editor is open).
    rommuser::pitchr::SpectrumSource inSpectrum, outSpectrum;
    std::atomic<double> currentSampleRate { 48000.0 };

private:
    rommuser::pitchr::PitchEngine engine;
    int reportedLatency = -1;

    std::atomic<float>* p (const char* id) const { return state.getRawParameterValue (id); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchRProcessor)
};
