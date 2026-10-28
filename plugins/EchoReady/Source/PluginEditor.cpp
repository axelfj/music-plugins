// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginEditor.h"

using namespace rommuser::ui;
using namespace rommuser::echoready;

namespace
{
constexpr int baseWidth = 920, baseHeight = 560;
} // namespace

//==============================================================================
void TapDisplay::setState (const EchoParams& p, float newDelayMs, float newDuckGain, float newWetPeak)
{
    params = p;
    delayMs = newDelayMs;
    duckGain = newDuckGain;
    wetPeak = newWetPeak;
    repaint();
}

void TapDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r, 8.0f);
    r = r.reduced (14.0f, 12.0f);

    // Time axis: up to 8 repeats or 4 seconds, whichever is shorter.
    const float t = std::max (delayMs, 1.0f);
    const float span = std::min (4000.0f, t * 8.5f);
    const bool pingPong = params.mode == Mode::pingPong;
    const float midY = r.getCentreY();

    g.setColour (colours::outline);
    g.drawLine (r.getX(), pingPong ? midY : r.getBottom(), r.getRight(), pingPong ? midY : r.getBottom(), 1.0f);

    // The dry hit.
    g.setColour (colours::text.withAlpha (0.8f));
    g.fillRoundedRectangle (r.getX(), r.getY(), 4.0f, r.getHeight(), 2.0f);

    float level = 1.0f;
    const float fb = std::clamp (params.feedback, 0.0f, 1.1f);
    for (int k = 1; k <= 24; ++k)
    {
        const float time = t * (float) k;
        if (time > span) break;
        const float x = r.getX() + time / span * r.getWidth();
        const float h = std::max (2.0f, level * (pingPong ? r.getHeight() * 0.5f : r.getHeight()));
        g.setColour (colours::accent.withAlpha (std::clamp (0.35f + 0.65f * level, 0.0f, 1.0f)));
        if (pingPong)
        {
            // Odd repeats left (up), even repeats right (down).
            if (k % 2 == 1) g.fillRoundedRectangle (x - 3.0f, midY - h, 6.0f, h, 2.0f);
            else g.fillRoundedRectangle (x - 3.0f, midY, 6.0f, h, 2.0f);
        }
        else
        {
            g.fillRoundedRectangle (x - 3.0f, r.getBottom() - h, 6.0f, h, 2.0f);
        }
        level *= fb;
        if (level < 0.02f) break;
    }

    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (11.0f));
    juce::String label = juce::String (delayMs, 1) + " ms";
    if (params.sync)
        label = Choices::divisions[juce::jlimit (0, Choices::divisions.size() - 1,
            (int) std::distance (std::begin (Choices::divisionBeats),
                                 std::find_if (std::begin (Choices::divisionBeats), std::end (Choices::divisionBeats),
                                               [&] (double b) { return std::fabs (b - params.beats) < 1e-9; })))]
              + juce::String::fromUTF8 ("  ·  ") + label + juce::String::fromUTF8 ("  ·  ") + juce::String (params.bpm, 0) + " bpm";
    g.drawText (label, r.removeFromTop (14.0f), juce::Justification::topRight);
    if (pingPong)
    {
        g.drawText ("L", juce::Rectangle<float> (r.getX(), r.getY(), 20.0f, 14.0f), juce::Justification::centredLeft);
        g.drawText ("R", juce::Rectangle<float> (r.getX(), r.getBottom() - 14.0f, 20.0f, 14.0f), juce::Justification::centredLeft);
    }
    if (params.duck > 0.001f && duckGain < 0.98f)
    {
        g.setColour (colours::good);
        g.drawText (tr ("ducking ", "ducking ") + juce::String (juce::roundToInt ((1.0f - duckGain) * 100.0f)) + " %",
                    r.removeFromBottom (14.0f), juce::Justification::bottomRight);
    }
}

