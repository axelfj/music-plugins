// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PluginProcessor.h"

#include "ui/RommuserLookAndFeel.h"

#include <juce_audio_processors/juce_audio_processors.h>

// The repeats as bars: time along x, level along y, left/right as up/down for ping-pong.
class TapDisplay : public juce::Component
{
public:
    void setState (const rommuser::echoready::EchoParams& p, float delayMs, float duckGain, float wetPeak);
    void paint (juce::Graphics&) override;

private:
    rommuser::echoready::EchoParams params;
    float delayMs = 0.0f, duckGain = 1.0f, wetPeak = 0.0f;
};

class EchoReadyEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit EchoReadyEditor (EchoReadyProcessor&);
    ~EchoReadyEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateEnablement();

    EchoReadyProcessor& processor;
    rommuser::ui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    rommuser::ui::Knob time, feedback, lowCut, highCut, diffusion, wow, duck, mix, output;
    juce::ComboBox divisionBox, modeBox, styleBox, timeChangeBox;
    juce::Label divisionLabel, modeLabel, styleLabel, timeChangeLabel;
    juce::TextButton sync, presetButtons[3];

    TapDisplay display;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;

    float wetPeakSmoothed = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EchoReadyEditor)
};
