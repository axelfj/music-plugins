// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginEditor.h"

using namespace rommuser::ui;
using namespace rommuser::warmdrive;

namespace
{
constexpr int baseWidth = 960, baseHeight = 600;
} // namespace

//==============================================================================
void CurveView::setState (Saturation newType, float newDriveDb, float newHitDb, bool isEnabled)
{
    type = newType;
    driveDb = newDriveDb;
    hitDb = newHitDb;
    enabled = isEnabled;
    repaint();
}

void CurveView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r, 8.0f);
    r = r.reduced (10.0f);
    if (! enabled)
        return;

    // x: input -1 .. 1 (at 0 dBFS), y: output of shape (x * drive) / drive * compensation, same scale.
    const float drive = std::pow (10.0f, driveDb / 20.0f);
    const float comp = 1.0f / std::max (curveGainFor (type, drive), 0.05f);
    g.setColour (colours::outline);
    g.drawLine (r.getX(), r.getCentreY(), r.getRight(), r.getCentreY(), 1.0f);
    g.drawLine (r.getCentreX(), r.getY(), r.getCentreX(), r.getBottom(), 1.0f);
    const float dash[] = { 3.0f, 3.0f };
    g.drawDashedLine (juce::Line<float> (r.getBottomLeft(), r.getTopRight()), dash, 2, 1.0f);

    juce::Path path;
    for (int i = 0; i <= 120; ++i)
    {
        const float x = -1.0f + 2.0f * (float) i / 120.0f;
        const float y = juce::jlimit (-1.0f, 1.0f, shape (x * drive, type) / drive * comp);
        const juce::Point<float> pt (r.getX() + (x + 1.0f) * 0.5f * r.getWidth(), r.getCentreY() - y * 0.5f * r.getHeight());
        i == 0 ? path.startNewSubPath (pt) : path.lineTo (pt);
    }
    g.setColour (hitDb > 0.1f ? colours::accent : colours::accent.withAlpha (0.6f));
    g.strokePath (path, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // How far into the curve the loudest peak went.
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText (hitDb > 0.1f ? "+" + juce::String (hitDb, 1) + " dB" : "", r.removeFromBottom (12.0f), juce::Justification::centredRight);
}

