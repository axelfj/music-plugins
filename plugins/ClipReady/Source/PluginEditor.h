// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PluginProcessor.h"

#include "ui/RommuserLookAndFeel.h"
#include "ui/ScalableEditor.h"

#include <juce_audio_processors/juce_audio_processors.h>

// Shows the clipping curve for the current settings: straight line, knee, ceiling.
class TransferCurve : public juce::Component
{
public:
    void setState (float softnessNorm, float ceilingDb, float inputDb)
    {
        if (juce::exactlyEqual (softnessNorm, softness) && juce::exactlyEqual (ceilingDb, ceiling) && juce::exactlyEqual (inputDb, input))
            return;
        softness = softnessNorm;
        ceiling = ceilingDb;
        input = inputDb;
        repaint();
    }

    void paint (juce::Graphics&) override;

private:
    float softness = 0.35f, ceiling = -0.3f, input = 0.0f;
};

// Vertical peak meter with peak-hold, in dBFS.
class PeakMeter : public juce::Component
{
public:
    PeakMeter (juce::String labelText, juce::Colour barColour) : label (std::move (labelText)), colour (barColour) {}

    void push (float linearPeak, float ceilingDb);
    void paint (juce::Graphics&) override;

private:
    juce::String label;
    juce::Colour colour;
    float levelDb = -60.0f, holdDb = -60.0f, ceilingLineDb = 0.0f;
    int holdFrames = 0;
};

// How many dB are being clipped right now (0 .. 12 dB scale).
class ClipMeter : public juce::Component
{
public:
    void push (float db);
    void paint (juce::Graphics&) override;

private:
    float valueDb = 0.0f;
};

class ClipReadyEditor : public juce::Component, private juce::Timer
{
public:
    explicit ClipReadyEditor (ClipReadyProcessor&);
    ~ClipReadyEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    ClipReadyProcessor& processor;
    rommuser::ui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    rommuser::ui::Knob input, ceiling, softness, output, mix;
    juce::ComboBox oversampling;
    juce::TextButton link, delta, strict, hq;
    juce::Label oversamplingLabel;

    TransferCurve curve;
    PeakMeter inMeter, outMeter;
    ClipMeter clipMeter;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::unique_ptr<ComboAttachment> oversamplingAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipReadyEditor)
};
