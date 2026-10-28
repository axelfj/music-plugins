// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginEditor.h"

using namespace rommuser::ui;
using namespace rommuser::tuneready;

namespace
{
constexpr int baseWidth = 980, baseHeight = 640;

juce::String noteName (int midi)
{
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return juce::String (names[((midi % 12) + 12) % 12]) + juce::String (midi / 12 - 1);
}
} // namespace

//==============================================================================
PitchDisplay::PitchDisplay (TuneReadyProcessor& p) : processor (p) {}

void PitchDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r, 8.0f);
    auto plot = r.reduced (14.0f, 12.0f);
    plot.removeFromTop (16.0f);
    auto labels = plot.removeFromLeft (34.0f);

    // Vertical range: follow the recent pitch, two octaves tall, snapped to a C.
    const int n = TuneReadyProcessor::historyLength;
    const int w = processor.writeIndex.load();
    float recent = 0.0f;
    for (int k = 1; k <= n && recent <= 0.0f; ++k)
        recent = processor.detectedHistory[(size_t) ((w - k + n) % n)].load();
    if (recent <= 0.0f) recent = 57.0f;   // A3 when nothing has been heard yet
    const int bottomNote = (int) std::floor ((recent - 12.0f) / 12.0f) * 12;   // a C
    const int topNote = bottomNote + 24;
    auto yFor = [&] (float midi) { return plot.getBottom() - (midi - (float) bottomNote) / (float) (topNote - bottomNote) * plot.getHeight(); };

    const auto params = processor.readParams();
    const uint16_t mask = scaleMask (params.scale, params.customMask);
    const float rowH = plot.getHeight() / 24.0f;
    for (int note = bottomNote; note < topNote; ++note)
    {
        const int pc = ((note - params.key) % 12 + 12) % 12;
        const bool inScale = (mask >> pc) & 1;
        const bool white = ! (note % 12 == 1 || note % 12 == 3 || note % 12 == 6 || note % 12 == 8 || note % 12 == 10);
        auto row = juce::Rectangle<float> (plot.getX(), yFor ((float) note + 1.0f), plot.getWidth(), rowH);
        g.setColour (white ? colours::background.brighter (0.06f) : colours::background);
        g.fillRect (row);
        if (inScale && params.mode == Mode::correct)
        {
            g.setColour (colours::accent.withAlpha (0.10f));
            g.fillRect (row);
        }
        if (note % 12 == 0)
        {
            g.setColour (colours::textDim);
            g.setFont (juce::FontOptions (10.0f));
            g.drawText (noteName (note), juce::Rectangle<float> (labels.getX(), row.getY(), labels.getWidth() - 4.0f, rowH), juce::Justification::centredRight);
        }
    }
    g.setColour (colours::outline);
    g.drawRect (plot, 1.0f);

    // Traces, newest at the right.
    auto trace = [&] (const std::array<std::atomic<float>, TuneReadyProcessor::historyLength>& hist, juce::Colour colour, float width)
    {
        juce::Path path;
        bool open = false;
        for (int k = 0; k < n; ++k)
        {
            const float midi = hist[(size_t) ((w + k) % n)].load();
            const float x = plot.getX() + (float) k / (float) (n - 1) * plot.getWidth();
            if (midi <= 0.0f) { open = false; continue; }
            const float y = juce::jlimit (plot.getY(), plot.getBottom(), yFor (midi + 0.5f));
            if (! open) { path.startNewSubPath (x, y); open = true; }
            else path.lineTo (x, y);
        }
        g.setColour (colour);
        g.strokePath (path, juce::PathStrokeType (width));
    };
    trace (processor.detectedHistory, colours::textDim.withAlpha (0.8f), 1.2f);
    trace (processor.correctedHistory, colours::accent, 2.0f);

    // Header: what we hear and what we do.
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (11.0f));
    juce::String info;
    if (processor.voiced.load())
    {
        const float hz = processor.inputHz.load();
        const float midi = (float) hzToMidi (hz);
        const int nearest = juce::roundToInt (midi);
        info << tr ("entrada", "input") << " " << noteName (nearest) << " " << juce::String (hz, 1) << " Hz ("
             << (midi - (float) nearest >= 0.0f ? "+" : "") << juce::String (juce::roundToInt ((midi - (float) nearest) * 100.0f)) << " c)"
             << juce::String::fromUTF8 ("   ·   ") << tr ("corrección", "shift") << " " << (processor.shiftSemis.load() >= 0.0f ? "+" : "")
             << juce::String (processor.shiftSemis.load() * 100.0f, 0) << " c";
    }
    else
        info << tr ("sin tono (silencio o consonante): la señal pasa sin cambios", "no pitch (silence or consonant): signal passes unchanged");
    if (processor.midiHeld.load())
        info << juce::String::fromUTF8 ("   ·   MIDI");
    g.drawText (info, juce::Rectangle<float> (plot.getX(), r.getY() + 6.0f, plot.getWidth(), 14.0f), juce::Justification::centredLeft);
    g.setColour (colours::accent);
    g.drawText (tr ("— corregido", "— corrected"), juce::Rectangle<float> (plot.getRight() - 160.0f, r.getY() + 6.0f, 80.0f, 14.0f), juce::Justification::centredRight);
    g.setColour (colours::textDim);
    g.drawText (tr ("— detectado", "— detected"), juce::Rectangle<float> (plot.getRight() - 80.0f, r.getY() + 6.0f, 80.0f, 14.0f), juce::Justification::centredRight);
}

