// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PluginProcessor.h"

#include "ui/RommuserLookAndFeel.h"
#include "ui/ScalableEditor.h"

#include <juce_audio_processors/juce_audio_processors.h>

// Last two beats of input with the captured slice and the repeat grid drawn over it.
class StutterDisplay : public juce::Component
{
public:
    explicit StutterDisplay (StutterReadyProcessor&);
    void paint (juce::Graphics&) override;

private:
    StutterReadyProcessor& processor;
};

// Step grid: 16 steps per bar, one row per bar. Click cycles the gesture, right-click clears.
class StepGrid : public juce::Component
{
public:
    explicit StepGrid (StutterReadyProcessor&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

    int selectedGesture = 0;   // what a click paints

private:
    int cellAt (juce::Point<int>) const;
    int bars() const;
    StutterReadyProcessor& processor;
    int paintValue = 0;
};

// Press-and-hold gesture button: pressing plays the gesture, clicking also selects it for editing.
class GestureButton : public juce::TextButton
{
public:
    std::function<void()> onPress, onRelease;
    void mouseDown (const juce::MouseEvent& e) override { juce::TextButton::mouseDown (e); if (onPress) onPress(); }
    void mouseUp (const juce::MouseEvent& e) override { juce::TextButton::mouseUp (e); if (onRelease) onRelease(); }
};

class StutterReadyEditor : public juce::Component, private juce::Timer
{
public:
    explicit StutterReadyEditor (StutterReadyProcessor&);
    ~StutterReadyEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void selectGesture (int g);

    StutterReadyProcessor& processor;
    rommuser::ui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    StutterDisplay display;
    StepGrid grid;
    GestureButton gestureButtons[rommuser::stutterready::numGestures];
    juce::TextButton clearButton;

    // Selected-gesture editor.
    juce::Label editLabel;
    juce::ComboBox divStartBox, divEndBox, filterBox;
    juce::Label divStartLabel, divEndLabel, filterLabel;
    juce::TextButton reverseButton, tapeStopButton;
    rommuser::ui::Knob pitch, gate, filterAmount, pan, crush, echo;

    // Common.
    rommuser::ui::Knob ramp, mix, output;
    juce::ComboBox quantizeBox, barsBox;
    juce::Label quantizeLabel, barsLabel;
    juce::TextButton seqButton;
    juce::TextButton presetButtons[6];

    int selected = -1;
    juce::Rectangle<int> editArea;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments, gestureSliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments, gestureButtonAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments, gestureComboAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StutterReadyEditor)
};
