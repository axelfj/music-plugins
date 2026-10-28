// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginEditor.h"

using namespace rommuser::ui;
using namespace rommuser::stutterready;

namespace
{
constexpr int baseWidth = 1040, baseHeight = 760;

juce::Colour gestureColour (int g)
{
    static const juce::Colour table[numGestures] = {
        colours::accent, colours::warn, colours::good, colours::good.darker (0.3f),
        colours::accent.brighter (0.5f), colours::warn.darker (0.4f), colours::text.darker (0.2f), colours::accent.darker (0.4f)
    };
    return g >= 0 && g < numGestures ? table[g] : colours::outline;
}

void styleLabel (juce::Label& l, const juce::String& text)
{
    l.setText (text, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centred);
    l.setColour (juce::Label::textColourId, colours::textDim);
    l.setFont (juce::FontOptions (12.0f, juce::Font::bold));
}

juce::String gestureName (int g) { return tr (Choices::gestureNamesEs[g], Choices::gestureNamesEn[g]); }
} // namespace

//==============================================================================
StutterDisplay::StutterDisplay (StutterReadyProcessor& p) : processor (p) {}

void StutterDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r, 8.0f);
    auto plot = r.reduced (14.0f, 12.0f);
    auto header = plot.removeFromTop (16.0f);

    g.setColour (colours::background);
    g.fillRect (plot);

    // Beat grid: two beats, 1/16 lines.
    for (int k = 1; k < 8; ++k)
    {
        const float x = plot.getX() + plot.getWidth() * (float) k / 8.0f;
        g.setColour (k == 4 ? colours::outline : colours::outline.withAlpha (0.4f));
        g.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());
    }

    // Slice region.
    const float sliceStart = processor.sliceStartFraction.load();
    const float sliceLen = processor.sliceLengthFraction.load();
    const int gesture = processor.currentGesture.load();
    if (sliceStart >= -1.0f && sliceLen > 0.0f && processor.active.load())
    {
        const float x0 = plot.getX() + juce::jlimit (0.0f, 1.0f, sliceStart) * plot.getWidth();
        const float x1 = plot.getX() + juce::jlimit (0.0f, 1.0f, sliceStart + sliceLen) * plot.getWidth();
        g.setColour (gestureColour (gesture).withAlpha (0.18f));
        g.fillRect (juce::Rectangle<float> (x0, plot.getY(), std::max (2.0f, x1 - x0), plot.getHeight()));
        g.setColour (gestureColour (gesture));
        g.drawVerticalLine (juce::roundToInt (x0), plot.getY(), plot.getBottom());
    }

    // Waveform.
    const int n = StutterReadyProcessor::waveformLength;
    const float mid = plot.getCentreY();
    juce::Path wave;
    for (int i = 0; i < n; ++i)
    {
        const float x = plot.getX() + plot.getWidth() * (float) i / (float) (n - 1);
        const float h = juce::jlimit (0.0f, 1.0f, processor.waveform[(size_t) i].load()) * plot.getHeight() * 0.48f;
        wave.addRectangle (x, mid - h, std::max (1.0f, plot.getWidth() / (float) n), 2.0f * h + 1.0f);
    }
    g.setColour (processor.active.load() ? gestureColour (gesture) : colours::textDim.withAlpha (0.8f));
    g.fillPath (wave);
    g.setColour (colours::outline);
    g.drawRect (plot, 1.0f);

    // Header.
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (11.0f));
    juce::String info;
    info << juce::String (juce::roundToInt (processor.hostBpm.load())) << " BPM";
    if (! processor.playing.load())
        info << juce::String::fromUTF8 ("   ·   ") << tr ("DAW parado: los gestos arrancan al instante y el secuenciador espera", "DAW stopped: gestures start at once and the sequencer waits");
    if (processor.active.load() && gesture >= 0)
    {
        const float beats = processor.divisionNow.load();
        info << juce::String::fromUTF8 ("   ·   ") << gestureName (gesture) << "  1/" << juce::String (juce::roundToInt (4.0f / std::max (0.001f, beats)))
             << juce::String::fromUTF8 ("   ·   ") << juce::String (juce::roundToInt (processor.progress.load() * 100.0f)) << " %";
    }
    g.drawText (info, header, juce::Justification::centredLeft);
    g.drawText (tr ("últimos 2 tiempos", "last 2 beats"), header, juce::Justification::centredRight);
}

