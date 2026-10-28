// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginEditor.h"

using namespace rommuser::ui;
using namespace rommuser::shimmerready;

namespace
{
constexpr int baseWidth = 980, baseHeight = 600;

juce::String noteName (double hz)
{
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    if (hz <= 0.0) return {};
    const int midi = juce::roundToInt (69.0 + 12.0 * std::log2 (hz / 440.0));
    return juce::String (names[((midi % 12) + 12) % 12]) + juce::String (midi / 12 - 1);
}
} // namespace

//==============================================================================
EchoDisplay::EchoDisplay (ShimmerReadyProcessor& p) : processor (p) {}

void EchoDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r, 8.0f);
    auto plot = r.reduced (14.0f, 12.0f);
    auto header = plot.removeFromTop (16.0f);
    plot.removeFromBottom (14.0f);

    const auto params = processor.readParams();
    const float delayMs = processor.delayMs.load();
    const float fb = params.freeze ? 1.0f : std::min (1.0f, params.feedback);
    const int repeats = 8;
    const float totalMs = delayMs * (repeats + 0.5f);
    g.setColour (colours::background);
    g.fillRect (plot);

    // Dry pulse at t = 0.
    auto xFor = [&] (float ms) { return plot.getX() + 16.0f + (plot.getWidth() - 32.0f) * (ms / std::max (1.0f, totalMs)); };
    g.setColour (colours::textDim);
    g.fillRoundedRectangle (juce::Rectangle<float> (xFor (0.0f) - 4.0f, plot.getY() + 6.0f, 8.0f, plot.getHeight() - 6.0f), 2.0f);

    float level = 1.0f;
    const bool pitched = std::fabs (params.pitchSemis) > 0.5f;
    for (int k = 1; k <= repeats; ++k)
    {
        if (k > 1) level *= fb;
        if (level < 0.01f) break;
        const float h = (plot.getHeight() - 6.0f) * level;
        const float x = xFor (delayMs * (float) k);
        auto bar = juce::Rectangle<float> (x - 5.0f, plot.getBottom() - h, 10.0f, h);
        g.setColour (colours::accent.withAlpha (0.35f + 0.65f * level));
        g.fillRoundedRectangle (bar, 2.0f);
        if (pitched && params.pitchSemis > -24.5f)
        {
            const float semis = params.pitchSemis * (float) (k - 1);
            g.setColour (colours::text.withAlpha (0.4f + 0.6f * level));
            g.setFont (juce::FontOptions (10.0f));
            g.drawText ((semis > 0.0f ? "+" : "") + juce::String (juce::roundToInt (semis)), juce::Rectangle<float> (x - 20.0f, plot.getBottom() + 1.0f, 40.0f, 12.0f), juce::Justification::centred);
        }
    }
    // Shimmer tail hint.
    if (params.shimmer > 0.01f)
    {
        juce::Path tail;
        tail.startNewSubPath (xFor (delayMs), plot.getBottom() - (plot.getHeight() - 6.0f) * 0.6f * params.shimmer);
        for (int i = 1; i <= 40; ++i)
        {
            const float t = (float) i / 40.0f;
            const float ms = delayMs + t * (totalMs - delayMs);
            const float lvl = 0.6f * params.shimmer * std::exp (-3.0f * (ms - delayMs) * 0.001f / std::max (0.5f, params.decaySeconds) * 2.3f);
            tail.lineTo (xFor (ms), plot.getBottom() - (plot.getHeight() - 6.0f) * lvl);
        }
        g.setColour (colours::good.withAlpha (0.8f));
        g.strokePath (tail, juce::PathStrokeType (1.5f));
    }
    g.setColour (colours::outline);
    g.drawRect (plot, 1.0f);

    // Live wet level.
    const float wet = juce::jlimit (0.0f, 1.0f, processor.wetPeak.load());
    g.setColour (colours::accent);
    g.fillRect (juce::Rectangle<float> (plot.getRight() - 6.0f, plot.getBottom() - plot.getHeight() * wet, 4.0f, plot.getHeight() * wet));

    juce::String info;
    info << tr ("retardo", "delay") << " " << juce::String (delayMs, 1) << " ms";
    if (params.sync) info << " (" << divisionNames[params.division] << juce::String::fromUTF8 (" · ") << juce::String (juce::roundToInt (processor.hostBpm.load())) << " BPM)";
    const float hz = processor.inputHz.load();
    if (hz > 0.0f) info << juce::String::fromUTF8 ("   ·   ") << tr ("tono en el bucle", "loop pitch") << " " << noteName (hz) << " " << juce::String (hz, 1) << " Hz";
    if (params.freeze) info << juce::String::fromUTF8 ("   ·   FREEZE");
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (info, header, juce::Justification::centredLeft);
}

