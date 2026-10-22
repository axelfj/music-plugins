// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PluginProcessor.h"

#include "ui/RommuserLookAndFeel.h"

#include <juce_audio_processors/juce_audio_processors.h>

// Waveform of the kick as it will sound with the current settings, with the pitch envelope on top.
class KickDisplay : public juce::Component
{
public:
    void setWaveform (std::vector<float> samples, double sampleRate, const rommuser::kickready::KickParams& params, int note);
    void setSounding (bool);
    void paint (juce::Graphics&) override;

private:
    std::vector<float> wave;
    double sr = 48000.0;
    rommuser::kickready::KickParams params;
    int note = 36;
    bool sounding = false;
};

class KickReadyEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit KickReadyEditor (KickReadyProcessor&);
    ~KickReadyEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshPreview (bool force);
    void exportWav();
    void updateEnablement();

    KickReadyProcessor& processor;
    rommuser::ui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    rommuser::ui::Knob tune, glide, velocity, pitchStart, pitchDecay, pitchCurve, attack, hold, decay, ampCurve,
                       clickLevel, clickDecay, clickTone, drive, clip, sub, highCut, output;
    juce::ComboBox noteBox, clickModeBox;
    juce::Label noteLabel, clickModeLabel;
    juce::TextButton follow, play, exportButton, presetButtons[4];

    KickDisplay display;
    juce::Rectangle<int> sections[5];
    juce::String sectionNames[5];

    std::unique_ptr<juce::FileChooser> chooser;
    juce::String lastParamsKey;
    int lastRenderNote = -1;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KickReadyEditor)
};
