// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ChordEngine.h"
#include "dsp/PreviewSynth.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include <array>
#include <atomic>
#include <vector>

namespace ParamIDs
{
    inline constexpr auto mode = "mode";
    inline constexpr auto key = "key";
    inline constexpr auto scale = "scale";
    inline constexpr auto snap = "snap";
    inline constexpr auto add7 = "add7";
    inline constexpr auto add9 = "add9";
    inline constexpr auto chordType = "chord";
    inline constexpr auto voicing = "voicing";
    inline constexpr auto inversion = "inversion";
    inline constexpr auto bassOct = "bassoct";
    inline constexpr auto topOct = "topoct";
    inline constexpr auto transpose = "transpose";
    inline constexpr auto strum = "strum";
    inline constexpr auto strumDir = "strumdir";
    inline constexpr auto spread = "spread";
    inline constexpr auto humanize = "humanize";
    inline constexpr auto sound = "sound";
    inline constexpr auto soundLevel = "soundlevel";
}

namespace Choices
{
    inline const juce::StringArray modes { "Scale", "Fixed" };
    inline juce::StringArray keys()
    {
        juce::StringArray out;
        for (auto* n : rommuser::midi::noteNames) out.add (n);
        return out;
    }
    inline juce::StringArray scales()
    {
        juce::StringArray out;
        for (const auto& s : rommuser::midi::scales) out.add (s.name);
        return out;
    }
    inline juce::StringArray chordTypes()
    {
        juce::StringArray out;
        for (const auto& t : rommuser::chordready::chordTypes) out.add (t.name);
        return out;
    }
    inline const juce::StringArray voicings { "Close", "Open", "Spread", "Wide" };
    inline const juce::StringArray strumDirs { "Up", "Down", "Alternate", "Random" };
}

struct ChordPreset
{
    const char* name;
    std::vector<std::pair<const char*, float>> values;
};

const std::vector<ChordPreset>& chordPresets();

class ChordReadyProcessor : public juce::AudioProcessor,
                            private juce::AudioProcessorValueTreeState::Listener
{
public:
    ChordReadyProcessor();
    ~ChordReadyProcessor() override;

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

    rommuser::chordready::ChordParams readParams() const;
    void applyPreset (const ChordPreset& preset);
    void applyPreset (int index);

    juce::AudioProcessorValueTreeState state;
    juce::MidiKeyboardState keyboardState;    // what the user plays (on-screen keyboard)
    juce::MidiKeyboardState outputKeys;       // what the plugin sends out (display only)

    // For the editor (written on the audio thread).
    std::atomic<int> displayInput { -1 };     // last input note, -1 = none
    std::atomic<int> displayCount { 0 };
    std::array<std::atomic<int>, 12> displayNotes {};
    std::atomic<int> heldInputs { 0 };
    std::atomic<float> outputPeak { 0.0f };
    std::atomic<int> lastPreset { 0 };        // index into chordPresets(), -1 = edited

private:
    rommuser::chordready::ChordEngine engine;
    rommuser::dsp::PreviewSynth synth;
    std::vector<rommuser::chordready::NoteEvent> inEvents, outEvents;
    juce::MidiBuffer outMidi;
    std::vector<float> left, right;
    double currentSampleRate = 48000.0;

    void parameterChanged (const juce::String&, float) override;
    std::atomic<bool> applyingPreset { false };

    std::atomic<float>* p (const char* id) const { return state.getRawParameterValue (id); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordReadyProcessor)
};
