// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginEditor.h"

using namespace rommuser::ui;
using namespace rommuser::arpready;

namespace
{
constexpr int baseWidth = 940, baseHeight = 640;
} // namespace

//==============================================================================
void ArpDisplay::update (const ArpReadyProcessor& p)
{
    const int step = p.currentStep.load();
    const int held = p.heldCount.load();
    running = step >= 0;
    const int len = juce::roundToInt (p.state.getRawParameterValue (ParamIDs::patternLength)->load());
    line1 = running ? tr ("PASO", "STEP") + " " + juce::String (step + 1) + " / " + juce::String (len)
                    : tr ("EN ESPERA", "IDLE");
    juce::String l2;
    l2 << juce::String (p.displayBpm.load(), 1) << " BPM  " << juce::String::fromUTF8 ("·") << "  "
       << (p.transportPlaying.load() ? tr ("sincronizado al DAW", "synced to the DAW") : tr ("reloj libre", "free clock"))
       << "  " << juce::String::fromUTF8 ("·") << "  " << held << " " << tr ("notas", "notes");
    if (! running) l2 = tr ("Mantén una o varias notas (teclado de abajo o MIDI).", "Hold one or more notes (keyboard below or MIDI).") + "  " + l2;
    line2 = l2;
    repaint();
}

void ArpDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r, 8.0f);
    const float s = r.getHeight() / 70.0f;
    auto inner = r.reduced (12.0f * s, 4.0f * s);
    g.setColour (running ? colours::accent : colours::text);
    g.setFont (juce::FontOptions (24.0f * s, juce::Font::bold));
    g.drawText (line1, inner.removeFromTop (inner.getHeight() * 0.58f), juce::Justification::centredLeft);
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (11.0f * s));
    g.drawText (line2, inner, juce::Justification::centredLeft);
}

//==============================================================================
Knob& ArpReadyEditor::addKnob (const char* id, const juce::String& label, const juce::String& tip)
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

ArpReadyEditor::Combo& ArpReadyEditor::addCombo (const char* id, const juce::String& label, const juce::StringArray& items, const juce::String& tip)
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

juce::TextButton& ArpReadyEditor::addToggle (const char* id, const juce::String& text, const juce::String& tip)
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

void ArpReadyEditor::styleKeyboard (juce::MidiKeyboardComponent& k, juce::Colour down)
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

