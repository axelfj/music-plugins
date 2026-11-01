// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PluginProcessor.h"

#include "ui/RommuserLookAndFeel.h"
#include "ui/ScalableEditor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>

// Scrolling history: the detector level as a filled area, with the attack and sustain gain
// contributions drawn as lines around a 0 dB centre line.
class ShapeDisplay : public juce::Component
{
public:
    ShapeDisplay();
    void push (float levelDb, float attackGainDb, float sustainGainDb);
    void paint (juce::Graphics&) override;

private:
    static constexpr int historyLength = 180;   // 6 s at 30 fps
    std::array<float, historyLength> levels {}, attacks {}, sustains {};
    int head = 0;
};

class PunchShapeEditor : public juce::Component, private juce::Timer
{
public:
    explicit PunchShapeEditor (PunchShapeProcessor&);
    ~PunchShapeEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    PunchShapeProcessor& processor;
    rommuser::ui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    rommuser::ui::Knob attack, sustain, speed, tail, output;
    juce::ComboBox detectorBox;
    juce::Label detectorLabel, clipLed, hint;
    juce::TextButton clip, presetButtons[4];

    ShapeDisplay display;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;

    int clipFrames = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PunchShapeEditor)
};
