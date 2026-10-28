// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginEditor.h"

using namespace rommuser::ui;
using namespace rommuser::duckready;

namespace
{
constexpr int baseWidth = 860, baseHeight = 520;
} // namespace

//==============================================================================
CurveDisplay::CurveDisplay (DuckReadyProcessor& p) : processor (p)
{
    depthParam = processor.state.getParameter (ParamIDs::depth);
    releaseParam = processor.state.getParameter (ParamIDs::release);
    setMouseCursor (juce::MouseCursor::UpDownLeftRightResizeCursor);
    setTooltip (tr ("Arrastra arriba/abajo para la profundidad, izquierda/derecha para el release. Doble clic: curva clásica.",
                    "Drag up/down for depth, left/right for release. Double-click: classic curve."));
}

void CurveDisplay::setState (const DuckParams& p, float phase, float gain, bool isPlaying)
{
    params = p;
    livePhase = phase;
    liveGain = gain;
    playing = isPlaying;
    repaint();
}

void CurveDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r, 8.0f);
    r = r.reduced (14.0f, 16.0f);

    const CurveShape shape (params);
    auto toPoint = [&] (float phase, float gain)
    {
        return juce::Point<float> (r.getX() + phase * r.getWidth(), r.getBottom() - gain * r.getHeight());
    };

    // Grid: quarters of the cycle and 25 % gain lines.
    g.setColour (colours::outline);
    for (int i = 0; i <= 4; ++i)
    {
        g.drawLine (juce::Line<float> (toPoint (i / 4.0f, 0.0f), toPoint (i / 4.0f, 1.0f)), 1.0f);
        g.drawLine (juce::Line<float> (toPoint (0.0f, i / 4.0f), toPoint (1.0f, i / 4.0f)), 1.0f);
    }

    // Filled curve. The offset shifts where the trigger lands in the display, so the kick stays at the left edge.
    juce::Path curve, fill;
    const int steps = juce::jmax (64, (int) r.getWidth());
    for (int i = 0; i <= steps; ++i)
    {
        const float phase = (float) i / (float) steps;
        float ph = phase + params.offset;
        ph -= std::floor (ph);
        const float gain = params.trigger == Trigger::tempo ? shape.gainAt (ph) : shape.gainAt (phase);
        const auto pt = toPoint (phase, gain);
        if (i == 0)
        {
            curve.startNewSubPath (pt);
            fill.startNewSubPath (toPoint (0.0f, 0.0f));
            fill.lineTo (pt);
        }
        else
        {
            curve.lineTo (pt);
            fill.lineTo (pt);
        }
    }
    fill.lineTo (toPoint (1.0f, 0.0f));
    fill.closeSubPath();
    g.setColour (colours::accent.withAlpha (0.18f));
    g.fillPath (fill);
    g.setColour (colours::accent);
    g.strokePath (curve, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Playhead and live gain.
    if (playing || params.trigger != Trigger::tempo)
    {
        float ph = livePhase;
        if (params.trigger == Trigger::tempo)
        {
            ph -= params.offset;
            ph -= std::floor (ph);
        }
        if (params.trigger == Trigger::tempo || ph < 1.0f)
        {
            const auto top = toPoint (ph, 1.0f), bottom = toPoint (ph, 0.0f);
            g.setColour (colours::text.withAlpha (0.5f));
            g.drawLine (juce::Line<float> (top, bottom), 1.0f);
            g.setColour (colours::good);
            g.fillEllipse (juce::Rectangle<float> (10.0f, 10.0f).withCentre (toPoint (ph, liveGain)));
        }
    }

    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (11.0f));
    const juce::String label = params.trigger == Trigger::tempo
        ? tr ("un ciclo (", "one cycle (") + Choices::rates[juce::jlimit (0, Choices::rates.size() - 1,
              (int) std::distance (std::begin (Choices::rateBeats),
                                   std::find_if (std::begin (Choices::rateBeats), std::end (Choices::rateBeats),
                                                 [&] (double b) { return std::fabs (b - params.beatsPerCycle) < 1e-9; })))] + ")"
        : tr ("desde el disparo", "from the trigger");
    g.drawText (label, r.withTop (r.getBottom() + 2.0f).withHeight (12.0f), juce::Justification::centredRight);
    g.drawText ("0 dB", r.withY (r.getY() - 14.0f).withHeight (12.0f), juce::Justification::topLeft);
    g.drawText (tr ("silencio", "silence"), r.withTop (r.getBottom() + 2.0f).withHeight (12.0f), juce::Justification::centredLeft);
}

