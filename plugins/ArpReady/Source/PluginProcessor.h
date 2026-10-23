// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ArpEngine.h"
#include "dsp/PreviewSynth.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include <array>
#include <atomic>
#include <vector>

namespace ParamIDs
{
    inline constexpr auto mode = "mode";
    inline constexpr auto division = "div";
    inline constexpr auto octaves = "octaves";
    inline constexpr auto octaveMode = "octmode";
    inline constexpr auto gate = "gate";
    inline constexpr auto swing = "swing";
    inline constexpr auto latch = "latch";
    inline constexpr auto hostSync = "sync";
    inline constexpr auto transpose = "transpose";
    inline constexpr auto patternLength = "patlen";
    inline constexpr const char* step[16] = { "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8",
                                              "s9", "s10", "s11", "s12", "s13", "s14", "s15", "s16" };
    inline constexpr auto velocityMode = "velmode";
    inline constexpr auto velocity = "velocity";
    inline constexpr auto accent = "accent";
    inline constexpr auto sound = "sound";
    inline constexpr auto soundLevel = "soundlevel";
}

namespace Choices
{
    inline juce::StringArray modes()
    {
        juce::StringArray out;
        for (auto* n : rommuser::arpready::modeNames) out.add (n);
        return out;
    }
    inline juce::StringArray divisions()
    {
        juce::StringArray out;
        for (const auto& d : rommuser::arpready::divisions) out.add (d.name);
        return out;
    }
    inline const juce::StringArray octaveModes { "Up", "Down", "Up-Down" };
    inline const juce::StringArray velocityModes { "As played", "Fixed" };
}

struct ArpPreset
{
    const char* name;
    std::vector<std::pair<const char*, float>> values;
};

const std::vector<ArpPreset>& arpPresets();

class ArpReadyProcessor : public juce::AudioProcessor,
                          private juce::AudioProcessorValueTreeState::Listener
{
public:
    ArpReadyProcessor();
    ~ArpReadyProcessor() override;

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
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    rommuser::arpready::ArpParams readParams() const;
    void applyPreset (const ArpPreset& preset);
    void applyPreset (int index);

    juce::AudioProcessorValueTreeState state;
    juce::MidiKeyboardState keyboardState;    // what the user plays (on-screen keyboard)
    juce::MidiKeyboardState outputKeys;       // what the plugin sends out (display only)

    // For the editor (written on the audio thread).
    std::atomic<int> currentStep { -1 };
    std::atomic<int> heldCount { 0 };
    std::atomic<float> displayBpm { 120.0f };
    std::atomic<bool> transportPlaying { false };
    std::atomic<float> outputPeak { 0.0f };
    std::atomic<int> lastPreset { 0 };        // index into arpPresets(), -1 = edited

private:
    rommuser::arpready::ArpEngine engine;
    rommuser::dsp::PreviewSynth synth;
    std::vector<rommuser::arpready::NoteEvent> inEvents, outEvents;
    juce::MidiBuffer outMidi;
    std::vector<float> left, right;
    double currentSampleRate = 48000.0;
    rommuser::arpready::Transport transport;

    void parameterChanged (const juce::String&, float) override;
    std::atomic<bool> applyingPreset { false };

    std::atomic<float>* p (const char* id) const { return state.getRawParameterValue (id); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArpReadyProcessor)
};