//==============================================================================
ShimmerReadyEditor::ShimmerReadyEditor (ShimmerReadyProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      display (p),
      time (tr ("TIEMPO", "TIME")),
      pitch (tr ("TONO", "PITCH")),
      grain (tr ("GRANO", "GRAIN")),
      feedback ("FEEDBACK"),
      lowCut (tr ("GRAVES", "LOW CUT")),
      highCut (tr ("AGUDOS", "HIGH CUT")),
      shimmer ("SHIMMER"),
      decay (tr ("COLA", "DECAY")),
      width (tr ("ANCHO", "WIDTH")),
      mix ("MIX"),
      output (tr ("SALIDA", "OUTPUT"))
{
    setLookAndFeel (&lnf);

    struct KnobInfo { Knob* knob; const char* id; juce::String tip; };
    const KnobInfo knobs[] = {
        { &time, ParamIDs::time, tr ("Tiempo libre del eco (con SYNC apagado).", "Free echo time (with SYNC off).") },
        { &pitch, ParamIDs::pitch, tr ("Semitonos que sube o baja cada repetición respecto a la anterior. +12 = cada eco una octava más arriba (shimmer). 0 = delay normal.",
                                       "Semitones each repeat climbs or drops from the previous one. +12 = every echo an octave up (shimmer). 0 = plain delay.") },
        { &grain, ParamIDs::grain, tr ("Tamaño de los granos del cambio de tono. Pequeño = más preciso en transitorios; grande = más suave en pads. Se ajusta solo al tono que detecta.",
                                       "Grain size of the pitch shift. Small = tighter on transients; big = smoother on pads. It locks itself to the detected pitch.") },
        { &feedback, ParamIDs::feedback, tr ("Cuánto vuelve al bucle. Más de 100 % crece hasta el limitador suave.", "How much goes back round the loop. Above 100% it grows up to the soft limiter.") },
        { &lowCut, ParamIDs::lowCut, tr ("Quita graves en cada vuelta del bucle.", "Removes lows on every pass round the loop.") },
        { &highCut, ParamIDs::highCut, tr ("Quita agudos en cada vuelta: los ecos se oscurecen.", "Removes highs on every pass: the echoes get darker.") },
        { &shimmer, ParamIDs::shimmer, tr ("Reverb añadida a los ecos (motor de Reverb-R).", "Reverb added to the echoes (Reverb-R engine).") },
        { &decay, ParamIDs::decay, tr ("Duración de la cola de la reverb.", "Length of the reverb tail.") },
        { &width, ParamIDs::width, tr ("Desplaza el eco derecho hasta un 15 % para abrir el estéreo.", "Offsets the right echo by up to 15% to widen the stereo.") },
        { &mix, ParamIDs::mix, tr ("Mezcla con la señal original.", "Blend with the original signal.") },
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

    struct ToggleInfo { juce::TextButton* b; const char* id; juce::String text, tip; };
    const ToggleInfo toggles[] = {
        { &syncButton, ParamIDs::sync, "SYNC", tr ("Tiempo del eco en divisiones del tempo del DAW.", "Echo time in divisions of the DAW tempo.") },
        { &reverseButton, ParamIDs::reverse, tr ("REVERSA", "REVERSE"), tr ("Cada eco suena al revés.", "Every echo plays backwards.") },
        { &freezeButton, ParamIDs::freeze, "FREEZE", tr ("Congela el bucle: deja de entrar señal y lo que hay se repite sin perder nivel.", "Freezes the loop: no new input and what is inside repeats without losing level.") },
    };
    for (const auto& t : toggles)
    {
        t.b->setButtonText (t.text);
        t.b->setClickingTogglesState (true);
        t.b->setTooltip (t.tip);
        addAndMakeVisible (*t.b);
        buttonAttachments.push_back (std::make_unique<ButtonAttachment> (processor.state, t.id, *t.b));
    }

    divisionBox.addItemList (Choices::divisions, 1);
    divisionBox.setTooltip (tr ("División del tempo (T = tresillo, D = con puntillo).", "Tempo division (T = triplet, D = dotted)."));
    addAndMakeVisible (divisionBox);
    comboAttachments.push_back (std::make_unique<ComboAttachment> (processor.state, ParamIDs::division, divisionBox));
    divisionLabel.setText (tr ("DIVISIÓN", "DIVISION"), juce::dontSendNotification);
    divisionLabel.setJustificationType (juce::Justification::centred);
    divisionLabel.setColour (juce::Label::textColourId, colours::textDim);
    divisionLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    addAndMakeVisible (divisionLabel);

    for (int i = 0; i < 6; ++i)
    {
        auto& b = presetButtons[i];
        b.setButtonText (tr (shimmerPresets[i].nameEs, shimmerPresets[i].nameEn));
        b.setTooltip (tr ("Punto de partida.", "Starting point."));
        b.onClick = [this, i] { processor.applyPreset (shimmerPresets[i]); };
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

ShimmerReadyEditor::~ShimmerReadyEditor()
{
    setLookAndFeel (nullptr);
}

void ShimmerReadyEditor::paint (juce::Graphics& g)
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

void ShimmerReadyEditor::resized()
{
    const float s = (float) getWidth() / baseWidth;
    auto px = [s] (int v) { return juce::roundToInt ((float) v * s); };

    auto r = getLocalBounds();
    r.removeFromTop (px (48));
    r.reduce (px (16), px (10));

    auto presets = r.removeFromBottom (px (36));
    r.removeFromBottom (px (6));
    const int pw = presets.getWidth() / 6;
    for (auto& b : presetButtons)
        b.setBounds (presets.removeFromLeft (pw).reduced (px (4), px (2)));

    // Two knob rows.
    auto row2 = r.removeFromBottom (px (104));
    r.removeFromBottom (px (6));
    auto row1 = r.removeFromBottom (px (104));
    r.removeFromBottom (px (8));

    // Row 1: SYNC | division | time | pitch | grain | feedback | REVERSE
    const int cw = row1.getWidth() / 7;
    {
        auto cell = row1.removeFromLeft (cw);
        cell.removeFromTop (px (40));
        syncButton.setBounds (cell.removeFromTop (px (26)).reduced (px (10), 0));
    }
    {
        auto cell = row1.removeFromLeft (cw);
        cell.removeFromTop (px (24));
        divisionLabel.setBounds (cell.removeFromTop (px (16)));
        divisionBox.setBounds (cell.removeFromTop (px (26)).reduced (px (10), 0));
    }
    time.setBounds (row1.removeFromLeft (cw));
    pitch.setBounds (row1.removeFromLeft (cw));
    grain.setBounds (row1.removeFromLeft (cw));
    feedback.setBounds (row1.removeFromLeft (cw));
    {
        auto cell = row1;
        cell.removeFromTop (px (40));
        reverseButton.setBounds (cell.removeFromTop (px (26)).reduced (px (10), 0));
    }
    // Row 2: low cut | high cut | FREEZE | shimmer | decay | width | mix | output
    const int cw2 = row2.getWidth() / 8;
    lowCut.setBounds (row2.removeFromLeft (cw2));
    highCut.setBounds (row2.removeFromLeft (cw2));
    {
        auto cell = row2.removeFromLeft (cw2);
        cell.removeFromTop (px (40));
        freezeButton.setBounds (cell.removeFromTop (px (26)).reduced (px (10), 0));
    }
    shimmer.setBounds (row2.removeFromLeft (cw2));
    decay.setBounds (row2.removeFromLeft (cw2));
    width.setBounds (row2.removeFromLeft (cw2));
    mix.setBounds (row2.removeFromLeft (cw2));
    output.setBounds (row2);

    display.setBounds (r);
}

void ShimmerReadyEditor::updateEnablement()
{
    const bool sync = processor.state.getRawParameterValue (ParamIDs::sync)->load() > 0.5f;
    divisionBox.setAlpha (sync ? 1.0f : 0.4f);
    divisionLabel.setAlpha (sync ? 1.0f : 0.4f);
    time.setAlpha (sync ? 0.4f : 1.0f);
    const bool shimmerOn = processor.state.getRawParameterValue (ParamIDs::shimmer)->load() > 0.5f;
    decay.setAlpha (shimmerOn ? 1.0f : 0.4f);
}

void ShimmerReadyEditor::timerCallback()
{
    updateEnablement();
    display.repaint();
}
