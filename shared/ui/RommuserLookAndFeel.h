// Shared look for every ROMMUSER plugin: dark background, one accent colour, big readable knobs.
// Colours come from the current theme (Theme.h), which also holds the tiny bilingual text helper.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "Theme.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace rommuser::ui
{

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel() { applyTheme(); }

    // Re-reads the theme colours; called again whenever the theme changes.
    void applyTheme()
    {
        // The V4 scheme colours popups, scrollbars and dialogs; the ids below then override it.
        setColourScheme ({ colours::background, colours::panel, colours::panel,
                           colours::outline, colours::text, colours::panel,
                           colours::text, colours::accent, colours::text });

        setColour (juce::ResizableWindow::backgroundColourId, colours::background);
        setColour (juce::Slider::textBoxTextColourId, colours::text);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::rotarySliderFillColourId, colours::accent);
        setColour (juce::Label::textColourId, colours::text);
        setColour (juce::ComboBox::backgroundColourId, colours::panel);
        setColour (juce::ComboBox::outlineColourId, colours::outline);
        setColour (juce::ComboBox::textColourId, colours::text);
        setColour (juce::ComboBox::arrowColourId, colours::textDim);
        setColour (juce::PopupMenu::backgroundColourId, colours::panel);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, colours::accent.withAlpha (0.3f));
        setColour (juce::TextButton::buttonColourId, colours::panel);
        setColour (juce::TextButton::buttonOnColourId, colours::accent);
        setColour (juce::TextButton::textColourOffId, colours::textDim);
        setColour (juce::TextButton::textColourOnId, colours::background);
        setColour (juce::TooltipWindow::backgroundColourId, colours::panel.brighter (0.1f));
        setColour (juce::TooltipWindow::textColourId, colours::text);
        setColour (juce::TooltipWindow::outlineColourId, colours::outline);
        setColour (juce::PopupMenu::textColourId, colours::text);
        setColour (juce::PopupMenu::highlightedTextColourId, colours::text);
        setColour (juce::PopupMenu::headerTextColourId, colours::textDim);
        setColour (juce::TextEditor::backgroundColourId, colours::background);
        setColour (juce::TextEditor::textColourId, colours::text);
        setColour (juce::TextEditor::outlineColourId, colours::outline);
        setColour (juce::TextEditor::focusedOutlineColourId, colours::accent);
        setColour (juce::TextEditor::highlightColourId, colours::accent.withAlpha (0.35f));
        setColour (juce::CaretComponent::caretColourId, colours::text);
        setColour (juce::ToggleButton::textColourId, colours::text);
        setColour (juce::ToggleButton::tickColourId, colours::accent);
        setColour (juce::ToggleButton::tickDisabledColourId, colours::textDim);
        setColour (juce::ColourSelector::backgroundColourId, colours::panel);
        setColour (juce::ColourSelector::labelTextColourId, colours::textDim);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float pos,
                           float startAngle, float endAngle, juce::Slider& slider) override
    {
        const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (6.0f);
        const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
        const auto centre = bounds.getCentre();
        const auto lineW = juce::jmax (3.0f, radius * 0.12f);
        const auto arcR = radius - lineW * 0.5f;
        const auto angle = startAngle + pos * (endAngle - startAngle);

        juce::Path track;
        track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
        g.setColour (colours::outline);
        g.strokePath (track, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (slider.isEnabled())
        {
            juce::Path value;
            value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, angle, true);
            g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId));
            g.strokePath (value, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        const auto knobR = arcR - lineW * 1.6f;
        g.setColour (colours::panel.brighter (0.08f));
        g.fillEllipse (juce::Rectangle<float> (knobR * 2.0f, knobR * 2.0f).withCentre (centre));

        juce::Path pointer;
        pointer.addRoundedRectangle (-lineW * 0.35f, -knobR + 4.0f, lineW * 0.7f, knobR * 0.45f, lineW * 0.35f);
        g.setColour (colours::text);
        g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.0f);
        const bool on = b.getToggleState();
        auto c = on ? colours::accent : colours::panel;
        if (over || down)
            c = c.brighter (0.08f);
        g.setColour (c);
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (on ? c : colours::outline);
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
    }
};

// A labelled rotary knob with the value underneath.
class Knob : public juce::Component
{
public:
    explicit Knob (const juce::String& labelText)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
        addAndMakeVisible (slider);

        label.setText (labelText, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setColour (juce::Label::textColourId, colours::textDim);
        label.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        addAndMakeVisible (label);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        label.setBounds (r.removeFromTop (18));
        slider.setBounds (r);
    }

    juce::Slider slider;
    juce::Label label;
};

// Space kept free at the right end of the header for the size / theme menu (ScalableEditor.h).
constexpr int headerMenuWidth = 34;

// The header every plugin shares: name on the left, "R - V1 - FREE" on the right, a rule below.
// `s` is the editor's scale against its base size (1 since editors are laid out at base size).
inline void drawHeader (juce::Graphics& g, juce::Rectangle<int> area, float s)
{
    const int width = area.getWidth();
    auto header = area.reduced (juce::roundToInt (20 * s), 0);

    g.setColour (colours::text);
    g.setFont (juce::FontOptions (24.0f * s, juce::Font::bold));
    g.drawText (juce::String (JucePlugin_Name).toUpperCase(), header, juce::Justification::centredLeft);

    header.removeFromRight (juce::roundToInt (headerMenuWidth * s));
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (12.0f * s));
    g.drawText ("R - V" + juce::String (PLUGIN_VERSION_STRING).upToFirstOccurrenceOf (".", false, false) + " - FREE",
                header, juce::Justification::centredRight);

    g.setColour (colours::outline);
    g.drawHorizontalLine (header.getBottom(), (float) area.getX(), (float) (area.getX() + width));
}

} // namespace rommuser::ui