//==============================================================================
StepGrid::StepGrid (StutterReadyProcessor& p) : processor (p) {}

int StepGrid::bars() const
{
    return Choices::barsValues[juce::jlimit (0, 2, juce::roundToInt (processor.state.getRawParameterValue (ParamIDs::seqBars)->load()))];
}

int StepGrid::cellAt (juce::Point<int> pt) const
{
    const auto r = getLocalBounds().reduced (2);
    if (! r.contains (pt)) return -1;
    const int col = juce::jlimit (0, stepsPerBar - 1, (pt.x - r.getX()) * stepsPerBar / std::max (1, r.getWidth()));
    const int row = juce::jlimit (0, bars() - 1, (pt.y - r.getY()) * bars() / std::max (1, r.getHeight()));
    return row * stepsPerBar + col;
}

void StepGrid::mouseDown (const juce::MouseEvent& e)
{
    const int cell = cellAt (e.getPosition());
    if (cell < 0) return;
    if (e.mods.isPopupMenu() || e.mods.isRightButtonDown())
        paintValue = -1;
    else
        paintValue = processor.getStep (cell) == selectedGesture ? -1 : selectedGesture;
    processor.setStep (cell, paintValue);
    repaint();
}

void StepGrid::mouseDrag (const juce::MouseEvent& e)
{
    const int cell = cellAt (e.getPosition());
    if (cell < 0) return;
    processor.setStep (cell, paintValue);
    repaint();
}

void StepGrid::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r, 8.0f);
    auto area = r.reduced (2.0f);
    const int rows = bars();
    const float cw = area.getWidth() / stepsPerBar, ch = area.getHeight() / (float) rows;
    const int current = processor.currentStep.load();
    const bool seqOn = processor.state.getRawParameterValue (ParamIDs::seqOn)->load() > 0.5f;
    for (int row = 0; row < rows; ++row)
        for (int col = 0; col < stepsPerBar; ++col)
        {
            const int index = row * stepsPerBar + col;
            auto cell = juce::Rectangle<float> (area.getX() + col * cw, area.getY() + row * ch, cw, ch).reduced (2.0f);
            const int gIdx = processor.getStep (index);
            g.setColour (gIdx >= 0 ? gestureColour (gIdx).withAlpha (seqOn ? 0.9f : 0.5f)
                                   : (col % 4 == 0 ? colours::background.brighter (0.08f) : colours::background));
            g.fillRoundedRectangle (cell, 3.0f);
            if (index == current && seqOn)
            {
                g.setColour (colours::text);
                g.drawRoundedRectangle (cell, 3.0f, 1.5f);
            }
            if (gIdx >= 0 && cw > 30.0f)
            {
                g.setColour (colours::background);
                g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
                g.drawText (juce::String (gIdx + 1), cell, juce::Justification::centred);
            }
        }
}

