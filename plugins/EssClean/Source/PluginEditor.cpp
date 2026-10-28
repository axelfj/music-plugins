// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginEditor.h"

using namespace rommuser::ui;
using namespace rommuser::essclean;

namespace
{
constexpr int baseWidth = 920, baseHeight = 560;
constexpr float grMeterRangeDb = 24.0f;
} // namespace

//==============================================================================
SpectrumDisplay::SpectrumDisplay (EssCleanProcessor& p) : processor (p)
{
    lowParam = processor.state.getParameter (ParamIDs::low);
    highParam = processor.state.getParameter (ParamIDs::high);
}

void SpectrumDisplay::update() { repaint(); }

juce::Rectangle<float> SpectrumDisplay::plotArea() const
{
    return getLocalBounds().toFloat().reduced (12.0f, 10.0f);
}

float SpectrumDisplay::xFor (float hz, juce::Rectangle<float> r) const
{
    const float t = std::log (juce::jlimit (minHz, maxHz, hz) / minHz) / std::log (maxHz / minHz);
    return r.getX() + t * r.getWidth();
}

float SpectrumDisplay::hzFor (float x, juce::Rectangle<float> r) const
{
    const float t = juce::jlimit (0.0f, 1.0f, (x - r.getX()) / r.getWidth());
    return minHz * std::pow (maxHz / minHz, t);
}

void SpectrumDisplay::paint (juce::Graphics& g)
{
    auto full = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (full, 8.0f);
    const auto r = plotArea();
    const auto params = processor.readParams();
    const double sr = processor.getSampleRateForDisplay();

    // Grid.
    g.setFont (juce::FontOptions (10.0f));
    for (float hz : { 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
    {
        const float x = xFor (hz, r);
        g.setColour (colours::outline);
        g.drawVerticalLine (juce::roundToInt (x), r.getY(), r.getBottom());
        g.setColour (colours::textDim);
        g.drawText (hz >= 1000.0f ? juce::String (hz / 1000.0f, 0) + "k" : juce::String ((int) hz),
                    juce::Rectangle<float> (x - 16.0f, r.getBottom() - 12.0f, 32.0f, 12.0f), juce::Justification::centred);
    }

    // Detection range.
    const float xl = xFor (params.lowHz, r), xh = xFor (params.highHz, r);
    g.setColour (colours::accent.withAlpha (0.18f));
    g.fillRect (juce::Rectangle<float> (xl, r.getY(), xh - xl, r.getHeight()));
    g.setColour (colours::accent);
    g.drawVerticalLine (juce::roundToInt (xl), r.getY(), r.getBottom());
    g.drawVerticalLine (juce::roundToInt (xh), r.getY(), r.getBottom());

    // Spectrum.
    juce::Path spec;
    const int bins = EssCleanProcessor::numBins;
    bool started = false;
    for (int k = 1; k < bins; ++k)
    {
        const float hz = (float) (k * sr / (double) EssCleanProcessor::fftSize);
        if (hz < minHz || hz > maxHz) continue;
        const float db = processor.spectrumDb[(size_t) k].load();
        const float y = r.getBottom() - juce::jlimit (0.0f, 1.0f, (db - minDb) / -minDb) * r.getHeight();
        const float x = xFor (hz, r);
        started ? spec.lineTo (x, y) : spec.startNewSubPath (x, y);
        started = true;
    }
    g.setColour (colours::text.withAlpha (0.85f));
    g.strokePath (spec, juce::PathStrokeType (1.4f));

    // Threshold line inside the range.
    const float ty = r.getBottom() - juce::jlimit (0.0f, 1.0f, (params.thresholdDb - minDb) / -minDb) * r.getHeight();
    g.setColour (colours::warn);
    const float dash[] = { 4.0f, 4.0f };
    g.drawDashedLine (juce::Line<float> (xl, ty, xh, ty), dash, 2, 1.5f);

    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText (tr ("arrastra el rango", "drag the range"), r.reduced (4.0f, 2.0f), juce::Justification::topRight);
}

void SpectrumDisplay::mouseMove (const juce::MouseEvent& e)
{
    const auto r = plotArea();
    const auto params = processor.readParams();
    const float xl = xFor (params.lowHz, r), xh = xFor (params.highHz, r), x = (float) e.x;
    if (std::fabs (x - xl) < 6.0f || std::fabs (x - xh) < 6.0f)
        setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
    else if (x > xl && x < xh)
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    else
        setMouseCursor (juce::MouseCursor::NormalCursor);
}

void SpectrumDisplay::mouseDown (const juce::MouseEvent& e)
{
    if (lowParam == nullptr || highParam == nullptr) return;
    const auto r = plotArea();
    const auto params = processor.readParams();
    dragLow = params.lowHz;
    dragHigh = params.highHz;
    const float xl = xFor (dragLow, r), xh = xFor (dragHigh, r), x = (float) e.x;
    if (std::fabs (x - xl) < 6.0f) drag = Drag::low;
    else if (std::fabs (x - xh) < 6.0f) drag = Drag::high;
    else if (x > xl && x < xh) drag = Drag::both;
    else drag = Drag::none;
    if (drag == Drag::low || drag == Drag::both) lowParam->beginChangeGesture();
    if (drag == Drag::high || drag == Drag::both) highParam->beginChangeGesture();
}

void SpectrumDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (drag == Drag::none) return;
    const auto r = plotArea();
    const float octaves = (float) e.getDistanceFromDragStartX() / r.getWidth() * std::log2 (maxHz / minHz);
    const float factor = std::pow (2.0f, octaves);
    auto set = [] (juce::RangedAudioParameter* p, float hz)
    {
        const auto& range = p->getNormalisableRange();
        p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (range.start, range.end, hz)));
    };
    if (drag == Drag::low) set (lowParam, std::min (dragLow * factor, dragHigh / 1.1f));
    else if (drag == Drag::high) set (highParam, std::max (dragHigh * factor, dragLow * 1.1f));
    else
    {
        set (lowParam, dragLow * factor);
        set (highParam, dragHigh * factor);
    }
}

