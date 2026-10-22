// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PluginProcessor.h"

#include "ui/RommuserLookAndFeel.h"

#include <juce_audio_processors/juce_audio_processors.h>

// The transfer curve of one band's saturation type at its drive, with a "how hard are we hitting it" light.
class CurveView : public juce::Component
{
public:
    void setState (rommuser::warmdrive::Saturation type, float driveDb, float hitDb, bool enabled);
    void paint (juce::Graphics&) override;

private:
    rommuser::warmdrive::Saturation type = rommuser::warmdrive::Saturation::tape;
    float driveDb = 0.0f, hitDb = 0.0f;
    bool enabled = true;
};

// One band: curve, type, drive, tone, mix.
struct BandColumn
{
    CurveView curve;
    juce::ComboBox typeBox;
    rommuser::ui::Knob drive { rommuser::ui::tr ("DRIVE", "DRIVE") },
                       tone { rommuser::ui::tr ("TONO", "TONE") },
                       mix { "MIX" };
    juce::Label title;
};

class WarmDriveEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit WarmDriveEditor (WarmDriveProcessor&);
    ~WarmDriveEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateEnablement();

    WarmDriveProcessor& processor;
    rommuser::ui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    juce::ComboBox bandsBox, oversamplingBox;
    juce::Label bandsLabel, oversamplingLabel;
    rommuser::ui::Knob xover[3] { rommuser::ui::Knob (rommuser::ui::tr ("CORTE 1", "SPLIT 1")),
                                  rommuser::ui::Knob (rommuser::ui::tr ("CORTE 2", "SPLIT 2")),
                                  rommuser::ui::Knob (rommuser::ui::tr ("CORTE 3", "SPLIT 3")) };
    rommuser::ui::Knob input { rommuser::ui::tr ("ENTRADA", "INPUT") }, output { rommuser::ui::tr ("SALIDA", "OUTPUT") }, mix { "MIX" };
    juce::TextButton autoGain, presetButtons[3];

    BandColumn bands[rommuser::warmdrive::maxBands];

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;

    float hitDb[rommuser::warmdrive::maxBands] = {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WarmDriveEditor)
};