//==============================================================================
WarmDriveEditor::WarmDriveEditor (WarmDriveProcessor& p)
    : AudioProcessorEditor (p),
      processor (p)
{
    setLookAndFeel (&lnf);

    struct KnobInfo { Knob* knob; juce::String id; juce::String tip; };
    std::vector<KnobInfo> knobs = {
        { &xover[0], ParamIDs::xover1, tr ("Frecuencia que separa la banda 1 de la 2.", "Frequency that splits band 1 from band 2.") },
        { &xover[1], ParamIDs::xover2, tr ("Frecuencia que separa la banda 2 de la 3.", "Frequency that splits band 2 from band 3.") },
        { &xover[2], ParamIDs::xover3, tr ("Frecuencia que separa la banda 3 de la 4.", "Frequency that splits band 3 from band 4.") },
        { &input, ParamIDs::input, tr ("Ganancia antes de saturar. Subirla es como subir todos los drives.",
                                       "Gain before saturating. Turning it up is like turning up every drive.") },
        { &output, ParamIDs::output, tr ("Ganancia final.", "Final gain.") },
        { &mix, ParamIDs::mix, tr ("Mezcla con la señal limpia (saturación paralela). 100 % = solo saturada.",
                                   "Blend with the clean signal (parallel saturation). 100% = saturated only.") },
    };
    for (int b = 0; b < maxBands; ++b)
    {
        auto& col = bands[b];
        knobs.push_back ({ &col.drive, ParamIDs::band (b, "drive"),
                           tr ("Cuánto empuja la señal contra la curva. 0 dB apenas la toca; 36 dB la aplasta.",
                               "How hard the signal is pushed into the curve. 0 dB barely touches it; 36 dB crushes it.") });
        knobs.push_back ({ &col.tone, ParamIDs::band (b, "tone"),
                           tr ("Filtro que oscurece los armónicos nuevos de esta banda. Off = sin filtro.",
                               "Filter that darkens the new harmonics of this band. Off = no filter.") });
        knobs.push_back ({ &col.mix, ParamIDs::band (b, "mix"),
                           tr ("Mezcla de esta banda: saturada vs. limpia.", "This band's blend: saturated vs. clean.") });
    }
    for (const auto& k : knobs)
    {
        addAndMakeVisible (*k.knob);
        k.knob->slider.setTooltip (k.tip);
        sliderAttachments.push_back (std::make_unique<SliderAttachment> (processor.state, k.id, k.knob->slider));
        if (auto* param = processor.state.getParameter (k.id))
            k.knob->slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    }

    struct ComboInfo { juce::ComboBox* box; juce::Label* label; juce::String id; const juce::StringArray& items; juce::String name, tip; };
    std::vector<ComboInfo> combos = {
        { &bandsBox, &bandsLabel, ParamIDs::bands, Choices::bands, tr ("BANDAS", "BANDS"),
          tr ("Cuántas bandas de frecuencia se saturan por separado. 1 = toda la señal junta.",
              "How many frequency bands are saturated separately. 1 = the whole signal together.") },
        { &oversamplingBox, &oversamplingLabel, ParamIDs::oversampling, Choices::oversampling, tr ("CALIDAD", "QUALITY"),
          tr ("Sobremuestreo. Más = menos aliasing (brillo digital falso) pero más CPU. 2x va bien casi siempre.",
              "Oversampling. More = less aliasing (fake digital brightness) but more CPU. 2x is fine almost always.") },
    };
    const juce::String typeTip = tr ("Tape: suave y redondo. Tube: cálido, con armónicos pares. Transistor: más duro y brillante. "
                                     "Soft Clip: limpio hasta que recorta. Rectify: sucio, con octava arriba.",
                                     "Tape: smooth and round. Tube: warm, with even harmonics. Transistor: harder and brighter. "
                                     "Soft Clip: clean until it clips. Rectify: dirty, with an octave up.");
    for (int b = 0; b < maxBands; ++b)
        combos.push_back ({ &bands[b].typeBox, &bands[b].title, ParamIDs::band (b, "type"), Choices::types,
                            tr ("BANDA ", "BAND ") + juce::String (b + 1), typeTip });
    for (const auto& c : combos)
    {
        c.box->addItemList (c.items, 1);
        c.box->setTooltip (c.tip);
        addAndMakeVisible (*c.box);
        comboAttachments.push_back (std::make_unique<ComboAttachment> (processor.state, c.id, *c.box));
        c.label->setText (c.name, juce::dontSendNotification);
        c.label->setJustificationType (juce::Justification::centred);
        c.label->setColour (juce::Label::textColourId, colours::textDim);
        c.label->setFont (juce::FontOptions (12.0f, juce::Font::bold));
        addAndMakeVisible (*c.label);
    }

    autoGain.setButtonText ("AUTO GAIN");
    autoGain.setClickingTogglesState (true);
    autoGain.setTooltip (tr ("Compensa el volumen que añade cada curva, para comparar a igual volumen y no engañarte.",
                             "Compensates the level each curve adds, so you compare at equal loudness and don't fool yourself."));
    addAndMakeVisible (autoGain);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (processor.state, ParamIDs::autoGain, autoGain));

    for (int i = 0; i < 3; ++i)
    {
        auto& b = presetButtons[i];
        b.setButtonText (tr (drivePresets[i].nameEs, drivePresets[i].nameEn));
        b.setTooltip (tr ("Carga un punto de partida. Después ajusta los drives a gusto.",
                          "Loads a starting point. Then tweak the drives to taste."));
        b.onClick = [this, i] { processor.applyPreset (drivePresets[i]); };
        addAndMakeVisible (b);
    }

    for (auto& col : bands)
        addAndMakeVisible (col.curve);

    setResizable (true, true);
    setResizeLimits (baseWidth * 3 / 4, baseHeight * 3 / 4, baseWidth * 2, baseHeight * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) baseWidth / baseHeight);
    setSize (baseWidth, baseHeight);

    updateEnablement();
    startTimerHz (30);
}

WarmDriveEditor::~WarmDriveEditor()
{
    setLookAndFeel (nullptr);
}

