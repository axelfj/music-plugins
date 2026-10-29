// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginEditor.h"

using namespace rommuser::ui;
using namespace rommuser::gateready;

namespace
{
constexpr int baseWidth = 920, baseHeight = 560;
constexpr float scopeMinDb = -90.0f;
constexpr float grMeterRangeDb = 60.0f;
} // namespace

//==============================================================================
LevelScope::LevelScope (GateReadyProcessor& p) : processor (p)
{
    levels.fill (scopeMinDb);
    thresholdParam = processor.state.getParameter (ParamIDs::threshold);
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

void LevelScope::push (float levelDb, float gainReductionDb, bool open)
{
    levels[(size_t) head] = levelDb;
    reductions[(size_t) head] = gainReductionDb;
    opens[(size_t) head] = open;
    head = (head + 1) % historyLength;
    repaint();
}

void LevelScope::setParams (const GateParams& p)
{
    params = p;
}

float LevelScope::yFor (float db, juce::Rectangle<float> r) const
{
    return r.getBottom() - juce::jlimit (0.0f, 1.0f, (db - scopeMinDb) / -scopeMinDb) * r.getHeight();
}

void LevelScope::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r, 8.0f);
    r = r.reduced (12.0f, 10.0f);
    auto levelArea = r.withTrimmedBottom (r.getHeight() * 0.28f);
    auto gainArea = r.withTop (levelArea.getBottom() + 6.0f);

    g.setFont (juce::FontOptions (10.0f));
    for (float db = -80.0f; db <= 0.0f; db += 20.0f)
    {
        const float y = yFor (db, levelArea);
        g.setColour (colours::outline);
        g.drawHorizontalLine (juce::roundToInt (y), levelArea.getX(), levelArea.getRight());
        g.setColour (colours::textDim);
        g.drawText (juce::String ((int) db), juce::Rectangle<float> (levelArea.getRight() - 26.0f, y - 11.0f, 26.0f, 11.0f), juce::Justification::centredRight);
    }

    // Level history, coloured by open / closed.
    const float dx = levelArea.getWidth() / (float) (historyLength - 1);
    for (int i = 1; i < historyLength; ++i)
    {
        const auto a = (size_t) ((head + i - 1) % historyLength), b = (size_t) ((head + i) % historyLength);
        const float x0 = levelArea.getX() + (float) (i - 1) * dx, x1 = x0 + dx;
        g.setColour (opens[b] ? colours::good : colours::textDim);
        g.drawLine (x0, yFor (levels[a], levelArea), x1, yFor (levels[b], levelArea), 1.5f);
    }

    // Threshold (open) and close lines.
    const float dash[] = { 4.0f, 4.0f };
    g.setColour (colours::accent);
    g.drawLine (levelArea.getX(), yFor (params.thresholdDb, levelArea), levelArea.getRight(), yFor (params.thresholdDb, levelArea), 2.0f);
    if (params.hysteresisDb > 0.05f)
    {
        g.setColour (colours::accent.withAlpha (0.6f));
        const float y = yFor (params.thresholdDb - params.hysteresisDb, levelArea);
        g.drawDashedLine (juce::Line<float> (levelArea.getX(), y, levelArea.getRight(), y), dash, 2, 1.0f);
    }
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText (tr ("abre", "opens"), juce::Rectangle<float> (levelArea.getX() + 2.0f, yFor (params.thresholdDb, levelArea) - 12.0f, 60.0f, 11.0f), juce::Justification::centredLeft);

    // Gain applied (0 dB at the top, range at the bottom).
    g.setColour (colours::outline);
    g.drawRect (gainArea, 1.0f);
    const float rangeAbs = std::max (1.0f, -params.rangeDb);
    juce::Path gr;
    for (int i = 0; i < historyLength; ++i)
    {
        const auto idx = (size_t) ((head + i) % historyLength);
        const float x = gainArea.getX() + (float) i * dx;
        const float y = gainArea.getY() + juce::jlimit (0.0f, 1.0f, reductions[idx] / rangeAbs) * gainArea.getHeight();
        i == 0 ? gr.startNewSubPath (x, y) : gr.lineTo (x, y);
    }
    g.setColour (colours::accent);
    g.strokePath (gr, juce::PathStrokeType (1.5f));
    g.setColour (colours::textDim);
    g.drawText (tr ("ganancia aplicada", "gain applied"), gainArea.reduced (4.0f, 1.0f), juce::Justification::topLeft);
}

