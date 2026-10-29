// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginEditor.h"

using namespace rommuser::ui;
using namespace rommuser::pressready;

namespace
{
constexpr int baseWidth = 900, baseHeight = 540;
constexpr float curveMinDb = -60.0f;
constexpr float meterFloorDb = -48.0f;
constexpr float grMeterRangeDb = 24.0f;
} // namespace

//==============================================================================
void TransferDisplay::setState (const CompParams& p, float liveLevelDb, float liveGrDb)
{
    params = p;
    levelDb = liveLevelDb;
    grDb = liveGrDb;
    repaint();
}

void TransferDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r, 8.0f);
    r = r.reduced (14.0f);

    auto toPoint = [&] (float inDb, float outDb)
    {
        const float x = r.getX() + (inDb - curveMinDb) / -curveMinDb * r.getWidth();
        const float y = r.getBottom() - (outDb - curveMinDb) / -curveMinDb * r.getHeight();
        return juce::Point<float> (x, y);
    };

    g.setFont (juce::FontOptions (10.0f));
    for (float db = curveMinDb; db <= 0.0f; db += 12.0f)
    {
        g.setColour (colours::outline);
        g.drawLine (juce::Line<float> (toPoint (db, curveMinDb), toPoint (db, 0.0f)), 1.0f);
        g.drawLine (juce::Line<float> (toPoint (curveMinDb, db), toPoint (0.0f, db)), 1.0f);
        g.setColour (colours::textDim);
        g.drawText (juce::String ((int) db), juce::Rectangle<float> (toPoint (db, curveMinDb).x - 14.0f, r.getBottom() + 1.0f, 28.0f, 12.0f), juce::Justification::centred);
    }

    // Unity line and threshold marker.
    const float dash[] = { 4.0f, 4.0f };
    g.setColour (colours::outline.brighter (0.4f));
    g.drawDashedLine (juce::Line<float> (toPoint (curveMinDb, curveMinDb), toPoint (0.0f, 0.0f)), dash, 2, 1.0f);
    g.setColour (colours::textDim.withAlpha (0.6f));
    g.drawDashedLine (juce::Line<float> (toPoint (params.thresholdDb, curveMinDb), toPoint (params.thresholdDb, 0.0f)), dash, 2, 1.0f);

    const float kneeDb = std::clamp (params.kneeDb + profileFor (params.character).kneeAddDb, 0.0f, 30.0f);
    juce::Path curve;
    for (int i = 0; i <= 240; ++i)
    {
        const float inDb = curveMinDb + (float) i / 240.0f * -curveMinDb;
        const float outDb = inDb + gainComputerDb (inDb, params.thresholdDb, params.ratio, kneeDb);
        const auto pt = toPoint (inDb, std::max (outDb, curveMinDb));
        i == 0 ? curve.startNewSubPath (pt) : curve.lineTo (pt);
    }
    g.setColour (colours::accent);
    g.strokePath (curve, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Live working point: where the input is right now and how much it is being turned down.
    if (levelDb > curveMinDb)
    {
        const auto pt = toPoint (levelDb, std::max (levelDb - grDb, curveMinDb));
        g.setColour (colours::good);
        g.fillEllipse (juce::Rectangle<float> (10.0f, 10.0f).withCentre (pt));
    }

    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (tr ("entrada dB", "input dB"), r.removeFromBottom (12.0f).translated (0.0f, 14.0f), juce::Justification::centredRight);
}

