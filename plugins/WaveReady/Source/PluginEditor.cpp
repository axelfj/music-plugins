// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginEditor.h"

using namespace rommuser::ui;
using namespace rommuser::waveready;

namespace
{
constexpr int baseWidth = 1180, baseHeight = 850;
} // namespace

//==============================================================================
void WaveScope::update (const WaveReadyProcessor& p)
{
    const int w = p.scopeWrite.load (std::memory_order_acquire);
    samples.resize ((size_t) WaveReadyProcessor::scopeSize);
    peak = 0.0f;
    for (int i = 0; i < WaveReadyProcessor::scopeSize; ++i)
    {
        const float v = p.scope[(size_t) ((w + i) & (WaveReadyProcessor::scopeSize - 1))].load (std::memory_order_relaxed);
        samples[(size_t) i] = v;
        peak = std::max (peak, std::fabs (v));
    }
    voices = p.activeVoices.load();
    cutoffHz = p.displayCutoff.load();
    repaint();
}

void WaveScope::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r, 8.0f);
    auto plot = r.reduced (10.0f, 6.0f);
    plot.removeFromTop (14.0f);
    const float mid = plot.getCentreY();

    g.setColour (colours::outline);
    g.drawHorizontalLine (juce::roundToInt (mid), plot.getX(), plot.getRight());

    if (! samples.empty())
    {
        juce::Path path;
        const int cols = std::max (1, (int) plot.getWidth());
        const float half = plot.getHeight() * 0.48f;
        for (int c = 0; c < cols; ++c)
        {
            const size_t a = (size_t) ((double) c / cols * (double) samples.size());
            const size_t b = std::max (a + 1, (size_t) ((double) (c + 1) / cols * (double) samples.size()));
            float lo = 0.0f, hi = 0.0f;
            for (size_t i = a; i < b && i < samples.size(); ++i) { lo = std::min (lo, samples[i]); hi = std::max (hi, samples[i]); }
            const float x = plot.getX() + (float) c;
            const float yTop = mid - juce::jlimit (-1.0f, 1.0f, hi) * half;
            const float yBot = mid - juce::jlimit (-1.0f, 1.0f, lo) * half;
            path.addRectangle (x, yTop, 1.0f, std::max (1.0f, yBot - yTop));
        }
        g.setColour (colours::accent.withAlpha (voices > 0 ? 0.9f : 0.5f));
        g.fillPath (path);
    }

    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (11.0f));
    juce::String info;
    info << tr ("voces", "voices") << " " << voices << " / " << maxVoices;
    if (voices > 0)
        info << juce::String::fromUTF8 ("  ·  ") << tr ("corte", "cutoff") << " " << (cutoffHz >= 1000.0f ? juce::String (cutoffHz / 1000.0f, 2) + " kHz" : juce::String (juce::roundToInt (cutoffHz)) + " Hz");
    info << juce::String::fromUTF8 ("  ·  ") << tr ("pico", "peak") << " " << juce::String (20.0f * std::log10 (std::max (peak, 1e-5f)), 1) + " dBFS";
    g.drawText (info, juce::Rectangle<float> (plot.getX(), r.getY() + 4.0f, plot.getWidth(), 14.0f), juce::Justification::centredLeft);
}

//==============================================================================
Knob& WaveReadyEditor::addKnob (const char* id, const juce::String& label, const juce::String& tip)
{
    auto k = std::make_unique<Knob> (label);
    addAndMakeVisible (*k);
    k->slider.setTooltip (tip);
    sliderAttachments.push_back (std::make_unique<SliderAttachment> (processor.state, id, k->slider));
    if (auto* param = processor.state.getParameter (id))
        k->slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    auto& ref = *k;
    knobs[id] = std::move (k);
    return ref;
}

WaveReadyEditor::Combo& WaveReadyEditor::addCombo (const char* id, const juce::String& label, const juce::StringArray& items, const juce::String& tip)
{
    auto c = std::make_unique<Combo>();
    c->box.addItemList (items, 1);
    c->box.setTooltip (tip);
    addAndMakeVisible (c->box);
    comboAttachments.push_back (std::make_unique<ComboAttachment> (processor.state, id, c->box));
    c->label.setText (label, juce::dontSendNotification);
    c->label.setJustificationType (juce::Justification::centred);
    c->label.setColour (juce::Label::textColourId, colours::textDim);
    c->label.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    addAndMakeVisible (c->label);
    auto& ref = *c;
    combos[id] = std::move (c);
    return ref;
}

