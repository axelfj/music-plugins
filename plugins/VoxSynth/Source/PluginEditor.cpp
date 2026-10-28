// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginEditor.h"

using namespace rommuser::ui;
using namespace rommuser::voxsynth;

namespace
{
constexpr int baseWidth = 1040, baseHeight = 760;

juce::String noteName (int midi)
{
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return juce::String (names[((midi % 12) + 12) % 12]) + juce::String (midi / 12 - 1);
}

juce::Colour voiceColour (int v)
{
    switch (v)
    {
        case 0: return colours::accent;
        case 1: return colours::good;
        case 2: return colours::warn;
        default: return colours::accent.brighter (0.6f);
    }
}

void styleLabel (juce::Label& l, const juce::String& text)
{
    l.setText (text, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centred);
    l.setColour (juce::Label::textColourId, colours::textDim);
    l.setFont (juce::FontOptions (12.0f, juce::Font::bold));
}
} // namespace

//==============================================================================
VoxDisplay::VoxDisplay (VoxSynthProcessor& p) : processor (p) {}

void VoxDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r, 8.0f);
    auto plot = r.reduced (14.0f, 12.0f);
    auto header = plot.removeFromTop (16.0f);

    const int mode = juce::roundToInt (processor.state.getRawParameterValue (ParamIDs::mode)->load());
    if (mode == 1)
        paintBands (g, plot, header);
    else
        paintPitch (g, plot, header, mode == 2);
}

void VoxDisplay::paintPitch (juce::Graphics& g, juce::Rectangle<float> plot, juce::Rectangle<float> header, bool robot)
{
    auto labels = plot.removeFromLeft (34.0f);

    const int n = VoxSynthProcessor::historyLength;
    const int w = processor.writeIndex.load();
    float recent = 0.0f;
    for (int k = 1; k <= n && recent <= 0.0f; ++k)
        recent = processor.detectedHistory[(size_t) ((w - k + n) % n)].load();
    if (recent <= 0.0f) recent = 57.0f;
    const int bottomNote = (int) std::floor ((recent - 18.0f) / 12.0f) * 12;
    const int topNote = bottomNote + 36;
    auto yFor = [&] (float midi) { return plot.getBottom() - (midi - (float) bottomNote) / (float) (topNote - bottomNote) * plot.getHeight(); };

    const auto params = processor.readParams();
    const uint16_t mask = rommuser::dsp::psola::scaleMask (params.scale, params.customMask);
    const float rowH = plot.getHeight() / 36.0f;
    for (int note = bottomNote; note < topNote; ++note)
    {
        const int pc = ((note - params.key) % 12 + 12) % 12;
        const bool inScale = (mask >> pc) & 1;
        const bool white = ! (note % 12 == 1 || note % 12 == 3 || note % 12 == 6 || note % 12 == 8 || note % 12 == 10);
        auto row = juce::Rectangle<float> (plot.getX(), yFor ((float) note + 1.0f), plot.getWidth(), rowH);
        g.setColour (white ? colours::background.brighter (0.06f) : colours::background);
        g.fillRect (row);
        if (inScale && ! robot && params.intervalMode == IntervalMode::scale)
        {
            g.setColour (colours::accent.withAlpha (0.08f));
            g.fillRect (row);
        }
        if (note % 12 == 0)
        {
            g.setColour (colours::textDim);
            g.setFont (juce::FontOptions (10.0f));
            g.drawText (noteName (note), juce::Rectangle<float> (labels.getX(), row.getY() - rowH, labels.getWidth() - 4.0f, rowH * 3.0f), juce::Justification::centredRight);
        }
    }
    g.setColour (colours::outline);
    g.drawRect (plot, 1.0f);

    auto trace = [&] (const std::array<std::atomic<float>, VoxSynthProcessor::historyLength>& hist, juce::Colour colour, float width)
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
    const int shown = robot ? 1 : maxVoices;
    for (int v = 0; v < shown; ++v)
        if (robot || params.voices[(size_t) v].enabled)
            trace (processor.voiceHistory[(size_t) v], voiceColour (v), 2.0f);

    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (11.0f));
    juce::String info;
    if (processor.voiced.load())
    {
        const float hz = processor.inputHz.load();
        const float midi = (float) rommuser::dsp::psola::hzToMidi (hz);
        info << tr ("entrada", "input") << " " << noteName (juce::roundToInt (midi)) << " " << juce::String (hz, 1) << " Hz";
    }
    else
        info << tr ("sin tono: la voz pasa tal cual", "no pitch: the voice passes unchanged");
    if (processor.midiHeld.load())
        info << juce::String::fromUTF8 ("   ·   MIDI");
    g.drawText (info, header.withTrimmedRight (300.0f), juce::Justification::centredLeft);

    auto legend = header.removeFromRight (robot ? 160.0f : 300.0f);
    g.setColour (colours::textDim);
    g.drawText (tr ("— voz", "— voice"), legend.removeFromLeft (60.0f), juce::Justification::centredRight);
    for (int v = 0; v < shown; ++v)
    {
        g.setColour (voiceColour (v));
        g.drawText (robot ? tr ("— robot", "— robot") : "— " + juce::String (v + 1), legend.removeFromLeft (robot ? 100.0f : 60.0f), juce::Justification::centredRight);
    }
}