//==============================================================================
void ReductionMeter::push (float grDb)
{
    // Instant attack, ~30 dB/s release at 30 fps.
    valueDb = grDb > valueDb ? grDb : juce::jmax (grDb, valueDb - 1.0f);
    if (grDb >= holdDb) { holdDb = grDb; holdFrames = 45; }
    else if (--holdFrames <= 0) holdDb = juce::jmax (valueDb, holdDb - 1.0f);
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
    for (int db = 0; db <= (int) grMeterRangeDb; db += 6)
    {
        const float y = bar.getY() + (float) db / grMeterRangeDb * bar.getHeight();
        g.setColour (colours::outline.brighter (0.3f));
        g.drawHorizontalLine (juce::roundToInt (y), bar.getRight() + 2.0f, bar.getRight() + 6.0f);
        g.setColour (colours::textDim);
        g.drawText (juce::String (-db), juce::Rectangle<float> (bar.getRight() + 8.0f, y - 6.0f, 24.0f, 12.0f), juce::Justification::centredLeft);
    }

    const float t = juce::jlimit (0.0f, 1.0f, valueDb / grMeterRangeDb);
    g.setColour (valueDb > 12.0f ? colours::warn : colours::accent);
    g.fillRoundedRectangle (bar.withHeight (bar.getHeight() * t), 4.0f);
    const float hy = bar.getY() + juce::jlimit (0.0f, 1.0f, holdDb / grMeterRangeDb) * bar.getHeight();
    g.setColour (colours::text);
    g.fillRect (bar.getX(), hy - 1.0f, bar.getWidth(), 2.0f);

    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText ("-" + juce::String (holdDb, 1) + " dB", value, juce::Justification::centred);
}

//==============================================================================
void LevelMeter::push (float linearPeak)
{
    const float db = juce::Decibels::gainToDecibels (linearPeak, -100.0f);
    levelDb = db > levelDb ? db : juce::jmax (db, levelDb - 0.7f);
    if (db >= holdDb) { holdDb = db; holdFrames = 45; }
    else if (--holdFrames <= 0) holdDb = juce::jmax (levelDb, holdDb - 0.7f);
    repaint();
}

void LevelMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (label, r.removeFromTop (18.0f), juce::Justification::centred);
    auto value = r.removeFromBottom (18.0f);
    auto bar = r.reduced (r.getWidth() * 0.3f, 4.0f);
    g.setColour (colours::panel);
    g.fillRoundedRectangle (bar, 3.0f);
    auto dbToY = [&] (float db) { return bar.getBottom() - juce::jlimit (0.0f, 1.0f, (db - meterFloorDb) / -meterFloorDb) * bar.getHeight(); };
    const float y = dbToY (levelDb);
    g.setColour (levelDb > -1.0f ? colours::warn : colours::good);
    g.fillRoundedRectangle (bar.withTop (y), 3.0f);
    g.setColour (colours::text);
    g.fillRect (bar.getX(), dbToY (holdDb) - 1.0f, bar.getWidth(), 2.0f);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (juce::String (holdDb, 1), value, juce::Justification::centred);
}

