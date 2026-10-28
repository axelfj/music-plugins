// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginEditor.h"

using namespace rommuser::ui;
using namespace rommuser::kickready;

namespace
{
constexpr int baseWidth = 980, baseHeight = 660;

juce::String noteName (int midi)
{
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return juce::String (names[((midi % 12) + 12) % 12]) + juce::String (midi / 12 - 1);
}
} // namespace

//==============================================================================
void KickDisplay::setWaveform (std::vector<float> samples, double sampleRate, const KickParams& p, int n)
{
    wave = std::move (samples);
    sr = sampleRate;
    params = p;
    note = n;
    repaint();
}

void KickDisplay::setSounding (bool s)
{
    if (s != sounding) { sounding = s; repaint(); }
}

void KickDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r, 8.0f);
    auto plot = r.reduced (14.0f, 12.0f);
    plot.removeFromTop (16.0f);
    plot.removeFromBottom (16.0f);

    const double lengthS = std::max (0.05, wave.empty() ? 0.5 : (double) wave.size() / sr);
    const float mid = plot.getCentreY();

    // Time grid every 100 ms.
    g.setColour (colours::outline);
    g.setFont (juce::FontOptions (10.0f));
    for (double t = 0.0; t < lengthS; t += 0.1)
    {
        const float x = plot.getX() + (float) (t / lengthS) * plot.getWidth();
        g.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());
        g.setColour (colours::textDim);
        g.drawText (juce::String (juce::roundToInt (t * 1000.0)) + " ms", juce::Rectangle<float> (x + 2.0f, plot.getBottom() + 2.0f, 50.0f, 12.0f), juce::Justification::left);
        g.setColour (colours::outline);
    }
    g.drawHorizontalLine (juce::roundToInt (mid), plot.getX(), plot.getRight());

    // Waveform: min / max per pixel column.
    if (! wave.empty())
    {
        juce::Path fill;
        const int cols = std::max (1, (int) plot.getWidth());
        std::vector<float> mins ((size_t) cols, 0.0f), maxs ((size_t) cols, 0.0f);
        for (int c = 0; c < cols; ++c)
        {
            const size_t a = (size_t) ((double) c / cols * (double) wave.size());
            const size_t b = std::max (a + 1, (size_t) ((double) (c + 1) / cols * (double) wave.size()));
            float lo = 0.0f, hi = 0.0f;
            for (size_t i = a; i < b && i < wave.size(); ++i) { lo = std::min (lo, wave[i]); hi = std::max (hi, wave[i]); }
            mins[(size_t) c] = lo;
            maxs[(size_t) c] = hi;
        }
        const float half = plot.getHeight() * 0.48f;
        for (int c = 0; c < cols; ++c)
        {
            const float x = plot.getX() + (float) c;
            const float yTop = mid - juce::jlimit (-1.0f, 1.0f, maxs[(size_t) c]) * half;
            if (c == 0) fill.startNewSubPath (x, yTop); else fill.lineTo (x, yTop);
        }
        for (int c = cols - 1; c >= 0; --c)
            fill.lineTo (plot.getX() + (float) c, mid - juce::jlimit (-1.0f, 1.0f, mins[(size_t) c]) * half);
        fill.closeSubPath();
        g.setColour (colours::accent.withAlpha (sounding ? 0.9f : 0.65f));
        g.fillPath (fill);
    }

    // Pitch envelope (semitones above the note), drawn on a 0 .. 72 st scale.
    {
        juce::Path pitch;
        const int points = 160;
        for (int i = 0; i <= points; ++i)
        {
            const double t = lengthS * i / points;
            const float semis = params.pitchStartSemis * pitchDecay ((float) t / (std::max (5.0f, params.pitchDecayMs) * 0.001f), params.pitchCurve);
            const float x = plot.getX() + (float) i / points * plot.getWidth();
            const float y = plot.getBottom() - juce::jlimit (0.0f, 1.0f, semis / 72.0f) * plot.getHeight();
            if (i == 0) pitch.startNewSubPath (x, y); else pitch.lineTo (x, y);
        }
        const float dashes[] = { 4.0f, 3.0f };
        juce::Path dashed;
        juce::PathStrokeType (1.5f).createDashedStroke (dashed, pitch, dashes, 2);
        g.setColour (colours::good);
        g.fillPath (dashed);
    }

    // Header line: note, frequency, length, peak.
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (11.0f));
    float peak = 0.0f;
    for (float v : wave) peak = std::max (peak, std::fabs (v));
    juce::String info;
    info << noteName (note) << juce::String::fromUTF8 ("  ·  ") << juce::String (midiToHz (note + params.tuneSemitones), 1) << " Hz"
         << juce::String::fromUTF8 ("  ·  ") << juce::String (juce::roundToInt (lengthS * 1000.0)) << " ms"
         << juce::String::fromUTF8 ("  ·  ") << tr ("pico", "peak") << " " << juce::String (20.0f * std::log10 (std::max (peak, 1e-5f)), 1) << " dBFS";
    g.drawText (info, juce::Rectangle<float> (plot.getX(), r.getY() + 6.0f, plot.getWidth() - 120.0f, 14.0f), juce::Justification::centredLeft);
    g.setColour (colours::good);
    g.drawText (tr ("— envolvente de tono", "— pitch envelope"), juce::Rectangle<float> (plot.getRight() - 120.0f, r.getY() + 6.0f, 120.0f, 14.0f), juce::Justification::centredRight);

    if (sounding)
    {
        g.setColour (colours::accent);
        g.fillEllipse (plot.getRight() - 10.0f, plot.getY() + 4.0f, 8.0f, 8.0f);
    }
}

