// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PluginProcessor.h"

#include "ui/RommuserLookAndFeel.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>

// Scrolling history of the gain reduction (down from the top) and the output peak.
class LimiterDisplay : public juce::Component
{
public:
    void push (float gainReductionDb, float outputPeakDb, float ceilingDb);
    void paint (juce::Graphics&) override;

private:
    static constexpr int historyLength = 180;   // 6 s at 30 fps
    std::array<float, historyLength> reductions {}, peaks {};
    int head = 0;
    float ceilingDb = -1.0f;
};

// Three LUFS bars (M, S, I) with the target line.
class LoudnessMeter : public juce::Component
{
public:
    void set (float momentary, float shortTerm, float integrated, float target);
    void paint (juce::Graphics&) override;

private:
    float m = -120.0f, s = -120.0f, i = -120.0f, target = -9.0f;
};

class PeakReadyEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PeakReadyEditor (PeakReadyProcessor&);
    ~PeakReadyEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    PeakReadyProcessor& processor;
    rommuser::ui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    rommuser::ui::Knob input, ceiling, release, target;
    juce::ComboBox styleBox;
    juce::Label styleLabel, grLabel, hint;
    juce::TextButton autoRelease, truePeak, resetLoudness, presetButtons[3];

    LimiterDisplay display;
    LoudnessMeter loudness;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;

    float grHold = 0.0f;
    int grHoldFrames = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PeakReadyEditor)
};
