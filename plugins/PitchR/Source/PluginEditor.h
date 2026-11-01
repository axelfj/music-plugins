// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PluginProcessor.h"

#include "ui/RommuserLookAndFeel.h"
#include "ui/ScalableEditor.h"

#include <juce_audio_processors/juce_audio_processors.h>

// Spectrum before (dim) and after (accent) the shift, on a log frequency axis, with the interval
// and the loudest note mapped from input to output written on top.
class PitchDisplay : public juce::Component
{
public:
    explicit PitchDisplay (PitchRProcessor&);
    void update();
    void paint (juce::Graphics&) override;

private:
    juce::Path curve (const rommuser::pitchr::SpectrumAnalyzer&, juce::Rectangle<float> plot) const;

    PitchRProcessor& processor;
    rommuser::pitchr::SpectrumAnalyzer inAnalyzer, outAnalyzer;
};

class PitchREditor : public juce::Component, private juce::Timer
{
public:
    explicit PitchREditor (PitchRProcessor&);
    ~PitchREditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    static constexpr int numIntervals = 10;

    void timerCallback() override;
    void refreshButtons();

    PitchRProcessor& processor;
    rommuser::ui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    rommuser::ui::Knob pitch, fine, formant, mix, output;
    juce::TextButton modeButtons[rommuser::pitchr::numModes], intervalButtons[numIntervals];
    juce::TextButton keepFormantsButton, originalButton;
    juce::Label intervalsLabel, modeLabel;

    PitchDisplay display;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchREditor)
};