void LevelScope::mouseDown (const juce::MouseEvent&)
{
    dragStartDb = params.thresholdDb;
    if (thresholdParam != nullptr) thresholdParam->beginChangeGesture();
}

void LevelScope::mouseDrag (const juce::MouseEvent& e)
{
    if (thresholdParam == nullptr) return;
    const float levelHeight = (float) getHeight() * 0.72f;
    const float db = juce::jlimit (-80.0f, 0.0f, dragStartDb - (float) e.getDistanceFromDragStartY() / levelHeight * -scopeMinDb);
    thresholdParam->setValueNotifyingHost (thresholdParam->convertTo0to1 (db));
}

void LevelScope::mouseUp (const juce::MouseEvent&)
{
    if (thresholdParam != nullptr) thresholdParam->endChangeGesture();
}

//==============================================================================
void ReductionMeter::push (float grDb)
{
    valueDb = grDb > valueDb ? grDb : juce::jmax (grDb, valueDb - 2.0f);
    if (grDb >= holdDb) { holdDb = grDb; holdFrames = 45; }
    else if (--holdFrames <= 0) holdDb = juce::jmax (valueDb, holdDb - 2.0f);
    repaint();
}

void ReductionMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText (tr ("REDUCCIÓN", "REDUCTION"), r.removeFromTop (18.0f), juce::Justification::centred);
    auto value = r.removeFromBottom (18.0f);
    auto bar = r.reduced (r.getWidth() * 0.3f, 4.0f);
    g.setColour (colours::panel);
    g.fillRoundedRectangle (bar, 4.0f);

    g.setFont (juce::FontOptions (10.0f));
    for (int db = 0; db <= (int) grMeterRangeDb; db += 20)
    {
        const float y = bar.getY() + (float) db / grMeterRangeDb * bar.getHeight();
        g.setColour (colours::outline.brighter (0.3f));
        g.drawHorizontalLine (juce::roundToInt (y), bar.getRight() + 2.0f, bar.getRight() + 6.0f);
        g.setColour (colours::textDim);
        g.drawText (juce::String (-db), juce::Rectangle<float> (bar.getRight() + 8.0f, y - 6.0f, 24.0f, 12.0f), juce::Justification::centredLeft);
    }

    const float t = juce::jlimit (0.0f, 1.0f, valueDb / grMeterRangeDb);
    g.setColour (colours::accent);
    g.fillRoundedRectangle (bar.withHeight (bar.getHeight() * t), 4.0f);
    const float hy = bar.getY() + juce::jlimit (0.0f, 1.0f, holdDb / grMeterRangeDb) * bar.getHeight();
    g.setColour (colours::text);
    g.fillRect (bar.getX(), hy - 1.0f, bar.getWidth(), 2.0f);

    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText ("-" + juce::String (holdDb, 1) + " dB", value, juce::Justification::centred);
}

