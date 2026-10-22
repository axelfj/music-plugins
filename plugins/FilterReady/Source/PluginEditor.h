// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PluginProcessor.h"

#include "ui/RommuserLookAndFeel.h"

#include <juce_audio_processors/juce_audio_processors.h>

// Live frequency response of the two filters (combined as routed) with the modulated cutoffs marked.
// Dragging in the display moves the cutoff (x) and resonance (y) of the nearest filter.
class ResponseDisplay : public juce::Component
{
public:
    explicit ResponseDisplay (FilterReadyProcessor&);

    void setState (const rommuser::filterready::FilterParams& p, float liveCutoffA, float liveCutoffB, float lfo, float env);
    void paint (juce::Graphics&) override;

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> plotArea() const;
    float xFor (double hz, const juce::Rectangle<float>& plot) const;
    double hzFor (float x, const juce::Rectangle<float>& plot) const;
    float responseDb (double hz) const;

    FilterReadyProcessor& processor;
    rommuser::filterready::FilterParams params;
    float cutoffA = 1000.0f, cutoffB = 150.0f, lfo = 0.0f, env = 0.0f;
    int dragging = -1;   // 0 = A, 1 = B
    juce::RangedAudioParameter* dragCutoff = nullptr;
    juce::RangedAudioParameter* dragRes = nullptr;
};

class FilterReadyEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit FilterReadyEditor (FilterReadyProcessor&);
    ~FilterReadyEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateEnablement();

    FilterReadyProcessor& processor;
    rommuser::ui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    struct StageControls
    {
        juce::Label title;
        juce::TextButton on;
        juce::ComboBox type, slope;
        rommuser::ui::Knob cutoff, res, lfoAmount, envAmount;
        StageControls (const juce::String& cut, const juce::String& r, const juce::String& l, const juce::String& e)
            : cutoff (cut), res (r), lfoAmount (l), envAmount (e) {}
    };
    StageControls stageA, stageB;

    juce::ComboBox routingBox, characterBox, divisionBox, shapeBox;
    juce::Label routingLabel, characterLabel, divisionLabel, shapeLabel;
    rommuser::ui::Knob drive, mix, output, lfoRate, envAttack, envRelease, envSens;
    juce::TextButton sync, presetButtons[4];

    ResponseDisplay display;
    int sectionTops[3] {};

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterReadyEditor)
};
