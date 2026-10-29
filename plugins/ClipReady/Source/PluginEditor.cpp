// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginEditor.h"

using namespace rommuser::ui;

namespace
{
constexpr int baseWidth = 720, baseHeight = 420;
constexpr float meterFloorDb = -48.0f;

float dbToY (float db, juce::Rectangle<float> r, float topDb = 6.0f)
{
    const float t = juce::jlimit (0.0f, 1.0f, (db - meterFloorDb) / (topDb - meterFloorDb));
    return r.getBottom() - t * r.getHeight();
}
} // namespace

//==============================================================================
void TransferCurve::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r, 8.0f);
    r = r.reduced (12.0f);

    // Axes cover input level 0 .. 2x the ceiling (linear), output 0 .. 1.25x the ceiling.
    const float xMax = 2.0f, yMax = 1.25f;
    auto toPoint = [&] (float x, float y)
    {
        return juce::Point<float> (r.getX() + x / xMax * r.getWidth(), r.getBottom() - y / yMax * r.getHeight());
    };

    g.setColour (colours::outline);
    g.drawLine (juce::Line<float> (toPoint (0.0f, 1.0f), toPoint (xMax, 1.0f)), 1.0f);
    g.drawLine (juce::Line<float> (toPoint (1.0f, 0.0f), toPoint (1.0f, yMax)), 1.0f);

    // Unity line, dashed.
    const float dash[] = { 4.0f, 4.0f };
    g.drawDashedLine (juce::Line<float> (toPoint (0.0f, 0.0f), toPoint (yMax, yMax)), dash, 2, 1.0f);

    juce::Path p;
    for (int i = 0; i <= 200; ++i)
    {
        const float x = xMax * (float) i / 200.0f;
        const float y = rommuser::clipready::clipCurve (x, softness);
        const auto pt = toPoint (x, y);
        i == 0 ? p.startNewSubPath (pt) : p.lineTo (pt);
    }
    g.setColour (colours::accent);
    g.strokePath (p, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (tr ("Techo", "Ceiling") + " " + juce::String (ceiling, 1) + " dB",
                r.withHeight (14.0f).translated (0.0f, r.getHeight() * (1.0f - 1.0f / yMax) - 16.0f),
                juce::Justification::centredLeft);
    g.drawText (softness < 0.01f ? tr ("Clip duro", "Hard clip") : tr ("Curva suave", "Soft knee"),
                r.removeFromBottom (14.0f), juce::Justification::centredRight);
}

//==============================================================================
void PeakMeter::push (float linearPeak, float ceilingDb)
{
    const float db = juce::Decibels::gainToDecibels (linearPeak, -100.0f);
    // Instant attack, ~20 dB/s release at 30 fps.
    levelDb = db > levelDb ? db : juce::jmax (db, levelDb - 0.7f);
    if (db >= holdDb)
    {
        holdDb = db;
        holdFrames = 45;
    }
    else if (--holdFrames <= 0)
    {
        holdDb = juce::jmax (levelDb, holdDb - 0.7f);
    }
    ceilingLineDb = ceilingDb;
    repaint();
}

void PeakMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto labelArea = r.removeFromBottom (16.0f);
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (label, labelArea, juce::Justification::centred);

    auto bar = r.reduced (r.getWidth() * 0.25f, 0.0f);
    g.setColour (colours::panel);
    g.fillRoundedRectangle (bar, 3.0f);

    const float y = dbToY (levelDb, bar);
    g.setColour (levelDb > ceilingLineDb + 0.01f ? colours::warn : colour);
    g.fillRoundedRectangle (bar.withTop (y), 3.0f);

    g.setColour (colours::text);
    g.fillRect (bar.withY (dbToY (holdDb, bar) - 1.0f).withHeight (2.0f));

    g.setColour (colours::accent.withAlpha (0.8f));
    g.fillRect (bar.withY (dbToY (ceilingLineDb, bar)).withHeight (1.0f).expanded (3.0f, 0.0f));
}

//==============================================================================
void ClipMeter::push (float db)
{
    valueDb = db > valueDb ? db : juce::jmax (db, valueDb - 0.4f);
    repaint();
}

void ClipMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto labelArea = r.removeFromBottom (16.0f);
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText ("CLIP", labelArea, juce::Justification::centred);

    auto bar = r.reduced (r.getWidth() * 0.25f, 0.0f);
    g.setColour (colours::panel);
    g.fillRoundedRectangle (bar, 3.0f);

    // Grows downwards like gain-reduction meters: 0 at the top, 12 dB at the bottom.
    const float t = juce::jlimit (0.0f, 1.0f, valueDb / 12.0f);
    g.setColour (valueDb > 6.0f ? colours::warn : colours::accent);
    g.fillRoundedRectangle (bar.withHeight (bar.getHeight() * t), 3.0f);

    g.setColour (colours::text);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (juce::String (valueDb, 1), bar.withHeight (16.0f), juce::Justification::centred);
}

//==============================================================================
ClipReadyEditor::ClipReadyEditor (ClipReadyProcessor& p)
    : processor (p),
      input (tr ("ENTRADA", "INPUT")),
      ceiling (tr ("TECHO", "CEILING")),
      softness (tr ("SUAVIDAD", "SOFTNESS")),
      output (tr ("SALIDA", "OUTPUT")),
      mix ("MIX"),
      inMeter (tr ("ENT", "IN"), colours::good),
      outMeter (tr ("SAL", "OUT"), colours::good)
{
    setLookAndFeel (&lnf);

    struct KnobInfo { Knob* knob; const char* id; juce::String tip; };
    const KnobInfo knobs[] = {
        { &input, ParamIDs::input,
          tr ("Cuánto subes la señal antes del clipper. Más entrada = más recorte y más volumen.",
              "How much you push the signal into the clipper. More input = more clipping and more loudness.") },
        { &ceiling, ParamIDs::ceiling,
          tr ("El nivel máximo de salida. Nada pasa de aquí. En el master usa -0.3 dB o menos.",
              "The maximum output level. Nothing goes above it. On the master, use -0.3 dB or lower.") },
        { &softness, ParamIDs::softness,
          tr ("0 % = clip duro (más agresivo). 100 % = curva suave (más cálido, empieza a saturar antes).",
              "0% = hard clip (more aggressive). 100% = soft curve (warmer, starts saturating earlier).") },
        { &output, ParamIDs::output,
          tr ("Volumen final después del clipper.", "Final volume after the clipper.") },
        { &mix, ParamIDs::mix,
          tr ("Mezcla entre la señal original y la recortada (clipping en paralelo).",
              "Blend between the original and the clipped signal (parallel clipping).") },
    };

    for (const auto& k : knobs)
    {
        addAndMakeVisible (*k.knob);
        k.knob->slider.setTooltip (k.tip);
        sliderAttachments.push_back (std::make_unique<SliderAttachment> (processor.state, k.id, k.knob->slider));
        if (auto* param = processor.state.getParameter (k.id))
            k.knob->slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    }

    oversampling.addItemList ({ "1x", "2x", "4x", "8x" }, 1);
    oversampling.setTooltip (tr ("Sobremuestreo: procesa a mayor resolución para que los agudos no se ensucien (aliasing). "
                                 "4x es buen balance; 8x para el master; 1x gasta menos CPU.",
                                 "Oversampling: processes at a higher rate so the highs stay clean (no aliasing). "
                                 "4x is a good balance; 8x for the master; 1x uses the least CPU."));
    addAndMakeVisible (oversampling);
    oversamplingAttachment = std::make_unique<ComboAttachment> (processor.state, ParamIDs::oversampling, oversampling);

    oversamplingLabel.setText (tr ("CALIDAD", "QUALITY"), juce::dontSendNotification);
    oversamplingLabel.setJustificationType (juce::Justification::centred);
    oversamplingLabel.setColour (juce::Label::textColourId, colours::textDim);
    oversamplingLabel.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    addAndMakeVisible (oversamplingLabel);

    struct ButtonInfo { juce::TextButton* b; const char* id; juce::String text, tip; };
    const ButtonInfo buttons[] = {
        { &link, ParamIDs::link, "LINK",
          tr ("Baja la salida lo mismo que subes la entrada, para comparar a igual volumen y no engañarte.",
              "Lowers the output as much as you raise the input, so you compare at equal volume.") },
        { &delta, ParamIDs::delta, "DELTA",
          tr ("Escucha solo lo que el clipper está quitando. Si suena a distorsión fuerte, te pasaste.",
              "Hear only what the clipper removes. If it sounds like heavy distortion, back off.") },
        { &strict, ParamIDs::strict, tr ("TECHO ESTRICTO", "STRICT CEILING"),
          tr ("Encendido: lo que sale del plugin nunca pasa del techo, aunque subas Salida o uses Mix (ideal en el master). "
              "Apagado: 1 ms menos de latencia, pero puede pasar el techo. DELTA no se limita.",
              "On: nothing leaving the plugin goes above the ceiling, even with Output or Mix (best on the master). "
              "Off: 1 ms less latency, but it can go over the ceiling. DELTA is never limited.") },
        { &hq, ParamIDs::hq, "HQ",
          tr ("Antialiasing extra (ADAA): mucho menos aliasing al recortar fuerte, a cambio de un poco más de CPU. "
              "Con HQ, 2x suena tan limpio como 4x sin HQ.",
              "Extra antialiasing (ADAA): far less aliasing when clipping hard, for a little more CPU. "
              "With HQ, 2x is as clean as 4x without it.") },
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
    for (juce::Component* c : { (juce::Component*) &curve, (juce::Component*) &inMeter,
                                (juce::Component*) &outMeter, (juce::Component*) &clipMeter })
        addAndMakeVisible (c);

    // Laid out at its base size; ScalableEditor zooms it as vectors to any window size.
    setSize (baseWidth, baseHeight);

    startTimerHz (30);
}

ClipReadyEditor::~ClipReadyEditor()
{
    setLookAndFeel (nullptr);
}

void ClipReadyEditor::paint (juce::Graphics& g)
{
    g.fillAll (colours::background);

    const float s = (float) getWidth() / baseWidth;
    drawHeader (g, getLocalBounds().removeFromTop (juce::roundToInt (52 * s)), s);
}

void ClipReadyEditor::resized()
{
    const float s = (float) getWidth() / baseWidth;
    auto px = [s] (int v) { return juce::roundToInt ((float) v * s); };

    auto r = getLocalBounds();
    r.removeFromTop (px (52));
    r.reduce (px (20), px (14));

    // Right: meters.
    auto meters = r.removeFromRight (px (150));
    const int mw = meters.getWidth() / 3;
    inMeter.setBounds (meters.removeFromLeft (mw));
    clipMeter.setBounds (meters.removeFromLeft (mw));
    outMeter.setBounds (meters);
    r.removeFromRight (px (16));

    // Bottom: buttons.
    auto bottom = r.removeFromBottom (px (34));
    r.removeFromBottom (px (12));
    const int bw = bottom.getWidth() * 2 / 11;        // four buttons + a 1.5-wide quality cell
    auto osArea = bottom.removeFromLeft (bw * 3 / 2);
    oversamplingLabel.setBounds (osArea.removeFromLeft (osArea.getWidth() / 2));
    oversampling.setBounds (osArea.reduced (px (4), px (3)));
    link.setBounds (bottom.removeFromLeft (bw).reduced (px (4), 0));
    delta.setBounds (bottom.removeFromLeft (bw).reduced (px (4), 0));
    strict.setBounds (bottom.removeFromLeft (bw).reduced (px (4), 0));
    hq.setBounds (bottom.reduced (px (4), 0));

    // Top row: curve + main knobs; second row: utility knobs.
    auto top = r.removeFromTop (r.getHeight() * 55 / 100);
    curve.setBounds (top.removeFromLeft (top.getWidth() * 45 / 100).reduced (0, px (4)));
    top.removeFromLeft (px (12));
    const int kw = top.getWidth() / 2;
    input.setBounds (top.removeFromLeft (kw));
    ceiling.setBounds (top);

    const int kw2 = r.getWidth() / 3;
    softness.setBounds (r.removeFromLeft (kw2));
    mix.setBounds (r.removeFromLeft (kw2));
    output.setBounds (r);
}

void ClipReadyEditor::timerCallback()
{
    const float ceilingDb = processor.state.getRawParameterValue (ParamIDs::ceiling)->load();
    inMeter.push (processor.inputPeak.exchange (0.0f), ceilingDb);
    outMeter.push (processor.outputPeak.exchange (0.0f), ceilingDb);
    clipMeter.push (processor.clipDb.exchange (0.0f));

    curve.setState (processor.state.getRawParameterValue (ParamIDs::softness)->load() / 100.0f,
                    ceilingDb,
                    processor.state.getRawParameterValue (ParamIDs::input)->load());
}
