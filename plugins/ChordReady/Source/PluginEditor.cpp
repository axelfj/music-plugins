// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginEditor.h"

using namespace rommuser::ui;
using namespace rommuser::chordready;

namespace
{
constexpr int baseWidth = 940, baseHeight = 640;

juce::StringArray translatedScales()
{
    static const char* es[] = { "Mayor", "Menor", "Menor armónica", "Dórico", "Mixolidio", "Lidio", "Frigio", "Pent. mayor", "Pent. menor", "Cromática" };
    juce::StringArray out;
    for (int i = 0; i < rommuser::midi::numScales; ++i)
        out.add (isSpanish() ? juce::String::fromUTF8 (es[i]) : juce::String (rommuser::midi::scales[i].name));
    return out;
}

juce::StringArray translatedChordTypes()
{
    juce::StringArray out;
    for (const auto& t : chordTypes)
    {
        juce::String n (t.name);
        if (isSpanish())
        {
            if (n == "Major") n = "Mayor";
            else if (n == "Minor") n = "Menor";
            else if (n == "Power") n = "Quinta";
            else if (n == "Octave") n = "Octava";
        }
        out.add (n);
    }
    return out;
}
} // namespace

//==============================================================================
void ChordDisplay::update (const ChordReadyProcessor& p)
{
    const int input = p.displayInput.load();
    held = p.heldInputs.load() > 0;
    if (input < 0)
    {
        name = juce::String::fromUTF8 ("—");
        notes.clear();
        hint = tr ("Toca una nota (teclado de abajo o MIDI) y sale un acorde.", "Play one note (keyboard below or MIDI) and a chord comes out.");
    }
    else
    {
        ChordEngine e;
        e.setParams (p.readParams());
        name = e.chordName (input);
        juce::String list;
        const int count = p.displayCount.load();
        for (int i = 0; i < count; ++i)
        {
            if (i > 0) list << "  ";
            list << rommuser::midi::noteName (p.displayNotes[(size_t) i].load());
        }
        notes = list;
        hint = tr ("tecla", "key") + " " + rommuser::midi::noteName (input) + juce::String::fromUTF8 ("  →  ") + juce::String (count) + " " + tr ("notas", "notes");
    }
    repaint();
}

void ChordDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r, 8.0f);
    const float s = r.getHeight() / 70.0f;
    auto inner = r.reduced (12.0f * s, 4.0f * s);

    auto left = inner.removeFromLeft (inner.getWidth() * 0.3f);
    g.setColour (held ? colours::accent : colours::text);
    g.setFont (juce::FontOptions (34.0f * s, juce::Font::bold));
    g.drawText (name, left, juce::Justification::centredLeft);

    g.setColour (colours::text);
    g.setFont (juce::FontOptions (15.0f * s));
    g.drawText (notes, inner.removeFromTop (inner.getHeight() * 0.55f), juce::Justification::centredLeft);
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (11.0f * s));
    g.drawText (hint, inner, juce::Justification::centredLeft);
}

//==============================================================================
Knob& ChordReadyEditor::addKnob (const char* id, const juce::String& label, const juce::String& tip)
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

ChordReadyEditor::Combo& ChordReadyEditor::addCombo (const char* id, const juce::String& label, const juce::StringArray& items, const juce::String& tip)
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

juce::TextButton& ChordReadyEditor::addToggle (const char* id, const juce::String& text, const juce::String& tip)
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

void ChordReadyEditor::styleKeyboard (juce::MidiKeyboardComponent& k, juce::Colour down)
{
    k.setAvailableRange (36, 96);
    k.setOctaveForMiddleC (4);
    k.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, colours::text);
    k.setColour (juce::MidiKeyboardComponent::blackNoteColourId, colours::background);
    k.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, colours::outline);
    k.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, down.withAlpha (0.85f));
    k.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, down.withAlpha (0.3f));
    k.setColour (juce::MidiKeyboardComponent::textLabelColourId, colours::textDim);
    k.setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, colours::panel);
    k.setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId, colours::textDim);
    k.setScrollButtonsVisible (false);
    addAndMakeVisible (k);
}

