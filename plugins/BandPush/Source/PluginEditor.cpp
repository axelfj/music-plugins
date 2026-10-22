// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginEditor.h"

using namespace rommuser::ui;
using namespace rommuser::bandpush;

namespace
{
constexpr int baseWidth = 900, baseHeight = 560;
constexpr float meterRangeDb = 24.0f;
} // namespace

//==============================================================================
void BandGainMeter::push (float gainDb)
{
    // Fast towards bigger changes, ~30 dB/s back to 0 at 30 fps.
    if (std::fabs (gainDb) > std::fabs (valueDb))
        valueDb = gainDb;
    else
        valueDb += 0.3f * (gainDb - valueDb);
    repaint();
}

void BandGainMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText (label, r.removeFromTop (18.0f), juce::Justification::centred);
    auto value = r.removeFromBottom (18.0f);
    auto bar = r.reduced (r.getWidth() * 0.35f, 4.0f);
    g.setColour (colours::panel.brighter (0.06f));
    g.fillRoundedRectangle (bar, 4.0f);

    const float midY = bar.getCentreY();
    g.setFont (juce::FontOptions (10.0f));
    for (int db = -24; db <= 24; db += 12)
    {
        const float y = midY - (float) db / meterRangeDb * bar.getHeight() * 0.5f;
        g.setColour (colours::outline.brighter (0.3f));
        g.drawHorizontalLine (juce::roundToInt (y), bar.getRight() + 2.0f, bar.getRight() + 6.0f);
        g.setColour (colours::textDim);
        g.drawText ((db > 0 ? "+" : "") + juce::String (db), juce::Rectangle<float> (bar.getRight() + 8.0f, y - 6.0f, 26.0f, 12.0f), juce::Justification::centredLeft);
    }

    const float t = juce::jlimit (-1.0f, 1.0f, valueDb / meterRangeDb);
    const float h = std::fabs (t) * bar.getHeight() * 0.5f;
    g.setColour (valueDb >= 0.0f ? colours::good : colours::accent);
    g.fillRoundedRectangle (valueDb >= 0.0f ? bar.withTop (midY - h).withHeight (h) : bar.withTop (midY).withHeight (h), 3.0f);
    g.setColour (colours::text);
    g.fillRect (bar.getX(), midY - 1.0f, bar.getWidth(), 2.0f);

    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText ((valueDb > 0.05f ? "+" : "") + juce::String (valueDb, 1) + " dB", value, juce::Justification::centred);
}

//==============================================================================
BandStrip::BandStrip (BandPushProcessor& p) : processor (p)
{
    for (auto& m : meters)
        addAndMakeVisible (m);
}

void BandStrip::setState (const PushParams& p)
{
    params = p;
    repaint();
}

void BandStrip::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r, 8.0f);

    // Crossover frequencies written between the bands.
    const float w = r.getWidth() / 3.0f;
    g.setFont (juce::FontOptions (11.0f));
    const float xo[] = { params.lowXoverHz, params.highXoverHz };
    for (int i = 0; i < 2; ++i)
    {
        const float x = r.getX() + w * (float) (i + 1);
        g.setColour (colours::outline);
        g.drawVerticalLine (juce::roundToInt (x), r.getY() + 8.0f, r.getBottom() - 8.0f);
        g.setColour (colours::textDim);
        const auto text = xo[i] >= 1000.0f ? juce::String (xo[i] / 1000.0f, 2) + " kHz" : juce::String (juce::roundToInt (xo[i])) + " Hz";
        g.drawText (text, juce::Rectangle<float> (x - 40.0f, r.getBottom() - 16.0f, 80.0f, 12.0f), juce::Justification::centred);
    }
}

void BandStrip::resized()
{
    auto r = getLocalBounds().reduced (8);
    const int w = r.getWidth() / 3;
    for (auto& m : meters)
        m.setBounds (r.removeFromLeft (w));
}

