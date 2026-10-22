// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginEditor.h"

using namespace rommuser::ui;
using namespace rommuser::peakready;

namespace
{
constexpr int baseWidth = 920, baseHeight = 540;
constexpr float grRangeDb = 18.0f;
constexpr float lufsMin = -36.0f, lufsMax = 0.0f;

juce::String lufsText (float v) { return v <= -99.0f ? juce::String ("—") : juce::String (v, 1); }
} // namespace

//==============================================================================
void LimiterDisplay::push (float gainReductionDb, float outputPeakDb, float ceiling)
{
    reductions[(size_t) head] = gainReductionDb;
    peaks[(size_t) head] = outputPeakDb;
    head = (head + 1) % historyLength;
    ceilingDb = ceiling;
    repaint();
}

void LimiterDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r, 8.0f);
    r = r.reduced (12.0f, 10.0f);

    const float dx = r.getWidth() / (float) (historyLength - 1);
    auto idx = [this] (int i) { return (size_t) ((head + i) % historyLength); };

    // Grid: 0, -6, -12, -18 dB of reduction from the top.
    g.setFont (juce::FontOptions (10.0f));
    for (float db = 0.0f; db <= grRangeDb; db += 6.0f)
    {
        const float y = r.getY() + db / grRangeDb * r.getHeight();
        g.setColour (colours::outline);
        g.drawHorizontalLine (juce::roundToInt (y), r.getX(), r.getRight());
        g.setColour (colours::textDim);
        g.drawText (juce::String (-(int) db), juce::Rectangle<float> (r.getRight() - 24.0f, y - 11.0f, 24.0f, 11.0f), juce::Justification::centredRight);
    }

    // Reduction as a filled area hanging from the top.
    juce::Path gr;
    gr.startNewSubPath (r.getX(), r.getY());
    for (int i = 0; i < historyLength; ++i)
        gr.lineTo (r.getX() + (float) i * dx, r.getY() + juce::jlimit (0.0f, 1.0f, reductions[idx (i)] / grRangeDb) * r.getHeight());
    gr.lineTo (r.getRight(), r.getY());
    gr.closeSubPath();
    g.setColour (colours::accent.withAlpha (0.55f));
    g.fillPath (gr);

    // Output peak relative to the ceiling (0 at the ceiling line near the bottom).
    const float ceilY = r.getBottom() - r.getHeight() * 0.12f;
    g.setColour (colours::warn.withAlpha (0.7f));
    g.drawHorizontalLine (juce::roundToInt (ceilY), r.getX(), r.getRight());
    juce::Path pk;
    for (int i = 0; i < historyLength; ++i)
    {
        const float below = juce::jlimit (0.0f, 30.0f, ceilingDb - peaks[idx (i)]);   // dB under the ceiling
        const float y = ceilY + below / 30.0f * (r.getBottom() - ceilY);
        const float x = r.getX() + (float) i * dx;
        i == 0 ? pk.startNewSubPath (x, y) : pk.lineTo (x, y);
    }
    g.setColour (colours::good);
    g.strokePath (pk, juce::PathStrokeType (1.5f));

    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (tr ("reducción", "reduction"), r.reduced (4.0f, 2.0f), juce::Justification::topLeft);
    g.drawText (tr ("techo", "ceiling"), juce::Rectangle<float> (r.getX() + 4.0f, ceilY - 13.0f, 80.0f, 12.0f), juce::Justification::centredLeft);
}

//==============================================================================
void LoudnessMeter::set (float momentary, float shortTerm, float integrated, float t)
{
    m = momentary; s = shortTerm; i = integrated; target = t;
    repaint();
}

void LoudnessMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText ("LUFS", r.removeFromTop (18.0f), juce::Justification::centred);

    auto footer = r.removeFromBottom (34.0f);
    auto bars = r.reduced (4.0f, 4.0f);
    const float bw = bars.getWidth() / 3.0f;
    const struct { float v; const char* es; const char* en; } cols[] = { { m, "M", "M" }, { s, "C", "S" }, { i, "I", "I" } };
    auto yFor = [&] (float lufs) { return bars.getBottom() - juce::jlimit (0.0f, 1.0f, (lufs - lufsMin) / (lufsMax - lufsMin)) * (bars.getHeight() - 14.0f); };

    for (int k = 0; k < 3; ++k)
    {
        auto col = juce::Rectangle<float> (bars.getX() + bw * (float) k, bars.getY(), bw, bars.getHeight()).reduced (5.0f, 0.0f);
        auto bar = col.withTrimmedTop (14.0f);
        g.setColour (colours::panel);
        g.fillRoundedRectangle (bar, 3.0f);
        const float y = yFor (cols[k].v);
        g.setColour (cols[k].v > target + 1.0f ? colours::warn : colours::accent);
        g.fillRoundedRectangle (bar.withTop (y), 3.0f);
        g.setColour (colours::textDim);
        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.drawText (tr (cols[k].es, cols[k].en), col.removeFromTop (12.0f), juce::Justification::centred);
    }
    // Target line.
    const float ty = yFor (target);
    g.setColour (colours::text);
    g.fillRect (bars.getX(), ty - 1.0f, bars.getWidth(), 2.0f);

    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.setColour (colours::text);
    g.drawText (tr ("I: ", "I: ") + lufsText (i), footer.removeFromTop (16.0f), juce::Justification::centred);
    g.setFont (juce::FontOptions (10.0f));
    g.setColour (colours::textDim);
    if (i > -99.0f)
    {
        const float diff = target - i;
        g.drawText ((diff >= 0.0f ? "+" : "") + juce::String (diff, 1) + tr (" dB para llegar", " dB to target"), footer, juce::Justification::centred);
    }
}