void CurveDisplay::mouseDown (const juce::MouseEvent&)
{
    dragStartDepth = params.depth * 100.0f;
    dragStartRelease = params.release * 100.0f;
    if (depthParam != nullptr) depthParam->beginChangeGesture();
    if (releaseParam != nullptr) releaseParam->beginChangeGesture();
}

void CurveDisplay::mouseDrag (const juce::MouseEvent& e)
{
    const auto d = e.getOffsetFromDragStart();
    if (depthParam != nullptr)
    {
        const float v = juce::jlimit (0.0f, 100.0f, dragStartDepth + (float) d.y / (float) getHeight() * 100.0f);
        depthParam->setValueNotifyingHost (depthParam->convertTo0to1 (v));
    }
    if (releaseParam != nullptr)
    {
        const float v = juce::jlimit (1.0f, 100.0f, dragStartRelease + (float) d.x / (float) getWidth() * 100.0f);
        releaseParam->setValueNotifyingHost (releaseParam->convertTo0to1 (v));
    }
}

void CurveDisplay::mouseUp (const juce::MouseEvent&)
{
    if (depthParam != nullptr) depthParam->endChangeGesture();
    if (releaseParam != nullptr) releaseParam->endChangeGesture();
}

void CurveDisplay::mouseDoubleClick (const juce::MouseEvent&)
{
    processor.applyPreset (curvePresets[0]);
}

//==============================================================================
DuckReadyEditor::DuckReadyEditor (DuckReadyProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      depth (tr ("PROFUNDIDAD", "DEPTH")),
      attack (tr ("ATAQUE", "ATTACK")),
      hold ("HOLD"),
      release ("RELEASE"),
      curve (tr ("CURVA", "CURVE")),
      offset ("OFFSET"),
      scThreshold (tr ("UMBRAL SC", "SC THRESHOLD")),
      crossover ("CROSSOVER"),
      display (p)
{
    setLookAndFeel (&lnf);

    struct KnobInfo { Knob* knob; const char* id; juce::String tip; };
    const KnobInfo knobs[] = {
        { &depth, ParamIDs::depth,
          tr ("Cuánto baja el volumen en cada golpe. 100 % = silencio total.",
              "How far the volume drops on every hit. 100% = complete silence.") },
        { &attack, ParamIDs::attack,
          tr ("Qué tan rápido baja, como porcentaje del ciclo. Corto = golpe seco; más largo = suave.",
              "How fast it drops, as a percentage of the cycle. Short = tight hit; longer = softer.") },
        { &hold, ParamIDs::hold,
          tr ("Cuánto tiempo se queda abajo antes de volver a subir.",
              "How long it stays down before coming back up.") },
        { &release, ParamIDs::release,
          tr ("Cuánto tarda en volver al volumen normal. Es lo que da la sensación de 'bombeo'.",
              "How long it takes to come back to full volume. This is what gives the 'pump' feel.") },
        { &curve, ParamIDs::curve,
          tr ("Forma de la subida y la bajada. Negativo = empieza rápido; positivo = empieza lento; 0 = recta.",
              "Shape of the drop and the rise. Negative = fast start; positive = slow start; 0 = straight.") },
        { &offset, ParamIDs::offset,
          tr ("Mueve toda la curva dentro del ciclo. Útil para que el bombo respire antes de que baje el volumen.",
              "Shifts the whole curve within the cycle. Useful to let the kick breathe before the volume drops.") },
        { &scThreshold, ParamIDs::scThreshold,
          tr ("Solo con disparo por Sidechain: nivel del bombo a partir del cual se dispara la curva.",
              "Sidechain trigger only: kick level above which the curve fires.") },
        { &crossover, ParamIDs::crossover,
          tr ("Solo en modo Low o High: frecuencia que separa los graves de los agudos.",
              "Low or High band only: frequency that splits lows from highs.") },
    };
    for (const auto& k : knobs)
    {
        addAndMakeVisible (*k.knob);
        k.knob->slider.setTooltip (k.tip);
        sliderAttachments.push_back (std::make_unique<SliderAttachment> (processor.state, k.id, k.knob->slider));
        if (auto* param = processor.state.getParameter (k.id))
            k.knob->slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    }

    struct ComboInfo { juce::ComboBox* box; juce::Label* label; const char* id; const juce::StringArray& items; juce::String name, tip; };
    const ComboInfo combos[] = {
        { &rateBox, &rateLabel, ParamIDs::rate, Choices::rates, tr ("VELOCIDAD", "RATE"),
          tr ("Cada cuánto se repite la curva, en figuras del tempo del DAW. 1/4 = en cada bombo de 4/4.",
              "How often the curve repeats, in note values of the DAW tempo. 1/4 = on every kick in 4/4.") },
        { &triggerBox, &triggerLabel, ParamIDs::trigger, Choices::triggers, tr ("DISPARO", "TRIGGER"),
          tr ("Tempo: sigue la rejilla del DAW. MIDI: cada nota reinicia la curva. Sidechain: el bombo que entra por el sidechain la reinicia.",
              "Tempo: follows the DAW grid. MIDI: every note restarts the curve. Sidechain: the kick on the sidechain input restarts it.") },
        { &bandBox, &bandLabel, ParamIDs::band, Choices::bands, tr ("BANDA", "BAND"),
          tr ("Full: todo el sonido. Low: solo los graves bajan (deja espacio al bombo sin apagar lo demás). High: solo los agudos.",
              "Full: the whole sound. Low: only the lows duck (room for the kick without killing the rest). High: only the highs.") },
    };
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

    for (int i = 0; i < 3; ++i)
    {
        auto& b = presetButtons[i];
        b.setButtonText (tr (curvePresets[i].nameEs, curvePresets[i].nameEn));
        b.setTooltip (tr ("Carga una curva lista. Después puedes ajustarla con las perillas.",
                          "Loads a ready-made curve. You can then tweak it with the knobs."));
        b.onClick = [this, i] { processor.applyPreset (curvePresets[i]); };
        addAndMakeVisible (b);
    }

    addAndMakeVisible (display);

    setResizable (true, true);
    setResizeLimits (baseWidth * 3 / 4, baseHeight * 3 / 4, baseWidth * 2, baseHeight * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) baseWidth / baseHeight);
    setSize (baseWidth, baseHeight);

    updateEnablement();
    startTimerHz (30);
}

