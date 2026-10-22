// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "KickEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <vector>

namespace ParamIDs
{
    inline constexpr auto follow = "follow";
    inline constexpr auto baseNote = "basenote";
    inline constexpr auto tune = "tune";
    inline constexpr auto glide = "glide";
    inline constexpr auto velocity = "velocity";
    inline constexpr auto pitchStart = "pitchstart";
    inline constexpr auto pitchDecay = "pitchdecay";
    inline constexpr auto pitchCurve = "pitchcurve";
    inline constexpr auto attack = "attack";
    inline constexpr auto hold = "hold";
    inline constexpr auto decay = "decay";
    inline constexpr auto ampCurve = "ampcurve";
    inline constexpr auto clickMode = "clickmode";
    inline constexpr auto clickLevel = "clicklevel";
    inline constexpr auto clickDecay = "clickdecay";
    inline constexpr auto clickTone = "clicktone";
    inline constexpr auto drive = "drive";
    inline constexpr auto clip = "clip";
    inline constexpr auto sub = "sub";
    inline constexpr auto highCut = "highcut";
    inline constexpr auto output = "output";
}

namespace Choices
{
    inline constexpr int firstNote = 24;   // C0
    inline constexpr int numNotes = 37;    // .. C3
    inline juce::StringArray noteNames()
    {
        static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        juce::StringArray out;
        for (int n = firstNote; n < firstNote + numNotes; ++n)
            out.add (juce::String (names[n % 12]) + juce::String (n / 12 - 1));
        return out;
    }
    inline const juce::StringArray clickModes { "Noise", "Tick" };
}

struct KickPreset
{
    const char* nameEs;
    const char* nameEn;
    int baseNote;
    float pitchStart, pitchDecay, pitchCurve;
    float attack, hold, decay, ampCurve;
    int clickMode; float clickLevel, clickDecay, clickTone;
    float drive, clip, sub, highCut;
};

inline constexpr KickPreset kickPresets[] = {
    { "HOUSE", "HOUSE",         41, 30.0f, 45.0f, 0.5f, 0.1f, 0.0f, 380.0f, 0.75f, 0, 0.25f, 5.0f, 3000.0f, 3.0f, 0.2f, 2.0f, 12000.0f },
    { "TECHNO", "TECHNO",       38, 42.0f, 35.0f, 0.4f, 0.1f, 10.0f, 320.0f, 0.8f, 0, 0.35f, 4.0f, 4000.0f, 9.0f, 0.5f, 3.0f, 9000.0f },
    { "HARDSTYLE", "HARDSTYLE", 36, 48.0f, 60.0f, 0.6f, 0.1f, 40.0f, 500.0f, 0.3f, 1, 0.3f, 8.0f, 1500.0f, 30.0f, 1.0f, 0.0f, 6000.0f },
    { "TRAP", "TRAP",           31, 24.0f, 90.0f, 0.5f, 0.1f, 0.0f, 900.0f, 0.9f, 0, 0.15f, 6.0f, 2500.0f, 0.0f, 0.1f, 4.0f, 20000.0f },
};

class KickReadyProcessor : public juce::AudioProcessor
{
public:
    KickReadyProcessor();

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
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    rommuser::kickready::KickParams readParams() const;
    void applyPreset (const KickPreset& preset);
    int getBaseNote() const;
    double getSampleRateForDisplay() const { return currentSampleRate; }

    // UI: play the kick at the base note on the next block.
    void triggerFromUi() { uiTrigger.store (true); }

    // Message thread: renders one kick with the current parameters (for the display and WAV export).
    std::vector<float> renderPreview (double sampleRate);
    // Writes a 24-bit mono WAV at the current sample rate. Returns false on failure.
    bool exportWav (const juce::File& file);

    juce::AudioProcessorValueTreeState state;

    std::atomic<float> outputPeak { 0.0f };
    std::atomic<int> lastNote { 36 };
    std::atomic<bool> sounding { false };

private:
    rommuser::kickready::KickEngine engine;
    std::vector<float> mono;
    std::atomic<bool> uiTrigger { false };
    double currentSampleRate = 48000.0;

    std::atomic<float>* p (const char* id) const { return state.getRawParameterValue (id); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KickReadyProcessor)
};
