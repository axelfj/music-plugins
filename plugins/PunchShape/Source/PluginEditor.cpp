// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginEditor.h"

using namespace rommuser::ui;
using namespace rommuser::punchshape;

namespace
{
constexpr int baseWidth = 880, baseHeight = 520;
constexpr float levelMinDb = -60.0f;
constexpr float gainRangeDb = 24.0f;
} // namespace

//==============================================================================
ShapeDisplay::ShapeDisplay()
{
    levels.fill (levelMinDb);
}

void ShapeDisplay::push (float levelDb, float attackGainDb, float sustainGainDb)
{
    levels[(size_t) head] = levelDb;
    attacks[(size_t) head] = attackGainDb;
    sustains[(size_t) head] = sustainGainDb;
    head = (head + 1) % historyLength;
    repaint();
}

void ShapeDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r, 8.0f);
    r = r.reduced (12.0f, 10.0f);

    const float dx = r.getWidth() / (float) (historyLength - 1);
    auto idx = [this] (int i) { return (size_t) ((head + i) % historyLength); };

    // Detector level as a filled area from the bottom.
    juce::Path level;
    level.startNewSubPath (r.getX(), r.getBottom());
    for (int i = 0; i < historyLength; ++i)
    {
        const float t = juce::jlimit (0.0f, 1.0f, (levels[idx (i)] - levelMinDb) / -levelMinDb);
        level.lineTo (r.getX() + (float) i * dx, r.getBottom() - t * r.getHeight());
    }
    level.lineTo (r.getRight(), r.getBottom());
    level.closeSubPath();
    g.setColour (colours::textDim.withAlpha (0.25f));
    g.fillPath (level);

    // Gain scale: 0 dB in the middle, +-24 dB at the edges.
    const float midY = r.getCentreY();
    auto yFor = [&] (float db) { return midY - juce::jlimit (-1.0f, 1.0f, db / gainRangeDb) * r.getHeight() * 0.5f; };
    g.setFont (juce::FontOptions (10.0f));
    for (float db = -24.0f; db <= 24.0f; db += 12.0f)
    {
        const float y = yFor (db);
        g.setColour (std::fabs (db) < 0.5f ? colours::outline.brighter (0.4f) : colours::outline);
        g.drawHorizontalLine (juce::roundToInt (y), r.getX(), r.getRight());
        g.setColour (colours::textDim);
        g.drawText ((db > 0.0f ? "+" : "") + juce::String ((int) db), juce::Rectangle<float> (r.getRight() - 28.0f, y - 11.0f, 28.0f, 11.0f), juce::Justification::centredRight);
    }

    auto drawGain = [&] (const std::array<float, historyLength>& values, juce::Colour colour)
    {
        juce::Path p;
        for (int i = 0; i < historyLength; ++i)
        {
            const float x = r.getX() + (float) i * dx, y = yFor (values[idx (i)]);
            i == 0 ? p.startNewSubPath (x, y) : p.lineTo (x, y);
        }
        g.setColour (colour);
        g.strokePath (p, juce::PathStrokeType (1.8f));
    };
    drawGain (sustains, colours::good);
    drawGain (attacks, colours::accent);

    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.setColour (colours::accent);
    g.drawText (tr ("ataque", "attack"), r.reduced (4.0f, 2.0f), juce::Justification::topLeft);
    g.setColour (colours::good);
    g.drawText (tr ("sustain", "sustain"), r.reduced (4.0f, 2.0f).translated (0.0f, 14.0f), juce::Justification::topLeft);
    g.setColour (colours::textDim);
    g.drawText (tr ("nivel", "level"), r.reduced (4.0f, 2.0f), juce::Justification::bottomLeft);
}