void VoxDisplay::paintBands (juce::Graphics& g, juce::Rectangle<float> plot, juce::Rectangle<float> header)
{
    g.setColour (colours::background);
    g.fillRect (plot);
    g.setColour (colours::outline);
    g.drawRect (plot, 1.0f);

    const int bandCount = juce::jlimit (8, maxBands, juce::roundToInt (processor.state.getRawParameterValue (ParamIDs::bands)->load()));
    const float bw = plot.getWidth() / (float) bandCount;
    for (int b = 0; b < bandCount; ++b)
    {
        const float level = juce::jlimit (0.0f, 1.0f, processor.bandLevels[(size_t) b].load());
        const float db = level > 1.0e-4f ? 20.0f * std::log10 (level) : -80.0f;
        const float h = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f) * (plot.getHeight() - 4.0f);
        auto bar = juce::Rectangle<float> (plot.getX() + (float) b * bw + 2.0f, plot.getBottom() - 2.0f - h, bw - 4.0f, h);
        g.setColour (colours::accent.withAlpha (0.35f + 0.65f * (h / plot.getHeight())));
        g.fillRoundedRectangle (bar, 2.0f);
    }
    // Band frequency labels at the bottom.
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (10.0f));
    const float lo = 100.0f, hi = 8000.0f;
    for (int b = 0; b < bandCount; b += std::max (1, bandCount / 8))
    {
        const float f = lo * std::pow (hi / lo, ((float) b + 0.5f) / (float) bandCount);
        const juce::String text = f >= 1000.0f ? juce::String (f / 1000.0f, 1) + "k" : juce::String (juce::roundToInt (f));
        g.drawText (text, juce::Rectangle<float> (plot.getX() + (float) b * bw, plot.getBottom() - 16.0f, bw * 2.0f, 14.0f), juce::Justification::centredLeft);
    }

    juce::String info;
    const float carrier = processor.carrierHz.load();
    info << tr ("bandas", "bands") << " " << bandCount;
    if (processor.midiHeld.load())
        info << juce::String::fromUTF8 ("   ·   ") << tr ("portadora: MIDI", "carrier: MIDI");
    else if (carrier > 0.0f)
        info << juce::String::fromUTF8 ("   ·   ") << tr ("portadora", "carrier") << " " << noteName (juce::roundToInt ((float) rommuser::dsp::psola::hzToMidi (carrier))) << " " << juce::String (carrier, 1) << " Hz";
    else
        info << juce::String::fromUTF8 ("   ·   ") << tr ("portadora externa (sidechain)", "external carrier (sidechain)");
    if (! processor.voiced.load())
        info << juce::String::fromUTF8 ("   ·   ") << tr ("sin voz", "no voice");
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (info, header, juce::Justification::centredLeft);
}