//==============================================================================
StutterReadyEditor::StutterReadyEditor (StutterReadyProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      display (p),
      grid (p),
      pitch (tr ("TONO", "PITCH")),
      gate ("GATE"),
      filterAmount (tr ("BARRIDO", "SWEEP")),
      pan ("PAN"),
      crush ("CRUSH"),
      echo ("ECO"),
      ramp (tr ("RAMPA", "RAMP")),
      mix ("MIX"),
      output (tr ("SALIDA", "OUTPUT"))
{
    setLookAndFeel (&lnf);

    auto addKnob = [this] (Knob& knob, const juce::String& id, const juce::String& tip, std::vector<std::unique_ptr<SliderAttachment>>& list)
    {
        addAndMakeVisible (knob);
        knob.slider.setTooltip (tip);
        list.push_back (std::make_unique<SliderAttachment> (processor.state, id, knob.slider));
        if (auto* param = processor.state.getParameter (id))
            knob.slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    };
    auto addCombo = [this] (juce::ComboBox& box, juce::Label& label, const juce::StringArray& items, const juce::String& name, const juce::String& tip)
    {
        box.addItemList (items, 1);
        box.setTooltip (tip);
        addAndMakeVisible (box);
        styleLabel (label, name);
        addAndMakeVisible (label);
    };

    addAndMakeVisible (display);
    addAndMakeVisible (grid);

    for (int g = 0; g < numGestures; ++g)
    {
        auto& b = gestureButtons[g];
        b.setButtonText (gestureName (g));
        b.setColour (juce::TextButton::buttonColourId, gestureColour (g).withAlpha (0.35f));
        b.setColour (juce::TextButton::buttonOnColourId, gestureColour (g));
        b.setTooltip (tr ("Mantén pulsado para disparar el gesto (también con la nota MIDI ", "Hold to play the gesture (also with MIDI note ") + juce::String (g + 1)
                      + tr (" de cada octava: C=1, C#=2…). Clic para editarlo abajo.", " of every octave: C=1, C#=2…). Click to edit it below."));
        b.onPress = [this, g] { processor.uiPress (g); selectGesture (g); };
        b.onRelease = [this] { processor.uiRelease(); };
        addAndMakeVisible (b);
    }
    clearButton.setButtonText (tr ("BORRAR PASOS", "CLEAR STEPS"));
    clearButton.setTooltip (tr ("Vacía el secuenciador.", "Empties the sequencer."));
    clearButton.onClick = [this] { processor.clearSteps(); grid.repaint(); };
    addAndMakeVisible (clearButton);

    styleLabel (editLabel, "");
    editLabel.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    editLabel.setColour (juce::Label::textColourId, colours::text);
    editLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (editLabel);
    addCombo (divStartBox, divStartLabel, Choices::divisions, tr ("DIVISIÓN", "DIVISION"), tr ("Tamaño de cada repetición al empezar.", "Length of each repeat at the start."));
    addCombo (divEndBox, divEndLabel, Choices::divisions, tr ("HASTA", "TO"), tr ("Tamaño al final de la rampa (igual = sin rampa; más corto = redoble que acelera).", "Length at the end of the ramp (same = no ramp; shorter = speeding-up roll)."));
    addCombo (filterBox, filterLabel, { "Off", tr ("Pasa bajos", "Low pass"), tr ("Pasa altos", "High pass") }, tr ("FILTRO", "FILTER"), tr ("Filtro que barre durante la rampa.", "Filter that sweeps during the ramp."));
    reverseButton.setButtonText (tr ("REVERSA", "REVERSE"));
    reverseButton.setClickingTogglesState (true);
    reverseButton.setTooltip (tr ("Las repeticiones suenan al revés.", "Repeats play backwards."));
    addAndMakeVisible (reverseButton);
    tapeStopButton.setButtonText ("TAPE STOP");
    tapeStopButton.setClickingTogglesState (true);
    tapeStopButton.setTooltip (tr ("La velocidad baja hasta cero durante la rampa.", "Speed drops to zero over the ramp."));
    addAndMakeVisible (tapeStopButton);
    for (auto* k : { &pitch, &gate, &filterAmount, &pan, &crush, &echo })
        addAndMakeVisible (*k);
    pitch.slider.setTooltip (tr ("Semitonos que sube o baja el tono durante la rampa.", "Semitones the pitch rises or falls over the ramp."));
    gate.slider.setTooltip (tr ("Corta la segunda mitad de cada repetición (efecto trance gate).", "Cuts the second half of each repeat (trance gate)."));
    filterAmount.slider.setTooltip (tr ("Hasta dónde llega el barrido del filtro.", "How far the filter sweep goes."));
    pan.slider.setTooltip (tr ("Alterna las repeticiones izquierda / derecha.", "Alternates repeats left / right."));
    crush.slider.setTooltip (tr ("Reduce bits y frecuencia de muestreo (lo-fi).", "Reduces bits and sample rate (lo-fi)."));
    echo.slider.setTooltip (tr ("Eco de una división de las repeticiones, con realimentación.", "One-division echo of the repeats, with feedback."));

    addKnob (ramp, ParamIDs::ramp, tr ("Duración de las rampas (división, tono, filtro, tape stop) en tiempos.", "Length of the ramps (division, pitch, filter, tape stop) in beats."), sliderAttachments);
    addKnob (mix, ParamIDs::mix, tr ("Mezcla con la señal original.", "Blend with the original signal."), sliderAttachments);
    addKnob (output, ParamIDs::output, tr ("Ganancia final.", "Final gain."), sliderAttachments);
    addCombo (quantizeBox, quantizeLabel, { "Off", "1/4", "1/8", "1/16" }, tr ("CUANTIZAR", "QUANTIZE"),
              tr ("Los gestos disparados a mano o por MIDI esperan a la siguiente línea de esta rejilla (con el DAW en marcha).", "Hand / MIDI triggers wait for the next line of this grid (while the DAW plays)."));
    comboAttachments.push_back (std::make_unique<ComboAttachment> (processor.state, ParamIDs::quantize, quantizeBox));
    addCombo (barsBox, barsLabel, { tr ("1 compás", "1 bar"), tr ("2 compases", "2 bars"), tr ("4 compases", "4 bars") }, tr ("COMPASES", "BARS"), tr ("Largo del patrón del secuenciador.", "Length of the sequencer pattern."));
    comboAttachments.push_back (std::make_unique<ComboAttachment> (processor.state, ParamIDs::seqBars, barsBox));
    seqButton.setButtonText (tr ("SECUENCIADOR", "SEQUENCER"));
    seqButton.setClickingTogglesState (true);
    seqButton.setTooltip (tr ("Toca el patrón de pasos mientras el DAW reproduce.", "Plays the step pattern while the DAW is playing."));
    addAndMakeVisible (seqButton);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (processor.state, ParamIDs::seqOn, seqButton));

    for (int i = 0; i < 6; ++i)
    {
        auto& b = presetButtons[i];
        b.setButtonText (tr (stutterPresets[i].nameEs, stutterPresets[i].nameEn));
        b.setTooltip (tr ("Patrón listo para builds y drops.", "Ready-made pattern for builds and drops."));
        b.onClick = [this, i] { processor.applyPreset (stutterPresets[i]); grid.repaint(); };
        addAndMakeVisible (b);
    }

    selectGesture (0);

    setResizable (true, true);
    setResizeLimits (baseWidth * 3 / 4, baseHeight * 3 / 4, baseWidth * 2, baseHeight * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) baseWidth / baseHeight);
    setSize (baseWidth, baseHeight);
    startTimerHz (30);
}

