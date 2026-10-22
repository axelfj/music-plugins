// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PluginProcessor.h"

#include "ui/RommuserLookAndFeel.h"

#include <juce_audio_processors/juce_audio_processors.h>

// Live frequency response of the phaser (notches sweeping) for both channels, plus the LFO position.
class ResponseDisplay : public juce::Component
{
public:
    void setState (const rommuser::phaseready::PhaseParams& p, const float* coeffsL, const float* coeffsR, int stages,
                   float lfoL, float lfoR);
    void paint (juce::Graphics&) override;

private:
    float responseDb (const float* coeffs, int stages, float feedback, float mix, double w) const;

    rommuser::phaseready::PhaseParams params;
    float coeffs[2][rommuser::phaseready::maxStages] {};
    int stages = 0;
    float lfoL = 0.0f, lfoR = 0.0f;
};

class PhaseReadyEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PhaseReadyEditor (PhaseReadyProcessor&);
    ~PhaseReadyEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateEnablement();

    PhaseReadyProcessor& processor;
    rommuser::ui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    rommuser::ui::Knob rate, depth, feedback, center, spread, mix, output;
    juce::ComboBox stagesBox, divisionBox, shapeBox;
    juce::Label stagesLabel, divisionLabel, shapeLabel;
    juce::TextButton sync, presetButtons[3];

    ResponseDisplay display;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhaseReadyEditor)
};
