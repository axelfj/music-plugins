// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginEditor.h"

#include "../../../shared/dsp/Biquad.h"

#include <complex>

using namespace rommuser::ui;
using namespace rommuser::filterready;

namespace
{
constexpr int baseWidth = 980, baseHeight = 720;
constexpr float minDb = -48.0f, maxDb = 24.0f;
constexpr double minHz = 20.0, maxHz = 20000.0;

rommuser::dsp::BiquadCoeffs coeffsFor (FilterType type, double fc, double q, double sr)
{
    using namespace rommuser::dsp::biquad;
    switch (type)
    {
        case FilterType::lowPass:  return lowPass (fc, q, sr);
        case FilterType::highPass: return highPass (fc, q, sr);
        case FilterType::bandPass: return bandPass (fc, q, sr);
        case FilterType::notch:    return notch (fc, q, sr);
    }
    return lowPass (fc, q, sr);
}

std::complex<double> response (const rommuser::dsp::BiquadCoeffs& c, double w)
{
    const std::complex<double> z1 = std::polar (1.0, -w), z2 = z1 * z1;
    return (c.b0 + c.b1 * z1 + c.b2 * z2) / (1.0 + c.a1 * z1 + c.a2 * z2);
}
} // namespace

//==============================================================================
ResponseDisplay::ResponseDisplay (FilterReadyProcessor& p) : processor (p) {}

void ResponseDisplay::setState (const FilterParams& p, float a, float b, float l, float e)
{
    params = p;
    cutoffA = a;
    cutoffB = b;
    lfo = l;
    env = e;
    repaint();
}

juce::Rectangle<float> ResponseDisplay::plotArea() const
{
    auto plot = getLocalBounds().toFloat().reduced (14.0f, 12.0f);
    plot.removeFromBottom (16.0f);
    plot.removeFromTop (16.0f);
    return plot;
}

float ResponseDisplay::xFor (double f, const juce::Rectangle<float>& plot) const
{
    return plot.getX() + (float) (std::log (f / minHz) / std::log (maxHz / minHz)) * plot.getWidth();
}

double ResponseDisplay::hzFor (float x, const juce::Rectangle<float>& plot) const
{
    const double t = juce::jlimit (0.0, 1.0, (double) (x - plot.getX()) / plot.getWidth());
    return minHz * std::pow (maxHz / minHz, t);
}

float ResponseDisplay::responseDb (double hz) const
{
    const double sr2 = processor.getSampleRateForDisplay() * oversampling;
    const double w = 2.0 * juce::MathConstants<double>::pi * hz / sr2;
    auto stage = [&] (const StageParams& sp, float fc)
    {
        const auto c = coeffsFor (sp.type, juce::jlimit ((double) minCutoffHz, sr2 * 0.45, (double) fc), resonanceToQ (sp.resonance), sr2);
        auto h = response (c, w);
        if (sp.slope == Slope::db24) h *= h;
        return h;
    };
    std::complex<double> h (1.0, 0.0);
    if (params.routing == Routing::parallel)
    {
        std::complex<double> sum (0.0, 0.0);
        int count = 0;
        if (params.a.enabled) { sum += stage (params.a, cutoffA); ++count; }
        if (params.b.enabled) { sum += stage (params.b, cutoffB); ++count; }
        h = count > 0 ? sum : h;
    }
    else
    {
        if (params.a.enabled) h *= stage (params.a, cutoffA);
        if (params.b.enabled) h *= stage (params.b, cutoffB);
    }
    return (float) (20.0 * std::log10 (std::max (std::abs (h), 1e-6)));
}

void ResponseDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r, 8.0f);
    const auto plot = plotArea();
    auto yFor = [&] (float db) { return plot.getBottom() - (juce::jlimit (minDb, maxDb, db) - minDb) / (maxDb - minDb) * plot.getHeight(); };

    g.setColour (colours::outline);
    for (double f : { 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0 })
        g.drawVerticalLine (juce::roundToInt (xFor (f, plot)), plot.getY(), plot.getBottom());
    for (float db : { -36.0f, -24.0f, -12.0f, 0.0f, 12.0f })
    {
        g.setColour (std::fabs (db) < 0.01f ? colours::textDim.withAlpha (0.6f) : colours::outline);
        g.drawHorizontalLine (juce::roundToInt (yFor (db)), plot.getX(), plot.getRight());
    }
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (10.0f));
    for (double f : { 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0 })
        g.drawText (f >= 1000.0 ? juce::String (f / 1000.0, 0) + "k" : juce::String (f, 0),
                    juce::Rectangle<float> (xFor (f, plot) - 20.0f, plot.getBottom() + 2.0f, 40.0f, 12.0f), juce::Justification::centred);
    for (float db : { -36.0f, -24.0f, -12.0f, 0.0f, 12.0f })
        g.drawText (juce::String ((int) db), juce::Rectangle<float> (plot.getX() + 2.0f, yFor (db) - 12.0f, 30.0f, 12.0f), juce::Justification::left);

    // Combined response.
    juce::Path path;
    const int points = 220;
    for (int i = 0; i <= points; ++i)
    {
        const double f = minHz * std::pow (maxHz / minHz, (double) i / points);
        const float x = xFor (f, plot), y = yFor (responseDb (f));
        if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
    }
    juce::Path fill (path);
    fill.lineTo (plot.getRight(), plot.getBottom());
    fill.lineTo (plot.getX(), plot.getBottom());
    fill.closeSubPath();
    g.setColour (colours::accent.withAlpha (0.12f));
    g.fillPath (fill);
    g.setColour (colours::accent);
    g.strokePath (path, juce::PathStrokeType (2.0f));

    // Cutoff markers: x = live (modulated) cutoff, y = resonance.
    auto marker = [&] (const StageParams& sp, float fc, juce::Colour colour, const char* name, bool active)
    {
        if (! sp.enabled) colour = colour.withAlpha (0.35f);
        const float x = xFor (juce::jlimit (minHz, maxHz, (double) fc), plot);
        const float y = plot.getBottom() - sp.resonance * plot.getHeight();
        g.setColour (colour.withAlpha (0.5f));
        g.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());
        g.setColour (colour);
        g.fillEllipse (x - 7.0f, y - 7.0f, 14.0f, 14.0f);
        if (active)
            g.drawEllipse (x - 10.0f, y - 10.0f, 20.0f, 20.0f, 1.5f);
        g.setColour (colours::background);
        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.drawText (name, juce::Rectangle<float> (x - 7.0f, y - 7.0f, 14.0f, 14.0f), juce::Justification::centred);
    };
    marker (params.b, cutoffB, colours::good, "B", dragging == 1);
    marker (params.a, cutoffA, colours::warn, "A", dragging == 0);

    // Top row: labels + LFO / env indicators.
    auto top = juce::Rectangle<float> (plot.getX(), r.getY() + 6.0f, plot.getWidth(), 14.0f);
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (11.0f));
    juce::String label;
    label << "A " << (cutoffA >= 1000.0f ? juce::String (cutoffA / 1000.0f, 2) + " kHz" : juce::String ((int) cutoffA) + " Hz");
    if (params.b.enabled)
        label << juce::String::fromUTF8 ("   ·   B ") << (cutoffB >= 1000.0f ? juce::String (cutoffB / 1000.0f, 2) + " kHz" : juce::String ((int) cutoffB) + " Hz");
    label << juce::String::fromUTF8 ("   ·   ") << tr ("arrastra A / B para mover corte y resonancia", "drag A / B to move cutoff and resonance");
    g.drawText (label, top, juce::Justification::centredLeft);

    auto lfoArea = juce::Rectangle<float> (top.getRight() - 80.0f, top.getY() + 1.0f, 80.0f, 12.0f);
    g.setColour (colours::outline);
    g.drawRoundedRectangle (lfoArea, 6.0f, 1.0f);
    g.setColour (colours::accent);
    g.fillEllipse (lfoArea.getX() + 4.0f + (lfo * 0.5f + 0.5f) * (lfoArea.getWidth() - 16.0f), lfoArea.getY() + 2.0f, 8.0f, 8.0f);
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("LFO", lfoArea.translated (-32.0f, 0.0f).withWidth (28.0f), juce::Justification::centredRight);

    auto envArea = juce::Rectangle<float> (lfoArea.getX() - 150.0f, lfoArea.getY(), 80.0f, 12.0f);
    g.setColour (colours::outline);
    g.drawRoundedRectangle (envArea, 6.0f, 1.0f);
    g.setColour (colours::good);
    g.fillRoundedRectangle (envArea.reduced (2.0f).withWidth (std::max (2.0f, (envArea.getWidth() - 4.0f) * juce::jlimit (0.0f, 1.0f, env))), 4.0f);
    g.setColour (colours::textDim);
    g.drawText ("ENV", envArea.translated (-32.0f, 0.0f).withWidth (28.0f), juce::Justification::centredRight);
}

