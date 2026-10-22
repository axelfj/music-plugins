// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PluginProcessor.h"

#include "ui/RommuserLookAndFeel.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>

// Scrolling history of the detector level with the threshold and the close level drawn on top, and
// the gain applied underneath. Drag the threshold line to move it.
class LevelScope : public juce::Component
{
public:
    explicit LevelScope (GateReadyProcessor&);

    void push (float levelDb, float gainReductionDb, bool open);
    void setParams (const rommuser::gateready::GateParams& p);
    void paint (juce::Graphics&) override;

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    float yFor (float db, juce::Rectangle<float> r) const;

    GateReadyProcessor& processor;
    rommuser::gateready::GateParams params;
    static constexpr int historyLength = 180;   // 6 s at 30 fps
    std::array<float, historyLength> levels {}, reductions {};
    std::array<bool, historyLength> opens {};
    int head = 0;
    juce::RangedAudioParameter* thresholdParam = nullptr;
    float dragStartDb = 0.0f;
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

class GateReadyEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit GateReadyEditor (GateReadyProcessor&);
    ~GateReadyEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    GateReadyProcessor& processor;
    rommuser::ui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    rommuser::ui::Knob threshold, ratio, attack, hold, release, range, knee, hysteresis;
    juce::ComboBox lookaheadBox, scHpfBox, scLpfBox;
    juce::Label lookaheadLabel, scHpfLabel, scLpfLabel;
    juce::TextButton external, listen, presetButtons[3];

    LevelScope scope;
    ReductionMeter grMeter;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GateReadyEditor)
};