//==============================================================================
PunchShapeEditor::PunchShapeEditor (PunchShapeProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      attack (tr ("ATAQUE", "ATTACK")),
      sustain ("SUSTAIN"),
      speed (tr ("VELOCIDAD", "SPEED")),
      tail (tr ("COLA", "TAIL")),
      output (tr ("SALIDA", "OUTPUT"))
{
    setLookAndFeel (&lnf);

    struct KnobInfo { Knob* knob; const char* id; juce::String tip; };
    const KnobInfo knobs[] = {
        { &attack, ParamIDs::attack,
          tr ("Sube (+) o suaviza (−) los golpes. +6 dB = más pegada; −6 dB = más suave y lejano.",
              "Boosts (+) or softens (−) the hits. +6 dB = more punch; −6 dB = softer, further away.") },
        { &sustain, ParamIDs::sustain,
          tr ("Alarga (+) o acorta (−) las colas. −8 dB seca una caja con mucho ambiente; +6 dB le da más cuerpo.",
              "Lengthens (+) or shortens (−) the tails. −8 dB dries a roomy snare; +6 dB gives it more body.") },
        { &speed, ParamIDs::speed,
          tr ("Cuánto dura un golpe para el detector. Corto para hi-hats, largo para bombos y bajos.",
              "How long a hit lasts for the detector. Short for hi-hats, long for kicks and bass.") },
        { &tail, ParamIDs::tail,
          tr ("Cuánto tiempo sigue escuchando el sustain después del golpe.",
              "How long the sustain stage keeps listening after the hit.") },
        { &output, ParamIDs::output, tr ("Volumen de salida.", "Output level.") },
    };
    for (const auto& k : knobs)
    {
        addAndMakeVisible (*k.knob);
        k.knob->slider.setTooltip (k.tip);
        sliderAttachments.push_back (std::make_unique<SliderAttachment> (processor.state, k.id, k.knob->slider));
        if (auto* param = processor.state.getParameter (k.id))
            k.knob->slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    }

    detectorBox.addItem (tr ("Todo", "All"), 1);
    detectorBox.addItem (tr ("Graves", "Lows"), 2);
    detectorBox.addItem (tr ("Agudos", "Highs"), 3);
    detectorBox.setTooltip (tr ("Qué escucha el detector. Graves = reacciona solo al bombo y al bajo; Agudos = solo a platos y hi-hats. El sonido completo se procesa igual.",
                                "What the detector listens to. Lows = reacts only to kick and bass; Highs = only to cymbals and hi-hats. The whole sound is still processed."));
    addAndMakeVisible (detectorBox);
    comboAttachments.push_back (std::make_unique<ComboAttachment> (processor.state, ParamIDs::detector, detectorBox));
    detectorLabel.setText (tr ("DETECTOR", "DETECTOR"), juce::dontSendNotification);
    detectorLabel.setJustificationType (juce::Justification::centred);
    detectorLabel.setColour (juce::Label::textColourId, colours::textDim);
    detectorLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    addAndMakeVisible (detectorLabel);

    clip.setButtonText ("CLIP");
    clip.setClickingTogglesState (true);
    clip.setTooltip (tr ("Clipper suave a −0.3 dBFS en la salida, para que los ataques subidos no pasen de 0.",
                         "Soft clipper at −0.3 dBFS on the output, so boosted attacks never go over 0."));
    addAndMakeVisible (clip);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (processor.state, ParamIDs::clip, clip));

    clipLed.setText ("", juce::dontSendNotification);
    clipLed.setJustificationType (juce::Justification::centred);
    clipLed.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    addAndMakeVisible (clipLed);

    for (int i = 0; i < 4; ++i)
    {
        auto& b = presetButtons[i];
        b.setButtonText (tr (punchPresets[i].nameEs, punchPresets[i].nameEn));
        b.setTooltip (tr ("Punto de partida. Luego ajusta Ataque y Sustain a oído.",
                          "Starting point. Then set Attack and Sustain by ear."));
        b.onClick = [this, i] { processor.applyPreset (punchPresets[i]); };
        addAndMakeVisible (b);
    }

    hint.setText (tr ("Ataque y Sustain en 0 = no cambia nada. El resultado es igual a cualquier volumen.",
                      "Attack and Sustain at 0 = nothing changes. The result is the same at any volume."),
                  juce::dontSendNotification);
    hint.setJustificationType (juce::Justification::centred);
    hint.setColour (juce::Label::textColourId, colours::textDim);
    hint.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (hint);

    addAndMakeVisible (display);

    setResizable (true, true);
    setResizeLimits (baseWidth * 3 / 4, baseHeight * 3 / 4, baseWidth * 2, baseHeight * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) baseWidth / baseHeight);
    setSize (baseWidth, baseHeight);

    startTimerHz (30);
}

