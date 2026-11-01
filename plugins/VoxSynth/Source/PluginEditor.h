// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PluginProcessor.h"

#include "ui/RommuserLookAndFeel.h"
#include "ui/ScalableEditor.h"

#include <juce_audio_processors/juce_audio_processors.h>

// Harmony / Robot: piano-roll with the detected pitch (dim) and each generated voice (coloured).
// Vocoder: the band envelopes as bars.
class VoxDisplay : public juce::Component
{
public:
    explicit VoxDisplay (VoxSynthProcessor&);
    void paint (juce::Graphics&) override;

private:
    void paintPitch (juce::Graphics&, juce::Rectangle<float> plot, juce::Rectangle<float> header, bool robot);
    void paintBands (juce::Graphics&, juce::Rectangle<float> plot, juce::Rectangle<float> header);

    VoxSynthProcessor& processor;
};

class VoxSynthEditor : public juce::Component, private juce::Timer
{
public:
    explicit VoxSynthEditor (VoxSynthProcessor&);
    ~VoxSynthEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateVisibility();

    VoxSynthProcessor& processor;
    rommuser::ui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    // Common
    juce::ComboBox modeBox, rangeBox, keyBox, scaleBox, intervalBox;
    juce::Label modeLabel, rangeLabel, keyLabel, scaleLabel, intervalLabel;
    rommuser::ui::Knob glide, dry, mix, output;

    // Harmony voices
    struct VoiceControls
    {
        juce::TextButton on;
        juce::ComboBox degree;
        rommuser::ui::Knob semis, detune, pan, level, formant;
        VoiceControls() : semis ("ST"), detune ("DETUNE"), pan ("PAN"), level (rommuser::ui::tr ("NIVEL", "LEVEL")), formant (rommuser::ui::tr ("FORMANTE", "FORMANT")) {}
    };
    VoiceControls voices[rommuser::voxsynth::maxVoices];

    // Vocoder
    juce::ComboBox carrierBox, carrierNoteBox;
    juce::Label carrierLabel, carrierNoteLabel;
    juce::TextButton followButton;
    rommuser::ui::Knob bands, attack, release, sibilance, carrierDetune;

    // Robot
    juce::ComboBox robotNoteBox;
    juce::Label robotNoteLabel;
    rommuser::ui::Knob robotFormant;

    juce::TextButton presetButtons[6];
    VoxDisplay display;

    juce::Rectangle<int> panelArea;
    int currentMode = -1, currentInterval = -1;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxSynthEditor)
};