//==============================================================================
PressReadyEditor::PressReadyEditor (PressReadyProcessor& p)
    : processor (p),
      threshold (tr ("UMBRAL", "THRESHOLD")),
      ratio ("RATIO"),
      attack (tr ("ATAQUE", "ATTACK")),
      release ("RELEASE"),
      knee ("KNEE"),
      makeup ("MAKEUP"),
      mix ("MIX")
{
    setLookAndFeel (&lnf);

    struct KnobInfo { Knob* knob; const char* id; juce::String tip; };
    const KnobInfo knobs[] = {
        { &threshold, ParamIDs::threshold,
          tr ("Nivel a partir del cual el compresor empieza a bajar el volumen. Más bajo = comprime más.",
              "Level above which the compressor starts turning the volume down. Lower = more compression.") },
        { &ratio, ParamIDs::ratio,
          tr ("Cuánto baja lo que pasa del umbral. 2:1 suave, 4:1 normal, 10:1 fuerte, ∞:1 limitador.",
              "How much of what goes over the threshold gets turned down. 2:1 gentle, 4:1 normal, 10:1 hard, ∞:1 limiter.") },
        { &attack, ParamIDs::attack,
          tr ("Qué tan rápido reacciona. Rápido (1–5 ms) controla picos; lento (20–50 ms) deja pasar el golpe y da punch.",
              "How fast it reacts. Fast (1–5 ms) controls peaks; slow (20–50 ms) lets the hit through for punch.") },
        { &release, ParamIDs::release,
          tr ("Qué tan rápido suelta. Corto = más fuerte y agresivo; largo = más suave y natural. Prueba AUTO.",
              "How fast it lets go. Short = louder and more aggressive; long = smoother and more natural. Try AUTO.") },
        { &knee, ParamIDs::knee,
          tr ("Transición suave alrededor del umbral. 0 dB = codo duro (preciso), 12 dB = muy suave (musical).",
              "Soft transition around the threshold. 0 dB = hard knee (precise), 12 dB = very soft (musical).") },
        { &makeup, ParamIDs::makeup,
          tr ("Volumen que devuelves después de comprimir. Compara a igual volumen para no engañarte.",
              "Volume you give back after compressing. Compare at equal loudness so you don't fool yourself.") },
        { &mix, ParamIDs::mix,
          tr ("Mezcla con la señal sin comprimir (compresión paralela). 100 % = solo comprimida.",
              "Blend with the uncompressed signal (parallel compression). 100% = compressed only.") },
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
        { &characterBox, &characterLabel, ParamIDs::character, Choices::characters, tr ("CARÁCTER", "CHARACTER"),
          tr ("Clean: transparente, hace lo que dicen las perillas. Punch: estilo FET, agarra rápido y colorea al comprimir. "
              "Glue: estilo bus, RMS, lento y con codo ancho, para pegar una mezcla o un grupo.",
              "Clean: transparent, does exactly what the knobs say. Punch: FET-style, grabs fast and colours while compressing. "
              "Glue: bus-style, RMS, slow with a wide knee, to glue a mix or a group.") },
        { &detectorBox, &detectorLabel, ParamIDs::detector, Choices::detectors, "DETECTOR",
          tr ("Pico reacciona a los golpes; RMS reacciona al volumen percibido (más suave, mejor para voces y buses).",
              "Peak reacts to hits; RMS reacts to perceived loudness (smoother, better for vocals and buses).") },
        { &scHpfBox, &scHpfLabel, ParamIDs::scHpf, Choices::scHpf, tr ("FILTRO SC", "SC FILTER"),
          tr ("Quita graves de lo que escucha el detector, para que el bombo o el bajo no hagan bombear la mezcla.",
              "Removes lows from what the detector hears, so the kick or bass don't make the mix pump.") },
        { &lookaheadBox, &lookaheadLabel, ParamIDs::lookahead, Choices::lookahead, "LOOKAHEAD",
          tr ("Mira unos milisegundos hacia adelante para atrapar picos antes de que lleguen. Añade latencia (el DAW la compensa).",
              "Looks a few milliseconds ahead to catch peaks before they arrive. Adds latency (the DAW compensates).") },
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
        { &autoRelease, ParamIDs::autoRelease, tr ("AUTO RELEASE", "AUTO RELEASE"),
          tr ("Release que depende del material: suelta rápido los golpes cortos y despacio lo sostenido.",
              "Program-dependent release: lets go of short hits fast and of sustained material slowly.") },
        { &autoMakeup, ParamIDs::autoMakeup, "AUTO GAIN",
          tr ("Añade automáticamente la mitad de la reducción que tendría una señal a 0 dBFS, para comparar a igual volumen.",
              "Automatically adds half the reduction a 0 dBFS signal would get, so you compare at equal loudness.") },
        { &external, ParamIDs::external, tr ("SC EXTERNO", "EXT SIDECHAIN"),
          tr ("El detector escucha la entrada de sidechain del DAW (por ejemplo el bombo) en vez de esta pista.",
              "The detector listens to the DAW's sidechain input (e.g. the kick) instead of this track.") },
    };
    for (const auto& b : buttons)
    {
        b.b->setButtonText (b.text);
        b.b->setClickingTogglesState (true);
        b.b->setTooltip (b.tip);
        addAndMakeVisible (*b.b);
        buttonAttachments.push_back (std::make_unique<ButtonAttachment> (processor.state, b.id, *b.b));
    }

    curve.setInterceptsMouseClicks (false, false);
    for (juce::Component* c : { (juce::Component*) &curve, (juce::Component*) &grMeter,
                                (juce::Component*) &inMeter, (juce::Component*) &outMeter })
        addAndMakeVisible (c);

    // Laid out at its base size; ScalableEditor zooms it as vectors to any window size.
    setSize (baseWidth, baseHeight);

    startTimerHz (30);
}

