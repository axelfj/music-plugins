// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PluginProcessor.h"

#include "ui/RommuserLookAndFeel.h"

#include <juce_audio_processors/juce_audio_processors.h>

// Goniometer (Lissajous, rotated 45 degrees so mono is vertical) with a fading trail.
class Goniometer : public juce::Component
{
public:
    void setSamples (const float* l, const float* r, int n);
    void paint (juce::Graphics&) override;

private:
    std::vector<float> xs, ys;
};

// Correlation bar from -1 to +1 with a hold on the worst recent value and a mono warning.
class CorrelationMeter : public juce::Component
{
public:
    void setCorrelation (float c);
    void paint (juce::Graphics&) override;
    bool isWarning() const { return warningFrames > 15; }

private:
    float value = 1.0f, worst = 1.0f;
    int holdFrames = 0, warningFrames = 0;
};

class LowMonoEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit LowMonoEditor (LowMonoProcessor&);
    ~LowMonoEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    LowMonoProcessor& processor;
    rommuser::ui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    rommuser::ui::Knob width, monoBelow, lowWidth;
    juce::TextButton steep, presetButtons[4];
    juce::Label hint;

    Goniometer scope;
    CorrelationMeter meter;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LowMonoEditor)
};
