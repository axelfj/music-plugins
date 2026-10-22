// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "Analyzer.h"
#include "EqEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>

namespace ParamIDs
{
    inline constexpr auto output = "output";
    inline constexpr auto autoGain = "autogain";
    inline constexpr auto bypass = "bypass";
    inline constexpr auto analyzer = "analyzer";   // Off / Pre / Post / Pre+Post

    // Per band: b<1..12><suffix>
    inline juce::String band (int index, const char* suffix) { return "b" + juce::String (index + 1) + suffix; }
    inline constexpr auto on = "on";
    inline constexpr auto type = "type";
    inline constexpr auto freq = "freq";
    inline constexpr auto gain = "gain";
    inline constexpr auto q = "q";
    inline constexpr auto slope = "slope";
    inline constexpr auto place = "place";
}

// Choice lists; their order matches the enums in Biquad.h / EqEngine.h.
namespace Choices
{
    inline const juce::StringArray types { "Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut",
                                           "Notch", "Band Pass", "Tilt", "All Pass" };
    inline const juce::StringArray slopes { "6 dB", "12 dB", "18 dB", "24 dB", "36 dB", "48 dB" };
    inline constexpr int slopeValues[] = { 6, 12, 18, 24, 36, 48 };
    inline const juce::StringArray placements { "Stereo", "Left", "Right", "Mid", "Side" };
    inline const juce::StringArray analyzer { "Off", "Pre", "Post", "Pre + Post" };

    // True for the types whose gain control means something.
    inline bool typeHasGain (rommuser::dsp::FilterType t)
    {
        using T = rommuser::dsp::FilterType;
        return t == T::bell || t == T::lowShelf || t == T::highShelf || t == T::tilt;
    }
    inline bool typeHasSlope (rommuser::dsp::FilterType t)
    {
        using T = rommuser::dsp::FilterType;
        return t == T::lowCut || t == T::highCut;
    }
}

class ShapeEQProcessor : public juce::AudioProcessor
{
public:
    ShapeEQProcessor();

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

    // Current parameter set, safe to call from any thread (atomic reads only). The editor uses it
    // to draw the curve without touching the engine.
    rommuser::shapeeq::EqParams readParams() const;

    juce::AudioProcessorValueTreeState state;

    rommuser::shapeeq::SpectrumSource preSpectrum, postSpectrum;
    std::atomic<double> currentSampleRate { 44100.0 };

private:
    struct BandPtrs
    {
        std::atomic<float>* on = nullptr;
        std::atomic<float>* type = nullptr;
        std::atomic<float>* freq = nullptr;
        std::atomic<float>* gain = nullptr;
        std::atomic<float>* q = nullptr;
        std::atomic<float>* slope = nullptr;
        std::atomic<float>* place = nullptr;
    };

    rommuser::shapeeq::EqEngine engine;
    std::array<BandPtrs, rommuser::shapeeq::maxBands> bands;
    std::atomic<float>* pOutput = nullptr;
    std::atomic<float>* pAutoGain = nullptr;
    std::atomic<float>* pBypass = nullptr;
    std::atomic<float>* pAnalyzer = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShapeEQProcessor)
};