void WarmDriveEditor::paint (juce::Graphics& g)
{
    g.fillAll (colours::background);

    const float s = (float) getWidth() / baseWidth;
    auto header = getLocalBounds().removeFromTop (juce::roundToInt (48 * s)).reduced (juce::roundToInt (20 * s), 0);

    g.setColour (colours::text);
    g.setFont (juce::FontOptions (24.0f * s, juce::Font::bold));
    g.drawText (juce::String (JucePlugin_Name).toUpperCase(), header, juce::Justification::centredLeft);

    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (12.0f * s));
    g.drawText ("R - V" + juce::String (PLUGIN_VERSION_STRING).upToFirstOccurrenceOf (".", false, false) + " - FREE",
                header, juce::Justification::centredRight);

    g.setColour (colours::outline);
    g.drawHorizontalLine (header.getBottom(), 0.0f, (float) getWidth());
}

void WarmDriveEditor::resized()
{
    const float s = (float) getWidth() / baseWidth;
    auto px = [s] (int v) { return juce::roundToInt ((float) v * s); };

    auto r = getLocalBounds();
    r.removeFromTop (px (48));
    r.reduce (px (16), px (12));

    // Top row: bands / quality combos, crossovers, in / out / mix, auto gain.
    auto top = r.removeFromTop (px (120));
    r.removeFromTop (px (8));
    const int gw = top.getWidth() / 9;
    for (const auto& c : { std::pair { &bandsBox, &bandsLabel }, std::pair { &oversamplingBox, &oversamplingLabel } })
    {
        auto cell = top.removeFromLeft (gw);
        c.second->setBounds (cell.removeFromTop (px (18)));
        c.first->setBounds (cell.removeFromTop (px (30)).reduced (px (6), px (2)));
    }
    for (auto& k : xover)
        k.setBounds (top.removeFromLeft (gw));
    input.setBounds (top.removeFromLeft (gw));
    output.setBounds (top.removeFromLeft (gw));
    mix.setBounds (top.removeFromLeft (gw));
    autoGain.setBounds (top.reduced (px (8), px (44)));

    // Bottom row: presets.
    auto bottom = r.removeFromBottom (px (34));
    r.removeFromBottom (px (8));
    const int pw = bottom.getWidth() / 3;
    for (auto& b : presetButtons)
        b.setBounds (bottom.removeFromLeft (pw).reduced (px (6), px (2)));

    // Band columns.
    const int cw = r.getWidth() / maxBands;
    for (auto& col : bands)
    {
        auto c = r.removeFromLeft (cw).reduced (px (6), 0);
        col.title.setBounds (c.removeFromTop (px (18)));
        col.typeBox.setBounds (c.removeFromTop (px (28)).reduced (px (10), px (2)));
        c.removeFromTop (px (6));
        col.curve.setBounds (c.removeFromTop (px (110)));
        c.removeFromTop (px (6));
        auto knobRow = c.removeFromTop (px (110));
        const int kw = knobRow.getWidth() / 3;
        col.drive.setBounds (knobRow.removeFromLeft (kw));
        col.tone.setBounds (knobRow.removeFromLeft (kw));
        col.mix.setBounds (knobRow);
    }
}

void WarmDriveEditor::updateEnablement()
{
    const auto p = processor.readParams();
    for (int b = 0; b < maxBands; ++b)
    {
        const bool on = b < p.numBands;
        auto& col = bands[b];
        for (juce::Component* c : { (juce::Component*) &col.typeBox, (juce::Component*) &col.drive,
                                    (juce::Component*) &col.tone, (juce::Component*) &col.mix, (juce::Component*) &col.title })
        {
            c->setEnabled (on);
            c->setAlpha (on ? 1.0f : 0.35f);
        }
        col.curve.setState (p.bands[(size_t) b].type, p.bands[(size_t) b].driveDb, hitDb[b], on);
    }
    for (int i = 0; i < 3; ++i)
    {
        const bool on = i < p.numBands - 1;
        xover[i].setEnabled (on);
        xover[i].setAlpha (on ? 1.0f : 0.35f);
    }
}

void WarmDriveEditor::timerCallback()
{
    for (int b = 0; b < maxBands; ++b)
    {
        const float v = processor.bandDriveDb[b].exchange (0.0f);
        hitDb[b] = v > hitDb[b] ? v : juce::jmax (v, hitDb[b] - 0.5f);
    }
    updateEnablement();
}
