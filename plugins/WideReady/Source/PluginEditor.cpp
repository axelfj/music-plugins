// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginEditor.h"

using namespace rommuser::ui;
using namespace rommuser::wideready;

namespace
{
constexpr int baseWidth = 920, baseHeight = 560;
} // namespace

//==============================================================================
void VoiceDisplay::setState (const WideParams& p, const float delays[2][3], int n)
{
    params = p;
    voices = juce::jlimit (0, maxVoices, n);
    for (int ch = 0; ch < 2; ++ch)
        for (int v = 0; v < maxVoices; ++v)
            delayMs[ch][v] = delays[ch][v];
    repaint();
}

void VoiceDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r, 8.0f);
    r = r.reduced (14.0f, 12.0f);

    auto title = r.removeFromTop (14.0f);
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (11.0f));
    juce::String label;
    switch (params.mode)
    {
        case Mode::chorus:     label = tr ("Chorus · ", "Chorus · ") + juce::String (voices) + tr (" voces", " voices") + (params.ensemble ? tr (" · ensemble", " · ensemble") : juce::String()); break;
        case Mode::flanger:    label = tr ("Flanger", "Flanger") + (params.throughZero ? tr (" · through-zero", " · through-zero") : juce::String()) + (params.invert ? tr (" · invertido", " · inverted") : juce::String()); break;
        case Mode::microPitch: label = tr ("Micro-pitch · L +", "Micro-pitch · L +") + juce::String (params.cents, 1) + tr (" ct · R −", " ct · R −") + juce::String (params.cents, 1) + " ct"; break;
    }
    g.drawText (label, title, juce::Justification::topLeft);

    // Axis: 0 .. 40 ms (log-ish so the flanger's sub-millisecond sweeps are visible).
    const float maxMs = 40.0f;
    auto xFor = [&] (float ms) { return r.getX() + std::log1p (std::max (ms, 0.0f) * 4.0f) / std::log1p (maxMs * 4.0f) * r.getWidth(); };
    auto axis = r.removeFromBottom (16.0f);
    g.setColour (colours::outline);
    for (float ms : { 0.1f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 20.0f, 40.0f })
    {
        g.drawVerticalLine (juce::roundToInt (xFor (ms)), r.getY(), r.getBottom());
        g.setColour (colours::textDim);
        g.drawText (ms < 1.0f ? juce::String (ms, 1) : juce::String (ms, 0), juce::Rectangle<float> (xFor (ms) - 16.0f, axis.getY() + 2.0f, 32.0f, 12.0f), juce::Justification::centred);
        g.setColour (colours::outline);
    }
    g.setColour (colours::textDim);
    g.drawText ("ms", axis.removeFromRight (24.0f), juce::Justification::centredRight);

    // Two rows: L and R.
    const float rowH = r.getHeight() / 2.0f;
    for (int ch = 0; ch < 2; ++ch)
    {
        auto row = juce::Rectangle<float> (r.getX(), r.getY() + rowH * (float) ch, r.getWidth(), rowH).reduced (0.0f, 6.0f);
        g.setColour (colours::outline);
        g.drawHorizontalLine (juce::roundToInt (row.getCentreY()), row.getX(), row.getRight());
        g.setColour (colours::textDim);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (ch == 0 ? "L" : "R", juce::Rectangle<float> (row.getX() - 12.0f, row.getCentreY() - 8.0f, 12.0f, 16.0f), juce::Justification::centredLeft);

        if (params.mode == Mode::flanger && params.throughZero)
        {
            // The delayed dry sits at the manual delay; the sweep crosses it.
            g.setColour (colours::text.withAlpha (0.6f));
            const float x = xFor (std::min (params.delayMs, 20.0f));
            g.drawLine (x, row.getY(), x, row.getBottom(), 1.0f);
        }
        for (int v = 0; v < voices; ++v)
        {
            const float ms = delayMs[ch][v];
            if (ms <= 0.0f) continue;
            const float x = xFor (ms);
            g.setColour ((ch == 0 ? colours::accent : colours::good).withAlpha (v == 0 ? 1.0f : 0.7f));
            g.fillRoundedRectangle (x - 3.0f, row.getY(), 6.0f, row.getHeight(), 3.0f);
        }
    }
}