//==============================================================================
KickReadyEditor::KickReadyEditor (KickReadyProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      tune (tr ("AFINAR", "TUNE")),
      glide ("GLIDE"),
      velocity (tr ("VELOCIDAD", "VELOCITY")),
      pitchStart (tr ("INICIO", "START")),
      pitchDecay (tr ("CAÍDA", "DECAY")),
      pitchCurve (tr ("CURVA", "CURVE")),
      attack (tr ("ATAQUE", "ATTACK")),
      hold ("HOLD"),
      decay (tr ("CAÍDA", "DECAY")),
      ampCurve (tr ("CURVA", "CURVE")),
      clickLevel (tr ("NIVEL", "LEVEL")),
      clickDecay (tr ("CAÍDA", "DECAY")),
      clickTone (tr ("TONO", "TONE")),
      drive ("DRIVE"),
      clip ("CLIP"),
      sub ("SUB"),
      highCut (tr ("CORTE ALTO", "HIGH CUT")),
      output (tr ("SALIDA", "OUTPUT"))
{
    setLookAndFeel (&lnf);

    struct KnobInfo { Knob* knob; const char* id; juce::String tip; };
    const KnobInfo knobs[] = {
        { &tune, ParamIDs::tune, tr ("Afinación fina en semitonos (0.01 = 1 cent). La nota ya sale afinada; úsalo para encajar con el bajo.", "Fine tune in semitones (0.01 = 1 cent). The note is already in tune; use it to sit with the bass.") },
        { &glide, ParamIDs::glide, tr ("Al tocar otra nota mientras suena el bombo, el tono se desliza desde la anterior.", "When another note plays while the kick sounds, the pitch slides from the previous one.") },
        { &velocity, ParamIDs::velocity, tr ("Cuánto cambia el volumen con la fuerza de la nota MIDI. 0 % = siempre igual.", "How much MIDI velocity changes the level. 0% = always the same.") },
        { &pitchStart, ParamIDs::pitchStart, tr ("Cuántos semitonos arriba de la nota empieza el bombo. Más = más 'punch' y click tonal.", "How many semitones above the note the kick starts. More = more punch and tonal click.") },
        { &pitchDecay, ParamIDs::pitchDecay, tr ("Qué tan rápido baja el tono hasta la nota. Corto = seco (techno), largo = 808.", "How fast the pitch falls to the note. Short = tight (techno), long = 808.") },
        { &pitchCurve, ParamIDs::pitchCurve, tr ("Forma de la caída de tono: izquierda cae rápido y luego lento; derecha aguanta y luego cae.", "Shape of the pitch fall: left drops fast then slow; right holds then drops.") },
        { &attack, ParamIDs::attack, tr ("Rampa de entrada. Déjalo casi en 0 para un golpe seco.", "Fade-in. Keep it near 0 for a sharp hit.") },
        { &hold, ParamIDs::hold, tr ("Tiempo a volumen máximo antes de caer.", "Time at full level before the decay.") },
        { &decay, ParamIDs::decay, tr ("Duración del bombo: tiempo hasta el silencio (-60 dB).", "Length of the kick: time to silence (-60 dB).") },
        { &ampCurve, ParamIDs::ampCurve, tr ("0 % = caída recta, 100 % = exponencial (natural).", "0% = straight fade, 100% = exponential (natural).") },
        { &clickLevel, ParamIDs::clickLevel, tr ("Volumen del click del inicio.", "Level of the click at the start.") },
        { &clickDecay, ParamIDs::clickDecay, tr ("Duración del click.", "Length of the click.") },
        { &clickTone, ParamIDs::clickTone, tr ("Noise: frecuencia desde la que pasa el ruido. Tick: tono del tick.", "Noise: frequency the noise passes from. Tick: pitch of the tick.") },
        { &drive, ParamIDs::drive, tr ("Saturación: redondea y añade armónicos. El pico se mantiene en 0 dBFS.", "Saturation: rounds and adds harmonics. Peak stays at 0 dBFS.") },
        { &clip, ParamIDs::clip, tr ("Recorte: 0 % apagado, 100 % duro. Aplana el pico para que el bombo pegue más fuerte.", "Clipping: 0% off, 100% hard. Flattens the peak so the kick hits harder.") },
        { &sub, ParamIDs::sub, tr ("Realce o corte de graves (60 Hz).", "Low boost or cut (60 Hz).") },
        { &highCut, ParamIDs::highCut, tr ("Quita brillo del click y la distorsión.", "Removes brightness from the click and the drive.") },
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
        { &noteBox, &noteLabel, ParamIDs::baseNote, Choices::noteNames(), tr ("NOTA", "NOTE"),
          tr ("Nota del bombo cuando no sigue MIDI; también la que usan PLAY y EXPORTAR.", "Kick note when not following MIDI; also used by PLAY and EXPORT.") },
        { &clickModeBox, &clickModeLabel, ParamIDs::clickMode, Choices::clickModes, tr ("TIPO", "TYPE"),
          tr ("Noise: ráfaga de ruido filtrado. Tick: tono corto.", "Noise: burst of filtered noise. Tick: short tone.") },
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

    follow.setButtonText (tr ("SEGUIR MIDI", "FOLLOW MIDI"));
    follow.setClickingTogglesState (true);
    follow.setTooltip (tr ("Encendido: cada nota MIDI afina el bombo. Apagado: siempre suena la nota elegida.",
                           "On: every MIDI note tunes the kick. Off: always plays the chosen note."));
    addAndMakeVisible (follow);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (processor.state, ParamIDs::follow, follow));

    play.setButtonText ("▶ PLAY");
    play.setTooltip (tr ("Escucha el bombo (nota elegida).", "Hear the kick (chosen note)."));
    play.onClick = [this] { processor.triggerFromUi(); };
    addAndMakeVisible (play);

    exportButton.setButtonText (tr ("EXPORTAR WAV", "EXPORT WAV"));
    exportButton.setTooltip (tr ("Guarda el bombo como WAV de 24 bits para usarlo como sample.", "Saves the kick as a 24-bit WAV to use as a sample."));
    exportButton.onClick = [this] { exportWav(); };
    addAndMakeVisible (exportButton);

    for (int i = 0; i < 4; ++i)
    {
        auto& b = presetButtons[i];
        b.setButtonText (tr (kickPresets[i].nameEs, kickPresets[i].nameEn));
        b.setTooltip (tr ("Punto de partida.", "Starting point."));
        b.onClick = [this, i] { processor.applyPreset (kickPresets[i]); };
        addAndMakeVisible (b);
    }

    addAndMakeVisible (display);

    sectionNames[0] = tr ("AFINACIÓN", "TUNING");
    sectionNames[1] = tr ("TONO", "PITCH");
    sectionNames[2] = tr ("VOLUMEN", "AMP");
    sectionNames[3] = "CLICK";
    sectionNames[4] = tr ("SUCIEDAD Y SALIDA", "DIRT & OUTPUT");

    setResizable (true, true);
    setResizeLimits (baseWidth * 3 / 4, baseHeight * 3 / 4, baseWidth * 2, baseHeight * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) baseWidth / baseHeight);
    setSize (baseWidth, baseHeight);

    updateEnablement();
    refreshPreview (true);
    startTimerHz (30);
}

