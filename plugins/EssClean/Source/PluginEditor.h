// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PluginProcessor.h"

#include "ui/RommuserLookAndFeel.h"

#include <juce_audio_processors/juce_audio_processors.h>

// Input spectrum (log frequency) with the detection range shaded. Drag inside the shaded area to move
// it, drag its edges to resize it.
class SpectrumDisplay : public juce::Component
{
public:
    explicit SpectrumDisplay (EssCleanProcessor&);

    void update();
    void paint (juce::Graphics&) override;

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;

private:
    float xFor (float hz, juce::Rectangle<float> r) const;
    float hzFor (float x, juce::Rectangle<float> r) const;
    juce::Rectangle<float> plotArea() const;

    EssCleanProcessor& processor;
    juce::RangedAudioParameter* lowParam = nullptr;
    juce::RangedAudioParameter* highParam = nullptr;
    enum class Drag { none, low, high, both } drag = Drag::none;
    float dragLow = 0.0f, dragHigh = 0.0f;
    static constexpr float minHz = 100.0f, maxHz = 20000.0f, minDb = -90.0f;
};

class ReductionMeter : public juce::Component
{
public:
    void push (float grDb);
    void paint (juce::Graphics&) override;

private:
    float valueDb = 0.0f, holdDb = 0.0f;
    int holdFrames = 0;
};

class EssCleanEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit EssCleanEditor (EssCleanProcessor&);
    ~EssCleanEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    EssCleanProcessor& processor;
    rommuser::ui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    rommuser::ui::Knob low, high, threshold, range, attack, release;
    juce::ComboBox modeBox, lookaheadBox;
    juce::Label modeLabel, lookaheadLabel, hint;
    juce::TextButton listen, presetButtons[3];

    SpectrumDisplay display;
    ReductionMeter grMeter;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EssCleanEditor)
};
