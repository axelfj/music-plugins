// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginEditor.h"

#include <complex>

using namespace rommuser::ui;
using namespace rommuser::phaseready;

namespace
{
constexpr int baseWidth = 920, baseHeight = 540;
constexpr float minDb = -36.0f, maxDb = 12.0f;
} // namespace

//==============================================================================
void ResponseDisplay::setState (const PhaseParams& p, const float* cL, const float* cR, int n, float l, float r)
{
    params = p;
    stages = juce::jlimit (0, maxStages, n);
    for (int k = 0; k < stages; ++k) { coeffs[0][k] = cL[k]; coeffs[1][k] = cR[k]; }
    lfoL = l;
    lfoR = r;
    repaint();
}

float ResponseDisplay::responseDb (const float* c, int n, float fb, float mixAmount, double w) const
{
    // H = (1 - mix) + mix * A / (1 - fb * A), A = product of (a + z^-1) / (1 + a z^-1)
    const std::complex<double> z1 = std::polar (1.0, -w);
    std::complex<double> a (1.0, 0.0);
    for (int k = 0; k < n; ++k)
    {
        const double ak = c[k];
        a *= (ak + z1) / (1.0 + ak * z1);
    }
    const std::complex<double> wet = a / (1.0 - (double) fb * a);
    const std::complex<double> h = (1.0 - (double) mixAmount) + (double) mixAmount * wet;
    return (float) (20.0 * std::log10 (std::max (std::abs (h), 1e-6)));
}

void ResponseDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r, 8.0f);
    auto plot = r.reduced (14.0f, 12.0f);
    plot.removeFromBottom (16.0f);

    const double sampleRate = 48000.0;
    auto xFor = [&] (double f) { return plot.getX() + (float) (std::log (f / 20.0) / std::log (20000.0 / 20.0)) * plot.getWidth(); };
    auto yFor = [&] (float db) { return plot.getBottom() - (juce::jlimit (minDb, maxDb, db) - minDb) / (maxDb - minDb) * plot.getHeight(); };

    g.setColour (colours::outline);
    for (double f : { 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0 })
        g.drawVerticalLine (juce::roundToInt (xFor (f)), plot.getY(), plot.getBottom());
    for (float db : { -24.0f, -12.0f, 0.0f })
        g.drawHorizontalLine (juce::roundToInt (yFor (db)), plot.getX(), plot.getRight());
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (10.0f));
    for (double f : { 100.0, 1000.0, 10000.0 })
        g.drawText (f >= 1000.0 ? juce::String (f / 1000.0, 0) + "k" : juce::String (f, 0),
                    juce::Rectangle<float> (xFor (f) - 20.0f, plot.getBottom() + 2.0f, 40.0f, 12.0f), juce::Justification::centred);

    if (stages > 0)
    {
        const bool stereo = params.spread > 0.001f;
        for (int ch = stereo ? 1 : 0; ch >= 0; --ch)
        {
            juce::Path path;
            const int points = 180;
            for (int i = 0; i <= points; ++i)
            {
                const double f = 20.0 * std::pow (1000.0, (double) i / points);
                const double w = 2.0 * juce::MathConstants<double>::pi * f / sampleRate;
                const float db = responseDb (coeffs[ch], stages, params.feedback, params.mix, w);
                const float x = xFor (f), y = yFor (db);
                if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
            }
            g.setColour (ch == 0 ? colours::accent : colours::good.withAlpha (0.7f));
            g.strokePath (path, juce::PathStrokeType (ch == 0 ? 2.0f : 1.5f));
        }
    }

    // LFO position indicators (top-right): two dots moving left-right.
    auto lfoArea = juce::Rectangle<float> (plot.getRight() - 90.0f, plot.getY() + 4.0f, 80.0f, 12.0f);
    g.setColour (colours::outline);
    g.drawRoundedRectangle (lfoArea, 6.0f, 1.0f);
    g.setColour (colours::accent);
    g.fillEllipse (lfoArea.getX() + 4.0f + (lfoL * 0.5f + 0.5f) * (lfoArea.getWidth() - 16.0f), lfoArea.getY() + 2.0f, 8.0f, 8.0f);
    g.setColour (colours::good.withAlpha (0.8f));
    g.fillEllipse (lfoArea.getX() + 4.0f + (lfoR * 0.5f + 0.5f) * (lfoArea.getWidth() - 16.0f), lfoArea.getY() + 2.0f, 8.0f, 8.0f);
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("LFO", lfoArea.translated (-32.0f, 0.0f).withWidth (28.0f), juce::Justification::centredRight);

    juce::String label = juce::String (stages) + " " + tr ("etapas", "stages");
    if (params.sync)
        label << "  ·  " << Choices::divisions[juce::jlimit (0, Choices::divisions.size() - 1,
            (int) std::distance (std::begin (Choices::divisionBeats),
                                 std::find_if (std::begin (Choices::divisionBeats), std::end (Choices::divisionBeats),
                                               [&] (double b) { return std::fabs (b - params.beats) < 1e-9; })))]
              << "  ·  " << juce::String (params.bpm, 0) << " bpm";
    else
        label << "  ·  " << juce::String (params.rateHz, params.rateHz < 1.0f ? 2 : 1) << " Hz";
    g.drawText (label, plot.removeFromTop (14.0f), juce::Justification::topLeft);
}