StutterReadyEditor::~StutterReadyEditor()
{
    setLookAndFeel (nullptr);
}

void StutterReadyEditor::selectGesture (int g)
{
    if (g == selected) return;
    selected = g;
    grid.selectedGesture = g;
    gestureSliderAttachments.clear();
    gestureButtonAttachments.clear();
    gestureComboAttachments.clear();
    auto id = [g] (const char* suffix) { return ParamIDs::gesture (g, suffix); };
    gestureComboAttachments.push_back (std::make_unique<ComboAttachment> (processor.state, id ("divstart"), divStartBox));
    gestureComboAttachments.push_back (std::make_unique<ComboAttachment> (processor.state, id ("divend"), divEndBox));
    gestureComboAttachments.push_back (std::make_unique<ComboAttachment> (processor.state, id ("filter"), filterBox));
    gestureButtonAttachments.push_back (std::make_unique<ButtonAttachment> (processor.state, id ("reverse"), reverseButton));
    gestureButtonAttachments.push_back (std::make_unique<ButtonAttachment> (processor.state, id ("tapestop"), tapeStopButton));
    struct K { Knob* knob; const char* suffix; };
    for (const auto& k : { K { &pitch, "pitch" }, K { &gate, "gate" }, K { &filterAmount, "filteramt" }, K { &pan, "pan" }, K { &crush, "crush" }, K { &echo, "echo" } })
    {
        gestureSliderAttachments.push_back (std::make_unique<SliderAttachment> (processor.state, id (k.suffix), k.knob->slider));
        if (auto* param = processor.state.getParameter (id (k.suffix)))
            k.knob->slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    }
    editLabel.setText (tr ("GESTO ", "GESTURE ") + juce::String (g + 1) + juce::String::fromUTF8 ("  ·  ") + gestureName (g), juce::dontSendNotification);
    editLabel.setColour (juce::Label::textColourId, gestureColour (g));
    for (int i = 0; i < numGestures; ++i)
        gestureButtons[i].setToggleState (i == g, juce::dontSendNotification);
    grid.repaint();
}