KickReadyEditor::~KickReadyEditor()
{
    setLookAndFeel (nullptr);
}

void KickReadyEditor::paint (juce::Graphics& g)
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

    g.setFont (juce::FontOptions (11.0f * s, juce::Font::bold));
    for (int i = 0; i < 5; ++i)
    {
        g.setColour (colours::outline);
        g.drawRoundedRectangle (sections[i].toFloat().reduced (2.0f), 6.0f, 1.0f);
        g.setColour (colours::textDim);
        g.drawText (sectionNames[i], sections[i].removeFromTop (juce::roundToInt (16 * s)), juce::Justification::centred);
    }
}

void KickReadyEditor::resized()
{
    const float s = (float) getWidth() / baseWidth;
    auto px = [s] (int v) { return juce::roundToInt ((float) v * s); };

    auto r = getLocalBounds();
    r.removeFromTop (px (48));
    r.reduce (px (16), px (10));

    // Bottom strip: play, export, presets.
    auto bottom = r.removeFromBottom (px (36));
    r.removeFromBottom (px (6));
    const int bw = bottom.getWidth() / 6;
    play.setBounds (bottom.removeFromLeft (bw).reduced (px (4), px (2)));
    exportButton.setBounds (bottom.removeFromLeft (bw).reduced (px (4), px (2)));
    for (auto& b : presetButtons)
        b.setBounds (bottom.removeFromLeft (bw).reduced (px (4), px (2)));

    display.setBounds (r.removeFromTop (r.getHeight() * 34 / 100));
    r.removeFromTop (px (8));

    const int rowH = r.getHeight() / 2;
    auto row1 = r.removeFromTop (rowH);
    auto row2 = r;
    const int unit = row1.getWidth() / 12;

    // Row 1: tuning (follow + note + 3 knobs = 5 units) | pitch (3) | amp (4)
    sections[0] = row1.removeFromLeft (unit * 5);
    {
        auto a = sections[0].reduced (px (4));
        a.removeFromTop (px (16));
        auto left = a.removeFromLeft (unit * 2 - px (8));
        left = left.withSizeKeepingCentre (left.getWidth(), px (96));
        follow.setBounds (left.removeFromTop (px (28)).reduced (px (4), 0));
        left.removeFromTop (px (8));
        noteLabel.setBounds (left.removeFromTop (px (16)));
        noteBox.setBounds (left.removeFromTop (px (26)).reduced (px (4), 0));
        const int kw = a.getWidth() / 3;
        tune.setBounds (a.removeFromLeft (kw));
        glide.setBounds (a.removeFromLeft (kw));
        velocity.setBounds (a);
    }
    sections[1] = row1.removeFromLeft (unit * 3);
    {
        auto a = sections[1].reduced (px (4));
        a.removeFromTop (px (16));
        const int kw = a.getWidth() / 3;
        pitchStart.setBounds (a.removeFromLeft (kw));
        pitchDecay.setBounds (a.removeFromLeft (kw));
        pitchCurve.setBounds (a);
    }
    sections[2] = row1;
    {
        auto a = sections[2].reduced (px (4));
        a.removeFromTop (px (16));
        const int kw = a.getWidth() / 4;
        attack.setBounds (a.removeFromLeft (kw));
        hold.setBounds (a.removeFromLeft (kw));
        decay.setBounds (a.removeFromLeft (kw));
        ampCurve.setBounds (a);
    }

    // Row 2: click (combo + 3 knobs = 5 units) | dirt & output (5 knobs = 7 units)
    sections[3] = row2.removeFromLeft (unit * 5);
    {
        auto a = sections[3].reduced (px (4));
        a.removeFromTop (px (16));
        auto left = a.removeFromLeft (unit * 2 - px (8));
        left = left.withSizeKeepingCentre (left.getWidth(), px (46));
        clickModeLabel.setBounds (left.removeFromTop (px (16)));
        clickModeBox.setBounds (left.removeFromTop (px (26)).reduced (px (4), 0));
        const int kw = a.getWidth() / 3;
        clickLevel.setBounds (a.removeFromLeft (kw));
        clickDecay.setBounds (a.removeFromLeft (kw));
        clickTone.setBounds (a);
    }
    sections[4] = row2;
    {
        auto a = sections[4].reduced (px (4));
        a.removeFromTop (px (16));
        const int kw = a.getWidth() / 5;
        drive.setBounds (a.removeFromLeft (kw));
        clip.setBounds (a.removeFromLeft (kw));
        sub.setBounds (a.removeFromLeft (kw));
        highCut.setBounds (a.removeFromLeft (kw));
        output.setBounds (a);
    }
}