ArpReadyEditor::ArpReadyEditor (ArpReadyProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      inputKeyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard),
      outputKeyboard (p.outputKeys, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    using namespace ParamIDs;
    setLookAndFeel (&lnf);

    // Arp
    addCombo (mode, tr ("MODO", "MODE"),
              { tr ("Arriba", "Up"), tr ("Abajo", "Down"), tr ("Arriba-abajo", "Up-Down"), tr ("Abajo-arriba", "Down-Up"), tr ("Como tocas", "As played"), tr ("Aleatorio", "Random"), tr ("Acorde", "Chord") },
              tr ("Orden en que suenan las notas que mantienes. Acorde: todas juntas en cada paso (ritmo de acordes).", "Order the held notes play in. Chord: all together on every step (chord rhythm)."));
    addCombo (division, tr ("VELOCIDAD", "RATE"), Choices::divisions(), tr ("Duración de cada paso en figuras del tempo. D = con puntillo, T = tresillo.", "Length of each step in tempo figures. D = dotted, T = triplet."));
    addKnob (octaves, tr ("OCTAVAS", "OCTAVES"), tr ("Cuántas octavas recorre antes de repetir.", "How many octaves it climbs before repeating."));
    addCombo (octaveMode, tr ("DIR. OCT.", "OCT. DIR."), { tr ("Arriba", "Up"), tr ("Abajo", "Down"), tr ("Arriba-abajo", "Up-Down") }, tr ("Hacia dónde van las octavas extra.", "Where the extra octaves go."));
    addToggle (latch, "LATCH", tr ("Encendido: sigue sonando al soltar las teclas; tocar un acorde nuevo lo reemplaza.", "On: keeps playing after you let go; playing a new chord replaces it."));
    addToggle (hostSync, "SYNC", tr ("Encendido: los pasos caen en la rejilla del DAW mientras reproduce. Apagado: reloj propio desde la primera nota.", "On: steps land on the DAW grid while it plays. Off: its own clock from the first note."));

    // Feel
    addKnob (gate, "GATE", tr ("Duración de cada nota como porcentaje del paso. 100 % = ligado.", "Length of each note as a percentage of the step. 100 % = legato."));
    addKnob (swing, "SWING", tr ("Retrasa los pasos pares para un ritmo con groove (100 % = tresillo).", "Delays the even steps for a groovy feel (100 % = triplet)."));
    addKnob (transpose, tr ("TRANSP.", "TRANSP."), tr ("Transposición en semitonos.", "Transpose in semitones."));
    addCombo (velocityMode, "VELOCITY", { tr ("Como tocas", "As played"), tr ("Fija", "Fixed") }, tr ("Fuerza de las notas: la que tocaste o un valor fijo.", "Note velocity: as you played it or a fixed value."));
    addKnob (velocity, tr ("FIJA", "FIXED"), tr ("Velocity fija (1-127).", "Fixed velocity (1-127)."));
    addKnob (accent, tr ("ACENTO", "ACCENT"), tr ("Refuerza el primer paso del patrón.", "Boosts the first step of the pattern."));

    // Pattern
    for (int i = 0; i < 16; ++i)
        addToggle (step[i], juce::String (i + 1), tr ("Paso encendido = suena, apagado = silencio.", "Step on = plays, off = rest."));
    addKnob (patternLength, tr ("LARGO", "LENGTH"), tr ("Cuántos pasos tiene el patrón antes de repetirse.", "How many steps the pattern has before it repeats."));

    // Built-in sound
    addToggle (sound, tr ("SONIDO", "SOUND"), tr ("Sonido interno sencillo para escuchar el arpegio al instante. Apágalo cuando mandes el MIDI a otro instrumento.", "Simple built-in sound to hear the arpeggio right away. Turn it off when routing the MIDI to another instrument."));
    addKnob (soundLevel, tr ("NIVEL", "LEVEL"), tr ("Volumen del sonido interno.", "Built-in sound level."));

    // Presets
    presetLabel.setText ("PRESET", juce::dontSendNotification);
    presetLabel.setColour (juce::Label::textColourId, colours::textDim);
    presetLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    presetLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (presetLabel);
    {
        int id = 1;
        for (const auto& pr : arpPresets())
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
    prevPreset.onClick = [this] { const int n = (int) arpPresets().size(); processor.applyPreset ((std::max (0, processor.lastPreset.load()) + n - 1) % n); };
    nextPreset.onClick = [this] { const int n = (int) arpPresets().size(); processor.applyPreset ((processor.lastPreset.load() + 1) % n); };
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

    sectionNames[0] = tr ("ARPEGIO", "ARPEGGIO");
    sectionNames[1] = tr ("TOQUE", "FEEL");
    sectionNames[2] = tr ("PATRÓN", "PATTERN");
    sectionNames[3] = tr ("SONIDO INTERNO", "BUILT-IN SOUND");

    setResizable (true, true);
    setResizeLimits (baseWidth * 3 / 4, baseHeight * 3 / 4, baseWidth * 2, baseHeight * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) baseWidth / baseHeight);
    setSize (baseWidth, baseHeight);

    timerCallback();
    startTimerHz (30);
}

ArpReadyEditor::~ArpReadyEditor()
{
    setLookAndFeel (nullptr);
}

void ArpReadyEditor::timerCallback()
{
    display.update (processor);
    const int idx = processor.lastPreset.load();
    const int wanted = idx >= 0 ? idx + 1 : 0;
    if (presetBox.getSelectedId() != wanted)
        presetBox.setSelectedId (wanted, juce::dontSendNotification);

    const int len = juce::roundToInt (processor.state.getRawParameterValue (ParamIDs::patternLength)->load());
    for (int i = 0; i < 16; ++i)
        toggle (ParamIDs::step[i]).setAlpha (i < len ? 1.0f : 0.3f);
    const int step = processor.currentStep.load();
    if (step != shownStep)
    {
        shownStep = step;
        repaint (stepArea);
    }
    const bool fixed = processor.state.getRawParameterValue (ParamIDs::velocityMode)->load() > 0.5f;
    knob (ParamIDs::velocity).setEnabled (fixed);
    knob (ParamIDs::soundLevel).setEnabled (processor.state.getRawParameterValue (ParamIDs::sound)->load() > 0.5f);
    const bool multi = processor.state.getRawParameterValue (ParamIDs::octaves)->load() > 1.5f;
    combo (ParamIDs::octaveMode).box.setEnabled (multi);
    combo (ParamIDs::octaveMode).label.setAlpha (multi ? 1.0f : 0.4f);
}