//==============================================================================
BandPushEditor::BandPushEditor (BandPushProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      depth (tr ("PROFUNDIDAD", "DEPTH")),
      time (tr ("TIEMPO", "TIME")),
      threshold (tr ("UMBRAL", "THRESHOLD")),
      input (tr ("ENTRADA", "INPUT")),
      output (tr ("SALIDA", "OUTPUT")),
      mix ("MIX"),
      lowXover (tr ("CORTE GRAVE", "LOW SPLIT")),
      highXover (tr ("CORTE AGUDO", "HIGH SPLIT")),
      strip (p)
{
    setLookAndFeel (&lnf);

    struct KnobInfo { Knob* knob; juce::String id; juce::String tip; };
    std::vector<KnobInfo> knobs = {
        { &depth, ParamIDs::depth,
          tr ("La perilla principal. 0 % no hace nada; 100 % aplasta todo (el sonido 'OTT'). Empieza en 30–50 %.",
              "The main knob. 0% does nothing; 100% squashes everything (the 'OTT' sound). Start around 30–50%.") },
        { &time, ParamIDs::time,
          tr ("Escala el ataque y el release de las tres bandas. Menos = más agresivo y con más distorsión; más = más suave.",
              "Scales the attack and release of all three bands. Less = more aggressive with more distortion; more = smoother.") },
        { &threshold, ParamIDs::threshold,
          tr ("Nivel al que se empuja cada banda: lo fuerte baja hasta aquí y lo suave sube hasta aquí.",
              "Level every band is pushed towards: loud parts come down to it, quiet parts come up to it.") },
        { &input, ParamIDs::input,
          tr ("Ganancia antes del procesado. Subirla es como bajar el umbral.",
              "Gain before processing. Turning it up is like lowering the threshold.") },
        { &output, ParamIDs::output,
          tr ("Ganancia después del procesado.", "Gain after processing.") },
        { &mix, ParamIDs::mix,
          tr ("Mezcla con la señal sin procesar (las bandas sumadas). 100 % = solo procesada.",
              "Blend with the unprocessed signal (the summed bands). 100% = processed only.") },
        { &lowXover, ParamIDs::lowXover,
          tr ("Frecuencia que separa graves y medios.", "Frequency that splits lows from mids.") },
        { &highXover, ParamIDs::highXover,
          tr ("Frecuencia que separa medios y agudos.", "Frequency that splits mids from highs.") },
    };
    const char* bandEs[] = { "graves", "medios", "agudos" };
    const char* bandEn[] = { "lows", "mids", "highs" };
    auto u = [] (const char* text) { return juce::String::fromUTF8 (text); };
    auto trs = [] (const juce::String& es, const juce::String& en) { return isSpanish() ? es : en; };
    for (int b = 0; b < numBands; ++b)
    {
        bandKnobs.push_back (std::make_unique<Knob> (tr ("ARRIBA", "UP")));
        knobs.push_back ({ bandKnobs.back().get(), ParamIDs::band (b, "up"),
                           trs (u ("Compresión hacia arriba de los ") + u (bandEs[b]) + u (": cuánto se suben las partes suaves."),
                                u ("Upward compression of the ") + u (bandEn[b]) + u (": how much the quiet parts are pulled up.")) });
        bandKnobs.push_back (std::make_unique<Knob> (tr ("ABAJO", "DOWN")));
        knobs.push_back ({ bandKnobs.back().get(), ParamIDs::band (b, "down"),
                           trs (u ("Compresión hacia abajo de los ") + u (bandEs[b]) + u (": cuánto se bajan las partes fuertes."),
                                u ("Downward compression of the ") + u (bandEn[b]) + u (": how much the loud parts are pushed down.")) });
        bandKnobs.push_back (std::make_unique<Knob> (tr ("GANANCIA", "GAIN")));
        knobs.push_back ({ bandKnobs.back().get(), ParamIDs::band (b, "gain"),
                           trs (u ("Volumen de los ") + u (bandEs[b]) + u (" después de comprimir."),
                                u ("Level of the ") + u (bandEn[b]) + u (" after compression.")) });
    }
    for (const auto& k : knobs)
    {
        addAndMakeVisible (*k.knob);
        k.knob->slider.setTooltip (k.tip);
        sliderAttachments.push_back (std::make_unique<SliderAttachment> (processor.state, k.id, k.knob->slider));
        if (auto* param = processor.state.getParameter (k.id))
            k.knob->slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    }

    addAndMakeVisible (strip);

    setResizable (true, true);
    setResizeLimits (baseWidth * 3 / 4, baseHeight * 3 / 4, baseWidth * 2, baseHeight * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) baseWidth / baseHeight);
    setSize (baseWidth, baseHeight);

    startTimerHz (30);
}

BandPushEditor::~BandPushEditor()
{
    setLookAndFeel (nullptr);
}

void BandPushEditor::paint (juce::Graphics& g)
{
    g.fillAll (colours::background);

    const float s = (float) getWidth() / baseWidth;
    auto header = getLocalBounds().removeFromTop (juce::roundToInt (48 * s)).reduced (juce::roundToInt (20 * s), 0);

    g.setColour (colours::text);
    g.setFont (juce::FontOptions (24.0f * s, juce::Font::bold));
    g.drawText ("BAND PUSH", header, juce::Justification::centredLeft);

    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (12.0f * s));
    g.drawText (juce::String ("ROMMUSER  ·  v") + PLUGIN_VERSION_STRING + "  ·  " + tr ("gratis y open source", "free & open source"),
                header, juce::Justification::centredRight);

    g.setColour (colours::outline);
    g.drawHorizontalLine (header.getBottom(), 0.0f, (float) getWidth());
}

void BandPushEditor::resized()
{
    const float s = (float) getWidth() / baseWidth;
    auto px = [s] (int v) { return juce::roundToInt ((float) v * s); };

    auto r = getLocalBounds();
    r.removeFromTop (px (48));
    r.reduce (px (16), px (12));

    // Top row: the global knobs.
    auto top = r.removeFromTop (px (130));
    r.removeFromTop (px (8));
    const int gw = top.getWidth() / 8;
    for (Knob* k : { &depth, &time, &threshold, &input, &output, &mix, &lowXover, &highXover })
        k->setBounds (top.removeFromLeft (gw));

    // Middle: band meters. Bottom: three columns of band knobs.
    strip.setBounds (r.removeFromTop (px (150)));
    r.removeFromTop (px (8));
    const int cw = r.getWidth() / 3;
    for (int b = 0; b < numBands; ++b)
    {
        auto col = r.removeFromLeft (cw).reduced (px (8), 0);
        const int kw = col.getWidth() / 3;
        for (int k = 0; k < 3; ++k)
            bandKnobs[(size_t) (b * 3 + k)]->setBounds (col.removeFromLeft (kw));
    }
}

void BandPushEditor::timerCallback()
{
    for (int b = 0; b < numBands; ++b)
        strip.meters[b].push (processor.bandGainDb[b].load());
    strip.setState (processor.readParams());
}
