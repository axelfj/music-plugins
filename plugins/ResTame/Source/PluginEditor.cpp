// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginEditor.h"

using namespace rommuser::ui;
using namespace rommuser::restame;

namespace
{
constexpr int baseWidth = 960, baseHeight = 580;
} // namespace

//==============================================================================
void ReductionDisplay::setState (const float* reduction, int points, float lo, float hi, float depth, bool d, bool ext)
{
    values.assign (reduction, reduction + points);
    lowHz = lo; highHz = hi; depthDb = depth; delta = d; external = ext;
    repaint();
}

void ReductionDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r, 8.0f);
    auto plot = r.reduced (14.0f, 12.0f);
    auto title = plot.removeFromTop (14.0f);
    plot.removeFromBottom (16.0f);

    const float maxDb = std::max (6.0f, std::ceil (depthDb / 6.0f) * 6.0f);
    auto xFor = [&] (double f) { return plot.getX() + (float) (std::log (f / 20.0) / std::log (1000.0)) * plot.getWidth(); };
    auto yFor = [&] (float red) { return plot.getY() + juce::jlimit (0.0f, 1.0f, red / maxDb) * plot.getHeight(); };

    // Active range shading.
    g.setColour (colours::accent.withAlpha (0.06f));
    g.fillRect (juce::Rectangle<float> (xFor (lowHz), plot.getY(), xFor (highHz) - xFor (lowHz), plot.getHeight()));

    g.setColour (colours::outline);
    for (double f : { 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0 })
        g.drawVerticalLine (juce::roundToInt (xFor (f)), plot.getY(), plot.getBottom());
    for (float db = 6.0f; db < maxDb + 0.1f; db += 6.0f)
        g.drawHorizontalLine (juce::roundToInt (yFor (db)), plot.getX(), plot.getRight());
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (10.0f));
    for (double f : { 100.0, 1000.0, 10000.0 })
        g.drawText (f >= 1000.0 ? juce::String (f / 1000.0, 0) + "k" : juce::String (f, 0),
                    juce::Rectangle<float> (xFor (f) - 20.0f, plot.getBottom() + 2.0f, 40.0f, 12.0f), juce::Justification::centred);
    for (float db = 6.0f; db < maxDb + 0.1f; db += 6.0f)
        g.drawText ("-" + juce::String (db, 0), juce::Rectangle<float> (plot.getX() + 2.0f, yFor (db) - 12.0f, 30.0f, 12.0f), juce::Justification::centredLeft);

    if (values.size() > 1)
    {
        juce::Path curve;
        curve.startNewSubPath (plot.getX(), plot.getY());
        for (size_t i = 0; i < values.size(); ++i)
        {
            const double f = 20.0 * std::pow (1000.0, (double) i / (double) (values.size() - 1));
            curve.lineTo (xFor (f), yFor (values[i]));
        }
        curve.lineTo (plot.getRight(), plot.getY());
        curve.closeSubPath();
        g.setColour (colours::accent.withAlpha (0.35f));
        g.fillPath (curve);
        g.setColour (colours::accent);
        g.strokePath (curve, juce::PathStrokeType (1.5f));
    }

    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (11.0f));
    juce::String label = tr ("reducción por frecuencia", "reduction per frequency");
    if (external) label << "  ·  " << tr ("detector: sidechain externo", "detector: external sidechain");
    if (delta) label << "  ·  DELTA";
    g.drawText (label, title, juce::Justification::topLeft);
}