void ArpReadyEditor::paint (juce::Graphics& g)
{
    g.fillAll (colours::background);

    const float s = (float) getWidth() / baseWidth;
    auto header = getLocalBounds().removeFromTop (juce::roundToInt (48 * s)).reduced (juce::roundToInt (20 * s), 0);

    g.setColour (colours::text);
    g.setFont (juce::FontOptions (24.0f * s, juce::Font::bold));
    g.drawText ("ARP READY", header, juce::Justification::centredLeft);

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

    // Step indicator under the pattern buttons.
    if (shownStep >= 0 && ! stepArea.isEmpty())
    {
        const float w = (float) stepArea.getWidth() / 16.0f;
        auto bar = juce::Rectangle<float> (stepArea.getX() + w * (float) shownStep, (float) stepArea.getY(), w, (float) stepArea.getHeight()).reduced (w * 0.2f, 0.0f);
        g.setColour (colours::good);
        g.fillRoundedRectangle (bar, 2.0f);
    }
}

void ArpReadyEditor::layoutKnobRow (juce::Rectangle<int> area, std::initializer_list<const char*> ids)
{
    const int n = (int) ids.size();
    if (n == 0) return;
    const int w = area.getWidth() / n;
    for (auto* id : ids)
        knob (id).setBounds (area.removeFromLeft (w));
}

void ArpReadyEditor::layoutCombo (Combo& c, juce::Rectangle<int> area, int px16, int px26)
{
    c.label.setBounds (area.removeFromTop (px16));
    c.box.setBounds (area.removeFromTop (px26).reduced (px16 / 4, 0));
}

void ArpReadyEditor::resized()
{
    using namespace ParamIDs;
    const float s = (float) getWidth() / baseWidth;
    auto px = [s] (int v) { return juce::roundToInt ((float) v * s); };
    const int title = px (16), gap = px (6), comboH = px (16) + px (26), btnH = px (28);

    auto r = getLocalBounds();
    r.removeFromTop (px (48));
    r.reduce (px (14), px (8));

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

    // Row A: arpeggio (50 %) | feel
    {
        sections[0] = rowA.removeFromLeft (rowA.getWidth() * 50 / 100);
        auto a = sections[0].reduced (gap);
        a.removeFromTop (title);
        auto c1 = a.removeFromLeft (px (130));
        {
            auto col = c1.withSizeKeepingCentre (c1.getWidth(), comboH * 2 + gap);
            layoutCombo (combo (mode), col.removeFromTop (comboH), px (16), px (26));
            col.removeFromTop (gap);
            layoutCombo (combo (division), col.removeFromTop (comboH), px (16), px (26));
        }
        auto c3 = a.removeFromRight (px (110));
        {
            auto col = c3.withSizeKeepingCentre (c3.getWidth(), comboH + btnH * 2 + gap * 2);
            layoutCombo (combo (octaveMode), col.removeFromTop (comboH), px (16), px (26));
            col.removeFromTop (gap);
            toggle (latch).setBounds (col.removeFromTop (btnH).reduced (px (8), 0));
            col.removeFromTop (gap);
            toggle (hostSync).setBounds (col.removeFromTop (btnH).reduced (px (8), 0));
        }
        layoutKnobRow (a, { octaves });

        sections[1] = rowA;
        a = sections[1].reduced (gap);
        a.removeFromTop (title);
        auto left = a.removeFromLeft (px (110));
        layoutCombo (combo (velocityMode), left.withSizeKeepingCentre (left.getWidth(), comboH), px (16), px (26));
        layoutKnobRow (a, { gate, swing, transpose, velocity, accent });
    }

    // Row B: pattern (76 %) | built-in sound
    {
        sections[2] = rowB.removeFromLeft (rowB.getWidth() * 76 / 100);
        auto a = sections[2].reduced (gap);
        a.removeFromTop (title);
        auto right = a.removeFromRight (px (90));
        knob (patternLength).setBounds (right);
        auto grid = a.withSizeKeepingCentre (a.getWidth() - px (8), btnH + px (8));
        stepArea = grid.removeFromBottom (px (5));
        grid.removeFromBottom (px (3));
        const int w = grid.getWidth() / 16;
        for (int i = 0; i < 16; ++i)
            toggle (step[i]).setBounds (grid.removeFromLeft (w).reduced (px (2), 0));

        sections[3] = rowB;
        a = sections[3].reduced (gap);
        a.removeFromTop (title);
        auto l = a.removeFromLeft (a.getWidth() / 2);
        toggle (sound).setBounds (l.withSizeKeepingCentre (l.getWidth() - px (16), btnH));
        knob (soundLevel).setBounds (a);
    }
}
