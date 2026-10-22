// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginEditor.h"

using namespace rommuser::ui;
using namespace rommuser::shapeeq;
using rommuser::dsp::FilterType;

namespace
{
constexpr int baseWidth = 920, baseHeight = 580;
constexpr double minFreq = 10.0, maxFreq = 30000.0;
constexpr float nodeRadius = 9.0f;

// Spectrum display: -90 .. 0 dB over the full graph height, tilted so pink noise reads flat.
constexpr float spectrumFloorDb = -90.0f;
constexpr float spectrumTiltDbPerOct = 4.5f;

const juce::Colour bandPalette[] = {
    juce::Colour (0xfff2a23a), juce::Colour (0xff58c98b), juce::Colour (0xff5aa9e6), juce::Colour (0xffe5534b),
    juce::Colour (0xffc884f0), juce::Colour (0xfff0d35a), juce::Colour (0xff4fd1c5), juce::Colour (0xffff8fab),
    juce::Colour (0xffa3e635), juce::Colour (0xffff9f68), juce::Colour (0xff8ab4ff), juce::Colour (0xffd4a373),
};

juce::String freqLabel (double f)
{
    return f >= 1000.0 ? juce::String (f / 1000.0, f >= 10000.0 ? 0 : 1).trimCharactersAtEnd ("0").trimCharactersAtEnd (".") + "k"
                       : juce::String (f, 0);
}
} // namespace

//==============================================================================
EqDisplay::EqDisplay (ShapeEQProcessor& p) : processor (p)
{
    setWantsKeyboardFocus (false);
    refresh();
}

juce::Colour EqDisplay::bandColour (int band) { return bandPalette[(size_t) band % std::size (bandPalette)]; }

void EqDisplay::refresh()
{
    params = processor.readParams();
    sampleRate = processor.currentSampleRate.load();
    for (int b = 0; b < maxBands; ++b)
        coeffs[(size_t) b] = params.bands[(size_t) b].enabled ? designBand (params.bands[(size_t) b], sampleRate) : BandCoeffs {};

    analyzerMode = juce::roundToInt (processor.state.getRawParameterValue (ParamIDs::analyzer)->load());
    preAnalyzer.setSampleRate (sampleRate);
    postAnalyzer.setSampleRate (sampleRate);
    if (analyzerMode == 1 || analyzerMode == 3) preAnalyzer.update (processor.preSpectrum); else preAnalyzer.clear();
    if (analyzerMode >= 2) postAnalyzer.update (processor.postSpectrum); else postAnalyzer.clear();
    repaint();
}

void EqDisplay::setSelectedBand (int band)
{
    if (band == selected) return;
    selected = band;
    repaint();
    if (onSelectionChanged) onSelectionChanged (selected);
}

//==============================================================================
juce::Rectangle<float> EqDisplay::graphArea() const
{
    return getLocalBounds().toFloat().reduced (10.0f, 8.0f).withTrimmedBottom (16.0f).withTrimmedLeft (30.0f);
}

float EqDisplay::freqToX (double f) const
{
    const auto r = graphArea();
    return r.getX() + (float) (std::log (f / minFreq) / std::log (maxFreq / minFreq)) * r.getWidth();
}

double EqDisplay::xToFreq (float x) const
{
    const auto r = graphArea();
    const double t = juce::jlimit (0.0, 1.0, (double) (x - r.getX()) / r.getWidth());
    return minFreq * std::pow (maxFreq / minFreq, t);
}

float EqDisplay::dbToY (double db) const
{
    const auto r = graphArea();
    return r.getCentreY() - (float) (db / rangeDb) * r.getHeight() * 0.5f;
}

double EqDisplay::yToDb (float y) const
{
    const auto r = graphArea();
    return juce::jlimit ((double) -rangeDb, (double) rangeDb, (double) (r.getCentreY() - y) / (r.getHeight() * 0.5) * rangeDb);
}

juce::Point<float> EqDisplay::nodePosition (int band) const
{
    const auto& bp = params.bands[(size_t) band];
    const double db = Choices::typeHasGain (bp.type) ? bp.gainDb : 0.0;
    return { freqToX (bp.freq), dbToY (db) };
}