void StutterReadyEditor::paint (juce::Graphics& g)
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

    g.setColour (colours::panel.withAlpha (0.5f));
    g.fillRoundedRectangle (editArea.toFloat(), 8.0f * s);
}

void StutterReadyEditor::resized()
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

    // Common row: ramp | quantize | sequencer | bars | mix | output
    auto common = r.removeFromBottom (px (104));
    r.removeFromBottom (px (8));
    const int cw = common.getWidth() / 6;
    ramp.setBounds (common.removeFromLeft (cw));
    {
        auto cell = common.removeFromLeft (cw);
        cell.removeFromTop (px (24));
        quantizeLabel.setBounds (cell.removeFromTop (px (16)));
        quantizeBox.setBounds (cell.removeFromTop (px (26)).reduced (px (8), 0));
    }
    {
        auto cell = common.removeFromLeft (cw);
        cell.removeFromTop (px (40));
        seqButton.setBounds (cell.removeFromTop (px (26)).reduced (px (8), 0));
    }
    {
        auto cell = common.removeFromLeft (cw);
        cell.removeFromTop (px (24));
        barsLabel.setBounds (cell.removeFromTop (px (16)));
        barsBox.setBounds (cell.removeFromTop (px (26)).reduced (px (8), 0));
    }
    mix.setBounds (common.removeFromLeft (cw));
    output.setBounds (common);

    // Gesture editor panel.
    auto edit = r.removeFromBottom (px (150));
    r.removeFromBottom (px (8));
    editArea = edit;
    edit.reduce (px (8), px (6));
    editLabel.setBounds (edit.removeFromTop (px (20)));
    const int ew = edit.getWidth() / 11;
    {
        auto cell = edit.removeFromLeft (ew * 3);
        auto top = cell.removeFromTop (px (44));
        auto a = top.removeFromLeft (top.getWidth() / 2);
        divStartLabel.setBounds (a.removeFromTop (px (16)));
        divStartBox.setBounds (a.removeFromTop (px (26)).reduced (px (4), 0));
        divEndLabel.setBounds (top.removeFromTop (px (16)));
        divEndBox.setBounds (top.removeFromTop (px (26)).reduced (px (4), 0));
        cell.removeFromTop (px (8));
        auto buttons = cell.removeFromTop (px (26));
        reverseButton.setBounds (buttons.removeFromLeft (buttons.getWidth() / 2).reduced (px (4), 0));
        tapeStopButton.setBounds (buttons.reduced (px (4), 0));
        cell.removeFromTop (px (6));
        filterLabel.setBounds (cell.removeFromTop (px (14)));
        filterBox.setBounds (cell.removeFromTop (px (24)).reduced (px (4), 0));
    }
    edit.removeFromLeft (ew / 2);
    const int kw = edit.getWidth() / 6;
    for (auto* k : { &pitch, &gate, &filterAmount, &pan, &crush, &echo })
        k->setBounds (edit.removeFromLeft (kw));

    // Gesture buttons row.
    auto gestures = r.removeFromBottom (px (34));
    r.removeFromBottom (px (8));
    clearButton.setBounds (gestures.removeFromRight (px (130)).reduced (px (4), 0));
    const int gw = gestures.getWidth() / numGestures;
    for (auto& b : gestureButtons)
        b.setBounds (gestures.removeFromLeft (gw).reduced (px (3), 0));

    // Step grid and display share the rest.
    grid.setBounds (r.removeFromBottom (px (118)));
    r.removeFromBottom (px (8));
    display.setBounds (r);
}

void StutterReadyEditor::timerCallback()
{
    const int playingGesture = processor.currentGesture.load();
    for (int i = 0; i < numGestures; ++i)
    {
        const bool lit = i == selected || (processor.active.load() && i == playingGesture);
        gestureButtons[i].setToggleState (lit, juce::dontSendNotification);
    }
    display.repaint();
    grid.repaint();
}
