// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PluginProcessor.h"

#include "ui/RommuserLookAndFeel.h"
#include "ui/ScalableEditor.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include <map>
#include <memory>
#include <string>

// Oscilloscope of the last rendered samples plus voice count and cutoff readout.
class WaveScope : public juce::Component
{
public:
    void update (const WaveReadyProcessor& p);
    void paint (juce::Graphics&) override;

private:
    std::vector<float> samples;
    int voices = 0;
    float cutoffHz = 0.0f;
    float peak = 0.0f;
};

class WaveReadyEditor : public juce::Component, private juce::Timer
{
public:
    explicit WaveReadyEditor (WaveReadyProcessor&);
    ~WaveReadyEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    struct Combo
    {
        juce::ComboBox box;
        juce::Label label;
    };

    rommuser::ui::Knob& addKnob (const char* id, const juce::String& label, const juce::String& tip);
    Combo& addCombo (const char* id, const juce::String& label, const juce::StringArray& items, const juce::String& tip);
    juce::TextButton& addToggle (const char* id, const juce::String& text, const juce::String& tip);

    rommuser::ui::Knob& knob (const char* id) { return *knobs.at (id); }
    Combo& combo (const char* id) { return *combos.at (id); }
    juce::TextButton& toggle (const char* id) { return *toggles.at (id); }

    // Layout helpers (scaled pixel units).
    void layoutKnobRow (juce::Rectangle<int> area, std::initializer_list<const char*> ids);
    void layoutCombo (Combo& c, juce::Rectangle<int> area, int px16, int px26);

    WaveReadyProcessor& processor;
    rommuser::ui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    std::map<std::string, std::unique_ptr<rommuser::ui::Knob>> knobs;
    std::map<std::string, std::unique_ptr<Combo>> combos;
    std::map<std::string, std::unique_ptr<juce::TextButton>> toggles;

    juce::ComboBox presetBox;
    juce::Label presetLabel;
    juce::TextButton prevPreset, nextPreset;
    WaveScope scope;
    juce::MidiKeyboardComponent keyboard;

    static constexpr int numSections = 10;
    juce::Rectangle<int> sections[numSections];
    juce::String sectionNames[numSections];

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveReadyEditor)
};