int EqDisplay::bandAt (juce::Point<float> p) const
{
    int best = -1;
    float bestDist = nodeRadius * 1.6f;
    for (int b = 0; b < maxBands; ++b)
    {
        if (! params.bands[(size_t) b].enabled) continue;
        const float d = nodePosition (b).getDistanceFrom (p);
        if (d < bestDist) { bestDist = d; best = b; }
    }
    return best;
}

int EqDisplay::firstFreeBand() const
{
    for (int b = 0; b < maxBands; ++b)
        if (! params.bands[(size_t) b].enabled) return b;
    return -1;
}

double EqDisplay::curveDb (double f, int onlyBand) const
{
    const double w = 2.0 * juce::MathConstants<double>::pi * f / sampleRate;
    std::complex<double> h (1.0, 0.0);
    for (int b = 0; b < maxBands; ++b)
        if (onlyBand < 0 || b == onlyBand)
            h *= coeffs[(size_t) b].response (w);
    return 20.0 * std::log10 (std::max (std::abs (h), 1e-9));
}

//==============================================================================
void EqDisplay::setParam (int band, const char* suffix, float value)
{
    if (auto* p = processor.state.getParameter (ParamIDs::band (band, suffix)))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void EqDisplay::gesture (int band, const char* suffix, bool begin)
{
    if (auto* p = processor.state.getParameter (ParamIDs::band (band, suffix)))
        begin ? p->beginChangeGesture() : p->endChangeGesture();
}

//==============================================================================
void EqDisplay::paint (juce::Graphics& g)
{
    auto full = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (full, 8.0f);

    const auto r = graphArea();
    g.setFont (juce::FontOptions (10.0f));

    // Frequency grid.
    for (double f : { 20.0, 30.0, 50.0, 100.0, 200.0, 300.0, 500.0, 1000.0, 2000.0, 3000.0, 5000.0, 10000.0, 20000.0 })
    {
        const float x = freqToX (f);
        const bool major = juce::exactlyEqual (f, 100.0) || juce::exactlyEqual (f, 1000.0) || juce::exactlyEqual (f, 10000.0);
        g.setColour (major ? colours::outline.brighter (0.25f) : colours::outline);
        g.drawVerticalLine (juce::roundToInt (x), r.getY(), r.getBottom());
        if (major || (int) f == 20 || (int) f == 50 || (int) f == 200 || (int) f == 500 || (int) f == 2000 || (int) f == 5000 || (int) f == 20000)
        {
            g.setColour (colours::textDim);
            g.drawText (freqLabel (f), juce::Rectangle<float> (x - 20.0f, r.getBottom() + 2.0f, 40.0f, 14.0f), juce::Justification::centred);
        }
    }

    // Gain grid.
    const float step = rangeDb > 15.0f ? 6.0f : 3.0f;
    for (float db = -rangeDb; db <= rangeDb + 0.01f; db += step)
    {
        const float y = dbToY (db);
        g.setColour (std::fabs (db) < 0.01f ? colours::outline.brighter (0.5f) : colours::outline);
        g.drawHorizontalLine (juce::roundToInt (y), r.getX(), r.getRight());
        g.setColour (colours::textDim);
        g.drawText ((db > 0 ? "+" : "") + juce::String ((int) db), juce::Rectangle<float> (full.getX() + 2.0f, y - 7.0f, 26.0f, 14.0f), juce::Justification::centredRight);
    }

    // Spectrum behind everything.
    {
        juce::Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (r.toNearestInt());
        if (analyzerMode == 1 || analyzerMode == 3)
            drawSpectrum (g, preAnalyzer, colours::textDim.withAlpha (0.35f), true);
        if (analyzerMode >= 2)
            drawSpectrum (g, postAnalyzer, colours::accent.withAlpha (analyzerMode == 3 ? 0.55f : 0.45f), analyzerMode == 2);
    }

    // Selected band alone (faint), then the total curve.
    {
        juce::Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (r.expanded (0.0f, 4.0f).toNearestInt());
        if (selected >= 0 && params.bands[(size_t) selected].enabled)
            drawCurve (g, selected, bandColour (selected).withAlpha (0.45f), 1.5f);
        drawCurve (g, -1, params.bypass ? colours::textDim : colours::text, 2.5f);
    }

    // Nodes.
    for (int b = 0; b < maxBands; ++b)
    {
        if (! params.bands[(size_t) b].enabled) continue;
        const auto pos = nodePosition (b);
        const bool isSel = b == selected, isHover = b == hovered;
        const float rad = nodeRadius + (isHover ? 1.5f : 0.0f);
        const auto c = bandColour (b);
        g.setColour (isSel ? c : c.withAlpha (0.8f));
        if (isSel) g.fillEllipse (juce::Rectangle<float> (rad * 2.0f, rad * 2.0f).withCentre (pos));
        else       g.drawEllipse (juce::Rectangle<float> (rad * 2.0f, rad * 2.0f).withCentre (pos), 2.0f);
        g.setColour (isSel ? colours::background : c);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (juce::String (b + 1), juce::Rectangle<float> (24.0f, 16.0f).withCentre (pos), juce::Justification::centred);
    }

    // Readout for the selected band.
    if (selected >= 0 && params.bands[(size_t) selected].enabled)
    {
        const auto& bp = params.bands[(size_t) selected];
        juce::String txt = Choices::types[(int) bp.type] + juce::String::fromUTF8 ("  ·  ") + freqLabel (bp.freq) + (bp.freq >= 1000.0 ? "Hz" : " Hz");
        if (Choices::typeHasGain (bp.type)) txt += juce::String::fromUTF8 ("  ·  ") + juce::String (bp.gainDb, 1) + " dB";
        if (Choices::typeHasSlope (bp.type)) txt += juce::String::fromUTF8 ("  ·  ") + juce::String (bp.slopeDb) + " dB/oct";
        else txt += juce::String::fromUTF8 ("  ·  Q ") + juce::String (bp.q, 2);
        if (bp.placement != Placement::stereo) txt += juce::String::fromUTF8 ("  ·  ") + Choices::placements[(int) bp.placement];
        g.setColour (bandColour (selected));
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (txt, r.withHeight (16.0f).translated (0.0f, 2.0f).reduced (6.0f, 0.0f), juce::Justification::centredRight);
    }

    if (params.bypass)
    {
        g.setColour (colours::warn);
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText ("BYPASS", r.withHeight (16.0f).translated (0.0f, 2.0f).reduced (6.0f, 0.0f), juce::Justification::centredLeft);
    }

    if (firstFreeBand() == 0)
    {
        g.setColour (colours::textDim);
        g.setFont (juce::FontOptions (13.0f));
        g.drawText (tr ("Doble clic para crear una banda", "Double-click to add a band"), r, juce::Justification::centred);
    }
}

void EqDisplay::drawSpectrum (juce::Graphics& g, const SpectrumAnalyzer& a, juce::Colour c, bool fill) const
{
    const auto r = graphArea();
    juce::Path p;
    const int steps = juce::jmax (32, (int) r.getWidth() / 2);
    for (int i = 0; i <= steps; ++i)
    {
        const float x = r.getX() + r.getWidth() * (float) i / (float) steps;
        const double f = xToFreq (x);
        const float tilt = spectrumTiltDbPerOct * (float) std::log2 (f / 1000.0);
        const float level = juce::jlimit (spectrumFloorDb, 0.0f, a.levelAt (f) + tilt);
        const float y = r.getBottom() - (level - spectrumFloorDb) / -spectrumFloorDb * r.getHeight();
        i == 0 ? p.startNewSubPath (x, y) : p.lineTo (x, y);
    }
    g.setColour (c);
    if (fill)
    {
        auto closed = p;
        closed.lineTo (r.getRight(), r.getBottom());
        closed.lineTo (r.getX(), r.getBottom());
        closed.closeSubPath();
        g.fillPath (closed);
    }
    else
    {
        g.strokePath (p, juce::PathStrokeType (1.5f));
    }
}

void EqDisplay::drawCurve (juce::Graphics& g, int onlyBand, juce::Colour c, float thickness) const
{
    const auto r = graphArea();
    juce::Path p;
    const int steps = juce::jmax (64, (int) r.getWidth());
    for (int i = 0; i <= steps; ++i)
    {
        const float x = r.getX() + r.getWidth() * (float) i / (float) steps;
        const float y = dbToY (curveDb (xToFreq (x), onlyBand));
        i == 0 ? p.startNewSubPath (x, y) : p.lineTo (x, y);
    }
    g.setColour (c);
    g.strokePath (p, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

//==============================================================================
void EqDisplay::mouseMove (const juce::MouseEvent& e)
{
    const int b = bandAt (e.position);
    if (b != hovered)
    {
        hovered = b;
        setMouseCursor (b >= 0 ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void EqDisplay::mouseExit (const juce::MouseEvent&)
{
    hovered = -1;
    repaint();
}

void EqDisplay::mouseDown (const juce::MouseEvent& e)
{
    const int b = bandAt (e.position);
    if (b >= 0)
        setSelectedBand (b);

    if (e.mods.isPopupMenu())
    {
        if (b >= 0) showBandMenu (b, e.getPosition());
        return;
    }
    if (b < 0) return;

    dragging = b;
    dragStartQ = params.bands[(size_t) b].q;
    dragStartY = e.position.y;
    gesture (b, ParamIDs::freq, true);
    gesture (b, Choices::typeHasGain (params.bands[(size_t) b].type) ? ParamIDs::gain : ParamIDs::q, true);
}

void EqDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging < 0) return;
    const auto& bp = params.bands[(size_t) dragging];
    setParam (dragging, ParamIDs::freq, (float) xToFreq (e.position.x));
    if (Choices::typeHasGain (bp.type))
        setParam (dragging, ParamIDs::gain, (float) yToDb (e.position.y));
    else
        // No gain to move: vertical drag narrows / widens the band instead (one octave of Q per 80 px).
        setParam (dragging, ParamIDs::q, (float) juce::jlimit (0.025, 40.0, dragStartQ * std::pow (2.0, (dragStartY - e.position.y) / 80.0)));
    refresh();
}

void EqDisplay::mouseUp (const juce::MouseEvent&)
{
    if (dragging < 0) return;
    gesture (dragging, ParamIDs::freq, false);
    gesture (dragging, Choices::typeHasGain (params.bands[(size_t) dragging].type) ? ParamIDs::gain : ParamIDs::q, false);
    dragging = -1;
}

void EqDisplay::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int b = bandAt (e.position);
    if (b >= 0)
    {
        setParam (b, ParamIDs::on, 0.0f);
        refresh();
        return;
    }
    const int fresh = firstFreeBand();
    if (fresh < 0) return;

    const double f = xToFreq (e.position.x);
    // A click near the edges makes a cut, anywhere else a bell at the clicked gain.
    const FilterType type = f < 40.0 ? FilterType::lowCut : f > 16000.0 ? FilterType::highCut : FilterType::bell;
    setParam (fresh, ParamIDs::type, (float) type);
    setParam (fresh, ParamIDs::freq, (float) f);
    setParam (fresh, ParamIDs::gain, type == FilterType::bell ? (float) yToDb (e.position.y) : 0.0f);
    setParam (fresh, ParamIDs::q, type == FilterType::bell ? 1.0f : 0.7071f);
    setParam (fresh, ParamIDs::slope, 1.0f);
    setParam (fresh, ParamIDs::place, 0.0f);
    setParam (fresh, ParamIDs::on, 1.0f);
    refresh();
    setSelectedBand (fresh);
}

void EqDisplay::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    int b = bandAt (e.position);
    if (b < 0) b = selected;
    if (b < 0 || ! params.bands[(size_t) b].enabled) return;
    const double q = params.bands[(size_t) b].q * std::pow (2.0, (double) wheel.deltaY * 1.5);
    setParam (b, ParamIDs::q, (float) juce::jlimit (0.025, 40.0, q));
    refresh();
}

void EqDisplay::showBandMenu (int band, juce::Point<int> where)
{
    const auto& bp = params.bands[(size_t) band];
    juce::PopupMenu menu, types, slopes, places;
    for (int i = 0; i < Choices::types.size(); ++i)
        types.addItem (100 + i, Choices::types[i], true, (int) bp.type == i);
    for (int i = 0; i < Choices::slopes.size(); ++i)
        slopes.addItem (200 + i, Choices::slopes[i] + "/oct", true, Choices::slopeValues[i] == bp.slopeDb);
    for (int i = 0; i < Choices::placements.size(); ++i)
        places.addItem (300 + i, Choices::placements[i], true, (int) bp.placement == i);

    menu.addSectionHeader (tr ("Banda ", "Band ") + juce::String (band + 1));
    menu.addSubMenu (tr ("Tipo", "Type"), types);
    menu.addSubMenu (tr ("Pendiente", "Slope"), slopes, Choices::typeHasSlope (bp.type));
    menu.addSubMenu (tr ("Canal", "Placement"), places);
    menu.addSeparator();
    menu.addItem (1, tr ("Quitar banda", "Remove band"));

    juce::Component::SafePointer<EqDisplay> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (juce::Rectangle<int> (where, where).expanded (1))),
                        [safe, band] (int result)
                        {
                            if (safe == nullptr || result == 0) return;
                            if (result == 1)            safe->setParam (band, ParamIDs::on, 0.0f);
                            else if (result >= 300)     safe->setParam (band, ParamIDs::place, (float) (result - 300));
                            else if (result >= 200)     safe->setParam (band, ParamIDs::slope, (float) (result - 200));
                            else if (result >= 100)     safe->setParam (band, ParamIDs::type, (float) (result - 100));
                            safe->refresh();
                        });
}