//==============================================================================
WideReadyEditor::WideReadyEditor (WideReadyProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      rate (tr ("VELOCIDAD", "RATE")),
      depth (tr ("PROFUNDIDAD", "DEPTH")),
      delay (tr ("RETARDO", "DELAY")),
      feedback ("FEEDBACK"),
      spread (tr ("ESTÉREO", "SPREAD")),
      cents (tr ("DESAFINE", "DETUNE")),
      mix ("MIX"),
      output (tr ("SALIDA", "OUTPUT"))
{
    setLookAndFeel (&lnf);

    struct KnobInfo { Knob* knob; const char* id; juce::String tip; };
    const KnobInfo knobs[] = {
        { &rate, ParamIDs::rate, tr ("Velocidad del vaivén.", "Speed of the wobble.") },
        { &depth, ParamIDs::depth, tr ("Cuánto se mueve el retardo. En Flanger, 100 % con through-zero cruza el cero.",
                                       "How far the delay moves. In Flanger, 100% with through-zero crosses zero.") },
        { &delay, ParamIDs::delay, tr ("Chorus: retardo base (5-40 ms suena a coro). Flanger: retardo manual (0.1-10 ms).",
                                       "Chorus: base delay (5-40 ms sounds like a choir). Flanger: manual delay (0.1-10 ms).") },
        { &feedback, ParamIDs::feedback, tr ("Flanger: realimenta la salida para un sonido más metálico. Negativo cambia el timbre.",
                                             "Flanger: feeds the output back for a more metallic sound. Negative changes the timbre.") },
        { &spread, ParamIDs::spread, tr ("Desfase del vaivén entre izquierda y derecha. 0 = mono.", "Wobble offset between left and right. 0 = mono.") },
        { &cents, ParamIDs::cents, tr ("Micro-pitch: desafine en cents. La izquierda sube y la derecha baja: ancho sin vaivén audible.",
                                       "Micro-pitch: detune in cents. Left goes up, right goes down: width without audible wobble.") },
        { &mix, ParamIDs::mix, tr ("Mezcla entre la señal original y las voces. 50 % es el clásico.", "Blend between the original and the voices. 50% is the classic.") },
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
        { &modeBox, &modeLabel, ParamIDs::mode, Choices::modes, tr ("MODO", "MODE"),
          tr ("Chorus: voces con retardo largo. Flanger: retardo corto con feedback. Micro-pitch: dos voces desafinadas, sin vaivén.",
              "Chorus: voices with a long delay. Flanger: short delay with feedback. Micro-pitch: two detuned voices, no wobble.") },
        { &voicesBox, &voicesLabel, ParamIDs::voices, Choices::voices, tr ("VOCES", "VOICES"),
          tr ("Número de voces del chorus.", "Number of chorus voices.") },
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

    struct ToggleInfo { juce::TextButton* button; const char* id; juce::String text, tip; };
    const ToggleInfo toggles[] = {
        { &ensemble, ParamIDs::ensemble, "ENSEMBLE", tr ("Cada voz va a su propia velocidad: más rico, estilo string machine.", "Each voice runs at its own speed: richer, string-machine style.") },
        { &throughZero, ParamIDs::throughZero, "THRU-ZERO", tr ("Retrasa también la señal original para que el barrido la cruce: el flanger de cinta clásico.",
                                                                  "Delays the original too so the sweep crosses it: the classic tape flanger.") },
        { &invert, ParamIDs::invert, tr ("INVERTIR", "INVERT"), tr ("Invierte la polaridad de las voces: con through-zero se cancela al cruzar.",
                                                                     "Flips the voices' polarity: with through-zero it cancels at the crossing.") },
    };
    for (const auto& t : toggles)
    {
        t.button->setButtonText (t.text);
        t.button->setClickingTogglesState (true);
        t.button->setTooltip (t.tip);
        addAndMakeVisible (*t.button);
        buttonAttachments.push_back (std::make_unique<ButtonAttachment> (processor.state, t.id, *t.button));
    }

    for (int i = 0; i < 4; ++i)
    {
        auto& b = presetButtons[i];
        b.setButtonText (tr (widePresets[i].nameEs, widePresets[i].nameEn));
        b.setTooltip (tr ("Punto de partida.", "Starting point."));
        b.onClick = [this, i] { processor.applyPreset (widePresets[i]); };
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

WideReadyEditor::~WideReadyEditor()
{
    setLookAndFeel (nullptr);
}

void WideReadyEditor::paint (juce::Graphics& g)
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

void WideReadyEditor::resized()
{
    const float s = (float) getWidth() / baseWidth;
    auto px = [s] (int v) { return juce::roundToInt ((float) v * s); };

    auto r = getLocalBounds();
    r.removeFromTop (px (48));
    r.reduce (px (16), px (12));

    // Bottom strip: mode, voices, toggles, presets.
    auto bottom = r.removeFromBottom (px (62));
    r.removeFromBottom (px (8));
    auto row = bottom.removeFromTop (px (44));
    const int cw = row.getWidth() / 9;
    for (const auto& c : { std::pair { &modeBox, &modeLabel }, std::pair { &voicesBox, &voicesLabel } })
    {
        auto cell = row.removeFromLeft (cw);
        c.second->setBounds (cell.removeFromTop (px (16)));
        c.first->setBounds (cell.reduced (px (4), px (2)));
    }
    for (auto* b : { &ensemble, &throughZero, &invert })
        b->setBounds (row.removeFromLeft (cw).reduced (px (4), px (10)));
    for (auto& b : presetButtons)
        b.setBounds (row.removeFromLeft (cw).reduced (px (4), px (10)));

    // Top: voice display. Then one row of 8 knobs.
    display.setBounds (r.removeFromTop (r.getHeight() * 42 / 100));
    r.removeFromTop (px (8));
    const int kw = r.getWidth() / 8;
    rate.setBounds (r.removeFromLeft (kw));
    depth.setBounds (r.removeFromLeft (kw));
    delay.setBounds (r.removeFromLeft (kw));
    feedback.setBounds (r.removeFromLeft (kw));
    spread.setBounds (r.removeFromLeft (kw));
    cents.setBounds (r.removeFromLeft (kw));
    mix.setBounds (r.removeFromLeft (kw));
    output.setBounds (r);
}

void WideReadyEditor::updateEnablement()
{
    const auto mode = (Mode) juce::jlimit (0, 2, juce::roundToInt (processor.state.getRawParameterValue (ParamIDs::mode)->load()));
    const bool chorus = mode == Mode::chorus, flanger = mode == Mode::flanger, micro = mode == Mode::microPitch;
    auto enable = [] (juce::Component& c, bool on) { c.setEnabled (on); c.setAlpha (on ? 1.0f : 0.4f); };
    enable (rate, ! micro);
    enable (depth, ! micro);
    enable (delay, ! micro);
    enable (spread, ! micro);
    enable (feedback, flanger);
    enable (throughZero, flanger);
    enable (invert, flanger);
    enable (voicesBox, chorus);
    enable (ensemble, chorus);
    enable (cents, micro);
}

void WideReadyEditor::timerCallback()
{
    updateEnablement();
    float delays[2][3];
    for (int ch = 0; ch < 2; ++ch)
        for (int v = 0; v < maxVoices; ++v)
            delays[ch][v] = processor.voiceDelayMs[(size_t) ch][(size_t) v].load();
    display.setState (processor.readParams(), delays, processor.activeVoices.load());
}