//==============================================================================
EchoReadyEditor::EchoReadyEditor (EchoReadyProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      time (tr ("TIEMPO", "TIME")),
      feedback ("FEEDBACK"),
      lowCut (tr ("CORTE GRAVES", "LOW CUT")),
      highCut (tr ("CORTE AGUDOS", "HIGH CUT")),
      diffusion (tr ("DIFUSIÓN", "DIFFUSION")),
      wow ("WOW"),
      duck ("DUCKING"),
      mix ("MIX"),
      output (tr ("SALIDA", "OUTPUT"))
{
    setLookAndFeel (&lnf);

    struct KnobInfo { Knob* knob; const char* id; juce::String tip; };
    const KnobInfo knobs[] = {
        { &time, ParamIDs::time, tr ("Tiempo del eco en milisegundos (cuando SYNC está apagado).", "Echo time in milliseconds (when SYNC is off).") },
        { &feedback, ParamIDs::feedback, tr ("Cuánto de cada repetición vuelve a entrar. Más de 100 % se auto-oscila (con límite suave).",
                                             "How much of each repeat is fed back in. Above 100% it self-oscillates (softly limited).") },
        { &lowCut, ParamIDs::lowCut, tr ("Quita graves de las repeticiones para que no ensucien el bajo.", "Removes lows from the repeats so they don't muddy the bass.") },
        { &highCut, ParamIDs::highCut, tr ("Quita agudos de las repeticiones: cada eco es más oscuro que el anterior, como una cinta.",
                                           "Removes highs from the repeats: each echo is darker than the last, like tape.") },
        { &diffusion, ParamIDs::diffusion, tr ("Emborrona un poco cada repetición para que suene menos digital.", "Smears each repeat a little so it sounds less digital.") },
        { &wow, ParamIDs::wow, tr ("Vaivén lento del tiempo, como una cinta vieja.", "Slow wobble of the time, like old tape.") },
        { &duck, ParamIDs::duck, tr ("Baja los ecos mientras la señal original suena y los sube en los huecos. Ideal para voces.",
                                     "Turns the echoes down while the original plays and up in the gaps. Great on vocals.") },
        { &mix, ParamIDs::mix, tr ("Mezcla entre la señal original y los ecos.", "Blend between the original and the echoes.") },
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

    struct ComboInfo { juce::ComboBox* box; juce::Label* label; const char* id; const juce::StringArray& items; juce::String name, tip; };
    const ComboInfo combos[] = {
        { &divisionBox, &divisionLabel, ParamIDs::division, Choices::divisions, tr ("FIGURA", "DIVISION"),
          tr ("Tiempo del eco en figuras del tempo del DAW (D = punteado, T = tresillo).", "Echo time in note values of the DAW tempo (D = dotted, T = triplet).") },
        { &modeBox, &modeLabel, ParamIDs::mode, Choices::modes, tr ("MODO", "MODE"),
          tr ("Stereo: cada lado repite lo suyo. Ping-Pong: los ecos saltan de izquierda a derecha.",
              "Stereo: each side repeats its own. Ping-Pong: the echoes bounce left to right.") },
        { &styleBox, &styleLabel, ParamIDs::style, Choices::styles, tr ("ESTILO", "STYLE"),
          tr ("Digital: limpio. Tape: saturación, agudos recortados y wow. Analog: más oscuro, estilo BBD.",
              "Digital: clean. Tape: saturation, rolled-off highs and wow. Analog: darker, BBD style.") },
        { &timeChangeBox, &timeChangeLabel, ParamIDs::timeChange, Choices::timeChanges, tr ("CAMBIO", "TIME CHANGE"),
          tr ("Qué pasa al cambiar el tiempo: Crossfade (sin clics ni cambio de tono) o Glide (el tono se dobla, como en una cinta).",
              "What happens when the time changes: Crossfade (no clicks, no pitch change) or Glide (the pitch bends, like tape).") },
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

    sync.setButtonText ("SYNC");
    sync.setClickingTogglesState (true);
    sync.setTooltip (tr ("Sigue el tempo del DAW con la figura elegida. Apagado: usa el tiempo en ms.",
                         "Follows the DAW tempo with the chosen division. Off: uses the time in ms."));
    addAndMakeVisible (sync);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (processor.state, ParamIDs::sync, sync));

    for (int i = 0; i < 3; ++i)
    {
        auto& b = presetButtons[i];
        b.setButtonText (tr (echoPresets[i].nameEs, echoPresets[i].nameEn));
        b.setTooltip (tr ("Punto de partida.", "Starting point."));
        b.onClick = [this, i] { processor.applyPreset (echoPresets[i]); };
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

EchoReadyEditor::~EchoReadyEditor()
{
    setLookAndFeel (nullptr);
}

void EchoReadyEditor::paint (juce::Graphics& g)
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

void EchoReadyEditor::resized()
{
    const float s = (float) getWidth() / baseWidth;
    auto px = [s] (int v) { return juce::roundToInt ((float) v * s); };

    auto r = getLocalBounds();
    r.removeFromTop (px (48));
    r.reduce (px (16), px (12));

    // Bottom strip: combos, sync, presets.
    auto bottom = r.removeFromBottom (px (62));
    r.removeFromBottom (px (8));
    auto row = bottom.removeFromTop (px (44));
    const int cw = row.getWidth() / 8;
    sync.setBounds (row.removeFromLeft (cw).reduced (px (4), px (10)));
    for (const auto& c : { std::pair { &divisionBox, &divisionLabel }, std::pair { &modeBox, &modeLabel },
                           std::pair { &styleBox, &styleLabel }, std::pair { &timeChangeBox, &timeChangeLabel } })
    {
        auto cell = row.removeFromLeft (cw);
        c.second->setBounds (cell.removeFromTop (px (16)));
        c.first->setBounds (cell.reduced (px (4), px (2)));
    }
    for (auto& b : presetButtons)
        b.setBounds (row.removeFromLeft (cw).reduced (px (4), px (10)));

    // Top: tap display. Then knobs in two rows (5 + 4).
    display.setBounds (r.removeFromTop (r.getHeight() * 36 / 100));
    r.removeFromTop (px (8));
    auto row1 = r.removeFromTop (r.getHeight() / 2);
    const int kw = row1.getWidth() / 5;
    time.setBounds (row1.removeFromLeft (kw));
    feedback.setBounds (row1.removeFromLeft (kw));
    lowCut.setBounds (row1.removeFromLeft (kw));
    highCut.setBounds (row1.removeFromLeft (kw));
    diffusion.setBounds (row1);
    const int kw2 = r.getWidth() / 4;
    wow.setBounds (r.removeFromLeft (kw2));
    duck.setBounds (r.removeFromLeft (kw2));
    mix.setBounds (r.removeFromLeft (kw2));
    output.setBounds (r);
}

void EchoReadyEditor::updateEnablement()
{
    const bool synced = processor.state.getRawParameterValue (ParamIDs::sync)->load() > 0.5f;
    time.setEnabled (! synced);
    time.setAlpha (synced ? 0.4f : 1.0f);
    divisionBox.setEnabled (synced);
    divisionBox.setAlpha (synced ? 1.0f : 0.4f);
}

void EchoReadyEditor::timerCallback()
{
    updateEnablement();
    const float wp = processor.wetPeak.exchange (0.0f);
    wetPeakSmoothed = wp > wetPeakSmoothed ? wp : wetPeakSmoothed * 0.9f;
    display.setState (processor.readParams(), processor.currentDelayMs.load(), processor.duckGain.load(), wetPeakSmoothed);
}