//==============================================================================
ResTameEditor::ResTameEditor (ResTameProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      depth (tr ("PROFUNDIDAD", "DEPTH")),
      sharpness (tr ("FINURA", "SHARPNESS")),
      selectivity (tr ("SELECTIVIDAD", "SELECTIVITY")),
      threshold (tr ("UMBRAL", "THRESHOLD")),
      attack (tr ("ATAQUE", "ATTACK")),
      release ("RELEASE"),
      low (tr ("DESDE", "LOW")),
      high (tr ("HASTA", "HIGH")),
      smoothed ((size_t) ResTameProcessor::displayPoints, 0.0f)
{
    setLookAndFeel (&lnf);

    struct KnobInfo { Knob* knob; const char* id; juce::String tip; };
    const KnobInfo knobs[] = {
        { &depth, ParamIDs::depth, tr ("Cuántos dB como máximo se quitan a cada resonancia.", "How many dB at most are taken from each resonance.") },
        { &sharpness, ParamIDs::sharpness, tr ("Qué tan estrechos quedan los cortes. Alto = cortes finos, bajo = cortes más anchos y suaves.",
                                               "How narrow the cuts stay. High = fine cuts, low = wider, gentler cuts.") },
        { &selectivity, ParamIDs::selectivity, tr ("Qué cuenta como resonancia: alto = solo picos muy estrechos; bajo = también bultos anchos del espectro.",
                                                   "What counts as a resonance: high = only very narrow peaks; low = broader bumps too.") },
        { &threshold, ParamIDs::threshold, tr ("Cuánto debe sobresalir un pico sobre su entorno para tocarlo.", "How far a peak must stick out above its surroundings before it is touched.") },
        { &attack, ParamIDs::attack, tr ("Qué tan rápido entra la reducción cuando aparece la resonancia.", "How fast the reduction kicks in when the resonance appears.") },
        { &release, ParamIDs::release, tr ("Qué tan rápido vuelve cuando la resonancia desaparece.", "How fast it lets go when the resonance disappears.") },
        { &low, ParamIDs::low, tr ("Límite inferior del rango que se procesa.", "Lower limit of the processed range.") },
        { &high, ParamIDs::high, tr ("Límite superior del rango que se procesa.", "Upper limit of the processed range.") },
    };
    for (const auto& k : knobs)
    {
        addAndMakeVisible (*k.knob);
        k.knob->slider.setTooltip (k.tip);
        sliderAttachments.push_back (std::make_unique<SliderAttachment> (processor.state, k.id, k.knob->slider));
        if (auto* param = processor.state.getParameter (k.id))
            k.knob->slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    }

    placementBox.addItemList (Choices::placements, 1);
    placementBox.setTooltip (tr ("Stereo: ambos canales con la misma reducción. Mid: solo el centro. Side: solo los lados.",
                                 "Stereo: both channels with the same reduction. Mid: centre only. Side: sides only."));
    addAndMakeVisible (placementBox);
    comboAttachments.push_back (std::make_unique<ComboAttachment> (processor.state, ParamIDs::placement, placementBox));
    placementLabel.setText (tr ("CANAL", "PLACEMENT"), juce::dontSendNotification);
    placementLabel.setJustificationType (juce::Justification::centred);
    placementLabel.setColour (juce::Label::textColourId, colours::textDim);
    placementLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    addAndMakeVisible (placementLabel);

    struct ToggleInfo { juce::TextButton* button; const char* id; juce::String text, tip; };
    const ToggleInfo toggles[] = {
        { &external, ParamIDs::external, "SIDECHAIN", tr ("Unmask: el detector escucha la otra pista y corta aquí las frecuencias donde ella suena fuerte.",
                                                          "Unmask: the detector listens to the other track and cuts here where it is loud.") },
        { &delta, ParamIDs::delta, "DELTA", tr ("Escucha solo lo que se está quitando.", "Listen to what is being removed.") },
    };
    for (const auto& t : toggles)
    {
        t.button->setButtonText (t.text);
        t.button->setClickingTogglesState (true);
        t.button->setTooltip (t.tip);
        addAndMakeVisible (*t.button);
        buttonAttachments.push_back (std::make_unique<ButtonAttachment> (processor.state, t.id, *t.button));
    }

    for (int i = 0; i < 4; ++i)
    {
        auto& b = presetButtons[i];
        b.setButtonText (tr (tamePresets[i].nameEs, tamePresets[i].nameEn));
        b.setTooltip (tr ("Punto de partida.", "Starting point."));
        b.onClick = [this, i] { processor.applyPreset (tamePresets[i]); };
        addAndMakeVisible (b);
    }

    addAndMakeVisible (display);

    setResizable (true, true);
    setResizeLimits (baseWidth * 3 / 4, baseHeight * 3 / 4, baseWidth * 2, baseHeight * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) baseWidth / baseHeight);
    setSize (baseWidth, baseHeight);

    startTimerHz (30);
}

ResTameEditor::~ResTameEditor()
{
    setLookAndFeel (nullptr);
}

void ResTameEditor::paint (juce::Graphics& g)
{
    g.fillAll (colours::background);

    const float s = (float) getWidth() / baseWidth;
    auto header = getLocalBounds().removeFromTop (juce::roundToInt (48 * s)).reduced (juce::roundToInt (20 * s), 0);

    g.setColour (colours::text);
    g.setFont (juce::FontOptions (24.0f * s, juce::Font::bold));
    g.drawText ("RES TAME", header, juce::Justification::centredLeft);

    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (12.0f * s));
    g.drawText (juce::String ("ROMMUSER  ·  v") + PLUGIN_VERSION_STRING + "  ·  " + tr ("gratis y open source", "free & open source"),
                header, juce::Justification::centredRight);

    g.setColour (colours::outline);
    g.drawHorizontalLine (header.getBottom(), 0.0f, (float) getWidth());
}

void ResTameEditor::resized()
{
    const float s = (float) getWidth() / baseWidth;
    auto px = [s] (int v) { return juce::roundToInt ((float) v * s); };

    auto r = getLocalBounds();
    r.removeFromTop (px (48));
    r.reduce (px (16), px (12));

    auto bottom = r.removeFromBottom (px (62));
    r.removeFromBottom (px (8));
    auto row = bottom.removeFromTop (px (44));
    const int cw = row.getWidth() / 7;
    {
        auto cell = row.removeFromLeft (cw);
        placementLabel.setBounds (cell.removeFromTop (px (16)));
        placementBox.setBounds (cell.reduced (px (4), px (2)));
    }
    external.setBounds (row.removeFromLeft (cw).reduced (px (4), px (10)));
    delta.setBounds (row.removeFromLeft (cw).reduced (px (4), px (10)));
    for (auto& b : presetButtons)
        b.setBounds (row.removeFromLeft (cw).reduced (px (4), px (10)));

    display.setBounds (r.removeFromTop (r.getHeight() * 48 / 100));
    r.removeFromTop (px (8));
    const int kw = r.getWidth() / 8;
    for (auto* k : { &depth, &sharpness, &selectivity, &threshold, &attack, &release, &low })
        k->setBounds (r.removeFromLeft (kw));
    high.setBounds (r);
}

void ResTameEditor::timerCallback()
{
    for (int i = 0; i < ResTameProcessor::displayPoints; ++i)
    {
        const float v = processor.displayReduction[(size_t) i].load();
        auto& sm = smoothed[(size_t) i];
        sm = v > sm ? v : sm * 0.8f + v * 0.2f;
    }
    const auto p = processor.readParams();
    display.setState (smoothed.data(), ResTameProcessor::displayPoints, p.lowHz, p.highHz, p.depthDb, p.delta, p.externalSidechain);
}