ChordReadyEditor::ChordReadyEditor (ChordReadyProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      inputKeyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard),
      outputKeyboard (p.outputKeys, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    using namespace ParamIDs;
    setLookAndFeel (&lnf);

    // Scale / chord
    addCombo (mode, tr ("MODO", "MODE"), { tr ("Escala", "Scale"), tr ("Fijo", "Fixed") },
              tr ("Escala: el acorde se arma con las notas de la tonalidad (una tecla blanca = un acorde correcto). Fijo: siempre el mismo tipo de acorde sobre la nota que toques.",
                  "Scale: the chord is built from the key's notes (one key = one correct chord). Fixed: always the same chord type on the note you play."));
    addCombo (key, tr ("TONALIDAD", "KEY"), Choices::keys(), tr ("Nota raíz de la tonalidad de tu canción.", "Root note of your song's key."));
    addCombo (scale, tr ("ESCALA", "SCALE"), translatedScales(), tr ("Tipo de escala. Mayor = alegre, menor = triste/oscuro.", "Scale type. Major = happy, minor = sad/dark."));
    addToggle (snap, tr ("AJUSTAR", "SNAP"), tr ("Encendido: una tecla fuera de la escala se mueve a la nota más cercana de la escala, así nunca suena mal.", "On: a key outside the scale moves to the nearest scale note, so it never sounds wrong."));
    addToggle (add7, tr ("7ª", "7TH"), tr ("Agrega la séptima (acordes de 4 notas, más jazz / lofi / house).", "Adds the seventh (4-note chords, more jazz / lofi / house)."));
    addToggle (add9, tr ("9ª", "9TH"), tr ("Agrega la novena (acordes más amplios y soñadores).", "Adds the ninth (wider, dreamier chords)."));
    addCombo (chordType, tr ("ACORDE FIJO", "FIXED CHORD"), translatedChordTypes(), tr ("Tipo de acorde en modo Fijo.", "Chord type in Fixed mode."));

    // Voicing
    addCombo (voicing, "VOICING", { tr ("Cerrado", "Close"), tr ("Abierto", "Open"), tr ("Extendido", "Spread"), tr ("Amplio", "Wide") },
              tr ("Cómo se reparten las notas. Cerrado: juntas. Abierto: la segunda nota más alta baja una octava (drop 2). Extendido: notas alternas suben. Amplio: la grave baja y la aguda sube.",
                  "How the notes are spread out. Close: together. Open: second-highest note drops an octave (drop 2). Spread: alternate notes go up. Wide: lowest down, highest up."));
    addKnob (inversion, tr ("INVERSIÓN", "INVERSION"), tr ("Mueve la nota más grave una octava arriba (+) o la más aguda una octava abajo (−). Útil para que los acordes no salten.", "Moves the lowest note up an octave (+) or the highest down (−). Keeps chord changes from jumping."));
    addKnob (transpose, tr ("TRANSP.", "TRANSP."), tr ("Transposición de todo el acorde en semitonos.", "Transposes the whole chord in semitones."));
    addToggle (bassOct, tr ("BAJO 8ª", "BASS 8VE"), tr ("Agrega la raíz una octava abajo (más cuerpo).", "Adds the root one octave down (more body)."));
    addToggle (topOct, tr ("AGUDO 8ª", "TOP 8VE"), tr ("Agrega la raíz arriba del acorde (más brillo, estilo trance).", "Adds the root above the chord (more sparkle, trance style)."));

    // Strum
    addKnob (strum, "STRUM", tr ("Tiempo entre una nota y la siguiente, como rasgar una guitarra. 0 = todas a la vez.", "Time between one note and the next, like strumming a guitar. 0 = all at once."));
    addCombo (strumDir, tr ("DIRECCIÓN", "DIRECTION"), { tr ("Arriba", "Up"), tr ("Abajo", "Down"), tr ("Alterno", "Alternate"), tr ("Aleatorio", "Random") },
              tr ("Orden del strum: de grave a agudo, al revés, alternando cada acorde o al azar.", "Strum order: low to high, reversed, alternating per chord or random."));
    addKnob (spread, tr ("SUAVIZAR", "SOFTEN"), tr ("Las notas agudas salen más suaves que la raíz, como una mano real.", "Higher notes come out softer than the root, like a real hand."));
    addKnob (humanize, "HUMANIZE", tr ("Variación aleatoria de la fuerza de cada nota.", "Random variation of each note's velocity."));

    // Built-in sound
    addToggle (sound, tr ("SONIDO", "SOUND"), tr ("Sonido interno sencillo para escuchar los acordes al instante. Apágalo cuando mandes el MIDI a otro instrumento.", "Simple built-in sound to hear the chords right away. Turn it off when routing the MIDI to another instrument."));
    addKnob (soundLevel, tr ("NIVEL", "LEVEL"), tr ("Volumen del sonido interno.", "Built-in sound level."));

    // Presets
    presetLabel.setText ("PRESET", juce::dontSendNotification);
    presetLabel.setColour (juce::Label::textColourId, colours::textDim);
    presetLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    presetLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (presetLabel);
    {
        int id = 1;
        for (const auto& pr : chordPresets())
            presetBox.addItem (pr.name, id++);
    }
    presetBox.setTextWhenNothingSelected (tr ("(editado)", "(edited)"));
    presetBox.setTooltip (tr ("Puntos de partida. Cualquier cambio deja el preset como 'editado'.", "Starting points. Any change marks the preset as 'edited'."));
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedId() - 1;
        if (idx >= 0 && idx != processor.lastPreset.load())
            processor.applyPreset (idx);
    };
    addAndMakeVisible (presetBox);
    prevPreset.setButtonText ("<");
    nextPreset.setButtonText (">");
    prevPreset.onClick = [this] { const int n = (int) chordPresets().size(); processor.applyPreset ((std::max (0, processor.lastPreset.load()) + n - 1) % n); };
    nextPreset.onClick = [this] { const int n = (int) chordPresets().size(); processor.applyPreset ((processor.lastPreset.load() + 1) % n); };
    addAndMakeVisible (prevPreset);
    addAndMakeVisible (nextPreset);

    addAndMakeVisible (display);

    styleKeyboard (inputKeyboard, colours::accent);
    styleKeyboard (outputKeyboard, colours::good);
    outputKeyboard.setInterceptsMouseClicks (false, false);
    for (auto* l : { &inputLabel, &outputLabel })
    {
        l->setColour (juce::Label::textColourId, colours::textDim);
        l->setFont (juce::FontOptions (11.0f, juce::Font::bold));
        l->setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (*l);
    }
    inputLabel.setText (tr ("TOCAS", "YOU PLAY"), juce::dontSendNotification);
    outputLabel.setText (tr ("SALE (MIDI OUT)", "OUT (MIDI OUT)"), juce::dontSendNotification);

    sectionNames[0] = tr ("ESCALA Y ACORDE", "SCALE & CHORD");
    sectionNames[1] = "VOICING";
    sectionNames[2] = tr ("STRUM Y TOQUE", "STRUM & FEEL");
    sectionNames[3] = tr ("SONIDO INTERNO", "BUILT-IN SOUND");

    setResizable (true, true);
    setResizeLimits (baseWidth * 3 / 4, baseHeight * 3 / 4, baseWidth * 2, baseHeight * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) baseWidth / baseHeight);
    setSize (baseWidth, baseHeight);

    timerCallback();
    startTimerHz (30);
}