//==============================================================================
ShapeEQEditor::ShapeEQEditor (ShapeEQProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      display (p),
      freq (tr ("FRECUENCIA", "FREQUENCY")),
      gain (tr ("GANANCIA", "GAIN")),
      q ("Q"),
      output (tr ("SALIDA", "OUTPUT"))
{
    setLookAndFeel (&lnf);
    processor.preSpectrum.active.store (true);
    processor.postSpectrum.active.store (true);

    addAndMakeVisible (display);
    display.onSelectionChanged = [this] (int b) { bindBand (b); };

    bandTitle.setJustificationType (juce::Justification::centredLeft);
    bandTitle.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    addAndMakeVisible (bandTitle);

    bandOn.setClickingTogglesState (true);
    bandOn.setTooltip (tr ("Activa o desactiva esta banda.", "Turns this band on or off."));
    addAndMakeVisible (bandOn);

    typeBox.addItemList (Choices::types, 1);
    typeBox.setTooltip (tr ("Bell: sube o baja alrededor de una frecuencia. Shelf: todo lo de arriba o lo de abajo. "
                            "Cut: elimina graves o agudos. Notch: quita una frecuencia exacta. Tilt: inclina todo el espectro.",
                            "Bell: boost or cut around a frequency. Shelf: everything above or below. "
                            "Cut: removes lows or highs. Notch: removes one exact frequency. Tilt: tilts the whole spectrum."));
    addAndMakeVisible (typeBox);

    slopeBox.addItemList (Choices::slopes, 1);
    slopeBox.setTooltip (tr ("Qué tan rápido corta el filtro (dB por octava). 12 dB es suave y natural; 48 dB es casi un muro.",
                             "How steeply the filter cuts (dB per octave). 12 dB is gentle and natural; 48 dB is almost a wall."));
    addAndMakeVisible (slopeBox);

    placeBox.addItemList (Choices::placements, 1);
    placeBox.setTooltip (tr ("Dónde actúa la banda: en todo el estéreo, solo izquierda/derecha, solo el centro (Mid) o solo los lados (Side).",
                             "Where the band acts: the whole stereo image, left/right only, the centre (Mid) or the sides (Side)."));
    addAndMakeVisible (placeBox);

    freq.slider.setTooltip (tr ("Frecuencia central de la banda.", "Centre frequency of the band."));
    gain.slider.setTooltip (tr ("Cuánto sube (+) o baja (-) la banda.", "How much the band boosts (+) or cuts (-)."));
    q.slider.setTooltip (tr ("Ancho de la banda: Q bajo = ancho y musical, Q alto = quirúrgico. En cortes de 12 dB controla la resonancia.",
                             "Band width: low Q = wide and musical, high Q = surgical. On 12 dB cuts it controls the resonance."));
    for (auto* k : { &freq, &gain, &q, &output })
        addAndMakeVisible (*k);

    outputAttachment = std::make_unique<SliderAttachment> (processor.state, ParamIDs::output, output.slider);
    output.slider.setDoubleClickReturnValue (true, 0.0);
    output.slider.setTooltip (tr ("Volumen final después del EQ.", "Final level after the EQ."));

    autoGain.setButtonText ("AUTO GAIN");
    autoGain.setClickingTogglesState (true);
    autoGain.setTooltip (tr ("Compensa el volumen que la curva añade o quita, para que compares sin engañarte por el volumen.",
                             "Compensates the loudness the curve adds or removes, so you compare without being fooled by level."));
    autoGainAttachment = std::make_unique<ButtonAttachment> (processor.state, ParamIDs::autoGain, autoGain);
    addAndMakeVisible (autoGain);

    bypass.setButtonText ("BYPASS");
    bypass.setClickingTogglesState (true);
    bypass.setTooltip (tr ("Escucha la señal sin el EQ.", "Hear the signal without the EQ."));
    bypassAttachment = std::make_unique<ButtonAttachment> (processor.state, ParamIDs::bypass, bypass);
    addAndMakeVisible (bypass);

    range.setButtonText (juce::String::fromUTF8 ("±30 dB"));
    range.setTooltip (tr ("Escala vertical de la gráfica: ±30 dB o ±12 dB para ver mejor los ajustes finos.",
                          "Vertical scale of the graph: ±30 dB, or ±12 dB to see fine moves better."));
    range.onClick = [this]
    {
        const bool wide = display.getRangeDb() > 15.0f;
        display.setRangeDb (wide ? 12.0f : 30.0f);
        range.setButtonText (wide ? juce::String::fromUTF8 ("±12 dB") : juce::String::fromUTF8 ("±30 dB"));
    };
    addAndMakeVisible (range);

    analyzerBox.addItemList (Choices::analyzer, 1);
    analyzerBox.setTooltip (tr ("Analizador: Pre muestra la señal antes del EQ, Post después. Apágalo para ahorrar CPU.",
                                "Analyzer: Pre shows the signal before the EQ, Post after. Turn it off to save CPU."));
    analyzerAttachment = std::make_unique<ComboAttachment> (processor.state, ParamIDs::analyzer, analyzerBox);
    addAndMakeVisible (analyzerBox);
    analyzerLabel.setText (tr ("ANALIZADOR", "ANALYZER"), juce::dontSendNotification);
    analyzerLabel.setJustificationType (juce::Justification::centred);
    analyzerLabel.setColour (juce::Label::textColourId, colours::textDim);
    analyzerLabel.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    addAndMakeVisible (analyzerLabel);

    bindBand (display.getSelectedBand());

    setResizable (true, true);
    setResizeLimits (baseWidth * 3 / 4, baseHeight * 3 / 4, baseWidth * 2, baseHeight * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) baseWidth / baseHeight);
    setSize (baseWidth, baseHeight);

    startTimerHz (30);
}

