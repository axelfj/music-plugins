// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PluginProcessor.h"

#include "ui/RommuserLookAndFeel.h"

#include <juce_audio_processors/juce_audio_processors.h>

// One band's gain meter: a bar that grows up from the centre (boost) or down (reduction).
class BandGainMeter : public juce::Component
{
public:
    explicit BandGainMeter (juce::String labelText) : label (std::move (labelText)) {}
    void push (float gainDb);
    void paint (juce::Graphics&) override;

private:
    juce::String label;
    float valueDb = 0.0f;
};

// The three bands side by side with the crossover frequencies in between.
class BandStrip : public juce::Component
{
public:
    explicit BandStrip (BandPushProcessor&);

    void setState (const rommuser::bandpush::PushParams& p);
    void paint (juce::Graphics&) override;
    void resized() override;

    BandGainMeter meters[rommuser::bandpush::numBands] {
        BandGainMeter (rommuser::ui::tr ("GRAVES", "LOW")),
        BandGainMeter (rommuser::ui::tr ("MEDIOS", "MID")),
        BandGainMeter (rommuser::ui::tr ("AGUDOS", "HIGH")) };

private:
    BandPushProcessor& processor;
    rommuser::bandpush::PushParams params;
};

class BandPushEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit BandPushEditor (BandPushProcessor&);
    ~BandPushEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    BandPushProcessor& processor;
    rommuser::ui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    rommuser::ui::Knob depth, time, threshold, input, output, mix, lowXover, highXover;
    std::vector<std::unique_ptr<rommuser::ui::Knob>> bandKnobs;  // up, down, gain per band

    BandStrip strip;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BandPushEditor)
};