void ResponseDisplay::mouseDown (const juce::MouseEvent& e)
{
    const auto plot = plotArea();
    auto dist = [&] (const StageParams& sp, float fc)
    {
        const float x = xFor (juce::jlimit (minHz, maxHz, (double) fc), plot);
        const float y = plot.getBottom() - sp.resonance * plot.getHeight();
        return e.position.getDistanceFrom ({ x, y });
    };
    const float dA = dist (params.a, cutoffA);
    const float dB = params.b.enabled ? dist (params.b, cutoffB) : 1e9f;
    dragging = dB < dA ? 1 : 0;
    dragCutoff = processor.state.getParameter (dragging == 0 ? ParamIDs::aCutoff : ParamIDs::bCutoff);
    dragRes = processor.state.getParameter (dragging == 0 ? ParamIDs::aRes : ParamIDs::bRes);
    if (dragCutoff != nullptr) dragCutoff->beginChangeGesture();
    if (dragRes != nullptr) dragRes->beginChangeGesture();
    mouseDrag (e);
}

void ResponseDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging < 0) return;
    const auto plot = plotArea();
    if (dragCutoff != nullptr)
        dragCutoff->setValueNotifyingHost (dragCutoff->convertTo0to1 ((float) hzFor (e.position.x, plot)));
    if (dragRes != nullptr)
        dragRes->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, (plot.getBottom() - e.position.y) / plot.getHeight()));
}

void ResponseDisplay::mouseUp (const juce::MouseEvent&)
{
    if (dragCutoff != nullptr) dragCutoff->endChangeGesture();
    if (dragRes != nullptr) dragRes->endChangeGesture();
    dragCutoff = dragRes = nullptr;
    dragging = -1;
    repaint();
}

void ResponseDisplay::mouseDoubleClick (const juce::MouseEvent& e)
{
    // Double-click toggles filter B on/off at that position.
    if (auto* on = processor.state.getParameter (ParamIDs::bOn))
    {
        const bool enabled = on->getValue() > 0.5f;
        on->setValueNotifyingHost (enabled ? 0.0f : 1.0f);
        if (! enabled)
            if (auto* c = processor.state.getParameter (ParamIDs::bCutoff))
                c->setValueNotifyingHost (c->convertTo0to1 ((float) hzFor (e.position.x, plotArea())));
    }
}