juce::TextButton& WaveReadyEditor::addToggle (const char* id, const juce::String& text, const juce::String& tip)
{
    auto b = std::make_unique<juce::TextButton> (text);
    b->setClickingTogglesState (true);
    b->setTooltip (tip);
    addAndMakeVisible (*b);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (processor.state, id, *b));
    auto& ref = *b;
    toggles[id] = std::move (b);
    return ref;
}

WaveReadyEditor::WaveReadyEditor (WaveReadyProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    using namespace ParamIDs;
    setLookAndFeel (&lnf);

    // Oscillators
    for (int o = 0; o < 2; ++o)
    {
        addToggle (oscOn[o], "ON", tr ("Enciende o apaga este oscilador.", "Turns this oscillator on or off."));
        addCombo (oscWave[o], tr ("ONDA", "WAVE"), Choices::waves(), tr ("Forma de onda. MORPH mezcla hacia la siguiente de la lista.", "Waveform. MORPH blends towards the next one in the list."));
        addCombo (oscOct[o], tr ("OCTAVA", "OCTAVE"), Choices::octaves, tr ("Transposición en octavas.", "Transpose in octaves."));
        addKnob (oscMorph[o], "MORPH", tr ("Mezcla gradual entre esta onda y la siguiente.", "Gradual blend between this wave and the next."));
        addKnob (oscSemi[o], "SEMI", tr ("Transposición en semitonos.", "Transpose in semitones."));
        addKnob (oscFine[o], tr ("FINO", "FINE"), tr ("Afinación fina en cents (100 = 1 semitono).", "Fine tune in cents (100 = 1 semitone)."));
        addKnob (oscLevel[o], tr ("NIVEL", "LEVEL"), tr ("Volumen del oscilador.", "Oscillator level."));
        addKnob (oscPan[o], "PAN", tr ("Posición estéreo.", "Stereo position."));
        addKnob (oscUnison[o], tr ("UNÍSONO", "UNISON"), tr ("Copias desafinadas del oscilador (1 = ninguna, 7 = supersaw).", "Detuned copies of the oscillator (1 = none, 7 = supersaw)."));
        addKnob (oscDetune[o], tr ("DESAFINE", "DETUNE"), tr ("Separación en cents entre las copias del unísono.", "Spread in cents between the unison copies."));
        addKnob (oscWidth[o], tr ("ANCHO", "WIDTH"), tr ("Cuánto se reparten las copias a izquierda y derecha. 0 = mono.", "How far the copies spread left and right. 0 = mono."));
        addKnob (oscPhase[o], tr ("FASE", "PHASE"), tr ("Fase inicial cuando RETRIG está activo.", "Start phase when RETRIG is on."));
        addToggle (oscRetrig[o], "RETRIG", tr ("Encendido: cada nota empieza en la misma fase (ataques idénticos, ideal para bajos). Apagado: fase libre.", "On: every note starts at the same phase (identical attacks, great for bass). Off: free-running."));
    }

    // Sub & noise
    addKnob (subLevel, "SUB", tr ("Oscilador sub (seno u onda cuadrada) una o dos octavas abajo.", "Sub oscillator (sine or square) one or two octaves below."));
    addCombo (subOct, tr ("OCTAVA", "OCTAVE"), Choices::subOctaves, tr ("Octavas por debajo de la nota.", "Octaves below the note."));
    addToggle (subSquare, tr ("CUADRADA", "SQUARE"), tr ("Sub con onda cuadrada en vez de seno.", "Square sub instead of sine."));
    addKnob (noiseLevel, tr ("RUIDO", "NOISE"), tr ("Capa de ruido (pasa por el filtro y la envolvente).", "Noise layer (goes through the filter and envelope)."));
    addCombo (noiseColour, "COLOR", Choices::noiseColours, tr ("Blanco = brillante, rosa = más grave.", "White = bright, pink = darker."));

    // Filter
    addCombo (filterMode, tr ("TIPO", "TYPE"), Choices::filterModes, tr ("LP = quita agudos, HP = quita graves, BP = deja una banda.", "LP = removes highs, HP = removes lows, BP = keeps a band."));
    addKnob (cutoff, tr ("CORTE", "CUTOFF"), tr ("Frecuencia del filtro.", "Filter frequency."));
    addKnob (resonance, tr ("RESONANCIA", "RESO"), tr ("Realce en la frecuencia de corte.", "Boost at the cutoff frequency."));
    addKnob (filterDrive, "DRIVE", tr ("Saturación dentro del filtro.", "Saturation inside the filter."));
    addKnob (keyTrack, tr ("SEGUIR", "KEY TRK"), tr ("El corte sube con la nota (100 % = una octava por octava).", "Cutoff follows the note (100% = one octave per octave)."));
    addKnob (envToCutoff, tr ("ENV>CORTE", "ENV>CUT"), tr ("Cuánto mueve la envolvente MOD el corte, en octavas.", "How much the MOD envelope moves the cutoff, in octaves."));
    addKnob (velToCutoff, tr ("VEL>CORTE", "VEL>CUT"), tr ("Notas más fuertes abren más el filtro.", "Harder notes open the filter more."));

    // Envelopes
    for (int e = 0; e < 2; ++e)
    {
        addKnob (envAttack[e], tr ("ATAQUE", "ATTACK"), tr ("Tiempo hasta el máximo.", "Time to reach the peak."));
        addKnob (envDecay[e], tr ("CAÍDA", "DECAY"), tr ("Tiempo hasta el nivel de sostenido.", "Time to fall to the sustain level."));
        addKnob (envSustain[e], tr ("SOSTENIDO", "SUSTAIN"), tr ("Nivel mientras la tecla está pulsada.", "Level while the key is held."));
        addKnob (envRelease[e], tr ("SOLTAR", "RELEASE"), tr ("Tiempo hasta el silencio tras soltar.", "Time to silence after release."));
    }
    addKnob (velToAmp, "VEL>AMP", tr ("Cuánto cambia el volumen con la fuerza de la nota.", "How much velocity changes the level."));
    addKnob (envToPitch, tr ("ENV>TONO", "ENV>PITCH"), tr ("La envolvente MOD mueve el tono (semitonos). Útil para 808 y hoover.", "MOD envelope moves the pitch (semitones). Great for 808s and hoovers."));

    // LFOs
    for (int l = 0; l < 2; ++l)
    {
        addCombo (lfoShape[l], tr ("FORMA", "SHAPE"), Choices::lfoShapes, tr ("Forma del LFO.", "LFO shape."));
        addToggle (lfoSync[l], "SYNC", tr ("Velocidad en divisiones del tempo del proyecto.", "Rate in divisions of the project tempo."));
        addKnob (lfoRate[l], tr ("VELOC.", "RATE"), tr ("Velocidad en Hz (cuando SYNC está apagado).", "Rate in Hz (when SYNC is off)."));
        addCombo (lfoDiv[l], tr ("DIVISIÓN", "DIVISION"), Choices::divisions(), tr ("Duración de un ciclo cuando SYNC está encendido.", "Length of one cycle when SYNC is on."));
        addKnob (lfoFade[l], "FADE", tr ("El LFO entra poco a poco tras cada nota.", "The LFO fades in after each note."));
        addToggle (lfoRetrig[l], "RETRIG", tr ("Reinicia el LFO en cada nota.", "Restart the LFO on every note."));
    }
    addKnob (lfo1ToPitch, tr (">TONO", ">PITCH"), tr ("Vibrato en semitonos.", "Vibrato in semitones."));
    addKnob (lfo1ToCutoff, tr (">CORTE", ">CUTOFF"), tr ("Mueve el filtro, en octavas.", "Moves the filter, in octaves."));
    addKnob (lfo1ToAmp, ">AMP", tr ("Trémolo.", "Tremolo."));
    addKnob (lfo2ToMorph, ">MORPH", tr ("Mueve el MORPH de los osciladores.", "Moves the oscillators' MORPH."));
    addKnob (lfo2ToCutoff, tr (">CORTE", ">CUTOFF"), tr ("Mueve el filtro, en octavas.", "Moves the filter, in octaves."));
    addKnob (lfo2ToPitch, tr (">TONO", ">PITCH"), tr ("Vibrato en semitonos.", "Vibrato in semitones."));

    // Voice
    addToggle (mono, "MONO", tr ("Una sola nota a la vez (bajos, leads).", "One note at a time (bass, leads)."));
    addToggle (legato, "LEGATO", tr ("En mono: una nota nueva sin soltar la anterior no reinicia las envolventes.", "In mono: a new note while holding the previous one does not restart the envelopes."));
    addKnob (glide, "GLIDE", tr ("Tiempo de deslizamiento entre notas.", "Slide time between notes."));
    addKnob (bendRange, "BEND", tr ("Rango de la rueda de pitch en semitonos.", "Pitch wheel range in semitones."));
    addKnob (tune, tr ("AFINAR", "TUNE"), tr ("Afinación global en cents.", "Global tune in cents."));
    addKnob (wheelToCutoff, tr ("RUEDA>CRT", "WHL>CUT"), tr ("La rueda de modulación abre el filtro.", "The mod wheel opens the filter."));
    addKnob (wheelToVibrato, tr ("RUEDA>VIB", "WHL>VIB"), tr ("La rueda de modulación añade vibrato (con LFO 1).", "The mod wheel adds vibrato (with LFO 1)."));

    // FX
    addKnob (drive, "DRIVE", tr ("Saturación global.", "Global saturation."));
    addKnob (chorusMix, "CHORUS", tr ("Cantidad de chorus.", "Chorus amount."));
    addKnob (chorusRate, tr ("VELOC.", "RATE"), tr ("Velocidad del chorus.", "Chorus rate."));
    addKnob (chorusDepth, tr ("PROF.", "DEPTH"), tr ("Profundidad del chorus.", "Chorus depth."));
    addKnob (delayMix, "DELAY", tr ("Cantidad de delay.", "Delay amount."));
    addToggle (delaySync, "SYNC", tr ("Tiempo del delay en divisiones del tempo.", "Delay time in tempo divisions."));
    addCombo (delayDiv, tr ("DIVISIÓN", "DIVISION"), Choices::divisions(), tr ("Tiempo del delay cuando SYNC está encendido.", "Delay time when SYNC is on."));
    addKnob (delayTime, tr ("TIEMPO", "TIME"), tr ("Tiempo del delay en ms (SYNC apagado).", "Delay time in ms (SYNC off)."));
    addKnob (delayFeedback, "FEEDBACK", tr ("Repeticiones.", "Repeats."));
    addToggle (delayPingPong, "PING-PONG", tr ("Las repeticiones alternan izquierda y derecha.", "Repeats alternate left and right."));
    addKnob (delayTone, tr ("TONO", "TONE"), tr ("Las repeticiones pierden agudos por encima de esta frecuencia.", "Repeats lose highs above this frequency."));
    addKnob (reverbMix, "REVERB", tr ("Cantidad de reverb.", "Reverb amount."));
    addKnob (reverbDecay, tr ("COLA", "DECAY"), tr ("Duración de la reverb.", "Reverb length."));
    addToggle (reverbPlate, "PLATE", tr ("Plate (brillante) en vez de sala.", "Plate (bright) instead of hall."));
    addKnob (output, tr ("SALIDA", "OUTPUT"), tr ("Ganancia final. Un limitador suave evita que pase de 0 dBFS.", "Final gain. A soft limiter keeps it under 0 dBFS."));

    // Presets
    presetLabel.setText ("PRESET", juce::dontSendNotification);
    presetLabel.setColour (juce::Label::textColourId, colours::textDim);
    presetLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    presetLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (presetLabel);
    {
        int id = 1;
        for (const auto& pr : wavePresets())
            presetBox.addItem (pr.name, id++);
    }
    presetBox.setTextWhenNothingSelected (tr ("(editado)", "(edited)"));
    presetBox.setTooltip (tr ("Puntos de partida. Cualquier cambio de un control deja el sonido como 'editado'.", "Starting points. Changing any control marks the sound as 'edited'."));
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedId() - 1;
        if (idx >= 0 && idx != processor.lastPreset.load())
            processor.applyPreset (idx);
    };
    addAndMakeVisible (presetBox);
    prevPreset.setButtonText ("<");
    nextPreset.setButtonText (">");
    prevPreset.onClick = [this] { const int n = (int) wavePresets().size(); processor.applyPreset ((std::max (0, processor.lastPreset.load()) + n - 1) % n); };
    nextPreset.onClick = [this] { const int n = (int) wavePresets().size(); processor.applyPreset ((processor.lastPreset.load() + 1) % n); };
    addAndMakeVisible (prevPreset);
    addAndMakeVisible (nextPreset);

    addAndMakeVisible (scope);

    keyboard.setAvailableRange (36, 96);
    keyboard.setOctaveForMiddleC (4);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, colours::text);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, colours::background);
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, colours::outline);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, colours::accent.withAlpha (0.8f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, colours::accent.withAlpha (0.3f));
    keyboard.setColour (juce::MidiKeyboardComponent::textLabelColourId, colours::textDim);
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, colours::panel);
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId, colours::textDim);
    addAndMakeVisible (keyboard);

    sectionNames[0] = "OSC 1";
    sectionNames[1] = "OSC 2";
    sectionNames[2] = tr ("SUB Y RUIDO", "SUB & NOISE");
    sectionNames[3] = tr ("FILTRO", "FILTER");
    sectionNames[4] = tr ("ENVOLVENTE AMP", "AMP ENV");
    sectionNames[5] = tr ("ENVOLVENTE MOD", "MOD ENV");
    sectionNames[6] = "LFO 1";
    sectionNames[7] = "LFO 2";
    sectionNames[8] = tr ("VOZ", "VOICE");
    sectionNames[9] = tr ("EFECTOS", "FX");

    setResizable (true, true);
    setResizeLimits (baseWidth * 3 / 4, baseHeight * 3 / 4, baseWidth * 2, baseHeight * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) baseWidth / baseHeight);
    setSize (baseWidth, baseHeight);

    timerCallback();
    startTimerHz (30);
}

