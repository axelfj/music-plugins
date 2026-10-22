// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PluginProcessor.h"

#include "ui/RommuserLookAndFeel.h"

#include <juce_audio_processors/juce_audio_processors.h>

// The voices as markers on a delay-time axis, one row per channel, moving with the LFO.
class VoiceDisplay : public juce::Component
{
public:
    void setState (const rommuser::wideready::WideParams& p, const float delays[2][3], int voices);
    void paint (juce::Graphics&) override;

private:
    rommuser::wideready::WideParams params;
    float delayMs[2][3] {};
    int voices = 0;
};

class WideReadyEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit WideReadyEditor (WideReadyProcessor&);
    ~WideReadyEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateEnablement();

    WideReadyProcessor& processor;
    rommuser::ui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    rommuser::ui::Knob rate, depth, delay, feedback, spread, cents, mix, output;
    juce::ComboBox modeBox, voicesBox;
    juce::Label modeLabel, voicesLabel;
    juce::TextButton ensemble, throughZero, invert, presetButtons[4];

    VoiceDisplay display;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WideReadyEditor)
};
