// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginEditor.h"

using namespace rommuser::ui;
using namespace rommuser::lowmono;

namespace
{
constexpr int baseWidth = 860, baseHeight = 540;
} // namespace

//==============================================================================
void Goniometer::setSamples (const float* l, const float* r, int n)
{
    xs.resize ((size_t) n);
    ys.resize ((size_t) n);
    for (int i = 0; i < n; ++i)
    {
        // Rotate 45 degrees: mono (L = R) goes straight up, side goes sideways.
        xs[(size_t) i] = (l[i] - r[i]) * 0.7071f;
        ys[(size_t) i] = (l[i] + r[i]) * 0.7071f;
    }
    repaint();
}

void Goniometer::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r, 8.0f);
    auto area = r.reduced (12.0f);
    const float side = std::min (area.getWidth(), area.getHeight());
    auto sq = juce::Rectangle<float> (side, side).withCentre (area.getCentre());

    g.setColour (colours::outline);
    g.drawLine (sq.getCentreX(), sq.getY(), sq.getCentreX(), sq.getBottom(), 1.0f);
    g.drawLine (sq.getX(), sq.getCentreY(), sq.getRight(), sq.getCentreY(), 1.0f);
    g.drawLine (sq.getX(), sq.getBottom(), sq.getRight(), sq.getY(), 1.0f);
    g.drawLine (sq.getX(), sq.getY(), sq.getRight(), sq.getBottom(), 1.0f);
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("L", juce::Rectangle<float> (sq.getX(), sq.getY(), 20.0f, 14.0f), juce::Justification::centredLeft);
    g.drawText ("R", juce::Rectangle<float> (sq.getRight() - 20.0f, sq.getY(), 20.0f, 14.0f), juce::Justification::centredRight);
    g.drawText ("M", juce::Rectangle<float> (sq.getCentreX() - 10.0f, sq.getY(), 20.0f, 14.0f), juce::Justification::centred);
    g.drawText ("S", juce::Rectangle<float> (sq.getX(), sq.getCentreY() - 7.0f, 20.0f, 14.0f), juce::Justification::centredLeft);

    const float scale = sq.getWidth() * 0.45f;
    const size_t n = xs.size();
    for (size_t i = 0; i < n; ++i)
    {
        const float a = 0.15f + 0.85f * (float) i / (float) std::max<size_t> (1, n - 1);
        g.setColour (colours::accent.withAlpha (a * 0.8f));
        const float x = sq.getCentreX() + juce::jlimit (-1.0f, 1.0f, xs[i]) * scale;
        const float y = sq.getCentreY() - juce::jlimit (-1.0f, 1.0f, ys[i]) * scale;
        g.fillEllipse (x - 1.2f, y - 1.2f, 2.4f, 2.4f);
    }
}

//==============================================================================
void CorrelationMeter::setCorrelation (float c)
{
    value = c;
    if (c < worst || holdFrames++ > 60) { worst = c; holdFrames = 0; }
    warningFrames = c < 0.0f ? std::min (warningFrames + 1, 1000) : std::max (0, warningFrames - 2);
    repaint();
}

void CorrelationMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r, 8.0f);
    auto inner = r.reduced (14.0f, 10.0f);
    auto title = inner.removeFromTop (16.0f);
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (tr ("CORRELACIÓN", "CORRELATION"), title, juce::Justification::centredLeft);
    g.drawText (juce::String (value, 2), title, juce::Justification::centredRight);

    auto bar = inner.removeFromTop (18.0f);
    g.setColour (colours::outline);
    g.fillRoundedRectangle (bar, 4.0f);
    auto xFor = [&] (float c) { return bar.getX() + (c + 1.0f) * 0.5f * bar.getWidth(); };
    const float x0 = xFor (0.0f), xv = xFor (value);
    g.setColour (value >= 0.0f ? colours::good : colours::warn);
    g.fillRoundedRectangle (juce::Rectangle<float>::leftTopRightBottom (std::min (x0, xv), bar.getY() + 2.0f, std::max (x0, xv), bar.getBottom() - 2.0f), 3.0f);
    g.setColour (colours::text);
    g.fillRect (xFor (worst) - 1.0f, bar.getY(), 2.0f, bar.getHeight());

    auto labels = inner.removeFromTop (14.0f);
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("-1", labels, juce::Justification::centredLeft);
    g.drawText ("0", labels, juce::Justification::centred);
    g.drawText ("+1", labels, juce::Justification::centredRight);

    inner.removeFromTop (6.0f);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    if (isWarning())
    {
        g.setColour (colours::warn);
        g.drawFittedText (tr ("Ojo: problemas de fase. En mono se cancelará parte de la mezcla.",
                              "Careful: phase problems. Part of the mix will cancel in mono."), inner.toNearestInt(), juce::Justification::topLeft, 3);
    }
    else
    {
        g.setColour (value > 0.5f ? colours::good : colours::textDim);
        g.drawFittedText (value > 0.5f ? tr ("Compatible con mono.", "Mono compatible.")
                                       : tr ("Muy ancho: revisa en mono.", "Very wide: check in mono."), inner.toNearestInt(), juce::Justification::topLeft, 3);
    }
}

