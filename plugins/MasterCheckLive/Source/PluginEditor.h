// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PluginProcessor.h"

#include "ui/RommuserLookAndFeel.h"

#include <juce_audio_processors/juce_audio_processors.h>

// 60 s history of the short-term loudness with the platform target drawn on top.
class HistoryDisplay : public juce::Component
{
public:
    explicit HistoryDisplay (MasterCheckLiveProcessor&);
    void paint (juce::Graphics&) override;

private:
    MasterCheckLiveProcessor& processor;
};

// One big number with a caption.
class Readout : public juce::Component, public juce::SettableTooltipClient
{
public:
    Readout (juce::String caption, juce::String unit, bool big = false);
    void set (float value, juce::Colour colour = {});
    void paint (juce::Graphics&) override;

private:
    juce::String caption, unit, text;
    juce::Colour colour;
    bool big;
};

class MasterCheckLiveEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit MasterCheckLiveEditor (MasterCheckLiveProcessor&);
    ~MasterCheckLiveEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    MasterCheckLiveProcessor& processor;
    rommuser::ui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    Readout integrated, shortTerm, momentary, lra, truePeak, plr, maxShortTerm;
    juce::ComboBox platformBox;
    juce::Label platformLabel, verdictLabel, detailLabel;
    juce::TextButton resetButton;
    HistoryDisplay history;

    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ComboAttachment> platformAttachment;

    float tpHold = -120.0f;
    int tpHoldFrames = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasterCheckLiveEditor)
};