ShapeEQEditor::~ShapeEQEditor()
{
    processor.preSpectrum.active.store (false);
    processor.postSpectrum.active.store (false);
    setLookAndFeel (nullptr);
}

void ShapeEQEditor::bindBand (int band)
{
    if (band == boundBand) return;
    boundBand = band;
    bandSliders.clear();
    bandCombos.clear();
    bandOnAttachment.reset();

    auto& st = processor.state;
    bandSliders.push_back (std::make_unique<SliderAttachment> (st, ParamIDs::band (band, ParamIDs::freq), freq.slider));
    bandSliders.push_back (std::make_unique<SliderAttachment> (st, ParamIDs::band (band, ParamIDs::gain), gain.slider));
    bandSliders.push_back (std::make_unique<SliderAttachment> (st, ParamIDs::band (band, ParamIDs::q), q.slider));
    bandCombos.push_back (std::make_unique<ComboAttachment> (st, ParamIDs::band (band, ParamIDs::type), typeBox));
    bandCombos.push_back (std::make_unique<ComboAttachment> (st, ParamIDs::band (band, ParamIDs::slope), slopeBox));
    bandCombos.push_back (std::make_unique<ComboAttachment> (st, ParamIDs::band (band, ParamIDs::place), placeBox));
    bandOnAttachment = std::make_unique<ButtonAttachment> (st, ParamIDs::band (band, ParamIDs::on), bandOn);
    gain.slider.setDoubleClickReturnValue (true, 0.0);
    q.slider.setDoubleClickReturnValue (true, 1.0);

    bandTitle.setText (tr ("BANDA ", "BAND ") + juce::String (band + 1), juce::dontSendNotification);
    bandTitle.setColour (juce::Label::textColourId, EqDisplay::bandColour (band));
    bandOn.setColour (juce::TextButton::buttonOnColourId, EqDisplay::bandColour (band));
    updateBandControls();
}