ChordReadyEditor::~ChordReadyEditor()
{
    setLookAndFeel (nullptr);
}

void ChordReadyEditor::timerCallback()
{
    display.update (processor);
    const int idx = processor.lastPreset.load();
    const int wanted = idx >= 0 ? idx + 1 : 0;
    if (presetBox.getSelectedId() != wanted)
        presetBox.setSelectedId (wanted, juce::dontSendNotification);

    const bool fixed = processor.state.getRawParameterValue (ParamIDs::mode)->load() > 0.5f;
    for (auto* id : { ParamIDs::key, ParamIDs::scale })
    {
        combo (id).box.setEnabled (! fixed);
        combo (id).label.setAlpha (fixed ? 0.4f : 1.0f);
    }
    for (auto* id : { ParamIDs::snap, ParamIDs::add7, ParamIDs::add9 })
        toggle (id).setEnabled (! fixed);
    combo (ParamIDs::chordType).box.setEnabled (fixed);
    combo (ParamIDs::chordType).label.setAlpha (fixed ? 1.0f : 0.4f);
    knob (ParamIDs::soundLevel).setEnabled (processor.state.getRawParameterValue (ParamIDs::sound)->load() > 0.5f);
}

void ChordReadyEditor::paint (juce::Graphics& g)
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

void ChordReadyEditor::layoutKnobRow (juce::Rectangle<int> area, std::initializer_list<const char*> ids)
{
    const int n = (int) ids.size();
    if (n == 0) return;
    const int w = area.getWidth() / n;
    for (auto* id : ids)
        knob (id).setBounds (area.removeFromLeft (w));
}

void ChordReadyEditor::layoutCombo (Combo& c, juce::Rectangle<int> area, int px16, int px26)
{
    c.label.setBounds (area.removeFromTop (px16));
    c.box.setBounds (area.removeFromTop (px26).reduced (px16 / 4, 0));
}

