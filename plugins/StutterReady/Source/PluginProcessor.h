// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "StutterEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>

namespace ParamIDs
{
    inline constexpr auto ramp = "ramp";
    inline constexpr auto quantize = "quantize";
    inline constexpr auto seqOn = "seqon";
    inline constexpr auto seqBars = "seqbars";
    inline constexpr auto mix = "mix";
    inline constexpr auto output = "output";

    // Per gesture (g = 1 .. 8): g<n>divstart, g<n>divend, g<n>pitch, g<n>reverse, g<n>tapestop,
    // g<n>gate, g<n>filter, g<n>filteramt, g<n>pan, g<n>crush, g<n>echo.
    inline juce::String gesture (int g, const char* suffix) { return "g" + juce::String (g + 1) + suffix; }
}

namespace Choices
{
    inline const juce::StringArray divisions { "1/4", "1/8", "1/16", "1/32", "1/64", "1/128" };
    inline const juce::StringArray quantize { "Off", "1/4", "1/8", "1/16" };
    inline const juce::StringArray bars { "1 bar", "2 bars", "4 bars" };
    inline constexpr int barsValues[] = { 1, 2, 4 };
    inline const juce::StringArray filters { "Off", "Low pass", "High pass" };
    inline const char* const gestureNamesEs[rommuser::stutterready::numGestures] = { "STUTTER", "BUZZ", "PITCH +", "PITCH −", "REVERSA", "TAPE STOP", "GATE", "FILTRO" };
    inline const char* const gestureNamesEn[rommuser::stutterready::numGestures] = { "STUTTER", "BUZZ", "PITCH UP", "PITCH DOWN", "REVERSE", "TAPE STOP", "GATE", "FILTER" };
}

// "Build" presets: a sequencer pattern (1/16 steps, -1 = off) plus bars and ramp length.
struct StutterPreset
{
    const char* nameEs;
    const char* nameEn;
    int bars;
    float rampBeats;
    const char* pattern;   // one char per step: '.' off, '0'..'7' gesture; shorter patterns repeat
};

inline constexpr StutterPreset stutterPresets[] = {
    { "BUILD 1 COMPÁS", "1 BAR BUILD",  1, 2.0f, "........0000111." },
    { "BUILD 2 COMPASES", "2 BAR BUILD", 2, 4.0f, "................00000000111111117777777711111111" },
    { "RELLENO", "DROP FILL",           1, 1.0f, "............0404" },
    { "GLITCH", "GLITCH",               1, 0.5f, "..0...4...2...6." },
    { "FRENAZO", "TAPE OUT",            1, 4.0f, "............5555" },
    { "LIMPIO", "CLEAR",                1, 2.0f, "" },
};

class StutterReadyProcessor : public juce::AudioProcessor
{
public:
    StutterReadyProcessor();

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
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    rommuser::stutterready::StutterParams readParams() const;
    void applyPreset (const StutterPreset& preset);

    // Step sequencer pattern (not automatable: stored as a property of the state tree).
    int getStep (int index) const { return steps[(size_t) juce::jlimit (0, rommuser::stutterready::maxSteps - 1, index)].load(); }
    void setStep (int index, int gesture);
    void clearSteps();

    // UI gesture buttons (press / release) — forwarded to the audio thread.
    void uiPress (int gesture) { uiRequest.store (gesture + 1); }
    void uiRelease() { uiRequest.store (-1); }

    juce::AudioProcessorValueTreeState state;

    // Display data.
    std::atomic<bool> active { false }, playing { false };
    std::atomic<int> currentGesture { -1 }, currentStep { -1 };
    std::atomic<float> progress { 0.0f }, divisionNow { 0.0f }, hostBpm { 120.0f }, wetPeak { 0.0f };
    static constexpr int waveformLength = 512;
    std::array<std::atomic<float>, waveformLength> waveform;   // last two beats, peak per bin
    std::atomic<float> sliceStartFraction { -1.0f }, sliceLengthFraction { 0.0f };   // in the waveform window

private:
    void loadStepsFromState();
    void storeStepsToState();

    rommuser::stutterready::StutterEngine engine;
    std::array<std::atomic<int>, rommuser::stutterready::maxSteps> steps;
    std::atomic<int> uiRequest { 0 };   // 0 none, g+1 press, -1 release
    double currentSampleRate = 48000.0;
    int waveformCountdown = 0;

    std::atomic<float>* p (const char* id) const { return state.getRawParameterValue (id); }
    std::atomic<float>* p (const juce::String& id) const { return state.getRawParameterValue (id); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StutterReadyProcessor)
};
