// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "WaveEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include <atomic>
#include <vector>

namespace ParamIDs
{
    // Oscillators (prefix o1 / o2)
    inline constexpr const char* oscOn[2]     = { "o1on", "o2on" };
    inline constexpr const char* oscWave[2]   = { "o1wave", "o2wave" };
    inline constexpr const char* oscMorph[2]  = { "o1morph", "o2morph" };
    inline constexpr const char* oscOct[2]    = { "o1oct", "o2oct" };
    inline constexpr const char* oscSemi[2]   = { "o1semi", "o2semi" };
    inline constexpr const char* oscFine[2]   = { "o1fine", "o2fine" };
    inline constexpr const char* oscLevel[2]  = { "o1level", "o2level" };
    inline constexpr const char* oscPan[2]    = { "o1pan", "o2pan" };
    inline constexpr const char* oscUnison[2] = { "o1uni", "o2uni" };
    inline constexpr const char* oscDetune[2] = { "o1det", "o2det" };
    inline constexpr const char* oscWidth[2]  = { "o1width", "o2width" };
    inline constexpr const char* oscPhase[2]  = { "o1phase", "o2phase" };
    inline constexpr const char* oscRetrig[2] = { "o1retrig", "o2retrig" };

    inline constexpr auto subLevel = "sublevel";
    inline constexpr auto subOct = "suboct";
    inline constexpr auto subSquare = "subsquare";
    inline constexpr auto noiseLevel = "noiselevel";
    inline constexpr auto noiseColour = "noisecolour";

    inline constexpr auto filterMode = "fmode";
    inline constexpr auto cutoff = "cutoff";
    inline constexpr auto resonance = "reso";
    inline constexpr auto filterDrive = "fdrive";
    inline constexpr auto keyTrack = "keytrack";
    inline constexpr auto envToCutoff = "env2cut";
    inline constexpr auto velToCutoff = "vel2cut";
    inline constexpr auto envToPitch = "env2pitch";

    inline constexpr const char* envAttack[2]  = { "a1", "a2" };
    inline constexpr const char* envDecay[2]   = { "d1", "d2" };
    inline constexpr const char* envSustain[2] = { "s1", "s2" };
    inline constexpr const char* envRelease[2] = { "r1", "r2" };
    inline constexpr auto velToAmp = "vel2amp";

    inline constexpr const char* lfoShape[2]  = { "l1shape", "l2shape" };
    inline constexpr const char* lfoSync[2]   = { "l1sync", "l2sync" };
    inline constexpr const char* lfoRate[2]   = { "l1rate", "l2rate" };
    inline constexpr const char* lfoDiv[2]    = { "l1div", "l2div" };
    inline constexpr const char* lfoFade[2]   = { "l1fade", "l2fade" };
    inline constexpr const char* lfoRetrig[2] = { "l1retrig", "l2retrig" };
    inline constexpr auto lfo1ToPitch = "l1pitch";
    inline constexpr auto lfo1ToCutoff = "l1cut";
    inline constexpr auto lfo1ToAmp = "l1amp";
    inline constexpr auto lfo2ToMorph = "l2morph";
    inline constexpr auto lfo2ToCutoff = "l2cut";
    inline constexpr auto lfo2ToPitch = "l2pitch";
    inline constexpr auto wheelToCutoff = "wheelcut";
    inline constexpr auto wheelToVibrato = "wheelvib";

    inline constexpr auto mono = "mono";
    inline constexpr auto legato = "legato";
    inline constexpr auto glide = "glide";
    inline constexpr auto bendRange = "bend";
    inline constexpr auto tune = "tune";

    inline constexpr auto drive = "drive";
    inline constexpr auto chorusMix = "chmix";
    inline constexpr auto chorusRate = "chrate";
    inline constexpr auto chorusDepth = "chdepth";
    inline constexpr auto delayMix = "dlmix";
    inline constexpr auto delaySync = "dlsync";
    inline constexpr auto delayDiv = "dldiv";
    inline constexpr auto delayTime = "dltime";
    inline constexpr auto delayFeedback = "dlfb";
    inline constexpr auto delayPingPong = "dlpp";
    inline constexpr auto delayTone = "dltone";
    inline constexpr auto reverbMix = "rvmix";
    inline constexpr auto reverbDecay = "rvdecay";
    inline constexpr auto reverbPlate = "rvplate";
    inline constexpr auto output = "output";
}

namespace Choices
{
    inline juce::StringArray waves()
    {
        juce::StringArray out;
        for (auto* n : rommuser::waveready::waveNames) out.add (n);
        return out;
    }
    inline juce::StringArray divisions()
    {
        juce::StringArray out;
        for (auto* n : rommuser::waveready::lfoDivisionNames) out.add (n);
        return out;
    }
    inline const juce::StringArray octaves { "-2", "-1", "0", "+1", "+2" };
    inline const juce::StringArray subOctaves { "-1 oct", "-2 oct" };
    inline const juce::StringArray noiseColours { "White", "Pink" };
    inline const juce::StringArray filterModes { "LP 12", "LP 24", "HP 12", "BP 12" };
    inline const juce::StringArray lfoShapes { "Sine", "Triangle", "Saw", "Square", "S&H" };
}

// A preset is a list of (parameter, value) pairs applied over the defaults.
struct WavePreset
{
    const char* name;
    std::vector<std::pair<const char*, float>> values;
};

const std::vector<WavePreset>& wavePresets();

class WaveReadyProcessor : public juce::AudioProcessor,
                           private juce::AudioProcessorValueTreeState::Listener
{
public:
    WaveReadyProcessor();
    ~WaveReadyProcessor() override;

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
    double getTailLengthSeconds() const override { return 10.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    rommuser::waveready::WaveParams readParams() const;
    void applyPreset (const WavePreset& preset);
    void applyPreset (int index);

    juce::AudioProcessorValueTreeState state;
    juce::MidiKeyboardState keyboardState;

    // For the editor (written on the audio thread).
    std::atomic<float> outputPeak { 0.0f };
    std::atomic<int> activeVoices { 0 };
    std::atomic<float> displayCutoff { 0.0f };
    std::atomic<int> lastPreset { 0 };   // index into wavePresets(), -1 = edited

    // Last rendered block (mono sum) for the oscilloscope. Lock-free single writer / single reader.
    static constexpr int scopeSize = 2048;
    std::array<std::atomic<float>, scopeSize> scope {};
    std::atomic<int> scopeWrite { 0 };

private:
    rommuser::waveready::WaveEngine engine;
    std::vector<float> left, right;
    double currentSampleRate = 48000.0;
    float lastBpm = 120.0f;

    void handleMidi (const juce::MidiMessage& msg);
    void parameterChanged (const juce::String&, float) override;
    std::atomic<bool> applyingPreset { false };

    std::atomic<float>* p (const char* id) const { return state.getRawParameterValue (id); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveReadyProcessor)
};
