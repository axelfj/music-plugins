// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PluginProcessor.h"

#include "ui/RommuserLookAndFeel.h"

#include <juce_audio_processors/juce_audio_processors.h>

// Scrolling pitch view: detected pitch (dim) and corrected pitch (accent) over the last seconds,
// on a piano-roll grid where the notes of the chosen scale are highlighted.
class PitchDisplay : public juce::Component
{
public:
    explicit PitchDisplay (TuneReadyProcessor&);
    void paint (juce::Graphics&) override;

private:
    TuneReadyProcessor& processor;
};

class TuneReadyEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TuneReadyEditor (TuneReadyProcessor&);
    ~TuneReadyEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateEnablement();

    TuneReadyProcessor& processor;
    rommuser::ui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    rommuser::ui::Knob speed, amount, humanize, fixedPitch, formant, mix, output;
    juce::ComboBox modeBox, keyBox, scaleBox, rangeBox;
    juce::Label modeLabel, keyLabel, scaleLabel, rangeLabel, notesLabel;
    juce::TextButton pitchClassButtons[12], midiButton, presetButtons[4];

    PitchDisplay display;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TuneReadyEditor)
};