//==============================================================================
FilterReadyEditor::FilterReadyEditor (FilterReadyProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      stageA (tr ("CORTE", "CUTOFF"), tr ("RESONANCIA", "RESONANCE"), "LFO →", "ENV →"),
      stageB (tr ("CORTE", "CUTOFF"), tr ("RESONANCIA", "RESONANCE"), "LFO →", "ENV →"),
      drive ("DRIVE"),
      mix ("MIX"),
      output (tr ("SALIDA", "OUTPUT")),
      lfoRate (tr ("VELOCIDAD", "RATE")),
      envAttack (tr ("ATAQUE", "ATTACK")),
      envRelease (tr ("CAÍDA", "RELEASE")),
      envSens (tr ("SENSIBILIDAD", "SENSITIVITY")),
      display (p)
{
    setLookAndFeel (&lnf);

    struct KnobInfo { Knob* knob; const char* id; juce::String tip; };
    const KnobInfo knobs[] = {
        { &stageA.cutoff, ParamIDs::aCutoff, tr ("Frecuencia de corte del filtro A.", "Cutoff frequency of filter A.") },
        { &stageA.res, ParamIDs::aRes, tr ("Resonancia: realza el corte. Arriba del 70 % silba.", "Resonance: boosts the cutoff. Above 70% it whistles.") },
        { &stageA.lfoAmount, ParamIDs::aLfo, tr ("Cuánto mueve el LFO el corte de A, en octavas (negativo invierte).", "How far the LFO moves A's cutoff, in octaves (negative inverts).") },
        { &stageA.envAmount, ParamIDs::aEnv, tr ("Cuánto abre el envolvente el corte de A cuando entra señal fuerte (auto-wah).", "How far the envelope opens A's cutoff on loud input (auto-wah).") },
        { &stageB.cutoff, ParamIDs::bCutoff, tr ("Frecuencia de corte del filtro B.", "Cutoff frequency of filter B.") },
        { &stageB.res, ParamIDs::bRes, tr ("Resonancia del filtro B.", "Resonance of filter B.") },
        { &stageB.lfoAmount, ParamIDs::bLfo, tr ("Modulación del LFO sobre el corte de B, en octavas.", "LFO modulation of B's cutoff, in octaves.") },
        { &stageB.envAmount, ParamIDs::bEnv, tr ("Modulación del envolvente sobre el corte de B, en octavas.", "Envelope modulation of B's cutoff, in octaves.") },
        { &drive, ParamIDs::drive, tr ("Saturación antes de los filtros: la resonancia se compacta y suena más agresivo.", "Saturation before the filters: the resonance compresses and bites harder.") },
        { &mix, ParamIDs::mix, tr ("Mezcla entre señal original y filtrada.", "Blend between the original and filtered signal.") },
        { &output, ParamIDs::output, tr ("Ganancia final.", "Final gain.") },
        { &lfoRate, ParamIDs::lfoRate, tr ("Velocidad del LFO en Hz (cuando SYNC está apagado).", "LFO speed in Hz (when SYNC is off).") },
        { &envAttack, ParamIDs::envAttack, tr ("Qué tan rápido reacciona el envolvente a un golpe.", "How fast the envelope reacts to a hit.") },
        { &envRelease, ParamIDs::envRelease, tr ("Qué tan rápido vuelve el envolvente cuando la señal baja.", "How fast the envelope falls back when the signal drops.") },
        { &envSens, ParamIDs::envSens, tr ("Ganancia hacia el seguidor de envolvente: súbela si la señal es muy baja.", "Gain into the envelope follower: raise it for quiet signals.") },
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
        { &stageA.type, nullptr, ParamIDs::aType, Choices::types, {}, tr ("Tipo de filtro A: pasa-bajos, pasa-altos, pasa-banda o notch.", "Filter A type: low-pass, high-pass, band-pass or notch.") },
        { &stageA.slope, nullptr, ParamIDs::aSlope, Choices::slopes, {}, tr ("Pendiente: 12 dB suave, 24 dB más marcada (estilo sinte).", "Slope: 12 dB gentle, 24 dB steeper (synth style).") },
        { &stageB.type, nullptr, ParamIDs::bType, Choices::types, {}, tr ("Tipo de filtro B.", "Filter B type.") },
        { &stageB.slope, nullptr, ParamIDs::bSlope, Choices::slopes, {}, tr ("Pendiente del filtro B.", "Slope of filter B.") },
        { &routingBox, &routingLabel, ParamIDs::routing, Choices::routing, tr ("RUTA", "ROUTING"),
          tr ("Serie: B filtra lo que sale de A. Paralelo: A y B se suman.", "Series: B filters A's output. Parallel: A and B are summed.") },
        { &characterBox, &characterLabel, ParamIDs::character, Choices::character, tr ("CARÁCTER", "CHARACTER"),
          tr ("Clean: respuesta exacta. Analog: la resonancia satura suavemente al subir el nivel.", "Clean: exact response. Analog: the resonance saturates softly as level rises.") },
        { &divisionBox, &divisionLabel, ParamIDs::lfoDivision, Choices::divisions, tr ("FIGURA", "DIVISION"),
          tr ("Duración de un ciclo del LFO en el tempo del DAW (D = punteado, T = tresillo).", "Length of one LFO cycle in DAW tempo (D = dotted, T = triplet).") },
        { &shapeBox, &shapeLabel, ParamIDs::lfoShape, Choices::shapes, tr ("FORMA", "SHAPE"),
          tr ("Forma del LFO: seno, triángulo, sierra (baja), cuadrada o random.", "LFO shape: sine, triangle, saw (down), square or random.") },
    };
    for (const auto& c : combos)
    {
        c.box->addItemList (c.items, 1);
        c.box->setTooltip (c.tip);
        addAndMakeVisible (*c.box);
        comboAttachments.push_back (std::make_unique<ComboAttachment> (processor.state, c.id, *c.box));
        if (c.label != nullptr)
        {
            c.label->setText (c.name, juce::dontSendNotification);
            c.label->setJustificationType (juce::Justification::centred);
            c.label->setColour (juce::Label::textColourId, colours::textDim);
            c.label->setFont (juce::FontOptions (12.0f, juce::Font::bold));
            addAndMakeVisible (*c.label);
        }
    }

    struct StageSetup { StageControls* s; const char* onId; juce::String title; juce::Colour colour; };
    for (const auto& st : { StageSetup { &stageA, ParamIDs::aOn, tr ("FILTRO A", "FILTER A"), colours::warn },
                            StageSetup { &stageB, ParamIDs::bOn, tr ("FILTRO B", "FILTER B"), colours::good } })
    {
        st.s->title.setText (st.title, juce::dontSendNotification);
        st.s->title.setColour (juce::Label::textColourId, st.colour);
        st.s->title.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        st.s->title.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (st.s->title);
        st.s->on.setButtonText ("ON");
        st.s->on.setClickingTogglesState (true);
        st.s->on.setTooltip (tr ("Activa o apaga este filtro.", "Turns this filter on or off."));
        addAndMakeVisible (st.s->on);
        buttonAttachments.push_back (std::make_unique<ButtonAttachment> (processor.state, st.onId, st.s->on));
    }

    sync.setButtonText ("SYNC");
    sync.setClickingTogglesState (true);
    sync.setTooltip (tr ("Sigue el tempo del DAW con la figura elegida. Apagado: usa la velocidad en Hz.",
                         "Follows the DAW tempo with the chosen division. Off: uses the rate in Hz."));
    addAndMakeVisible (sync);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (processor.state, ParamIDs::lfoSync, sync));

    for (int i = 0; i < 4; ++i)
    {
        auto& b = presetButtons[i];
        b.setButtonText (tr (filterPresets[i].nameEs, filterPresets[i].nameEn));
        b.setTooltip (tr ("Punto de partida.", "Starting point."));
        b.onClick = [this, i] { processor.applyPreset (filterPresets[i]); };
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

FilterReadyEditor::~FilterReadyEditor()
{
    setLookAndFeel (nullptr);
}

void FilterReadyEditor::paint (juce::Graphics& g)
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

    // Section labels for the global row.
    auto px = [s] (int v) { return juce::roundToInt ((float) v * s); };
    g.setFont (juce::FontOptions (11.0f * s, juce::Font::bold));
    g.setColour (colours::textDim);
    const int rowTop = sectionTops[2];
    g.drawText ("LFO", juce::Rectangle<int> (lfoRate.getX(), rowTop, shapeBox.getRight() - lfoRate.getX(), px (14)), juce::Justification::centred);
    g.drawText (tr ("ENVOLVENTE", "ENVELOPE"), juce::Rectangle<int> (envAttack.getX(), rowTop, envSens.getRight() - envAttack.getX(), px (14)), juce::Justification::centred);
}

void FilterReadyEditor::resized()
{
    const float s = (float) getWidth() / baseWidth;
    auto px = [s] (int v) { return juce::roundToInt ((float) v * s); };

    auto r = getLocalBounds();
    r.removeFromTop (px (48));
    r.reduce (px (16), px (10));

    // Presets at the bottom.
    auto presets = r.removeFromBottom (px (36));
    r.removeFromBottom (px (6));
    const int pw = presets.getWidth() / 4;
    for (auto& b : presetButtons)
        b.setBounds (presets.removeFromLeft (pw).reduced (px (4), px (2)));

    // Display on top.
    display.setBounds (r.removeFromTop (r.getHeight() * 40 / 100));
    r.removeFromTop (px (6));

    // Three rows: filter A, filter B, global.
    const int rowH = r.getHeight() / 3;
    auto layoutStage = [&] (StageControls& st, juce::Rectangle<int> row)
    {
        auto left = row.removeFromLeft (px (180));
        left.reduce (px (4), px (6));
        st.title.setBounds (left.removeFromTop (px (20)));
        auto onRow = left.removeFromTop (px (26));
        st.on.setBounds (onRow.removeFromLeft (px (56)).reduced (0, px (1)));
        left.removeFromTop (px (4));
        st.type.setBounds (left.removeFromTop (px (26)).reduced (0, px (1)));
        left.removeFromTop (px (4));
        st.slope.setBounds (left.removeFromTop (px (26)).reduced (0, px (1)));
        const int kw = row.getWidth() / 4;
        st.cutoff.setBounds (row.removeFromLeft (kw));
        st.res.setBounds (row.removeFromLeft (kw));
        st.lfoAmount.setBounds (row.removeFromLeft (kw));
        st.envAmount.setBounds (row);
    };
    sectionTops[0] = r.getY();
    layoutStage (stageA, r.removeFromTop (rowH));
    sectionTops[1] = r.getY();
    layoutStage (stageB, r.removeFromTop (rowH));
    sectionTops[2] = r.getY();

    // Global row: routing/character | drive mix output | sync rate division shape | env attack release sens
    auto row = r;
    row.removeFromTop (px (14));
    auto left = row.removeFromLeft (px (180));
    left.reduce (px (4), px (6));
    routingLabel.setBounds (left.removeFromTop (px (16)));
    routingBox.setBounds (left.removeFromTop (px (26)).reduced (0, px (1)));
    left.removeFromTop (px (4));
    characterLabel.setBounds (left.removeFromTop (px (16)));
    characterBox.setBounds (left.removeFromTop (px (26)).reduced (0, px (1)));

    const int unit = row.getWidth() / 11;   // 3 knobs + (sync + rate + division + shape = 1 + 1 + 1.5 + 1.5) + 3 knobs
    drive.setBounds (row.removeFromLeft (unit));
    mix.setBounds (row.removeFromLeft (unit));
    output.setBounds (row.removeFromLeft (unit));

    auto syncCell = row.removeFromLeft (unit * 3 / 4);
    sync.setBounds (syncCell.withSizeKeepingCentre (syncCell.getWidth() - px (8), px (28)));
    lfoRate.setBounds (row.removeFromLeft (unit));
    for (const auto& c : { std::pair { &divisionBox, &divisionLabel }, std::pair { &shapeBox, &shapeLabel } })
    {
        auto cell = row.removeFromLeft (unit * 9 / 8);
        cell = cell.withSizeKeepingCentre (cell.getWidth() - px (6), px (46));
        c.second->setBounds (cell.removeFromTop (px (16)));
        c.first->setBounds (cell.reduced (0, px (2)));
    }
    const int kw = row.getWidth() / 3;
    envAttack.setBounds (row.removeFromLeft (kw));
    envRelease.setBounds (row.removeFromLeft (kw));
    envSens.setBounds (row);
}

void FilterReadyEditor::updateEnablement()
{
    const bool synced = processor.state.getRawParameterValue (ParamIDs::lfoSync)->load() > 0.5f;
    lfoRate.setEnabled (! synced);
    lfoRate.setAlpha (synced ? 0.4f : 1.0f);
    divisionBox.setEnabled (synced);
    divisionBox.setAlpha (synced ? 1.0f : 0.4f);

    const bool aOn = processor.state.getRawParameterValue (ParamIDs::aOn)->load() > 0.5f;
    const bool bOn = processor.state.getRawParameterValue (ParamIDs::bOn)->load() > 0.5f;
    auto dim = [] (StageControls& st, bool on)
    {
        juce::Component* parts[] = { &st.type, &st.slope, &st.cutoff, &st.res, &st.lfoAmount, &st.envAmount };
        for (auto* c : parts)
            c->setAlpha (on ? 1.0f : 0.4f);
    };
    dim (stageA, aOn);
    dim (stageB, bOn);
}

void FilterReadyEditor::timerCallback()
{
    updateEnablement();
    display.setState (processor.readParams(), processor.cutoffA.load(), processor.cutoffB.load(), processor.lfo.load(), processor.env.load());
}