//==============================================================================
PeakReadyEditor::PeakReadyEditor (PeakReadyProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      input (tr ("ENTRADA", "INPUT")),
      ceiling (tr ("TECHO", "CEILING")),
      release ("RELEASE"),
      target (tr ("OBJETIVO", "TARGET"))
{
    setLookAndFeel (&lnf);

    struct KnobInfo { Knob* knob; const char* id; juce::String tip; };
    const KnobInfo knobs[] = {
        { &input, ParamIDs::input,
          tr ("Cuánto empujas la mezcla contra el techo. Sube hasta que el medidor LUFS llegue al objetivo sin que la reducción pase de 3–6 dB.",
              "How hard you push the mix into the ceiling. Raise it until the LUFS meter hits the target without more than 3–6 dB of reduction.") },
        { &ceiling, ParamIDs::ceiling,
          tr ("Nivel máximo real de salida (true peak). −1 dB es seguro para streaming.",
              "Maximum true-peak output level. −1 dB is safe for streaming.") },
        { &release, ParamIDs::release,
          tr ("Qué tan rápido vuelve después de limitar. Corto = más fuerte pero puede distorsionar graves; con Auto se adapta solo.",
              "How fast it recovers after limiting. Short = louder but may distort lows; with Auto it adapts by itself.") },
        { &target, ParamIDs::target,
          tr ("Volumen objetivo. Streaming: −14 LUFS. Club / EDM: −8 a −6 LUFS.",
              "Loudness target. Streaming: −14 LUFS. Club / EDM: −8 to −6 LUFS.") },
    };
    for (const auto& k : knobs)
    {
        addAndMakeVisible (*k.knob);
        k.knob->slider.setTooltip (k.tip);
        sliderAttachments.push_back (std::make_unique<SliderAttachment> (processor.state, k.id, k.knob->slider));
        if (auto* param = processor.state.getParameter (k.id))
            k.knob->slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    }

    styleBox.addItem (tr ("Transparente", "Transparent"), 1);
    styleBox.addItem ("Punchy", 2);
    styleBox.addItem (tr ("Agresivo", "Aggressive"), 3);
    styleBox.setTooltip (tr ("Transparente: lookahead largo, limpio. Punchy: más corto, deja respirar los golpes. Agresivo: muy corto + clipper, para EDM fuerte.",
                             "Transparent: long lookahead, clean. Punchy: shorter, lets the hits breathe. Aggressive: very short + clipper, for loud EDM."));
    addAndMakeVisible (styleBox);
    comboAttachments.push_back (std::make_unique<ComboAttachment> (processor.state, ParamIDs::style, styleBox));
    styleLabel.setText (tr ("ESTILO", "STYLE"), juce::dontSendNotification);
    styleLabel.setJustificationType (juce::Justification::centred);
    styleLabel.setColour (juce::Label::textColourId, colours::textDim);
    styleLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    addAndMakeVisible (styleLabel);

    struct ButtonInfo { juce::TextButton* b; const char* id; juce::String text, tip; };
    const ButtonInfo buttons[] = {
        { &autoRelease, ParamIDs::autoRelease, "AUTO",
          tr ("El release se adapta a cuánto se está limitando.", "The release adapts to how much limiting is happening.") },
        { &truePeak, ParamIDs::truePeak, "TRUE PEAK",
          tr ("Mide los picos entre muestras (4x). Apágalo solo para ahorrar CPU; añade un poco de latencia.",
              "Measures the peaks between samples (4x). Turn it off only to save CPU; adds a little latency.") },
    };
    for (const auto& b : buttons)
    {
        b.b->setButtonText (b.text);
        b.b->setClickingTogglesState (true);
        b.b->setTooltip (b.tip);
        addAndMakeVisible (*b.b);
        buttonAttachments.push_back (std::make_unique<ButtonAttachment> (processor.state, b.id, *b.b));
    }

    resetLoudness.setButtonText (tr ("REINICIAR LUFS", "RESET LUFS"));
    resetLoudness.setTooltip (tr ("Empieza de cero la medición integrada (hazlo al inicio de la canción).",
                                  "Restart the integrated measurement (do it at the start of the song)."));
    resetLoudness.onClick = [this] { processor.requestLoudnessReset(); };
    addAndMakeVisible (resetLoudness);

    for (int i = 0; i < 3; ++i)
    {
        auto& b = presetButtons[i];
        b.setButtonText (tr (peakPresets[i].nameEs, peakPresets[i].nameEn));
        b.setTooltip (tr ("Techo, objetivo y estilo para ese destino. Luego sube Entrada hasta llegar al objetivo.",
                          "Ceiling, target and style for that destination. Then raise Input until you reach the target."));
        b.onClick = [this, i] { processor.applyPreset (peakPresets[i]); };
        addAndMakeVisible (b);
    }

    grLabel.setJustificationType (juce::Justification::centred);
    grLabel.setColour (juce::Label::textColourId, colours::text);
    grLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    addAndMakeVisible (grLabel);

    hint.setText (tr ("El pico real de salida nunca pasa del techo. Sube Entrada hasta el objetivo LUFS.",
                      "The output true peak never passes the ceiling. Raise Input until you reach the LUFS target."),
                  juce::dontSendNotification);
    hint.setJustificationType (juce::Justification::centred);
    hint.setColour (juce::Label::textColourId, colours::textDim);
    hint.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (hint);

    addAndMakeVisible (display);
    addAndMakeVisible (loudness);

    setResizable (true, true);
    setResizeLimits (baseWidth * 3 / 4, baseHeight * 3 / 4, baseWidth * 2, baseHeight * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) baseWidth / baseHeight);
    setSize (baseWidth, baseHeight);

    startTimerHz (30);
}

