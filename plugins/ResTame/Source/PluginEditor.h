// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PluginProcessor.h"

#include "ui/RommuserLookAndFeel.h"

#include <juce_audio_processors/juce_audio_processors.h>

// The live reduction curve: how many dB are being taken at each frequency, with the active range.
class ReductionDisplay : public juce::Component
{
public:
    void setState (const float* reduction, int points, float lowHz, float highHz, float depthDb, bool delta, bool external);
    void paint (juce::Graphics&) override;

private:
    std::vector<float> values;
    float lowHz = 80.0f, highHz = 16000.0f, depthDb = 8.0f;
    bool delta = false, external = false;
};

class ResTameEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ResTameEditor (ResTameProcessor&);
    ~ResTameEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    ResTameProcessor& processor;
    rommuser::ui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    rommuser::ui::Knob depth, sharpness, selectivity, threshold, attack, release, low, high;
    juce::ComboBox placementBox;
    juce::Label placementLabel;
    juce::TextButton external, delta, presetButtons[4];

    ReductionDisplay display;
    std::vector<float> smoothed;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResTameEditor)
};
