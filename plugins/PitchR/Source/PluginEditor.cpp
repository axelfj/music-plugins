// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginEditor.h"

using namespace rommuser::ui;
using namespace rommuser::pitchr;

namespace
{
constexpr int baseWidth = 980, baseHeight = 640;

// Display: -90 .. 0 dB over the plot height, tilted so pink noise reads flat (as in EQ-R).
constexpr float floorDb = -90.0f;
constexpr float tiltDbPerOct = 4.5f;
constexpr double minHz = 20.0, maxHz = 20000.0;

constexpr int intervalSteps[] = { -12, -7, -5, -3, 0, 3, 4, 5, 7, 12 };

juce::String noteName (double hz)
{
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const int midi = juce::roundToInt (69.0 + 12.0 * std::log2 (hz / 440.0));
    return juce::String (names[((midi % 12) + 12) % 12]) + juce::String (midi / 12 - 1);
}

// "Quinta justa arriba" / "Perfect fifth up" for a whole number of semitones.
juce::String intervalName (int semis)
{
    if (semis == 0) return tr ("Sin cambio", "No change");
    static const char* es[] = { "Unísono", "Segunda menor", "Segunda mayor", "Tercera menor", "Tercera mayor", "Cuarta justa", "Tritono",
                                "Quinta justa", "Sexta menor", "Sexta mayor", "Séptima menor", "Séptima mayor" };
    static const char* en[] = { "Unison", "Minor second", "Major second", "Minor third", "Major third", "Perfect fourth", "Tritone",
                                "Perfect fifth", "Minor sixth", "Major sixth", "Minor seventh", "Major seventh" };
    const int a = std::abs (semis), octaves = a / 12, rest = a % 12;
    juce::String name;
    if (rest == 0)
        name = octaves == 1 ? tr ("Octava", "Octave") : tr ("Dos octavas", "Two octaves");
    else
    {
        name = tr (es[rest], en[rest]);
        if (octaves > 0) name << (octaves == 1 ? tr (" + octava", " + octave") : tr (" + 2 octavas", " + 2 octaves"));
    }
    return name + (semis > 0 ? tr (" arriba", " up") : tr (" abajo", " down"));
}

float xForHz (double hz, juce::Rectangle<float> plot)
{
    return plot.getX() + (float) (std::log (hz / minHz) / std::log (maxHz / minHz)) * plot.getWidth();
}
} // namespace

//==============================================================================
PitchDisplay::PitchDisplay (PitchRProcessor& p) : processor (p) {}

void PitchDisplay::update()
{
    const double sr = processor.currentSampleRate.load();
    inAnalyzer.setSampleRate (sr);
    outAnalyzer.setSampleRate (sr);
    inAnalyzer.update (processor.inSpectrum);
    outAnalyzer.update (processor.outSpectrum);
    repaint();
}

juce::Path PitchDisplay::curve (const SpectrumAnalyzer& a, juce::Rectangle<float> plot) const
{
    juce::Path path;
    const int points = juce::jmax (64, (int) plot.getWidth());
    for (int i = 0; i <= points; ++i)
    {
        const float x = plot.getX() + plot.getWidth() * (float) i / (float) points;
        const double f = minHz * std::pow (maxHz / minHz, (double) i / points);
        // 1/6-octave power average: the shape reads like an ear hears it, not like FFT grass
        double power = 0.0;
        for (int j = -4; j <= 4; ++j)
            power += std::pow (10.0, a.levelAt (f * std::pow (2.0, j / 48.0)) / 10.0);
        const float smoothed = (float) (10.0 * std::log10 (power / 9.0 + 1.0e-12));
        const float level = juce::jlimit (floorDb, 0.0f, smoothed + tiltDbPerOct * (float) std::log2 (f / 1000.0));
        const float y = plot.getBottom() - (level - floorDb) / -floorDb * plot.getHeight();
        if (i == 0) path.startNewSubPath (x, y);
        else path.lineTo (x, y);
    }
    return path;
}

void PitchDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r, 8.0f);

    auto plot = r.reduced (16.0f, 12.0f);
    auto axis = plot.removeFromBottom (16.0f);

    // Grid: decades bright, the 2 / 5 steps faint; level lines every 18 dB.
    for (int f : { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 })
    {
        const float x = xForHz ((double) f, plot);
        const bool decade = f == 100 || f == 1000 || f == 10000;
        g.setColour (colours::outline.withAlpha (decade ? 1.0f : 0.5f));
        g.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());
        g.setColour (colours::textDim);
        g.setFont (juce::FontOptions (10.0f));
        const auto label = f >= 1000 ? juce::String (f / 1000) + "k" : juce::String (f);
        g.drawText (label, juce::Rectangle<float> (x - 20.0f, axis.getY() + 2.0f, 40.0f, 12.0f), juce::Justification::centred);
    }
    for (float db = 0.0f; db > floorDb; db -= 18.0f)
    {
        const float y = plot.getBottom() - (db - floorDb) / -floorDb * plot.getHeight();
        g.setColour (colours::outline.withAlpha (0.45f));
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
    }

    // Input: dim filled area. Output: accent line over a soft gradient.
    {
        auto in = curve (inAnalyzer, plot);
        auto fill = in;
        fill.lineTo (plot.getRight(), plot.getBottom());
        fill.lineTo (plot.getX(), plot.getBottom());
        fill.closeSubPath();
        g.setColour (colours::textDim.withAlpha (0.16f));
        g.fillPath (fill);
        g.setColour (colours::textDim.withAlpha (0.55f));
        g.strokePath (in, juce::PathStrokeType (1.0f));
    }
    {
        auto out = curve (outAnalyzer, plot);
        auto fill = out;
        fill.lineTo (plot.getRight(), plot.getBottom());
        fill.lineTo (plot.getX(), plot.getBottom());
        fill.closeSubPath();
        g.setGradientFill (juce::ColourGradient (colours::accent.withAlpha (0.30f), 0.0f, plot.getY(),
                                                 colours::accent.withAlpha (0.02f), 0.0f, plot.getBottom(), false));
        g.fillPath (fill);
        g.setColour (colours::accent);
        g.strokePath (out, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Overlay: the shift in big type, its name, and the loudest note mapped from input to output.
    const auto params = processor.readParams();
    const int semis = juce::roundToInt (params.semitones);
    const int cents = juce::roundToInt (params.cents);
    const float ratio = std::pow (2.0f, ((float) semis + (float) cents * 0.01f) / 12.0f);

    auto info = plot.reduced (12.0f, 8.0f);
    juce::String amount = (semis > 0 ? "+" : "") + juce::String (semis) + " st";
    if (cents != 0) amount << "  " << (cents > 0 ? "+" : "") << juce::String (cents) << " c";
    g.setColour (params.original ? colours::textDim : colours::accent);
    g.setFont (juce::FontOptions (30.0f, juce::Font::bold));
    auto top = info.removeFromTop (34.0f);
    g.drawText (amount, top, juce::Justification::centredLeft);
    g.setColour (colours::text);
    g.setFont (juce::FontOptions (14.0f));
    g.drawText (intervalName (semis) + (cents != 0 ? tr (" (ajuste fino)", " (fine-tuned)") : juce::String()),
                info.removeFromTop (20.0f), juce::Justification::centredLeft);

    const double loudest = inAnalyzer.strongestHz (40.0, 2000.0, -60.0f);
    if (loudest > 0.0)
    {
        g.setColour (colours::textDim);
        g.setFont (juce::FontOptions (12.0f));
        g.drawText (tr ("nota más fuerte  ", "loudest note  ") + noteName (loudest) + juce::String::fromUTF8 ("  →  ") + noteName (loudest * ratio),
                    info.removeFromTop (18.0f), juce::Justification::centredLeft);
    }

    // Top right: legend and state.
    auto legend = plot.reduced (12.0f, 10.0f).removeFromTop (16.0f);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.setColour (colours::accent);
    g.drawText (tr ("SALIDA", "OUTPUT"), legend.removeFromRight (56.0f), juce::Justification::centredLeft);
    g.fillRect (legend.removeFromRight (18.0f).withSizeKeepingCentre (12.0f, 2.0f));
    legend.removeFromRight (14.0f);
    g.setColour (colours::textDim);
    g.drawText (tr ("ENTRADA", "INPUT"), legend.removeFromRight (56.0f), juce::Justification::centredLeft);
    g.fillRect (legend.removeFromRight (18.0f).withSizeKeepingCentre (12.0f, 2.0f));

    juce::String status;
    if (params.original)
        status = tr ("ORIGINAL: estás oyendo la señal sin cambiar (A/B)", "ORIGINAL: you are hearing the unchanged signal (A/B)");
    else if (semis == 0 && cents == 0 && std::abs (params.formantSemis) < 0.05f)
        status = tr ("0 st: la señal pasa idéntica", "0 st: the signal passes untouched");
    if (status.isNotEmpty())
    {
        g.setColour (params.original ? colours::accent : colours::textDim);
        g.setFont (juce::FontOptions (11.0f));
        g.drawText (status, plot.reduced (12.0f, 10.0f).withTrimmedTop (18.0f).removeFromTop (14.0f), juce::Justification::centredRight);
    }
}

//==============================================================================
PitchREditor::PitchREditor (PitchRProcessor& p)
    : processor (p),
      pitch (tr ("TONO", "PITCH")),
      fine (tr ("AJUSTE FINO", "FINE")),
      formant (tr ("FORMANTE", "FORMANT")),
      mix ("MIX"),
      output (tr ("SALIDA", "OUTPUT")),
      display (p)
{
    setLookAndFeel (&lnf);

    struct KnobInfo { Knob* knob; const char* id; juce::String tip; };
    const KnobInfo knobs[] = {
        { &pitch, ParamIDs::semitones, tr ("Cuántos semitonos sube o baja. 12 = una octava. Doble clic vuelve a 0.",
                                           "How many semitones up or down. 12 = one octave. Double-click returns to 0.") },
        { &fine, ParamIDs::cents, tr ("Ajuste fino en cents (100 cents = 1 semitono), para afinar un sample a tu canción.",
                                      "Fine tuning in cents (100 cents = 1 semitone), to tune a sample to your song.") },
        { &formant, ParamIDs::formant, tr ("Mueve el timbre sin cambiar la nota: negativo = voz más grande / grave, positivo = más pequeña / aguda.",
                                           "Moves the timbre without changing the note: negative = bigger / deeper voice, positive = smaller / brighter.") },
        { &mix, ParamIDs::mix, tr ("Mezcla con la señal original (por ejemplo 50 % para una octava doblada).",
                                   "Blend with the original signal (for example 50% for an octave double).") },
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
    pitch.label.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    pitch.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 22);

    const juce::String modeTips[] = {
        tr ("Limpio: la mejor calidad, para mezclas, acordes, pads, bajos y samples. Más latencia.",
            "Clean: best quality, for mixes, chords, pads, bass and samples. Most latency."),
        tr ("Voz: para una sola voz o instrumento, con la mitad de latencia. Activa Conservar formantes para voces.",
            "Voice: for one voice or instrument, at half the latency. Turn on Keep formants for vocals."),
        tr ("Rápido: la menor latencia, para tocar o cantar en vivo. Los acordes suenan menos limpios.",
            "Fast: lowest latency, for playing or singing live. Chords sound less clean."),
    };
    for (int m = 0; m < numModes; ++m)
    {
        auto& b = modeButtons[m];
        b.setRadioGroupId (1001);
        b.setTooltip (modeTips[m]);
        b.onClick = [this, m] { processor.setParameter (ParamIDs::mode, (float) m); };
        addAndMakeVisible (b);
    }
    modeLabel.setText (tr ("MODO", "MODE"), juce::dontSendNotification);
    intervalsLabel.setText (tr ("INTERVALOS", "INTERVALS"), juce::dontSendNotification);
    for (auto* l : { &modeLabel, &intervalsLabel })
    {
        l->setJustificationType (juce::Justification::centred);
        l->setColour (juce::Label::textColourId, colours::textDim);
        l->setFont (juce::FontOptions (12.0f, juce::Font::bold));
        addAndMakeVisible (*l);
    }

    for (int i = 0; i < numIntervals; ++i)
    {
        auto& b = intervalButtons[i];
        const int st = intervalSteps[i];
        b.setButtonText ((st > 0 ? "+" : "") + juce::String (st));
        b.setTooltip (intervalName (st));
        b.onClick = [this, st]
        {
            processor.setParameter (ParamIDs::semitones, (float) st);
            processor.setParameter (ParamIDs::cents, 0.0f);
        };
        addAndMakeVisible (b);
    }

    keepFormantsButton.setButtonText (tr ("CONSERVAR FORMANTES", "KEEP FORMANTS"));
    keepFormantsButton.setClickingTogglesState (true);
    keepFormantsButton.setTooltip (tr ("La voz cambia de nota pero sigue sonando a la misma persona (sin efecto ardilla ni monstruo). Para voces; en instrumentos y mezclas déjalo apagado.",
                                       "The voice changes note but still sounds like the same person (no chipmunk or monster). For vocals; leave it off on instruments and mixes."));
    addAndMakeVisible (keepFormantsButton);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (processor.state, ParamIDs::keepFormants, keepFormantsButton));

    originalButton.setButtonText ("ORIGINAL (A/B)");
    originalButton.setClickingTogglesState (true);
    originalButton.setTooltip (tr ("Escucha la señal sin cambiar, con la misma latencia, para comparar.",
                                   "Hear the unchanged signal, with the same latency, to compare."));
    addAndMakeVisible (originalButton);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (processor.state, ParamIDs::original, originalButton));

    addAndMakeVisible (display);
    processor.inSpectrum.active.store (true);
    processor.outSpectrum.active.store (true);

    // Laid out at its base size; ScalableEditor zooms it as vectors to any window size.
    setSize (baseWidth, baseHeight);

    refreshButtons();
    startTimerHz (30);
}