PressReadyEditor::~PressReadyEditor()
{
    setLookAndFeel (nullptr);
}

void PressReadyEditor::paint (juce::Graphics& g)
{
    g.fillAll (colours::background);

    const float s = (float) getWidth() / baseWidth;
    drawHeader (g, getLocalBounds().removeFromTop (juce::roundToInt (48 * s)), s);
}

void PressReadyEditor::resized()
{
    const float s = (float) getWidth() / baseWidth;
    auto px = [s] (int v) { return juce::roundToInt ((float) v * s); };

    auto r = getLocalBounds();
    r.removeFromTop (px (48));
    r.reduce (px (16), px (12));

    // Right column: meters.
    auto meters = r.removeFromRight (px (200));
    r.removeFromRight (px (12));
    grMeter.setBounds (meters.removeFromLeft (px (110)));
    const int lw = meters.getWidth() / 2;
    inMeter.setBounds (meters.removeFromLeft (lw));
    outMeter.setBounds (meters);

    // Bottom strip: combos + buttons.
    auto bottom = r.removeFromBottom (px (62));
    r.removeFromBottom (px (8));
    auto comboRow = bottom.removeFromTop (px (44));
    const int cw = comboRow.getWidth() / 7;
    for (const auto& c : { std::pair { &characterBox, &characterLabel }, std::pair { &detectorBox, &detectorLabel },
                     std::pair { &scHpfBox, &scHpfLabel }, std::pair { &lookaheadBox, &lookaheadLabel } })
    {
        auto cell = comboRow.removeFromLeft (cw);
        c.second->setBounds (cell.removeFromTop (px (16)));
        c.first->setBounds (cell.reduced (px (4), px (2)));
    }
    autoRelease.setBounds (comboRow.removeFromLeft (cw).reduced (px (4), px (10)));
    autoMakeup.setBounds (comboRow.removeFromLeft (cw).reduced (px (4), px (10)));
    external.setBounds (comboRow.reduced (px (4), px (10)));

    // Left: curve. Middle: knobs in two rows.
    auto left = r.removeFromLeft (r.getWidth() * 38 / 100);
    r.removeFromLeft (px (12));
    curve.setBounds (left.reduced (0, px (4)));

    auto row1 = r.removeFromTop (r.getHeight() / 2);
    const int kw = row1.getWidth() / 4;
    threshold.setBounds (row1.removeFromLeft (kw));
    ratio.setBounds (row1.removeFromLeft (kw));
    attack.setBounds (row1.removeFromLeft (kw));
    release.setBounds (row1);
    const int kw2 = r.getWidth() / 3;
    knee.setBounds (r.removeFromLeft (kw2));
    makeup.setBounds (r.removeFromLeft (kw2));
    mix.setBounds (r);
}

void PressReadyEditor::timerCallback()
{
    const float gr = processor.gainReductionDb.exchange (0.0f);
    grMeter.push (gr);
    inMeter.push (processor.inputPeak.exchange (0.0f));
    outMeter.push (processor.outputPeak.exchange (0.0f));

    const float lvl = processor.levelDb.load();
    liveLevelDb = lvl > liveLevelDb ? lvl : juce::jmax (lvl, liveLevelDb - 1.5f);
    curve.setState (processor.readParams(), liveLevelDb, gr);
}
