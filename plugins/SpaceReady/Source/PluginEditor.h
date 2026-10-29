// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PluginProcessor.h"

#include "ui/RommuserLookAndFeel.h"
#include "ui/ScalableEditor.h"

#include <juce_audio_processors/juce_audio_processors.h>

// The decay envelope (pre-delay, then exponential RT60 curve) with the live wet level on top.
class TailDisplay : public juce::Component
{
public:
    void setState (const rommuser::spaceready::SpaceParams& p, float wetRms, float duckGain);
    void paint (juce::Graphics&) override;

private:
    rommuser::spaceready::SpaceParams params;
    float wetRms = 0.0f, duckGain = 1.0f;
    std::vector<float> history;
};

class SpaceReadyEditor : public juce::Component, private juce::Timer
{
public:
    explicit SpaceReadyEditor (SpaceReadyProcessor&);
    ~SpaceReadyEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    SpaceReadyProcessor& processor;
    rommuser::ui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    rommuser::ui::Knob size, decay, preDelay, damping, diffusion, modulation, width, lowCut, highCut, duck, mix, output;
    juce::ComboBox modeBox;
    juce::Label modeLabel;
    juce::TextButton freeze, presetButtons[4];

    TailDisplay display;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;

    float wetSmoothed = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpaceReadyEditor)
};