//==============================================================================
VoxSynthEditor::VoxSynthEditor (VoxSynthProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      glide ("GLIDE"),
      dry ("DRY"),
      mix ("MIX"),
      output (tr ("SALIDA", "OUTPUT")),
      bands (tr ("BANDAS", "BANDS")),
      attack (tr ("ATAQUE", "ATTACK")),
      release ("RELEASE"),
      sibilance (tr ("SIBILANTES", "SIBILANCE")),
      carrierDetune ("DETUNE"),
      robotFormant (tr ("FORMANTE", "FORMANT")),
      display (p)
{
    setLookAndFeel (&lnf);

    auto addKnob = [this] (Knob& knob, const char* id, const juce::String& tip)
    {
        addAndMakeVisible (knob);
        knob.slider.setTooltip (tip);
        sliderAttachments.push_back (std::make_unique<SliderAttachment> (processor.state, id, knob.slider));
        if (auto* param = processor.state.getParameter (id))
            knob.slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    };
    auto addCombo = [this] (juce::ComboBox& box, juce::Label& label, const char* id, const juce::StringArray& items, const juce::String& name, const juce::String& tip)
    {
        box.addItemList (items, 1);
        box.setTooltip (tip);
        addAndMakeVisible (box);
        comboAttachments.push_back (std::make_unique<ComboAttachment> (processor.state, id, box));
        styleLabel (label, name);
        addAndMakeVisible (label);
    };
    auto addToggle = [this] (juce::TextButton& b, const char* id, const juce::String& text, const juce::String& tip)
    {
        b.setButtonText (text);
        b.setClickingTogglesState (true);
        b.setTooltip (tip);
        addAndMakeVisible (b);
        buttonAttachments.push_back (std::make_unique<ButtonAttachment> (processor.state, id, b));
    };

    addCombo (modeBox, modeLabel, ParamIDs::mode, { tr ("Armonías", "Harmony"), "Vocoder", "Robot" }, tr ("MODO", "MODE"),
              tr ("Armonías: hasta 4 voces afinadas a la escala o a MIDI. Vocoder: tu voz 'habla' a través de un sintetizador. Robot: una sola nota fija.",
                  "Harmony: up to 4 voices tuned to the scale or to MIDI. Vocoder: your voice 'speaks' through a synth. Robot: one fixed note."));
    addCombo (rangeBox, rangeLabel, ParamIDs::range, { tr ("Voz grave (50 Hz)", "Low voice (50 Hz)"), "Normal (60 Hz)", tr ("Voz aguda (80 Hz)", "High voice (80 Hz)"), tr ("Baja latencia (110 Hz)", "Low latency (110 Hz)") }, tr ("RANGO", "RANGE"),
              tr ("Nota más grave que sigue. Más alta = menos latencia.", "Lowest note it tracks. Higher = less latency."));
    addCombo (keyBox, keyLabel, ParamIDs::key, Choices::keys, tr ("TONALIDAD", "KEY"), tr ("Nota raíz de la escala.", "Root note of the scale."));
    addCombo (scaleBox, scaleLabel, ParamIDs::scale, { tr ("Cromática", "Chromatic"), tr ("Mayor", "Major"), tr ("Menor", "Minor") }, tr ("ESCALA", "SCALE"),
              tr ("Las armonías por grados se calculan dentro de esta escala.", "Scale-degree harmonies are computed inside this scale."));
    addCombo (intervalBox, intervalLabel, ParamIDs::intervalMode, { tr ("Grados de escala", "Scale degrees"), tr ("Semitonos", "Semitones"), tr ("Notas MIDI", "MIDI notes") }, tr ("INTERVALOS", "INTERVALS"),
              tr ("Grados: tercera, quinta… siempre dentro de la escala (suena bien solo). Semitonos: intervalo fijo. MIDI: cada voz canta la nota que toques en el teclado.",
                  "Degrees: third, fifth… always inside the scale (sounds right by itself). Semitones: fixed interval. MIDI: each voice sings a note you hold on the keyboard."));

    addKnob (glide, ParamIDs::glide, tr ("Suavizado del cambio de nota de las armonías.", "Smoothing of the harmony pitch changes."));
    addKnob (dry, ParamIDs::dry, tr ("Nivel de tu voz original. Al mínimo = solo el efecto (vocoder / robot puro).", "Level of your original voice. Minimum = effect only (pure vocoder / robot)."));
    addKnob (mix, ParamIDs::mix, tr ("Nivel del efecto.", "Effect level."));
    addKnob (output, ParamIDs::output, tr ("Ganancia final.", "Final gain."));

    for (int v = 0; v < maxVoices; ++v)
    {
        auto& vc = voices[v];
        addToggle (vc.on, ParamIDs::voiceOn[v], tr ("VOZ ", "VOICE ") + juce::String (v + 1), tr ("Activa esta voz.", "Enable this voice."));
        vc.on.setColour (juce::TextButton::buttonOnColourId, voiceColour (v));
        vc.degree.addItemList (Choices::degrees, 1);
        vc.degree.setTooltip (tr ("Grados de la escala por encima (+) o por debajo (-) de tu nota: +2 = tercera, +4 = quinta, +7 = octava.",
                                  "Scale degrees above (+) or below (-) your note: +2 = third, +4 = fifth, +7 = octave."));
        addAndMakeVisible (vc.degree);
        comboAttachments.push_back (std::make_unique<ComboAttachment> (processor.state, ParamIDs::voiceDegrees[v], vc.degree));
        addKnob (vc.semis, ParamIDs::voiceSemis[v], tr ("Semitonos fijos (modo Semitonos).", "Fixed semitones (Semitones mode)."));
        addKnob (vc.detune, ParamIDs::voiceDetune[v], tr ("Desafinación en cents, para engordar el coro.", "Detune in cents, to thicken the choir."));
        addKnob (vc.pan, ParamIDs::voicePan[v], tr ("Posición estéreo.", "Stereo position."));
        addKnob (vc.level, ParamIDs::voiceLevel[v], tr ("Nivel de la voz.", "Voice level."));
        addKnob (vc.formant, ParamIDs::voiceFormant[v], tr ("Tamaño de esta voz: negativo = más grande, positivo = más pequeña.", "Size of this voice: negative = bigger, positive = smaller."));
    }

    addCombo (carrierBox, carrierLabel, ParamIDs::carrier, { tr ("Sierra", "Saw"), tr ("Pulso", "Pulse"), tr ("Ruido", "Noise"), tr ("Externa (sidechain)", "External (sidechain)") }, tr ("PORTADORA", "CARRIER"),
              tr ("El sonido que 'habla'. Externa: manda un sinte por la entrada sidechain del plugin.", "The sound that 'speaks'. External: feed a synth into the plugin's sidechain input."));
    addToggle (followButton, ParamIDs::carrierFollow, tr ("SIGUE LA VOZ", "FOLLOW VOICE"),
               tr ("La portadora toca la nota que cantas (talkbox). Apagado: la nota fija o las notas MIDI (acordes).", "The carrier plays the note you sing (talkbox). Off: the fixed note or MIDI notes (chords)."));
    addCombo (carrierNoteBox, carrierNoteLabel, ParamIDs::carrierNote, Choices::noteNames(), tr ("NOTA", "NOTE"),
              tr ("Nota de la portadora cuando no sigue la voz y no hay MIDI.", "Carrier note when not following the voice and no MIDI is held."));
    addKnob (bands, ParamIDs::bands, tr ("Más bandas = más inteligible, más CPU.", "More bands = more intelligible, more CPU."));
    addKnob (attack, ParamIDs::attack, tr ("Velocidad con la que cada banda sube.", "How fast each band rises."));
    addKnob (release, ParamIDs::release, tr ("Velocidad con la que cada banda cae.", "How fast each band falls."));
    addKnob (sibilance, ParamIDs::sibilance, tr ("Deja pasar las consonantes (s, t, k) para entender las palabras.", "Lets consonants (s, t, k) through so words stay clear."));
    addKnob (carrierDetune, ParamIDs::carrierDetune, tr ("Desafinación de la portadora en cents.", "Carrier detune in cents."));

    addCombo (robotNoteBox, robotNoteLabel, ParamIDs::robotNote, Choices::noteNames(), tr ("NOTA", "NOTE"), tr ("La única nota que canta el robot.", "The only note the robot sings."));
    addKnob (robotFormant, ParamIDs::robotFormant, tr ("Tamaño del robot: negativo = monstruo, positivo = pequeño.", "Robot size: negative = monster, positive = tiny."));

    for (int i = 0; i < 6; ++i)
    {
        auto& b = presetButtons[i];
        b.setButtonText (tr (voxPresets[i].nameEs, voxPresets[i].nameEn));
        b.setTooltip (tr ("Punto de partida.", "Starting point."));
        b.onClick = [this, i] { processor.applyPreset (voxPresets[i]); };
        addAndMakeVisible (b);
    }

    addAndMakeVisible (display);

    setResizable (true, true);
    setResizeLimits (baseWidth * 3 / 4, baseHeight * 3 / 4, baseWidth * 2, baseHeight * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) baseWidth / baseHeight);
    setSize (baseWidth, baseHeight);

    updateVisibility();
    startTimerHz (30);
}