//==============================================================================
LowMonoEditor::LowMonoEditor (LowMonoProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      width (tr ("ANCHO", "WIDTH")),
      monoBelow (tr ("MONO DEBAJO DE", "MONO BELOW")),
      lowWidth (tr ("ANCHO GRAVES", "LOW WIDTH"))
{
    setLookAndFeel (&lnf);

    struct KnobInfo { Knob* knob; const char* id; juce::String tip; };
    const KnobInfo knobs[] = {
        { &width, ParamIDs::width, tr ("Ancho estéreo: 100 % = sin cambios, 0 % = mono, 200 % = el doble de lados.", "Stereo width: 100% = unchanged, 0% = mono, 200% = twice the sides.") },
        { &monoBelow, ParamIDs::monoBelow, tr ("Los graves por debajo de esta frecuencia pasan a mono (crossover Linkwitz-Riley). Off = sin crossover.",
                                               "Lows below this frequency become mono (Linkwitz-Riley crossover). Off = no crossover.") },
        { &lowWidth, ParamIDs::lowWidth, tr ("Cuánto estéreo dejar en los graves. 0 % = mono total.", "How much stereo to leave in the lows. 0% = fully mono.") },
    };
    for (const auto& k : knobs)
    {
        addAndMakeVisible (*k.knob);
        k.knob->slider.setTooltip (k.tip);
        sliderAttachments.push_back (std::make_unique<SliderAttachment> (processor.state, k.id, k.knob->slider));
        if (auto* param = processor.state.getParameter (k.id))
            k.knob->slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    }

    steep.setButtonText ("48 dB/oct");
    steep.setClickingTogglesState (true);
    steep.setTooltip (tr ("Crossover más abrupto (48 en vez de 24 dB/oct): separa mejor, gira más la fase.",
                          "Steeper crossover (48 instead of 24 dB/oct): separates better, shifts phase more."));
    addAndMakeVisible (steep);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (processor.state, ParamIDs::steep, steep));

    for (int i = 0; i < 4; ++i)
    {
        auto& b = presetButtons[i];
        b.setButtonText (tr (monoPresets[i].nameEs, monoPresets[i].nameEn));
        b.setTooltip (tr ("Punto de partida.", "Starting point."));
        b.onClick = [this, i] { processor.applyPreset (monoPresets[i]); };
        addAndMakeVisible (b);
    }

    hint.setText (tr ("Graves en mono = kick y bajo firmes en cualquier parlante. Vigila la correlación: por debajo de 0 hay cancelación en mono.",
                      "Mono lows = kick and bass solid on any speaker. Watch the correlation: below 0 the mix cancels in mono."), juce::dontSendNotification);
    hint.setColour (juce::Label::textColourId, colours::textDim);
    hint.setFont (juce::FontOptions (12.0f));
    hint.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (hint);

    addAndMakeVisible (scope);
    addAndMakeVisible (meter);

    setResizable (true, true);
    setResizeLimits (baseWidth * 3 / 4, baseHeight * 3 / 4, baseWidth * 2, baseHeight * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) baseWidth / baseHeight);
    setSize (baseWidth, baseHeight);

    startTimerHz (30);
}

LowMonoEditor::~LowMonoEditor()
{
    setLookAndFeel (nullptr);
}

void LowMonoEditor::paint (juce::Graphics& g)
{
    g.fillAll (colours::background);

    const float s = (float) getWidth() / baseWidth;
    auto header = getLocalBounds().removeFromTop (juce::roundToInt (48 * s)).reduced (juce::roundToInt (20 * s), 0);

    g.setColour (colours::text);
    g.setFont (juce::FontOptions (24.0f * s, juce::Font::bold));
    g.drawText ("LOW MONO", header, juce::Justification::centredLeft);

    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (12.0f * s));
    g.drawText (juce::String::fromUTF8 ("ROMMUSER  ·  v") + PLUGIN_VERSION_STRING + juce::String::fromUTF8 ("  ·  ") + tr ("gratis y open source", "free & open source"),
                header, juce::Justification::centredRight);

    g.setColour (colours::outline);
    g.drawHorizontalLine (header.getBottom(), 0.0f, (float) getWidth());
}

void LowMonoEditor::resized()
{
    const float s = (float) getWidth() / baseWidth;
    auto px = [s] (int v) { return juce::roundToInt ((float) v * s); };

    auto r = getLocalBounds();
    r.removeFromTop (px (48));
    r.reduce (px (16), px (12));

    auto bottom = r.removeFromBottom (px (62));
    r.removeFromBottom (px (8));
    auto row = bottom.removeFromTop (px (44));
    const int cw = row.getWidth() / 5;
    steep.setBounds (row.removeFromLeft (cw).reduced (px (4), px (10)));
    for (auto& b : presetButtons)
        b.setBounds (row.removeFromLeft (cw).reduced (px (4), px (10)));

    // Left: goniometer (square). Right: correlation meter on top, knobs below, hint at the bottom.
    auto left = r.removeFromLeft (r.getHeight());
    scope.setBounds (left);
    r.removeFromLeft (px (12));
    meter.setBounds (r.removeFromTop (px (120)));
    r.removeFromTop (px (8));
    hint.setBounds (r.removeFromBottom (px (40)));
    const int kw = r.getWidth() / 3;
    width.setBounds (r.removeFromLeft (kw));
    monoBelow.setBounds (r.removeFromLeft (kw));
    lowWidth.setBounds (r);
}

void LowMonoEditor::timerCallback()
{
    float l[LowMonoProcessor::scopePoints], rr[LowMonoProcessor::scopePoints];
    for (int i = 0; i < LowMonoProcessor::scopePoints; ++i)
    {
        l[i] = processor.scopeL[(size_t) i].load();
        rr[i] = processor.scopeR[(size_t) i].load();
    }
    scope.setSamples (l, rr, LowMonoProcessor::scopePoints);
    meter.setCorrelation (processor.correlation.load());
    const bool crossoverOn = processor.state.getRawParameterValue (ParamIDs::monoBelow)->load() >= 20.0f;
    lowWidth.setEnabled (crossoverOn);
    lowWidth.setAlpha (crossoverOn ? 1.0f : 0.4f);
    steep.setEnabled (crossoverOn);
    steep.setAlpha (crossoverOn ? 1.0f : 0.4f);
}