void KickReadyEditor::updateEnablement()
{
    const bool following = processor.state.getRawParameterValue (ParamIDs::follow)->load() > 0.5f;
    noteBox.setAlpha (following ? 0.6f : 1.0f);
}

void KickReadyEditor::refreshPreview (bool force)
{
    // Re-render only when a parameter changed (cheap string key of all raw values).
    juce::String key;
    for (auto* param : processor.getParameters())
        key << juce::String (param->getValue(), 4) << ',';
    const int note = processor.getBaseNote();
    if (! force && key == lastParamsKey && note == lastRenderNote)
        return;
    lastParamsKey = key;
    lastRenderNote = note;
    const double sr = processor.getSampleRateForDisplay() > 1000.0 ? processor.getSampleRateForDisplay() : 48000.0;
    display.setWaveform (processor.renderPreview (sr), sr, processor.readParams(), note);
}

void KickReadyEditor::exportWav()
{
    const auto name = "Kick " + noteName (processor.getBaseNote()) + ".wav";
    chooser = std::make_unique<juce::FileChooser> (tr ("Guardar bombo como WAV", "Save kick as WAV"),
                                                   juce::File::getSpecialLocation (juce::File::userDesktopDirectory).getChildFile (name), "*.wav");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this] (const juce::FileChooser& fc)
                          {
                              auto file = fc.getResult();
                              if (file == juce::File()) return;
                              if (! file.hasFileExtension ("wav")) file = file.withFileExtension ("wav");
                              if (! processor.exportWav (file))
                                  juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, JucePlugin_Name,
                                                                          tr ("No se pudo guardar el archivo.", "Could not save the file."));
                          });
}

void KickReadyEditor::timerCallback()
{
    updateEnablement();
    refreshPreview (false);
    display.setSounding (processor.sounding.load());
}