VoxSynthEditor::~VoxSynthEditor()
{
    setLookAndFeel (nullptr);
}

void VoxSynthEditor::paint (juce::Graphics& g)
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

    g.setColour (colours::panel.withAlpha (0.5f));
    g.fillRoundedRectangle (panelArea.toFloat(), 8.0f * s);
}

void VoxSynthEditor::resized()
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

    // Lower area: mode panel (left) + common knobs (right).
    auto lower = r.removeFromBottom (px (216));
    r.removeFromBottom (px (8));
    auto common = lower.removeFromRight (lower.getWidth() * 28 / 100);
    lower.removeFromRight (px (8));
    panelArea = lower;
    {
        auto c = common;
        auto top = c.removeFromTop (c.getHeight() / 2);
        glide.setBounds (top.removeFromLeft (top.getWidth() / 2));
        dry.setBounds (top);
        mix.setBounds (c.removeFromLeft (c.getWidth() / 2));
        output.setBounds (c);
    }

    // Combo row.
    auto row = r.removeFromBottom (px (56));
    r.removeFromBottom (px (8));
    const int cw = row.getWidth() / 5;
    for (const auto& c : { std::pair { &modeBox, &modeLabel }, std::pair { &rangeBox, &rangeLabel }, std::pair { &keyBox, &keyLabel },
                           std::pair { &scaleBox, &scaleLabel }, std::pair { &intervalBox, &intervalLabel } })
    {
        auto cell = row.removeFromLeft (cw);
        c.second->setBounds (cell.removeFromTop (px (16)));
        c.first->setBounds (cell.removeFromTop (px (26)).reduced (px (6), 0));
    }

    display.setBounds (r);

    // Harmony panel: 4 voice columns.
    auto panel = lower.reduced (px (6), px (4));
    {
        auto h = panel;
        const int colW = h.getWidth() / maxVoices;
        for (int v = 0; v < maxVoices; ++v)
        {
            auto col = h.removeFromLeft (colW).reduced (px (4), 0);
            auto top = col.removeFromTop (px (26));
            voices[v].on.setBounds (top.removeFromLeft (top.getWidth() * 45 / 100).reduced (px (2), 0));
            voices[v].degree.setBounds (top.reduced (px (2), 0));
            col.removeFromTop (px (4));
            auto k1 = col.removeFromTop (col.getHeight() / 2);
            const int kw = k1.getWidth() / 3;
            voices[v].semis.setBounds (k1.removeFromLeft (kw));
            voices[v].detune.setBounds (k1.removeFromLeft (kw));
            voices[v].pan.setBounds (k1);
            voices[v].level.setBounds (col.removeFromLeft (kw));
            voices[v].formant.setBounds (col.removeFromLeft (kw));
        }
    }
    // Vocoder panel.
    {
        auto vp = panel;
        auto top = vp.removeFromTop (px (44));
        const int tw = top.getWidth() / 3;
        auto a = top.removeFromLeft (tw);
        carrierLabel.setBounds (a.removeFromTop (px (16)));
        carrierBox.setBounds (a.removeFromTop (px (26)).reduced (px (6), 0));
        auto b = top.removeFromLeft (tw);
        b.removeFromTop (px (16));
        followButton.setBounds (b.removeFromTop (px (26)).reduced (px (6), 0));
        auto c = top;
        carrierNoteLabel.setBounds (c.removeFromTop (px (16)));
        carrierNoteBox.setBounds (c.removeFromTop (px (26)).reduced (px (6), 0));
        vp.removeFromTop (px (6));
        vp = vp.removeFromTop (px (110));
        const int kw = vp.getWidth() / 5;
        bands.setBounds (vp.removeFromLeft (kw));
        attack.setBounds (vp.removeFromLeft (kw));
        release.setBounds (vp.removeFromLeft (kw));
        sibilance.setBounds (vp.removeFromLeft (kw));
        carrierDetune.setBounds (vp);
    }
    // Robot panel.
    {
        auto rp = panel;
        auto top = rp.removeFromTop (px (44));
        auto a = top.removeFromLeft (top.getWidth() / 3);
        robotNoteLabel.setBounds (a.removeFromTop (px (16)));
        robotNoteBox.setBounds (a.removeFromTop (px (26)).reduced (px (6), 0));
        rp.removeFromTop (px (6));
        robotFormant.setBounds (rp.removeFromTop (px (110)).removeFromLeft (rp.getWidth() / 5));
    }
}