void ChordReadyEditor::resized()
{
    using namespace ParamIDs;
    const float s = (float) getWidth() / baseWidth;
    auto px = [s] (int v) { return juce::roundToInt ((float) v * s); };
    const int title = px (16), gap = px (6), comboH = px (16) + px (26), btnH = px (28);

    auto r = getLocalBounds();
    r.removeFromTop (px (48));
    r.reduce (px (14), px (8));

    // Keyboards at the bottom: what you play, what comes out.
    auto kbOut = r.removeFromBottom (px (62));
    outputLabel.setBounds (kbOut.removeFromTop (px (14)));
    outputKeyboard.setKeyWidth ((float) kbOut.getWidth() / 36.0f);
    outputKeyboard.setBounds (kbOut);
    r.removeFromBottom (gap);
    auto kbIn = r.removeFromBottom (px (62));
    inputLabel.setBounds (kbIn.removeFromTop (px (14)));
    inputKeyboard.setKeyWidth ((float) kbIn.getWidth() / 36.0f);
    inputKeyboard.setBounds (kbIn);
    r.removeFromBottom (gap);

    // Top strip: preset selector + chord display.
    auto top = r.removeFromTop (px (70));
    {
        auto left = top.removeFromLeft (top.getWidth() * 36 / 100);
        auto row = left.withSizeKeepingCentre (left.getWidth(), px (28));
        presetLabel.setBounds (row.removeFromLeft (px (64)));
        row.removeFromLeft (gap);
        prevPreset.setBounds (row.removeFromLeft (px (28)));
        row.removeFromLeft (gap);
        nextPreset.setBounds (row.removeFromRight (px (28)));
        row.removeFromRight (gap);
        presetBox.setBounds (row);
        top.removeFromLeft (px (10));
        display.setBounds (top);
    }
    r.removeFromTop (gap);

    const int h = r.getHeight();
    auto rowA = r.removeFromTop (h * 54 / 100); r.removeFromTop (gap);
    auto rowB = r;

    // Row A: scale & chord (44 %) | voicing (rest)
    {
        sections[0] = rowA.removeFromLeft (rowA.getWidth() * 44 / 100);
        auto a = sections[0].reduced (gap);
        a.removeFromTop (title);
        auto colW = a.getWidth() / 3;
        auto c1 = a.removeFromLeft (colW), c2 = a.removeFromLeft (colW), c3 = a;
        {
            auto col = c1.withSizeKeepingCentre (c1.getWidth(), comboH * 2 + gap);
            layoutCombo (combo (mode), col.removeFromTop (comboH), px (16), px (26));
            col.removeFromTop (gap);
            layoutCombo (combo (chordType), col.removeFromTop (comboH), px (16), px (26));
        }
        {
            auto col = c2.withSizeKeepingCentre (c2.getWidth(), comboH * 2 + gap);
            layoutCombo (combo (key), col.removeFromTop (comboH), px (16), px (26));
            col.removeFromTop (gap);
            layoutCombo (combo (scale), col.removeFromTop (comboH), px (16), px (26));
        }
        {
            auto col = c3.withSizeKeepingCentre (c3.getWidth(), btnH * 3 + gap * 2);
            toggle (snap).setBounds (col.removeFromTop (btnH).reduced (px (10), 0));
            col.removeFromTop (gap);
            toggle (add7).setBounds (col.removeFromTop (btnH).reduced (px (10), 0));
            col.removeFromTop (gap);
            toggle (add9).setBounds (col.removeFromTop (btnH).reduced (px (10), 0));
        }

        sections[1] = rowA;
        a = sections[1].reduced (gap);
        a.removeFromTop (title);
        auto left = a.removeFromLeft (px (120));
        {
            auto col = left.withSizeKeepingCentre (left.getWidth(), comboH + btnH * 2 + gap * 2);
            layoutCombo (combo (voicing), col.removeFromTop (comboH), px (16), px (26));
            col.removeFromTop (gap);
            toggle (bassOct).setBounds (col.removeFromTop (btnH).reduced (px (8), 0));
            col.removeFromTop (gap);
            toggle (topOct).setBounds (col.removeFromTop (btnH).reduced (px (8), 0));
        }
        layoutKnobRow (a, { inversion, transpose });
    }

    // Row B: strum (70 %) | built-in sound
    {
        sections[2] = rowB.removeFromLeft (rowB.getWidth() * 70 / 100);
        auto a = sections[2].reduced (gap);
        a.removeFromTop (title);
        auto left = a.removeFromLeft (px (120));
        layoutCombo (combo (strumDir), left.withSizeKeepingCentre (left.getWidth(), comboH), px (16), px (26));
        layoutKnobRow (a, { strum, spread, humanize });

        sections[3] = rowB;
        a = sections[3].reduced (gap);
        a.removeFromTop (title);
        auto l = a.removeFromLeft (a.getWidth() / 2);
        toggle (sound).setBounds (l.withSizeKeepingCentre (l.getWidth() - px (16), btnH));
        knob (soundLevel).setBounds (a);
    }
}