void ShapeEQEditor::updateBandControls()
{
    const auto p = processor.readParams();
    const auto& bp = p.bands[(size_t) boundBand];
    bandOn.setButtonText (bp.enabled ? tr ("ACTIVA", "ON") : tr ("APAGADA", "OFF"));
    gain.setEnabled (Choices::typeHasGain (bp.type));
    gain.setAlpha (gain.isEnabled() ? 1.0f : 0.4f);
    slopeBox.setEnabled (Choices::typeHasSlope (bp.type));
    slopeBox.setAlpha (slopeBox.isEnabled() ? 1.0f : 0.4f);
    // For cuts above 12 dB the Butterworth sections fix the Q; the knob then does nothing.
    const bool qActive = ! Choices::typeHasSlope (bp.type) || bp.slopeDb <= 12;
    q.setAlpha (qActive ? 1.0f : 0.4f);
    autoGain.setButtonText (p.autoGain ? "AUTO " + juce::String (estimateAutoGainDb (p, processor.currentSampleRate.load()), 1) + " dB" : "AUTO GAIN");
}

void ShapeEQEditor::paint (juce::Graphics& g)
{
    g.fillAll (colours::background);

    const float s = (float) getWidth() / baseWidth;
    auto header = getLocalBounds().removeFromTop (juce::roundToInt (48 * s)).reduced (juce::roundToInt (20 * s), 0);

    g.setColour (colours::text);
    g.setFont (juce::FontOptions (24.0f * s, juce::Font::bold));
    g.drawText ("SHAPE EQ", header, juce::Justification::centredLeft);

    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (12.0f * s));
    g.drawText (juce::String::fromUTF8 ("ROMMUSER  ·  v") + PLUGIN_VERSION_STRING + juce::String::fromUTF8 ("  ·  ") + tr ("gratis y open source", "free & open source"),
                header, juce::Justification::centredRight);

    g.setColour (colours::outline);
    g.drawHorizontalLine (header.getBottom(), 0.0f, (float) getWidth());
}

