// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PluginProcessor.h"

#include "ui/RommuserLookAndFeel.h"
#include "ui/ScalableEditor.h"

#include <juce_audio_processors/juce_audio_processors.h>

// Input level vs output level, with the knee, and a dot that follows the signal.
class TransferDisplay : public juce::Component
{
public:
    void setState (const rommuser::pressready::CompParams& p, float liveLevelDb, float liveGrDb);
    void paint (juce::Graphics&) override;

private:
    rommuser::pressready::CompParams params;
    float levelDb = -100.0f, grDb = 0.0f;
};

// Big gain-reduction meter, growing downwards from 0 dB.
class ReductionMeter : public juce::Component
{
public:
    void push (float grDb);
    void paint (juce::Graphics&) override;

private:
    float valueDb = 0.0f, holdDb = 0.0f;
    int holdFrames = 0;
};

class LevelMeter : public juce::Component
{
public:
    explicit LevelMeter (juce::String labelText) : label (std::move (labelText)) {}
    void push (float linearPeak);
    void paint (juce::Graphics&) override;

private:
    juce::String label;
    float levelDb = -60.0f, holdDb = -60.0f;
    int holdFrames = 0;
};

class PressReadyEditor : public juce::Component, private juce::Timer
{
public:
    explicit PressReadyEditor (PressReadyProcessor&);
    ~PressReadyEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    PressReadyProcessor& processor;
    rommuser::ui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    rommuser::ui::Knob threshold, ratio, attack, release, knee, makeup, mix;
    juce::ComboBox characterBox, detectorBox, scHpfBox, lookaheadBox;
    juce::Label characterLabel, detectorLabel, scHpfLabel, lookaheadLabel;
    juce::TextButton autoRelease, autoMakeup, external;

    TransferDisplay curve;
    ReductionMeter grMeter;
    LevelMeter inMeter { rommuser::ui::tr ("ENT", "IN") }, outMeter { rommuser::ui::tr ("SAL", "OUT") };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;

    float liveLevelDb = -100.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PressReadyEditor)
};