//==============================================================================
PhaseReadyEditor::PhaseReadyEditor (PhaseReadyProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      rate (tr ("VELOCIDAD", "RATE")),
      depth (tr ("PROFUNDIDAD", "DEPTH")),
      feedback ("FEEDBACK"),
      center (tr ("CENTRO", "CENTER")),
      spread (tr ("ESTÉREO", "SPREAD")),
      mix ("MIX"),
      output (tr ("SALIDA", "OUTPUT"))
{
    setLookAndFeel (&lnf);

    struct KnobInfo { Knob* knob; const char* id; juce::String tip; };
    const KnobInfo knobs[] = {
        { &rate, ParamIDs::rate, tr ("Velocidad del barrido en Hz (cuando SYNC está apagado).", "Sweep speed in Hz (when SYNC is off).") },
        { &depth, ParamIDs::depth, tr ("Cuánto se mueve el barrido: hasta ±2.5 octavas alrededor del centro.", "How far the sweep moves: up to ±2.5 octaves around the centre.") },
        { &feedback, ParamIDs::feedback, tr ("Realimenta la salida: más resonante y metálico. Negativo cambia dónde caen las muescas.",
                                             "Feeds the output back in: more resonant and metallic. Negative moves where the notches fall.") },
        { &center, ParamIDs::center, tr ("Frecuencia alrededor de la que gira el barrido.", "Frequency the sweep revolves around.") },
        { &spread, ParamIDs::spread, tr ("Desfase del LFO entre izquierda y derecha, hasta 180°. 0 = mono.", "LFO offset between left and right, up to 180°. 0 = mono.") },
        { &mix, ParamIDs::mix, tr ("50 % = phaser clásico con muescas. 100 % = solo el giro de fase (sin muescas).",
                                   "50% = classic phaser with notches. 100% = phase shift only (no notches).") },
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
        { &stagesBox, &stagesLabel, ParamIDs::stages, Choices::stages, tr ("ETAPAS", "STAGES"),
          tr ("Número de etapas all-pass: más etapas = más muescas y un sonido más denso. Cambia sin clics.",
              "Number of all-pass stages: more stages = more notches and a thicker sound. Switches without clicks.") },
        { &divisionBox, &divisionLabel, ParamIDs::division, Choices::divisions, tr ("FIGURA", "DIVISION"),
          tr ("Duración de un ciclo del LFO en el tempo del DAW (D = punteado, T = tresillo).", "Length of one LFO cycle in DAW tempo (D = dotted, T = triplet).") },
        { &shapeBox, &shapeLabel, ParamIDs::shape, Choices::shapes, tr ("FORMA", "SHAPE"),
          tr ("Forma del LFO: seno (suave), triángulo (lineal) o random suave.", "LFO shape: sine (smooth), triangle (linear) or smooth random.") },
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
    sync.setTooltip (tr ("Sigue el tempo del DAW con la figura elegida. Apagado: usa la velocidad en Hz.",
                         "Follows the DAW tempo with the chosen division. Off: uses the rate in Hz."));
    addAndMakeVisible (sync);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (processor.state, ParamIDs::sync, sync));

    for (int i = 0; i < 3; ++i)
    {
        auto& b = presetButtons[i];
        b.setButtonText (tr (phasePresets[i].nameEs, phasePresets[i].nameEn));
        b.setTooltip (tr ("Punto de partida.", "Starting point."));
        b.onClick = [this, i] { processor.applyPreset (phasePresets[i]); };
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

PhaseReadyEditor::~PhaseReadyEditor()
{
    setLookAndFeel (nullptr);
}

void PhaseReadyEditor::paint (juce::Graphics& g)
{
    g.fillAll (colours::background);

    const float s = (float) getWidth() / baseWidth;
    auto header = getLocalBounds().removeFromTop (juce::roundToInt (48 * s)).reduced (juce::roundToInt (20 * s), 0);

    g.setColour (colours::text);
    g.setFont (juce::FontOptions (24.0f * s, juce::Font::bold));
    g.drawText ("PHASE READY", header, juce::Justification::centredLeft);

    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (12.0f * s));
    g.drawText (juce::String ("ROMMUSER  ·  v") + PLUGIN_VERSION_STRING + "  ·  " + tr ("gratis y open source", "free & open source"),
                header, juce::Justification::centredRight);

    g.setColour (colours::outline);
    g.drawHorizontalLine (header.getBottom(), 0.0f, (float) getWidth());
}

void PhaseReadyEditor::resized()
{
    const float s = (float) getWidth() / baseWidth;
    auto px = [s] (int v) { return juce::roundToInt ((float) v * s); };

    auto r = getLocalBounds();
    r.removeFromTop (px (48));
    r.reduce (px (16), px (12));

    // Bottom strip: sync, combos, presets.
    auto bottom = r.removeFromBottom (px (62));
    r.removeFromBottom (px (8));
    auto row = bottom.removeFromTop (px (44));
    const int cw = row.getWidth() / 7;
    sync.setBounds (row.removeFromLeft (cw).reduced (px (4), px (10)));
    for (const auto& c : { std::pair { &stagesBox, &stagesLabel }, std::pair { &divisionBox, &divisionLabel }, std::pair { &shapeBox, &shapeLabel } })
    {
        auto cell = row.removeFromLeft (cw);
        c.second->setBounds (cell.removeFromTop (px (16)));
        c.first->setBounds (cell.reduced (px (4), px (2)));
    }
    for (auto& b : presetButtons)
        b.setBounds (row.removeFromLeft (cw).reduced (px (4), px (10)));

    // Top: response display. Then one row of 7 knobs.
    display.setBounds (r.removeFromTop (r.getHeight() * 48 / 100));
    r.removeFromTop (px (8));
    const int kw = r.getWidth() / 7;
    rate.setBounds (r.removeFromLeft (kw));
    depth.setBounds (r.removeFromLeft (kw));
    feedback.setBounds (r.removeFromLeft (kw));
    center.setBounds (r.removeFromLeft (kw));
    spread.setBounds (r.removeFromLeft (kw));
    mix.setBounds (r.removeFromLeft (kw));
    output.setBounds (r);
}

void PhaseReadyEditor::updateEnablement()
{
    const bool synced = processor.state.getRawParameterValue (ParamIDs::sync)->load() > 0.5f;
    rate.setEnabled (! synced);
    rate.setAlpha (synced ? 0.4f : 1.0f);
    divisionBox.setEnabled (synced);
    divisionBox.setAlpha (synced ? 1.0f : 0.4f);
}

void PhaseReadyEditor::timerCallback()
{
    updateEnablement();
    float cL[maxStages], cR[maxStages];
    const int n = processor.displayStages.load();
    for (int k = 0; k < maxStages; ++k)
    {
        cL[k] = processor.displayCoeffs[0][(size_t) k].load();
        cR[k] = processor.displayCoeffs[1][(size_t) k].load();
    }
    display.setState (processor.readParams(), cL, cR, n, processor.lfoL.load(), processor.lfoR.load());
}