void SpectrumDisplay::mouseUp (const juce::MouseEvent&)
{
    if (drag == Drag::low || drag == Drag::both) lowParam->endChangeGesture();
    if (drag == Drag::high || drag == Drag::both) highParam->endChangeGesture();
    drag = Drag::none;
}

//==============================================================================
void ReductionMeter::push (float grDb)
{
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
    g.setColour (colours::accent);
    g.fillRoundedRectangle (bar.withHeight (bar.getHeight() * t), 4.0f);
    const float hy = bar.getY() + juce::jlimit (0.0f, 1.0f, holdDb / grMeterRangeDb) * bar.getHeight();
    g.setColour (colours::text);
    g.fillRect (bar.getX(), hy - 1.0f, bar.getWidth(), 2.0f);

    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText ("-" + juce::String (holdDb, 1) + " dB", value, juce::Justification::centred);
}

//==============================================================================
EssCleanEditor::EssCleanEditor (EssCleanProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      low (tr ("DESDE", "FROM")),
      high (tr ("HASTA", "TO")),
      threshold (tr ("UMBRAL", "THRESHOLD")),
      range (tr ("REDUCCIÓN", "RANGE")),
      attack (tr ("ATAQUE", "ATTACK")),
      release ("RELEASE"),
      display (p)
{
    setLookAndFeel (&lnf);

    struct KnobInfo { Knob* knob; const char* id; juce::String tip; };
    const KnobInfo knobs[] = {
        { &low, ParamIDs::low,
          tr ("Donde empieza la zona de la \"s\". Voces graves: 3–4 kHz; agudas: 6 kHz.",
              "Where the \"s\" zone starts. Low voices: 3–4 kHz; high voices: 6 kHz.") },
        { &high, ParamIDs::high,
          tr ("Donde termina la zona de la \"s\". Normalmente 8–12 kHz.",
              "Where the \"s\" zone ends. Usually 8–12 kHz.") },
        { &threshold, ParamIDs::threshold,
          tr ("Nivel de la zona a partir del cual se baja. Bájalo hasta que solo se muevan las \"s\" (mira el medidor).",
              "Level of the zone above which it turns down. Lower it until only the \"s\" sounds move (watch the meter).") },
        { &range, ParamIDs::range,
          tr ("Máximo que puede bajar. 6–10 dB suele ser suficiente; más suena a ceceo.",
              "The most it can turn down. 6–10 dB is usually enough; more sounds lispy.") },
        { &attack, ParamIDs::attack,
          tr ("Qué tan rápido reacciona. Muy rápido (o usa Lookahead) para no dejar pasar el inicio de la \"s\".",
              "How fast it reacts. Very fast (or use Lookahead) so the start of the \"s\" doesn't slip through.") },
        { &release, ParamIDs::release,
          tr ("Qué tan rápido vuelve. Corto = natural en voces rápidas; largo = más suave.",
              "How fast it recovers. Short = natural on fast vocals; long = smoother.") },
    };
    for (const auto& k : knobs)
    {
        addAndMakeVisible (*k.knob);
        k.knob->slider.setTooltip (k.tip);
        sliderAttachments.push_back (std::make_unique<SliderAttachment> (processor.state, k.id, k.knob->slider));
        if (auto* param = processor.state.getParameter (k.id))
            k.knob->slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    }

    modeBox.addItem (tr ("Banda", "Band"), 1);
    modeBox.addItem (tr ("Todo", "Broadband"), 2);
    modeBox.setTooltip (tr ("Banda = solo baja la zona de la \"s\" (transparente). Todo = baja la voz completa un momento (clásico, más notorio).",
                            "Band = only turns down the \"s\" zone (transparent). Broadband = turns the whole voice down for a moment (classic, more audible)."));
    lookaheadBox.addItemList (Choices::lookahead, 1);
    lookaheadBox.setTooltip (tr ("Mira unos milisegundos adelante para bajar la \"s\" desde su primer instante. Añade latencia.",
                                 "Looks a few milliseconds ahead to turn the \"s\" down from its very first instant. Adds latency."));
    struct ComboInfo { juce::ComboBox* box; juce::Label* label; const char* id; juce::String name; };
    const ComboInfo combos[] = {
        { &modeBox, &modeLabel, ParamIDs::mode, tr ("MODO", "MODE") },
        { &lookaheadBox, &lookaheadLabel, ParamIDs::lookahead, "LOOKAHEAD" },
    };
    for (const auto& c : combos)
    {
        addAndMakeVisible (*c.box);
        comboAttachments.push_back (std::make_unique<ComboAttachment> (processor.state, c.id, *c.box));
        c.label->setText (c.name, juce::dontSendNotification);
        c.label->setJustificationType (juce::Justification::centred);
        c.label->setColour (juce::Label::textColourId, colours::textDim);
        c.label->setFont (juce::FontOptions (12.0f, juce::Font::bold));
        addAndMakeVisible (*c.label);
    }

    listen.setButtonText (tr ("ESCUCHAR", "LISTEN"));
    listen.setClickingTogglesState (true);
    listen.setTooltip (tr ("Escucha solo lo que se está quitando, para ajustar el rango. Apágalo después.",
                           "Hear only what is being removed, to set the range. Turn it off afterwards."));
    addAndMakeVisible (listen);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (processor.state, ParamIDs::listen, listen));

    for (int i = 0; i < 3; ++i)
    {
        auto& b = presetButtons[i];
        b.setButtonText (tr (essPresets[i].nameEs, essPresets[i].nameEn));
        b.setTooltip (tr ("Punto de partida según la voz. Luego ajusta el umbral.", "Starting point for the voice type. Then set the threshold."));
        b.onClick = [this, i] { processor.applyPreset (essPresets[i]); };
        addAndMakeVisible (b);
    }

    hint.setText (tr ("Por debajo del umbral no cambia nada. Prueba con voces en español e inglés.",
                      "Below the threshold nothing changes. Try it on Spanish and English vocals."),
                  juce::dontSendNotification);
    hint.setJustificationType (juce::Justification::centred);
    hint.setColour (juce::Label::textColourId, colours::textDim);
    hint.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (hint);

    addAndMakeVisible (display);
    addAndMakeVisible (grMeter);

    setResizable (true, true);
    setResizeLimits (baseWidth * 3 / 4, baseHeight * 3 / 4, baseWidth * 2, baseHeight * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) baseWidth / baseHeight);
    setSize (baseWidth, baseHeight);

    startTimerHz (30);
}