PeakReadyEditor::~PeakReadyEditor()
{
    setLookAndFeel (nullptr);
}

void PeakReadyEditor::paint (juce::Graphics& g)
{
    g.fillAll (colours::background);

    const float s = (float) getWidth() / baseWidth;
    auto header = getLocalBounds().removeFromTop (juce::roundToInt (48 * s)).reduced (juce::roundToInt (20 * s), 0);

    g.setColour (colours::text);
    g.setFont (juce::FontOptions (24.0f * s, juce::Font::bold));
    g.drawText ("PEAK READY", header, juce::Justification::centredLeft);

    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (12.0f * s));
    g.drawText (juce::String::fromUTF8 ("ROMMUSER  ·  v") + PLUGIN_VERSION_STRING + juce::String::fromUTF8 ("  ·  ") + tr ("gratis y open source", "free & open source"),
                header, juce::Justification::centredRight);

    g.setColour (colours::outline);
    g.drawHorizontalLine (header.getBottom(), 0.0f, (float) getWidth());
}

void PeakReadyEditor::resized()
{
    const float s = (float) getWidth() / baseWidth;
    auto px = [s] (int v) { return juce::roundToInt ((float) v * s); };

    auto r = getLocalBounds();
    r.removeFromTop (px (48));
    r.reduce (px (16), px (12));

    loudness.setBounds (r.removeFromRight (px (150)));
    r.removeFromRight (px (12));

    hint.setBounds (r.removeFromBottom (px (18)));
    r.removeFromBottom (px (4));

    auto bottom = r.removeFromBottom (px (62));
    r.removeFromBottom (px (8));
    auto row = bottom.removeFromTop (px (44));
    const int cw = row.getWidth() / 7;
    {
        auto cell = row.removeFromLeft (cw);
        styleLabel.setBounds (cell.removeFromTop (px (16)));
        styleBox.setBounds (cell.reduced (px (4), px (2)));
    }
    autoRelease.setBounds (row.removeFromLeft (cw).reduced (px (4), px (10)));
    truePeak.setBounds (row.removeFromLeft (cw).reduced (px (4), px (10)));
    resetLoudness.setBounds (row.removeFromLeft (cw).reduced (px (4), px (10)));
    for (auto& b : presetButtons)
        b.setBounds (row.removeFromLeft (cw).reduced (px (4), px (10)));

    display.setBounds (r.removeFromTop (r.getHeight() * 46 / 100));
    r.removeFromTop (px (4));
    grLabel.setBounds (r.removeFromTop (px (20)));
    r.removeFromTop (px (4));
    const int kw = r.getWidth() / 4;
    input.setBounds (r.removeFromLeft (kw));
    ceiling.setBounds (r.removeFromLeft (kw));
    release.setBounds (r.removeFromLeft (kw));
    target.setBounds (r);
}

void PeakReadyEditor::timerCallback()
{
    const float gr = processor.gainReductionDb.exchange (0.0f);
    const float outPeak = processor.outputPeak.exchange (0.0f);
    processor.inputPeak.exchange (0.0f);
    const auto params = processor.readParams();
    display.push (gr, outPeak > 1.0e-6f ? 20.0f * std::log10 (outPeak) : -120.0f, params.ceilingDb);

    if (gr >= grHold) { grHold = gr; grHoldFrames = 45; }
    else if (--grHoldFrames <= 0) grHold = juce::jmax (gr, grHold - 0.5f);
    grLabel.setText (tr ("Reducción: ", "Reduction: ") + juce::String (grHold, 1) + " dB", juce::dontSendNotification);

    float targetLufs = -9.0f;
    if (auto* t = processor.state.getRawParameterValue (ParamIDs::target))
        targetLufs = t->load();
    loudness.set (processor.momentaryLufs.load(), processor.shortTermLufs.load(), processor.integratedLufs.load(), targetLufs);
}