PunchShapeEditor::~PunchShapeEditor()
{
    setLookAndFeel (nullptr);
}

void PunchShapeEditor::paint (juce::Graphics& g)
{
    g.fillAll (colours::background);

    const float s = (float) getWidth() / baseWidth;
    auto header = getLocalBounds().removeFromTop (juce::roundToInt (48 * s)).reduced (juce::roundToInt (20 * s), 0);

    g.setColour (colours::text);
    g.setFont (juce::FontOptions (24.0f * s, juce::Font::bold));
    g.drawText ("PUNCH SHAPE", header, juce::Justification::centredLeft);

    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (12.0f * s));
    g.drawText (juce::String::fromUTF8 ("ROMMUSER  ·  v") + PLUGIN_VERSION_STRING + juce::String::fromUTF8 ("  ·  ") + tr ("gratis y open source", "free & open source"),
                header, juce::Justification::centredRight);

    g.setColour (colours::outline);
    g.drawHorizontalLine (header.getBottom(), 0.0f, (float) getWidth());
}

void PunchShapeEditor::resized()
{
    const float s = (float) getWidth() / baseWidth;
    auto px = [s] (int v) { return juce::roundToInt ((float) v * s); };

    auto r = getLocalBounds();
    r.removeFromTop (px (48));
    r.reduce (px (16), px (12));

    hint.setBounds (r.removeFromBottom (px (18)));
    r.removeFromBottom (px (4));

    auto bottom = r.removeFromBottom (px (62));
    r.removeFromBottom (px (8));
    auto row = bottom.removeFromTop (px (44));
    const int cw = row.getWidth() / 7;
    {
        auto cell = row.removeFromLeft (cw);
        detectorLabel.setBounds (cell.removeFromTop (px (16)));
        detectorBox.setBounds (cell.reduced (px (4), px (2)));
    }
    {
        auto cell = row.removeFromLeft (cw);
        clipLed.setBounds (cell.removeFromTop (px (16)));
        clip.setBounds (cell.reduced (px (4), px (2)));
    }
    row.removeFromLeft (cw / 2);
    for (auto& b : presetButtons)
        b.setBounds (row.removeFromLeft ((row.getWidth()) / 4).reduced (px (4), px (10)));

    display.setBounds (r.removeFromTop (r.getHeight() * 46 / 100));
    r.removeFromTop (px (8));
    const int kw = r.getWidth() / 5;
    attack.setBounds (r.removeFromLeft (kw));
    sustain.setBounds (r.removeFromLeft (kw));
    speed.setBounds (r.removeFromLeft (kw));
    tail.setBounds (r.removeFromLeft (kw));
    output.setBounds (r);
}

void PunchShapeEditor::timerCallback()
{
    const float a = processor.attackGainDb.exchange (0.0f);
    const float su = processor.sustainGainDb.exchange (0.0f);
    processor.inputPeak.exchange (0.0f);
    processor.outputPeak.exchange (0.0f);
    display.push (processor.levelDb.load(), a, su);

    if (processor.clipped.exchange (false))
        clipFrames = 15;
    else if (clipFrames > 0)
        --clipFrames;
    clipLed.setText (clipFrames > 0 ? tr ("recortando", "clipping") : juce::String(), juce::dontSendNotification);
    clipLed.setColour (juce::Label::textColourId, colours::warn);
}