//==============================================================================
GateReadyEditor::GateReadyEditor (GateReadyProcessor& p)
    : processor (p),
      threshold (tr ("UMBRAL", "THRESHOLD")),
      ratio ("RATIO"),
      attack (tr ("ATAQUE", "ATTACK")),
      hold ("HOLD"),
      release ("RELEASE"),
      range (tr ("RANGO", "RANGE")),
      knee ("KNEE"),
      hysteresis (tr ("HISTÉRESIS", "HYSTERESIS")),
      scope (p)
{
    setLookAndFeel (&lnf);

    struct KnobInfo { Knob* knob; const char* id; juce::String tip; };
    const KnobInfo knobs[] = {
        { &threshold, ParamIDs::threshold,
          tr ("Nivel por encima del cual la puerta se abre. Lo que queda debajo se baja.",
              "Level above which the gate opens. Whatever stays below gets turned down.") },
        { &ratio, ParamIDs::ratio,
          tr ("1:2 a 1:10 = expansor suave (baja lo flojo poco a poco). Gate = corta de golpe hasta el Rango.",
              "1:2 to 1:10 = gentle expander (turns quiet parts down gradually). Gate = cuts straight to Range.") },
        { &attack, ParamIDs::attack,
          tr ("Qué tan rápido abre. Muy rápido para drums (0.1–1 ms), más lento para voces.",
              "How fast it opens. Very fast for drums (0.1–1 ms), slower for vocals.") },
        { &hold, ParamIDs::hold,
          tr ("Cuánto tiempo se queda abierta después de que la señal baja, antes de empezar a cerrar.",
              "How long it stays open after the signal drops, before it starts closing.") },
        { &release, ParamIDs::release,
          tr ("Qué tan rápido cierra. Corto = seco; largo = natural.",
              "How fast it closes. Short = tight; long = natural.") },
        { &range, ParamIDs::range,
          tr ("Cuánto baja cuando está cerrada. -80 dB = silencio; -20 dB = solo atenúa.",
              "How much it turns down when closed. -80 dB = silence; -20 dB = just attenuates.") },
        { &knee, ParamIDs::knee,
          tr ("Suaviza la transición alrededor del umbral.", "Softens the transition around the threshold.") },
        { &hysteresis, ParamIDs::hysteresis,
          tr ("Abre en el umbral pero solo cierra cuando la señal baja estos dB por debajo. Evita que tartamudee.",
              "Opens at the threshold but only closes once the signal drops this many dB below it. Stops the chattering.") },
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
        { &lookaheadBox, &lookaheadLabel, ParamIDs::lookahead, Choices::lookahead, "LOOKAHEAD",
          tr ("Mira unos milisegundos adelante para abrir antes de que llegue el golpe (no se come el transitorio). Añade latencia.",
              "Looks a few milliseconds ahead to open before the hit arrives (keeps the transient). Adds latency.") },
        { &scHpfBox, &scHpfLabel, ParamIDs::scHpf, Choices::scHpf, tr ("SC GRAVES", "SC LOW CUT"),
          tr ("Quita graves de lo que escucha el detector: por ejemplo para que el bombo no abra la puerta de la caja.",
              "Removes lows from what the detector hears: e.g. so the kick doesn't open the snare's gate.") },
        { &scLpfBox, &scLpfLabel, ParamIDs::scLpf, Choices::scLpf, tr ("SC AGUDOS", "SC HIGH CUT"),
          tr ("Quita agudos de lo que escucha el detector: para que los platos no abran la puerta del bombo.",
              "Removes highs from what the detector hears: so the cymbals don't open the kick's gate.") },
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

    struct ButtonInfo { juce::TextButton* b; const char* id; juce::String text, tip; };
    const ButtonInfo buttons[] = {
        { &external, ParamIDs::external, tr ("SC EXTERNO", "EXT SIDECHAIN"),
          tr ("El detector escucha la entrada de sidechain del DAW en vez de esta pista.",
              "The detector listens to the DAW's sidechain input instead of this track.") },
        { &listen, ParamIDs::listen, tr ("ESCUCHAR", "LISTEN"),
          tr ("Escucha lo que oye el detector (con sus filtros) para ajustar los cortes. Apágalo después.",
              "Hear what the detector hears (with its filters) to set the cuts. Turn it off afterwards.") },
    };
    for (const auto& b : buttons)
    {
        b.b->setButtonText (b.text);
        b.b->setClickingTogglesState (true);
        b.b->setTooltip (b.tip);
        addAndMakeVisible (*b.b);
        buttonAttachments.push_back (std::make_unique<ButtonAttachment> (processor.state, b.id, *b.b));
    }

    for (int i = 0; i < 3; ++i)
    {
        auto& b = presetButtons[i];
        b.setButtonText (tr (gatePresets[i].nameEs, gatePresets[i].nameEn));
        b.setTooltip (tr ("Punto de partida. Luego mueve el umbral arrastrando la línea naranja.",
                          "Starting point. Then move the threshold by dragging the orange line."));
        b.onClick = [this, i] { processor.applyPreset (gatePresets[i]); };
        addAndMakeVisible (b);
    }

    addAndMakeVisible (scope);
    addAndMakeVisible (grMeter);

    // Laid out at its base size; ScalableEditor zooms it as vectors to any window size.
    setSize (baseWidth, baseHeight);

    startTimerHz (30);
}

GateReadyEditor::~GateReadyEditor()
{
    setLookAndFeel (nullptr);
}

void GateReadyEditor::paint (juce::Graphics& g)
{
    g.fillAll (colours::background);

    const float s = (float) getWidth() / baseWidth;
    drawHeader (g, getLocalBounds().removeFromTop (juce::roundToInt (48 * s)), s);
}

void GateReadyEditor::resized()
{
    const float s = (float) getWidth() / baseWidth;
    auto px = [s] (int v) { return juce::roundToInt ((float) v * s); };

    auto r = getLocalBounds();
    r.removeFromTop (px (48));
    r.reduce (px (16), px (12));

    // Right: reduction meter.
    grMeter.setBounds (r.removeFromRight (px (110)));
    r.removeFromRight (px (12));

    // Bottom strip: combos, buttons, presets.
    auto bottom = r.removeFromBottom (px (62));
    r.removeFromBottom (px (8));
    auto row = bottom.removeFromTop (px (44));
    const int cw = row.getWidth() / 8;
    for (const auto& c : { std::pair { &lookaheadBox, &lookaheadLabel }, std::pair { &scHpfBox, &scHpfLabel },
                           std::pair { &scLpfBox, &scLpfLabel } })
    {
        auto cell = row.removeFromLeft (cw);
        c.second->setBounds (cell.removeFromTop (px (16)));
        c.first->setBounds (cell.reduced (px (4), px (2)));
    }
    external.setBounds (row.removeFromLeft (cw).reduced (px (4), px (10)));
    listen.setBounds (row.removeFromLeft (cw).reduced (px (4), px (10)));
    for (auto& b : presetButtons)
        b.setBounds (row.removeFromLeft (cw).reduced (px (4), px (10)));

    // Top: scope. Then two rows of knobs.
    scope.setBounds (r.removeFromTop (r.getHeight() * 44 / 100));
    r.removeFromTop (px (8));
    auto row1 = r.removeFromTop (r.getHeight() / 2);
    const int kw = row1.getWidth() / 4;
    threshold.setBounds (row1.removeFromLeft (kw));
    ratio.setBounds (row1.removeFromLeft (kw));
    attack.setBounds (row1.removeFromLeft (kw));
    hold.setBounds (row1);
    release.setBounds (r.removeFromLeft (kw));
    range.setBounds (r.removeFromLeft (kw));
    knee.setBounds (r.removeFromLeft (kw));
    hysteresis.setBounds (r);
}

void GateReadyEditor::timerCallback()
{
    const float gr = processor.gainReductionDb.exchange (0.0f);
    processor.inputPeak.exchange (0.0f);
    processor.outputPeak.exchange (0.0f);
    grMeter.push (gr);
    scope.setParams (processor.readParams());
    scope.push (processor.levelDb.load(), gr, processor.gateOpen.load());
}
