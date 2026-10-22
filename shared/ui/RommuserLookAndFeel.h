// Shared look for every ROMMUSER plugin: dark background, one accent colour, big readable knobs.
// Also holds the tiny bilingual text helper (Spanish when the OS language is Spanish, else English).
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace rommuser::ui
{

namespace colours
{
    inline const juce::Colour background { 0xff15161a };
    inline const juce::Colour panel      { 0xff1f2127 };
    inline const juce::Colour outline    { 0xff2e3139 };
    inline const juce::Colour text       { 0xffe8e9ee };
    inline const juce::Colour textDim    { 0xff8a8f9c };
    inline const juce::Colour accent     { 0xfff2a23a }; // warm orange
    inline const juce::Colour good       { 0xff58c98b };
    inline const juce::Colour warn       { 0xffe5534b };
}

inline bool isSpanish()
{
    static const bool spanish = juce::SystemStats::getUserLanguage().startsWithIgnoreCase ("es")
                             || juce::SystemStats::getDisplayLanguage().startsWithIgnoreCase ("es");
    return spanish;
}

// tr ("Entrada", "Input")
inline juce::String tr (const char* spanish, const char* english)
{
    return juce::String::fromUTF8 (isSpanish() ? spanish : english);
}

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel()
    {
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

} // namespace rommuser::ui
