// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginEditor.h"

using namespace rommuser::ui;
using namespace rommuser::spaceready;

namespace
{
constexpr int baseWidth = 960, baseHeight = 600;
} // namespace

//==============================================================================
void TailDisplay::setState (const SpaceParams& p, float rms, float duck)
{
    params = p;
    wetRms = rms;
    duckGain = duck;
    history.push_back (rms);
    if (history.size() > 150) history.erase (history.begin());
    repaint();
}

void TailDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r, 8.0f);
    auto plot = r.reduced (14.0f, 12.0f);
    auto title = plot.removeFromTop (14.0f);
    plot.removeFromBottom (16.0f);

    // Time axis: 0 .. max (1 s, 1.5 x decay), capped at 30 s.
    const float span = params.freeze ? 5.0f : std::min (30.0f, std::max (1.0f, params.decaySeconds * 1.5f));
    const float pre = params.preDelayMs * 0.001f;
    auto xFor = [&] (float t) { return plot.getX() + t / span * plot.getWidth(); };
    auto yForDb = [&] (float db) { return plot.getY() + juce::jlimit (0.0f, 1.0f, -db / 72.0f) * plot.getHeight(); };

    g.setColour (colours::outline);
    for (float db : { -12.0f, -24.0f, -36.0f, -48.0f, -60.0f })
        g.drawHorizontalLine (juce::roundToInt (yForDb (db)), plot.getX(), plot.getRight());
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("-60 dB", juce::Rectangle<float> (plot.getX(), yForDb (-60.0f) - 12.0f, 40.0f, 12.0f), juce::Justification::centredLeft);
    const float tickStep = span <= 2.0f ? 0.25f : span <= 6.0f ? 1.0f : span <= 15.0f ? 2.0f : 5.0f;
    for (float t = 0.0f; t <= span + 1e-3f; t += tickStep)
    {
        g.setColour (colours::outline);
        g.drawVerticalLine (juce::roundToInt (xFor (t)), plot.getY(), plot.getBottom());
        g.setColour (colours::textDim);
        g.drawText (juce::String (t, t < 1.0f && tickStep < 1.0f ? 2 : 0) + " s", juce::Rectangle<float> (xFor (t) - 20.0f, plot.getBottom() + 2.0f, 40.0f, 12.0f), juce::Justification::centred);
    }

    // The envelope: silence during the pre-delay, then -60 dB over Decay seconds.
    juce::Path env;
    env.startNewSubPath (xFor (0.0f), plot.getBottom());
    env.lineTo (xFor (pre), plot.getBottom());
    env.lineTo (xFor (pre), yForDb (0.0f));
    const int points = 120;
    for (int i = 1; i <= points; ++i)
    {
        const float t = pre + (span - pre) * (float) i / points;
        const float db = params.freeze ? 0.0f : -60.0f * (t - pre) / std::max (0.05f, params.decaySeconds);
        env.lineTo (xFor (t), yForDb (db));
    }
    g.setColour (colours::accent.withAlpha (0.25f));
    juce::Path fill = env;
    fill.lineTo (xFor (span), plot.getBottom());
    fill.closeSubPath();
    g.fillPath (fill);
    g.setColour (colours::accent);
    g.strokePath (env, juce::PathStrokeType (2.0f));

    // Live wet level history (right-aligned, newest at the right edge).
    if (history.size() > 1)
    {
        juce::Path live;
        const float w = plot.getWidth() * 0.35f;
        for (size_t i = 0; i < history.size(); ++i)
        {
            const float x = plot.getRight() - w + w * (float) i / (float) (history.size() - 1);
            const float db = 20.0f * std::log10 (std::max (history[i], 1e-5f));
            const float y = yForDb (db);
            if (i == 0) live.startNewSubPath (x, y); else live.lineTo (x, y);
        }
        g.setColour (colours::good.withAlpha (0.9f));
        g.strokePath (live, juce::PathStrokeType (1.5f));
    }

    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (11.0f));
    juce::String label = Choices::modes[(int) params.mode] + juce::String::fromUTF8 ("  ·  ") + tr ("tamaño ", "size ") + juce::String (juce::roundToInt (params.size * 100.0f)) + juce::String::fromUTF8 (" %  ·  ");
    label << (params.freeze ? tr ("FREEZE", "FREEZE") : juce::String ("RT60 ") + juce::String (params.decaySeconds, params.decaySeconds < 10.0f ? 2 : 1) + " s");
    g.drawText (label, title, juce::Justification::topLeft);
    g.drawText (tr ("nivel de la cola", "tail level"), title, juce::Justification::topRight);
    if (params.duck > 0.001f && duckGain < 0.98f)
    {
        g.setColour (colours::good);
        g.drawText (tr ("ducking ", "ducking ") + juce::String (juce::roundToInt ((1.0f - duckGain) * 100.0f)) + " %",
                    juce::Rectangle<float> (plot.getRight() - 120.0f, plot.getY() + 4.0f, 120.0f, 14.0f), juce::Justification::centredRight);
    }
}

