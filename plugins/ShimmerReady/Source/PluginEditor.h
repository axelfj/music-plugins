// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PluginProcessor.h"

#include "ui/RommuserLookAndFeel.h"

#include <juce_audio_processors/juce_audio_processors.h>

// The echoes as a staircase: one bar per repeat at its time, height = level, label = pitch.
class EchoDisplay : public juce::Component
{
public:
    explicit EchoDisplay (ShimmerReadyProcessor&);
    void paint (juce::Graphics&) override;

private:
    ShimmerReadyProcessor& processor;
};

class ShimmerReadyEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ShimmerReadyEditor (ShimmerReadyProcessor&);
    ~ShimmerReadyEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateEnablement();

    ShimmerReadyProcessor& processor;
    rommuser::ui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    EchoDisplay display;
    juce::TextButton syncButton, reverseButton, freezeButton;
    juce::ComboBox divisionBox;
    juce::Label divisionLabel;
    rommuser::ui::Knob time, pitch, grain, feedback, lowCut, highCut, shimmer, decay, width, mix, output;
    juce::TextButton presetButtons[6];

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShimmerReadyEditor)
};