DuckReadyEditor::~DuckReadyEditor()
{
    setLookAndFeel (nullptr);
}

void DuckReadyEditor::paint (juce::Graphics& g)
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

void DuckReadyEditor::resized()
{
    const float s = (float) getWidth() / baseWidth;
    auto px = [s] (int v) { return juce::roundToInt ((float) v * s); };

    auto r = getLocalBounds();
    r.removeFromTop (px (48));
    r.reduce (px (16), px (12));

    // Bottom strip: combos and presets.
    auto bottom = r.removeFromBottom (px (62));
    r.removeFromBottom (px (8));
    auto comboRow = bottom.removeFromTop (px (44));
    const int cw = comboRow.getWidth() / 6;
    for (const auto& c : { std::pair { &rateBox, &rateLabel }, std::pair { &triggerBox, &triggerLabel },
                           std::pair { &bandBox, &bandLabel } })
    {
        auto cell = comboRow.removeFromLeft (cw);
        c.second->setBounds (cell.removeFromTop (px (16)));
        c.first->setBounds (cell.reduced (px (4), px (2)));
    }
    for (auto& b : presetButtons)
        b.setBounds (comboRow.removeFromLeft (cw).reduced (px (4), px (10)));

    // Top: the curve. Below: two rows of knobs.
    display.setBounds (r.removeFromTop (r.getHeight() * 46 / 100));
    r.removeFromTop (px (8));
    auto row1 = r.removeFromTop (r.getHeight() / 2);
    const int kw = row1.getWidth() / 4;
    depth.setBounds (row1.removeFromLeft (kw));
    attack.setBounds (row1.removeFromLeft (kw));
    hold.setBounds (row1.removeFromLeft (kw));
    release.setBounds (row1);
    curve.setBounds (r.removeFromLeft (kw));
    offset.setBounds (r.removeFromLeft (kw));
    scThreshold.setBounds (r.removeFromLeft (kw));
    crossover.setBounds (r);
}

void DuckReadyEditor::updateEnablement()
{
    const auto p = processor.readParams();
    scThreshold.setEnabled (p.trigger == Trigger::sidechain);
    scThreshold.setAlpha (p.trigger == Trigger::sidechain ? 1.0f : 0.4f);
    offset.setEnabled (p.trigger == Trigger::tempo);
    offset.setAlpha (p.trigger == Trigger::tempo ? 1.0f : 0.4f);
    crossover.setEnabled (p.band != Band::full);
    crossover.setAlpha (p.band != Band::full ? 1.0f : 0.4f);
}

void DuckReadyEditor::timerCallback()
{
    updateEnablement();
    display.setState (processor.readParams(), processor.currentPhase.load(), processor.currentGain.load(),
                      processor.hostPlaying.load());
}