WaveReadyEditor::~WaveReadyEditor()
{
    setLookAndFeel (nullptr);
}

void WaveReadyEditor::paint (juce::Graphics& g)
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

    g.setFont (juce::FontOptions (11.0f * s, juce::Font::bold));
    for (int i = 0; i < numSections; ++i)
    {
        g.setColour (colours::outline);
        g.drawRoundedRectangle (sections[i].toFloat().reduced (2.0f), 6.0f, 1.0f);
        g.setColour (colours::textDim);
        g.drawText (sectionNames[i], sections[i].withHeight (juce::roundToInt (16 * s)), juce::Justification::centred);
    }
}

void WaveReadyEditor::layoutKnobRow (juce::Rectangle<int> area, std::initializer_list<const char*> ids)
{
    const int n = (int) ids.size();
    if (n == 0) return;
    const int w = area.getWidth() / n;
    for (auto* id : ids)
        knob (id).setBounds (area.removeFromLeft (w));
}

void WaveReadyEditor::layoutCombo (Combo& c, juce::Rectangle<int> area, int px16, int px26)
{
    c.label.setBounds (area.removeFromTop (px16));
    c.box.setBounds (area.removeFromTop (px26).reduced (px16 / 4, 0));
}

void WaveReadyEditor::resized()
{
    using namespace ParamIDs;
    const float s = (float) getWidth() / baseWidth;
    auto px = [s] (int v) { return juce::roundToInt ((float) v * s); };
    const int title = px (16), gap = px (4), comboH = px (16) + px (26);

    auto r = getLocalBounds();
    r.removeFromTop (px (48));
    r.reduce (px (14), px (8));

    // Keyboard at the bottom.
    auto kb = r.removeFromBottom (px (64));
    keyboard.setKeyWidth ((float) kb.getWidth() / 36.0f);
    keyboard.setBounds (kb);
    r.removeFromBottom (gap);

    // Top strip: preset selector + scope.
    auto top = r.removeFromTop (px (70));
    {
        auto left = top.removeFromLeft (top.getWidth() * 38 / 100);
        auto row = left.withSizeKeepingCentre (left.getWidth(), px (28));
        presetLabel.setBounds (row.removeFromLeft (px (64)));
        row.removeFromLeft (gap);
        prevPreset.setBounds (row.removeFromLeft (px (28)));
        row.removeFromLeft (gap);
        nextPreset.setBounds (row.removeFromRight (px (28)));
        row.removeFromRight (gap);
        presetBox.setBounds (row);
        top.removeFromLeft (px (10));
        scope.setBounds (top);
    }
    r.removeFromTop (gap);

    // Rows: A (oscillators) 27 %, B (filter / envelopes) 22 %, C (LFO / voice) 22 %, D (fx) rest.
    const int h = r.getHeight();
    auto rowA = r.removeFromTop (h * 29 / 100); r.removeFromTop (gap);
    auto rowB = r.removeFromTop (h * 22 / 100); r.removeFromTop (gap);
    auto rowC = r.removeFromTop (h * 22 / 100); r.removeFromTop (gap);
    auto rowD = r;

    // Row A
    {
        const int oscW = rowA.getWidth() * 40 / 100;
        for (int o = 0; o < 2; ++o)
        {
            sections[o] = rowA.removeFromLeft (oscW);
            auto a = sections[o].reduced (gap);
            a.removeFromTop (title);
            auto left = a.removeFromLeft (px (96));
            {
                auto col = left.withSizeKeepingCentre (left.getWidth(), comboH * 2 + px (28) * 2 + gap * 3);
                toggle (oscOn[o]).setBounds (col.removeFromTop (px (28)).reduced (px (6), 0));
                col.removeFromTop (gap);
                layoutCombo (combo (oscWave[o]), col.removeFromTop (comboH), px (16), px (26));
                col.removeFromTop (gap);
                layoutCombo (combo (oscOct[o]), col.removeFromTop (comboH), px (16), px (26));
                col.removeFromTop (gap);
                toggle (oscRetrig[o]).setBounds (col.removeFromTop (px (28)).reduced (px (6), 0));
            }
            const int rh = a.getHeight() / 2;
            layoutKnobRow (a.removeFromTop (rh), { oscMorph[o], oscSemi[o], oscFine[o], oscLevel[o], oscPan[o] });
            layoutKnobRow (a, { oscUnison[o], oscDetune[o], oscWidth[o], oscPhase[o] });
        }
        sections[2] = rowA;
        auto a = sections[2].reduced (gap);
        a.removeFromTop (title);
        const int rh = a.getHeight() / 2;
        auto r1 = a.removeFromTop (rh);
        auto r2 = a;
        knob (subLevel).setBounds (r1.removeFromLeft (r1.getWidth() / 2));
        {
            auto col = r1.withSizeKeepingCentre (r1.getWidth(), comboH + px (28) + gap);
            layoutCombo (combo (subOct), col.removeFromTop (comboH), px (16), px (26));
            col.removeFromTop (gap);
            toggle (subSquare).setBounds (col.removeFromTop (px (28)).reduced (px (6), 0));
        }
        knob (noiseLevel).setBounds (r2.removeFromLeft (r2.getWidth() / 2));
        {
            auto col = r2.withSizeKeepingCentre (r2.getWidth(), comboH);
            layoutCombo (combo (noiseColour), col, px (16), px (26));
        }
    }

    // Row B
    {
        sections[3] = rowB.removeFromLeft (rowB.getWidth() * 42 / 100);
        auto a = sections[3].reduced (gap);
        a.removeFromTop (title);
        auto left = a.removeFromLeft (px (90));
        layoutCombo (combo (filterMode), left.withSizeKeepingCentre (left.getWidth(), comboH), px (16), px (26));
        layoutKnobRow (a, { cutoff, resonance, filterDrive, keyTrack, envToCutoff, velToCutoff });

        sections[4] = rowB.removeFromLeft (rowB.getWidth() / 2);
        a = sections[4].reduced (gap);
        a.removeFromTop (title);
        layoutKnobRow (a, { envAttack[0], envDecay[0], envSustain[0], envRelease[0], velToAmp });

        sections[5] = rowB;
        a = sections[5].reduced (gap);
        a.removeFromTop (title);
        layoutKnobRow (a, { envAttack[1], envDecay[1], envSustain[1], envRelease[1], envToPitch });
    }

    // Row C
    {
        const int lfoW = rowC.getWidth() * 35 / 100;
        for (int l = 0; l < 2; ++l)
        {
            sections[6 + l] = rowC.removeFromLeft (lfoW);
            auto a = sections[6 + l].reduced (gap);
            a.removeFromTop (title);
            auto left = a.removeFromLeft (px (136));
            {
                auto col = left.withSizeKeepingCentre (left.getWidth(), comboH * 2 + px (28) + gap * 2);
                layoutCombo (combo (lfoShape[l]), col.removeFromTop (comboH), px (16), px (26));
                col.removeFromTop (gap);
                layoutCombo (combo (lfoDiv[l]), col.removeFromTop (comboH), px (16), px (26));
                col.removeFromTop (gap);
                auto bt = col.removeFromTop (px (28));
                toggle (lfoSync[l]).setBounds (bt.removeFromLeft (bt.getWidth() / 2).reduced (px (3), 0));
                toggle (lfoRetrig[l]).setBounds (bt.reduced (px (3), 0));
            }
            if (l == 0) layoutKnobRow (a, { lfoRate[0], lfoFade[0], lfo1ToPitch, lfo1ToCutoff, lfo1ToAmp });
            else        layoutKnobRow (a, { lfoRate[1], lfoFade[1], lfo2ToMorph, lfo2ToCutoff, lfo2ToPitch });
        }
        sections[8] = rowC;
        auto a = sections[8].reduced (gap);
        a.removeFromTop (title);
        auto left = a.removeFromLeft (px (90));
        {
            auto col = left.withSizeKeepingCentre (left.getWidth(), px (28) * 2 + gap);
            toggle (mono).setBounds (col.removeFromTop (px (28)).reduced (px (6), 0));
            col.removeFromTop (gap);
            toggle (legato).setBounds (col.removeFromTop (px (28)).reduced (px (6), 0));
        }
        layoutKnobRow (a, { glide, bendRange, tune, wheelToCutoff, wheelToVibrato });
    }

    // Row D: FX
    {
        sections[9] = rowD;
        auto a = sections[9].reduced (gap);
        a.removeFromTop (title);
        const int unit = a.getWidth() / 15;
        layoutKnobRow (a.removeFromLeft (unit * 4), { drive, chorusMix, chorusRate, chorusDepth });
        auto dl = a.removeFromLeft (unit * 7);
        layoutKnobRow (dl.removeFromLeft (unit), { delayMix });
        {
            auto col = dl.removeFromLeft (unit * 2);
            col = col.withSizeKeepingCentre (col.getWidth(), comboH + px (28) * 2 + gap * 2);
            layoutCombo (combo (delayDiv), col.removeFromTop (comboH), px (16), px (26));
            col.removeFromTop (gap);
            toggle (delaySync).setBounds (col.removeFromTop (px (28)).reduced (px (10), 0));
            col.removeFromTop (gap);
            toggle (delayPingPong).setBounds (col.removeFromTop (px (28)).reduced (px (10), 0));
        }
        layoutKnobRow (dl, { delayTime, delayFeedback, delayTone });
        layoutKnobRow (a.removeFromLeft (unit * 2), { reverbMix, reverbDecay });
        {
            auto col = a.removeFromLeft (unit);
            toggle (reverbPlate).setBounds (col.withSizeKeepingCentre (col.getWidth() - px (8), px (28)));
        }
        layoutKnobRow (a, { output });
    }
}