//==============================================================================
TuneReadyEditor::TuneReadyEditor (TuneReadyProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      speed (tr ("VELOCIDAD", "SPEED")),
      amount (tr ("CANTIDAD", "AMOUNT")),
      humanize ("HUMANIZE"),
      fixedPitch (tr ("TONO", "PITCH")),
      formant (tr ("FORMANTE", "FORMANT")),
      mix ("MIX"),
      output (tr ("SALIDA", "OUTPUT")),
      display (p)
{
    setLookAndFeel (&lnf);

    struct KnobInfo { Knob* knob; const char* id; juce::String tip; };
    const KnobInfo knobs[] = {
        { &speed, ParamIDs::speed, tr ("Qué tan rápido llega a la nota. 0 ms = efecto robótico (T-Pain); 50-150 ms = natural.", "How fast it reaches the note. 0 ms = robotic (T-Pain); 50-150 ms = natural.") },
        { &amount, ParamIDs::amount, tr ("Cuánto de la corrección aplicar. 100 % = exacto a la nota.", "How much of the correction to apply. 100% = exactly on the note.") },
        { &humanize, ParamIDs::humanize, tr ("Deriva lenta y aleatoria de la nota objetivo, en cents, para que no suene perfecto.", "Slow random drift of the target note, in cents, so it does not sound perfect.") },
        { &fixedPitch, ParamIDs::fixedPitch, tr ("Modo Fijo: transpone la voz estos semitonos, sin corregir.", "Fixed mode: transposes the voice by this many semitones, no correction.") },
        { &formant, ParamIDs::formant, tr ("Tamaño de la voz: negativo = más grande / grave, positivo = más pequeña / aguda. No cambia la nota.", "Voice size: negative = bigger / deeper, positive = smaller / brighter. Does not change the note.") },
        { &mix, ParamIDs::mix, tr ("Mezcla con la voz original (para doblar).", "Blend with the original voice (for doubling).") },
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

    struct ComboInfo { juce::ComboBox* box; juce::Label* label; const char* id; juce::StringArray items; juce::String name, tip; };
    const ComboInfo combos[] = {
        { &modeBox, &modeLabel, ParamIDs::mode, { tr ("Corregir", "Correct"), tr ("Fijo", "Fixed") }, tr ("MODO", "MODE"),
          tr ("Corregir: lleva cada nota a la escala. Fijo: transpone y cambia formantes (voz grave, aguda, robot).",
              "Correct: pulls every note to the scale. Fixed: transposes and changes formants (deep, high, robot voice).") },
        { &keyBox, &keyLabel, ParamIDs::key, Choices::keys, tr ("TONALIDAD", "KEY"), tr ("Nota raíz de la escala.", "Root note of the scale.") },
        { &scaleBox, &scaleLabel, ParamIDs::scale, { tr ("Cromática", "Chromatic"), tr ("Mayor", "Major"), tr ("Menor", "Minor"), tr ("Personalizada", "Custom") }, tr ("ESCALA", "SCALE"),
          tr ("Cromática: cualquier semitono. Mayor / Menor: solo las notas de la tonalidad. Personalizada: elige las notas abajo.",
              "Chromatic: any semitone. Major / Minor: only the notes of the key. Custom: pick the notes below.") },
        { &rangeBox, &rangeLabel, ParamIDs::range, { tr ("Voz grave (50 Hz)", "Low voice (50 Hz)"), tr ("Normal (60 Hz)", "Normal (60 Hz)"), tr ("Voz aguda (80 Hz)", "High voice (80 Hz)"), tr ("Baja latencia (110 Hz)", "Low latency (110 Hz)") }, tr ("RANGO", "RANGE"),
          tr ("Nota más grave que sigue. Más alta = menos latencia (unos 30 ms con 110 Hz, unos 60 ms con 50 Hz).",
              "Lowest note it tracks. Higher = less latency (about 30 ms at 110 Hz, about 60 ms at 50 Hz).") },
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

    notesLabel.setText (tr ("NOTAS PERMITIDAS", "ALLOWED NOTES"), juce::dontSendNotification);
    notesLabel.setJustificationType (juce::Justification::centred);
    notesLabel.setColour (juce::Label::textColourId, colours::textDim);
    notesLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    addAndMakeVisible (notesLabel);
    for (int i = 0; i < 12; ++i)
    {
        auto& b = pitchClassButtons[i];
        b.setButtonText (Choices::keys[i]);
        b.setClickingTogglesState (true);
        b.setTooltip (tr ("Escala personalizada: nota permitida (relativa a la tonalidad).", "Custom scale: allowed note (relative to the key)."));
        addAndMakeVisible (b);
        buttonAttachments.push_back (std::make_unique<ButtonAttachment> (processor.state, ParamIDs::pitchClass[i], b));
    }

    midiButton.setButtonText ("MIDI");
    midiButton.setClickingTogglesState (true);
    midiButton.setTooltip (tr ("Las notas MIDI que mantengas pulsadas mandan: la voz se lleva a esa nota exacta.",
                               "Held MIDI notes take over: the voice is pulled to that exact note."));
    addAndMakeVisible (midiButton);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (processor.state, ParamIDs::midiPriority, midiButton));

    for (int i = 0; i < 4; ++i)
    {
        auto& b = presetButtons[i];
        b.setButtonText (tr (tunePresets[i].nameEs, tunePresets[i].nameEn));
        b.setTooltip (tr ("Punto de partida.", "Starting point."));
        b.onClick = [this, i] { processor.applyPreset (tunePresets[i]); };
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

TuneReadyEditor::~TuneReadyEditor()
{
    setLookAndFeel (nullptr);
}

void TuneReadyEditor::paint (juce::Graphics& g)
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

void TuneReadyEditor::resized()
{
    const float s = (float) getWidth() / baseWidth;
    auto px = [s] (int v) { return juce::roundToInt ((float) v * s); };

    auto r = getLocalBounds();
    r.removeFromTop (px (48));
    r.reduce (px (16), px (10));

    auto presets = r.removeFromBottom (px (36));
    r.removeFromBottom (px (6));
    const int pw = presets.getWidth() / 4;
    for (auto& b : presetButtons)
        b.setBounds (presets.removeFromLeft (pw).reduced (px (4), px (2)));

    display.setBounds (r.removeFromTop (r.getHeight() * 44 / 100));
    r.removeFromTop (px (8));

    // Row: mode | key | scale | 12 note buttons | range | MIDI
    auto row = r.removeFromTop (px (56));
    r.removeFromTop (px (4));
    const int cw = row.getWidth() / 20;
    for (const auto& c : { std::pair { &modeBox, &modeLabel }, std::pair { &keyBox, &keyLabel }, std::pair { &scaleBox, &scaleLabel } })
    {
        auto cell = row.removeFromLeft (cw * 2);
        c.second->setBounds (cell.removeFromTop (px (16)));
        c.first->setBounds (cell.removeFromTop (px (26)).reduced (px (4), 0));
    }
    {
        auto cell = row.removeFromLeft (cw * 10);
        notesLabel.setBounds (cell.removeFromTop (px (16)));
        auto buttons = cell.removeFromTop (px (26));
        const int bw = buttons.getWidth() / 12;
        for (auto& b : pitchClassButtons)
            b.setBounds (buttons.removeFromLeft (bw).reduced (px (1), 0));
    }
    {
        auto cell = row.removeFromLeft (cw * 3);
        rangeLabel.setBounds (cell.removeFromTop (px (16)));
        rangeBox.setBounds (cell.removeFromTop (px (26)).reduced (px (4), 0));
    }
    {
        auto cell = row;
        cell.removeFromTop (px (16));
        midiButton.setBounds (cell.removeFromTop (px (26)).reduced (px (6), 0));
    }

    // Knobs row.
    const int kw = r.getWidth() / 7;
    speed.setBounds (r.removeFromLeft (kw));
    amount.setBounds (r.removeFromLeft (kw));
    humanize.setBounds (r.removeFromLeft (kw));
    fixedPitch.setBounds (r.removeFromLeft (kw));
    formant.setBounds (r.removeFromLeft (kw));
    mix.setBounds (r.removeFromLeft (kw));
    output.setBounds (r);
}

void TuneReadyEditor::updateEnablement()
{
    const bool fixed = juce::roundToInt (processor.state.getRawParameterValue (ParamIDs::mode)->load()) == 1;
    const bool custom = juce::roundToInt (processor.state.getRawParameterValue (ParamIDs::scale)->load()) == 3;
    for (auto* k : { &speed, &amount, &humanize })
        k->setAlpha (fixed ? 0.4f : 1.0f);
    fixedPitch.setAlpha (fixed ? 1.0f : 0.4f);
    keyBox.setAlpha (fixed ? 0.4f : 1.0f);
    scaleBox.setAlpha (fixed ? 0.4f : 1.0f);
    midiButton.setAlpha (fixed ? 0.4f : 1.0f);
    const bool notesActive = custom && ! fixed;
    for (auto& b : pitchClassButtons)
    {
        b.setEnabled (notesActive);
        b.setAlpha (notesActive ? 1.0f : 0.4f);
    }
}

void TuneReadyEditor::timerCallback()
{
    updateEnablement();
    display.repaint();
}
