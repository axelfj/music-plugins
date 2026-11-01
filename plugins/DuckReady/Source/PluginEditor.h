// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PluginProcessor.h"

#include "ui/RommuserLookAndFeel.h"
#include "ui/ScalableEditor.h"

#include <juce_audio_processors/juce_audio_processors.h>

// The volume curve over one cycle, with a playhead that follows the audio thread.
// Drag vertically to change the depth, horizontally to change the release.
class CurveDisplay : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit CurveDisplay (DuckReadyProcessor&);

    void setState (const rommuser::duckready::DuckParams& p, float phase, float gain, bool playing);
    void paint (juce::Graphics&) override;

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    DuckReadyProcessor& processor;
    rommuser::duckready::DuckParams params;
    float livePhase = 0.0f, liveGain = 1.0f;
    bool playing = false;

    float dragStartDepth = 0.0f, dragStartRelease = 0.0f;
    juce::RangedAudioParameter* depthParam = nullptr;
    juce::RangedAudioParameter* releaseParam = nullptr;
};

class DuckReadyEditor : public juce::Component, private juce::Timer
{
public:
    explicit DuckReadyEditor (DuckReadyProcessor&);
    ~DuckReadyEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateEnablement();

    DuckReadyProcessor& processor;
    rommuser::ui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    rommuser::ui::Knob depth, attack, hold, release, curve, offset, scThreshold, crossover;
    juce::ComboBox rateBox, triggerBox, bandBox;
    juce::Label rateLabel, triggerLabel, bandLabel;
    juce::TextButton presetButtons[3];

    CurveDisplay display;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DuckReadyEditor)
};