void WaveReadyEditor::timerCallback()
{
    scope.update (processor);
    processor.outputPeak.store (0.0f);

    const int preset = processor.lastPreset.load();
    const int wantId = preset >= 0 ? preset + 1 : 0;
    if (presetBox.getSelectedId() != wantId)
        presetBox.setSelectedId (wantId, juce::dontSendNotification);

    const bool monoOn = processor.state.getRawParameterValue (ParamIDs::mono)->load() > 0.5f;
    toggle (ParamIDs::legato).setAlpha (monoOn ? 1.0f : 0.5f);
    knob (ParamIDs::glide).setAlpha (monoOn || processor.state.getRawParameterValue (ParamIDs::glide)->load() > 0.0f ? 1.0f : 0.7f);
    for (int o = 0; o < 2; ++o)
    {
        const bool on = processor.state.getRawParameterValue (ParamIDs::oscOn[o])->load() > 0.5f;
        for (auto* id : { ParamIDs::oscMorph[o], ParamIDs::oscSemi[o], ParamIDs::oscFine[o], ParamIDs::oscLevel[o], ParamIDs::oscPan[o],
                          ParamIDs::oscUnison[o], ParamIDs::oscDetune[o], ParamIDs::oscWidth[o], ParamIDs::oscPhase[o] })
            knob (id).setAlpha (on ? 1.0f : 0.45f);
    }
    for (int l = 0; l < 2; ++l)
    {
        const bool sync = processor.state.getRawParameterValue (ParamIDs::lfoSync[l])->load() > 0.5f;
        knob (ParamIDs::lfoRate[l]).setAlpha (sync ? 0.45f : 1.0f);
        combo (ParamIDs::lfoDiv[l]).box.setAlpha (sync ? 1.0f : 0.45f);
    }
    const bool dsync = processor.state.getRawParameterValue (ParamIDs::delaySync)->load() > 0.5f;
    knob (ParamIDs::delayTime).setAlpha (dsync ? 0.45f : 1.0f);
    combo (ParamIDs::delayDiv).box.setAlpha (dsync ? 1.0f : 0.45f);
}
