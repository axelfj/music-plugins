// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "DriveEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

namespace ParamIDs
{
    inline constexpr auto bands = "bands";
    inline constexpr auto xover1 = "xover1";
    inline constexpr auto xover2 = "xover2";
    inline constexpr auto xover3 = "xover3";
    inline constexpr auto autoGain = "autogain";
    inline constexpr auto input = "input";
    inline constexpr auto output = "output";
    inline constexpr auto mix = "mix";
    inline constexpr auto oversampling = "oversampling";
    inline constexpr auto hq = "hq";

    // "b1type", "b2drive", "b3tone", "b4mix"
    inline juce::String band (int index, const char* suffix) { return "b" + juce::String (index + 1) + suffix; }
}

namespace Choices
{
    inline const juce::StringArray bands { "1", "2", "3", "4" };
    inline const juce::StringArray types { "Tape", "Tube", "Transistor", "Soft Clip", "Rectify" };
    inline const juce::StringArray oversampling { "1x", "2x", "4x" };
    inline constexpr int oversamplingValues[] = { 1, 2, 4 };
}

// Beginner presets: number of bands, per-band type and drive. Everything else at default.
struct DrivePreset
{
    const char* nameEs;
    const char* nameEn;
    int numBands;
    rommuser::warmdrive::Saturation types[rommuser::warmdrive::maxBands];
    float driveDb[rommuser::warmdrive::maxBands];
};

inline constexpr DrivePreset drivePresets[] = {
    { "CALENTAR BAJO", "WARM BASS", 1,
      { rommuser::warmdrive::Saturation::tape, rommuser::warmdrive::Saturation::tape, rommuser::warmdrive::Saturation::tape, rommuser::warmdrive::Saturation::tape },
      { 9.0f, 6.0f, 6.0f, 6.0f } },
    { "BRILLO EN VOCES", "VOCAL SHINE", 3,
      { rommuser::warmdrive::Saturation::tape, rommuser::warmdrive::Saturation::tube, rommuser::warmdrive::Saturation::tube, rommuser::warmdrive::Saturation::tube },
      { 0.0f, 4.0f, 12.0f, 12.0f } },
    { "EMPUJE DE DRUMS", "DRUM PUNCH", 2,
      { rommuser::warmdrive::Saturation::transistor, rommuser::warmdrive::Saturation::softClip, rommuser::warmdrive::Saturation::softClip, rommuser::warmdrive::Saturation::softClip },
      { 8.0f, 6.0f, 6.0f, 6.0f } },
};

class WarmDriveProcessor : public juce::AudioProcessor
{
public:
    WarmDriveProcessor();

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

    // Safe from any thread (atomic reads only).
    rommuser::warmdrive::DriveParams readParams() const;

    void applyPreset (const DrivePreset& preset);

    juce::AudioProcessorValueTreeState state;

    // For the editor (reset on read): how far each band drove into its curve, output peak.
    std::atomic<float> bandDriveDb[rommuser::warmdrive::maxBands] { 0.0f, 0.0f, 0.0f, 0.0f };
    std::atomic<float> outputPeak { 0.0f };

private:
    rommuser::warmdrive::DriveEngine engine;
    int reportedLatency = -1;

    std::atomic<float>* pBands = nullptr;
    std::atomic<float>* pXover[3] = {};
    std::atomic<float>* pAutoGain = nullptr;
    std::atomic<float>* pHq = nullptr;
    std::atomic<float>* pInput = nullptr;
    std::atomic<float>* pOutput = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pOversampling = nullptr;
    std::atomic<float>* pType[rommuser::warmdrive::maxBands] = {};
    std::atomic<float>* pDrive[rommuser::warmdrive::maxBands] = {};
    std::atomic<float>* pTone[rommuser::warmdrive::maxBands] = {};
    std::atomic<float>* pBandMix[rommuser::warmdrive::maxBands] = {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WarmDriveProcessor)
};