EssCleanEditor::~EssCleanEditor()
{
    setLookAndFeel (nullptr);
}

void EssCleanEditor::paint (juce::Graphics& g)
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

void EssCleanEditor::resized()
{
    const float s = (float) getWidth() / baseWidth;
    auto px = [s] (int v) { return juce::roundToInt ((float) v * s); };

    auto r = getLocalBounds();
    r.removeFromTop (px (48));
    r.reduce (px (16), px (12));

    grMeter.setBounds (r.removeFromRight (px (110)));
    r.removeFromRight (px (12));

    hint.setBounds (r.removeFromBottom (px (18)));
    r.removeFromBottom (px (4));

    auto bottom = r.removeFromBottom (px (62));
    r.removeFromBottom (px (8));
    auto row = bottom.removeFromTop (px (44));
    const int cw = row.getWidth() / 6;
    for (const auto& c : { std::pair { &modeBox, &modeLabel }, std::pair { &lookaheadBox, &lookaheadLabel } })
    {
        auto cell = row.removeFromLeft (cw);
        c.second->setBounds (cell.removeFromTop (px (16)));
        c.first->setBounds (cell.reduced (px (4), px (2)));
    }
    listen.setBounds (row.removeFromLeft (cw).reduced (px (4), px (10)));
    for (auto& b : presetButtons)
        b.setBounds (row.removeFromLeft (cw).reduced (px (4), px (10)));

    display.setBounds (r.removeFromTop (r.getHeight() * 50 / 100));
    r.removeFromTop (px (8));
    const int kw = r.getWidth() / 6;
    low.setBounds (r.removeFromLeft (kw));
    high.setBounds (r.removeFromLeft (kw));
    threshold.setBounds (r.removeFromLeft (kw));
    range.setBounds (r.removeFromLeft (kw));
    attack.setBounds (r.removeFromLeft (kw));
    release.setBounds (r);
}

void EssCleanEditor::timerCallback()
{
    const float gr = processor.reductionDb.exchange (0.0f);
    processor.inputPeak.exchange (0.0f);
    processor.outputPeak.exchange (0.0f);
    grMeter.push (gr);
    display.update();
}