void VoxSynthEditor::updateVisibility()
{
    const int mode = juce::roundToInt (processor.state.getRawParameterValue (ParamIDs::mode)->load());
    const int interval = juce::roundToInt (processor.state.getRawParameterValue (ParamIDs::intervalMode)->load());
    if (mode == currentMode && interval == currentInterval)
        return;
    currentMode = mode;
    currentInterval = interval;

    const bool harmony = mode == 0, vocoder = mode == 1, robot = mode == 2;
    for (auto& vc : voices)
    {
        juce::Component* parts[] = { &vc.on, &vc.degree, &vc.semis, &vc.detune, &vc.pan, &vc.level, &vc.formant };
        for (auto* c : parts)
            c->setVisible (harmony);
        vc.degree.setAlpha (interval == 0 ? 1.0f : 0.4f);
        vc.semis.setAlpha (interval == 1 ? 1.0f : 0.4f);
    }
    juce::Component* vocoderParts[] = { &carrierBox, &carrierLabel, &followButton, &carrierNoteBox, &carrierNoteLabel, &bands, &attack, &release, &sibilance, &carrierDetune };
    for (auto* c : vocoderParts)
        c->setVisible (vocoder);
    juce::Component* robotParts[] = { &robotNoteBox, &robotNoteLabel, &robotFormant };
    for (auto* c : robotParts)
        c->setVisible (robot);

    const bool scaleUsed = harmony && interval == 0;
    keyBox.setAlpha (scaleUsed ? 1.0f : 0.4f);
    scaleBox.setAlpha (scaleUsed ? 1.0f : 0.4f);
    intervalBox.setAlpha (harmony ? 1.0f : 0.4f);
    glide.setAlpha (harmony ? 1.0f : 0.4f);
}

void VoxSynthEditor::timerCallback()
{
    updateVisibility();
    display.repaint();
}