void ShapeEQEditor::resized()
{
    const float s = (float) getWidth() / baseWidth;
    auto px = [s] (int v) { return juce::roundToInt ((float) v * s); };

    auto r = getLocalBounds();
    r.removeFromTop (px (48));
    r.reduce (px (16), px (10));

    // Bottom strip: selected band on the left, globals on the right.
    auto bottom = r.removeFromBottom (px (140));
    r.removeFromBottom (px (10));
    display.setBounds (r);

    auto globals = bottom.removeFromRight (px (260));
    bottom.removeFromRight (px (16));

    // Band strip.
    auto top = bottom.removeFromTop (px (30));
    bandTitle.setBounds (top.removeFromLeft (px (90)));
    bandOn.setBounds (top.removeFromLeft (px (90)).reduced (px (4), px (2)));
    top.removeFromLeft (px (8));
    const int cw = top.getWidth() / 3;
    typeBox.setBounds (top.removeFromLeft (cw).reduced (px (4), px (2)));
    slopeBox.setBounds (top.removeFromLeft (cw).reduced (px (4), px (2)));
    placeBox.setBounds (top.reduced (px (4), px (2)));
    const int kw = bottom.getWidth() / 3;
    freq.setBounds (bottom.removeFromLeft (kw));
    gain.setBounds (bottom.removeFromLeft (kw));
    q.setBounds (bottom);

    // Globals: output knob + a column of buttons.
    output.setBounds (globals.removeFromLeft (px (110)));
    globals.removeFromLeft (px (8));
    const int bh = globals.getHeight() / 4;
    auto analyzerRow = globals.removeFromTop (bh);
    analyzerLabel.setBounds (analyzerRow.removeFromLeft (analyzerRow.getWidth() * 45 / 100));
    analyzerBox.setBounds (analyzerRow.reduced (px (3), px (4)));
    autoGain.setBounds (globals.removeFromTop (bh).reduced (px (3), px (4)));
    bypass.setBounds (globals.removeFromTop (bh).reduced (px (3), px (4)));
    range.setBounds (globals.reduced (px (3), px (4)));
}

void ShapeEQEditor::timerCallback()
{
    display.refresh();
    updateBandControls();
}