//==============================================================================
SpaceReadyEditor::SpaceReadyEditor (SpaceReadyProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      size (tr ("TAMAÑO", "SIZE")),
      decay (tr ("DECAY", "DECAY")),
      preDelay ("PRE-DELAY"),
      damping (tr ("AMORTIGUACIÓN", "DAMPING")),
      diffusion (tr ("DIFUSIÓN", "DIFFUSION")),
      modulation (tr ("MODULACIÓN", "MODULATION")),
      width (tr ("ANCHO", "WIDTH")),
      lowCut (tr ("CORTE GRAVES", "LOW CUT")),
      highCut (tr ("CORTE AGUDOS", "HIGH CUT")),
      duck ("DUCKING"),
      mix ("MIX"),
      output (tr ("SALIDA", "OUTPUT"))
{
    setLookAndFeel (&lnf);

    struct KnobInfo { Knob* knob; const char* id; juce::String tip; };
    const KnobInfo knobs[] = {
        { &size, ParamIDs::size, tr ("Tamaño del espacio: cambia las distancias de los rebotes.", "Size of the space: changes the distances of the reflections.") },
        { &decay, ParamIDs::decay, tr ("Cuánto tarda la cola en bajar 60 dB (RT60). Medido en los tests: ±15 %.", "How long the tail takes to fall 60 dB (RT60). Measured in the tests: ±15%.") },
        { &preDelay, ParamIDs::preDelay, tr ("Silencio antes de que empiece la reverb: separa la voz de la cola.", "Silence before the reverb starts: separates the voice from the tail.") },
        { &damping, ParamIDs::damping, tr ("Los agudos mueren antes que los graves, como en una sala real. Más bajo = más oscuro.",
                                           "Highs die before lows, like in a real room. Lower = darker.") },
        { &diffusion, ParamIDs::diffusion, tr ("Emborrona las primeras reflexiones. Bajo = ecos sueltos, alto = nube densa.", "Smears the first reflections. Low = distinct echoes, high = dense cloud.") },
        { &modulation, ParamIDs::modulation, tr ("Vaivén lento de las líneas: evita el sonido metálico y da vida al tail.", "Slow drift of the lines: avoids metallic ringing and brings the tail to life.") },
        { &width, ParamIDs::width, tr ("Ancho estéreo de la cola. 0 = mono.", "Stereo width of the tail. 0 = mono.") },
        { &lowCut, ParamIDs::lowCut, tr ("Quita graves de la reverb para que no ensucie el bajo.", "Removes lows from the reverb so it doesn't muddy the bass.") },
        { &highCut, ParamIDs::highCut, tr ("Quita agudos de la reverb para que no pise la voz.", "Removes highs from the reverb so it doesn't step on the voice.") },
        { &duck, ParamIDs::duck, tr ("Baja la reverb mientras la señal original suena y la sube en los huecos.", "Turns the reverb down while the original plays and up in the gaps.") },
        { &mix, ParamIDs::mix, tr ("Mezcla entre la señal original y la reverb.", "Blend between the original and the reverb.") },
        { &output, ParamIDs::output, tr ("Ganancia final.", "Final gain.") },
    };
    for (const auto& k : knobs)
    {
        addAndMakeVisible (*k.knob);
        k.knob->slider.setTooltip (k.tip);
        sliderAttachments.push_back (std::make_unique<SliderAttachment> (processor.state, k.id, k.knob->slider));
        if (auto* param = processor.state.getParameter (k.id))
            k.knob->slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    }

    modeBox.addItemList (Choices::modes, 1);
    modeBox.setTooltip (tr ("Room: sala pequeña. Plate: brillante y denso para voces. Hall: sala grande. Ambience: espacio enorme, estilo supermassive.",
                            "Room: small room. Plate: bright and dense for vocals. Hall: large hall. Ambience: huge space, supermassive style."));
    addAndMakeVisible (modeBox);
    comboAttachments.push_back (std::make_unique<ComboAttachment> (processor.state, ParamIDs::mode, modeBox));
    modeLabel.setText (tr ("MODO", "MODE"), juce::dontSendNotification);
    modeLabel.setJustificationType (juce::Justification::centred);
    modeLabel.setColour (juce::Label::textColourId, colours::textDim);
    modeLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    addAndMakeVisible (modeLabel);

    freeze.setButtonText ("FREEZE");
    freeze.setClickingTogglesState (true);
    freeze.setTooltip (tr ("Congela la cola para siempre y silencia la entrada nueva.", "Holds the tail forever and mutes new input."));
    addAndMakeVisible (freeze);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (processor.state, ParamIDs::freeze, freeze));

    for (int i = 0; i < 4; ++i)
    {
        auto& b = presetButtons[i];
        b.setButtonText (tr (spacePresets[i].nameEs, spacePresets[i].nameEn));
        b.setTooltip (tr ("Punto de partida.", "Starting point."));
        b.onClick = [this, i] { processor.applyPreset (spacePresets[i]); };
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

SpaceReadyEditor::~SpaceReadyEditor()
{
    setLookAndFeel (nullptr);
}

void SpaceReadyEditor::paint (juce::Graphics& g)
{
    g.fillAll (colours::background);

    const float s = (float) getWidth() / baseWidth;
    auto header = getLocalBounds().removeFromTop (juce::roundToInt (48 * s)).reduced (juce::roundToInt (20 * s), 0);

    g.setColour (colours::text);
    g.setFont (juce::FontOptions (24.0f * s, juce::Font::bold));
    g.drawText (juce::String (JucePlugin_Name).toUpperCase(), header, juce::Justification::centredLeft);

    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (12.0f * s));
    g.drawText (juce::String::fromUTF8 ("ROMMUSER  ·  v") + PLUGIN_VERSION_STRING + juce::String::fromUTF8 ("  ·  ") + tr ("gratis y open source", "free & open source"),
                header, juce::Justification::centredRight);

    g.setColour (colours::outline);
    g.drawHorizontalLine (header.getBottom(), 0.0f, (float) getWidth());
}

void SpaceReadyEditor::resized()
{
    const float s = (float) getWidth() / baseWidth;
    auto px = [s] (int v) { return juce::roundToInt ((float) v * s); };

    auto r = getLocalBounds();
    r.removeFromTop (px (48));
    r.reduce (px (16), px (12));

    // Bottom strip: mode, freeze, presets.
    auto bottom = r.removeFromBottom (px (62));
    r.removeFromBottom (px (8));
    auto row = bottom.removeFromTop (px (44));
    const int cw = row.getWidth() / 6;
    {
        auto cell = row.removeFromLeft (cw);
        modeLabel.setBounds (cell.removeFromTop (px (16)));
        modeBox.setBounds (cell.reduced (px (4), px (2)));
    }
    freeze.setBounds (row.removeFromLeft (cw).reduced (px (4), px (10)));
    for (auto& b : presetButtons)
        b.setBounds (row.removeFromLeft (cw).reduced (px (4), px (10)));

    // Top: tail display. Then two rows of 6 knobs.
    display.setBounds (r.removeFromTop (r.getHeight() * 34 / 100));
    r.removeFromTop (px (8));
    auto row1 = r.removeFromTop (r.getHeight() / 2);
    const int kw = row1.getWidth() / 6;
    for (auto* k : { &size, &decay, &preDelay, &damping, &diffusion })
        k->setBounds (row1.removeFromLeft (kw));
    modulation.setBounds (row1);
    for (auto* k : { &width, &lowCut, &highCut, &duck, &mix })
        k->setBounds (r.removeFromLeft (kw));
    output.setBounds (r);
}

void SpaceReadyEditor::timerCallback()
{
    const float rms = processor.wetRms.load();
    wetSmoothed = rms > wetSmoothed ? rms : wetSmoothed * 0.85f + rms * 0.15f;
    display.setState (processor.readParams(), wetSmoothed, processor.duckGain.load());
}