PitchREditor::~PitchREditor()
{
    processor.inSpectrum.active.store (false);
    processor.outSpectrum.active.store (false);
    setLookAndFeel (nullptr);
}

void PitchREditor::paint (juce::Graphics& g)
{
    g.fillAll (colours::background);

    const float s = (float) getWidth() / baseWidth;
    drawHeader (g, getLocalBounds().removeFromTop (juce::roundToInt (48 * s)), s);
}

void PitchREditor::resized()
{
    const float s = (float) getWidth() / baseWidth;
    auto px = [s] (int v) { return juce::roundToInt ((float) v * s); };

    auto r = getLocalBounds();
    r.removeFromTop (px (48));
    r.reduce (px (16), px (12));

    display.setBounds (r.removeFromTop (px (300)));
    r.removeFromTop (px (12));

    // Row: MODE [clean | voice | fast] ......... [keep formants] [original]
    auto row = r.removeFromTop (px (34));
    modeLabel.setBounds (row.removeFromLeft (px (56)));
    for (auto& b : modeButtons)
        b.setBounds (row.removeFromLeft (px (148)).reduced (px (2), 0));
    originalButton.setBounds (row.removeFromRight (px (160)).reduced (px (2), 0));
    row.removeFromRight (px (8));
    keepFormantsButton.setBounds (row.removeFromRight (px (210)).reduced (px (2), 0));
    r.removeFromTop (px (10));

    // Big pitch knob, interval buttons, then the small knobs.
    pitch.setBounds (r.removeFromLeft (px (210)));
    auto grid = r.removeFromLeft (px (270)).reduced (px (8), 0);
    {
        auto g = grid.withSizeKeepingCentre (grid.getWidth(), px (108));
        intervalsLabel.setBounds (g.removeFromTop (px (20)));
        const int bw = g.getWidth() / 5;
        auto row1 = g.removeFromTop (px (42)), row2 = g.removeFromTop (px (42));
        for (int i = 0; i < numIntervals; ++i)
            intervalButtons[i].setBounds ((i < 5 ? row1 : row2).removeFromLeft (bw).reduced (px (2), px (3)));
    }
    r.removeFromLeft (px (8));
    const int kw = r.getWidth() / 4;
    auto small = r.withSizeKeepingCentre (r.getWidth(), juce::jmin (r.getHeight(), px (150)));
    fine.setBounds (small.removeFromLeft (kw));
    formant.setBounds (small.removeFromLeft (kw));
    mix.setBounds (small.removeFromLeft (kw));
    output.setBounds (small);
}

void PitchREditor::refreshButtons()
{
    const auto params = processor.readParams();
    static const char* namesEs[] = { "LIMPIO", "VOZ", "RÁPIDO" };
    static const char* namesEn[] = { "CLEAN", "VOICE", "FAST" };
    for (int m = 0; m < numModes; ++m)
    {
        auto& b = modeButtons[m];
        b.setButtonText (tr (namesEs[m], namesEn[m]) + juce::String::fromUTF8 ("  ·  ")
                         + juce::String (juce::roundToInt (processor.latencyMsFor ((Mode) m))) + " ms");
        b.setToggleState ((int) params.mode == m, juce::dontSendNotification);
    }
    const int semis = juce::roundToInt (params.semitones);
    const bool onStep = juce::roundToInt (params.cents) == 0;
    for (int i = 0; i < numIntervals; ++i)
        intervalButtons[i].setToggleState (onStep && intervalSteps[i] == semis, juce::dontSendNotification);
}

void PitchREditor::timerCallback()
{
    refreshButtons();
    display.update();
}
