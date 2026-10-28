// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginEditor.h"

using namespace rommuser::ui;
using namespace rommuser::mastercheck;

namespace
{
constexpr int baseWidth = 920, baseHeight = 560;
constexpr float histMin = -40.0f, histMax = 0.0f;

juce::String fmt (float v, int decimals = 1) { return v <= -99.0f ? juce::String ("—") : juce::String (v, decimals); }
juce::String signedFmt (float v) { return (v > 0.0f ? "+" : "") + juce::String (v, 1); }
} // namespace

//==============================================================================
HistoryDisplay::HistoryDisplay (MasterCheckLiveProcessor& p) : processor (p) {}

void HistoryDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r, 8.0f);
    r = r.reduced (12.0f, 10.0f);
    auto yFor = [&] (float lufs) { return r.getBottom() - juce::jlimit (0.0f, 1.0f, (lufs - histMin) / (histMax - histMin)) * r.getHeight(); };

    g.setFont (juce::FontOptions (10.0f));
    for (float l = -40.0f; l <= 0.0f; l += 10.0f)
    {
        const float y = yFor (l);
        g.setColour (colours::outline);
        g.drawHorizontalLine (juce::roundToInt (y), r.getX(), r.getRight());
        g.setColour (colours::textDim);
        g.drawText (juce::String ((int) l), juce::Rectangle<float> (r.getRight() - 26.0f, y - 11.0f, 26.0f, 11.0f), juce::Justification::centredRight);
    }

    const auto& platform = processor.currentPlatform();
    if (isClub (platform))
    {
        g.setColour (colours::good.withAlpha (0.15f));
        g.fillRect (juce::Rectangle<float> (r.getX(), yFor (platform.clubHighLufs), r.getWidth(), yFor (platform.clubLowLufs) - yFor (platform.clubHighLufs)));
    }
    else
    {
        g.setColour (colours::good);
        const float dash[] = { 5.0f, 4.0f };
        g.drawDashedLine (juce::Line<float> (r.getX(), yFor (platform.targetLufs), r.getRight(), yFor (platform.targetLufs)), dash, 2, 1.5f);
    }

    const int len = MasterCheckLiveProcessor::historyLength;
    const int head = processor.historyHead.load();
    const float dx = r.getWidth() / (float) (len - 1);
    juce::Path p;
    bool started = false;
    for (int i = 0; i < len; ++i)
    {
        const float v = processor.shortTermHistory[(size_t) ((head + i) % len)].load();
        const float x = r.getX() + (float) i * dx;
        if (v <= -99.0f) { started = false; continue; }
        const float y = yFor (v);
        started ? p.lineTo (x, y) : p.startNewSubPath (x, y);
        started = true;
    }
    g.setColour (colours::accent);
    g.strokePath (p, juce::PathStrokeType (1.6f));

    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (tr ("corto plazo, últimos 60 s", "short-term, last 60 s"), r.reduced (4.0f, 2.0f), juce::Justification::topLeft);
}

//==============================================================================
Readout::Readout (juce::String c, juce::String u, bool b) : caption (std::move (c)), unit (std::move (u)), big (b) {}

void Readout::set (float value, juce::Colour col)
{
    text = fmt (value);
    colour = col;
    repaint();
}

void Readout::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::panel);
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (big ? 13.0f : 11.0f, juce::Font::bold));
    g.drawText (caption, r.reduced (8.0f, 6.0f), juce::Justification::topLeft);
    g.setColour (colour.isTransparent() ? colours::text : colour);
    g.setFont (juce::FontOptions (big ? r.getHeight() * 0.42f : r.getHeight() * 0.36f, juce::Font::bold));
    g.drawText (text, r.reduced (8.0f, 4.0f), juce::Justification::centred);
    g.setColour (colours::textDim);
    g.setFont (juce::FontOptions (big ? 12.0f : 10.0f));
    g.drawText (unit, r.reduced (8.0f, 6.0f), juce::Justification::bottomRight);
}

//==============================================================================
MasterCheckLiveEditor::MasterCheckLiveEditor (MasterCheckLiveProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      integrated (tr ("INTEGRADO", "INTEGRATED"), "LUFS", true),
      shortTerm (tr ("CORTO PLAZO", "SHORT-TERM"), "LUFS"),
      momentary (tr ("MOMENTÁNEO", "MOMENTARY"), "LUFS"),
      lra (tr ("RANGO (LRA)", "RANGE (LRA)"), "LU"),
      truePeak ("TRUE PEAK", "dBTP"),
      plr ("PLR", "dB"),
      maxShortTerm (tr ("MÁX. CORTO PLAZO", "MAX SHORT-TERM"), "LUFS"),
      history (p)
{
    setLookAndFeel (&lnf);

    for (auto* r : { &integrated, &shortTerm, &momentary, &lra, &truePeak, &plr, &maxShortTerm })
        addAndMakeVisible (*r);
    integrated.setTooltip (tr ("Volumen de toda la canción desde el último reinicio. Es el número que miran las plataformas.",
                               "Loudness of the whole song since the last reset. This is the number the platforms look at."));
    lra.setTooltip (tr ("Cuánto varía el volumen a lo largo de la canción. EDM: 3–6 LU es normal.",
                        "How much the loudness varies through the song. EDM: 3–6 LU is normal."));
    truePeak.setTooltip (tr ("Pico real (entre muestras, 4x). Las plataformas piden −1 dBTP o menos.",
                             "Real peak (between samples, 4x). Platforms ask for −1 dBTP or less."));
    plr.setTooltip (tr ("Pico menos loudness: cuánta pegada queda. Menos de 8 dB = muy aplastado.",
                        "Peak minus loudness: how much punch is left. Under 8 dB = very squashed."));

    platformBox.addItemList (Choices::platform, 1);
    platformBox.setTooltip (tr ("Para qué plataforma estás revisando. Los valores son los mismos que en la app Master Check.",
                                "Which platform you are checking for. Same values as the Master Check app."));
    addAndMakeVisible (platformBox);
    platformAttachment = std::make_unique<ComboAttachment> (processor.state, ParamIDs::platform, platformBox);
    platformLabel.setText (tr ("PLATAFORMA", "PLATFORM"), juce::dontSendNotification);
    platformLabel.setJustificationType (juce::Justification::centred);
    platformLabel.setColour (juce::Label::textColourId, colours::textDim);
    platformLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    addAndMakeVisible (platformLabel);

    verdictLabel.setJustificationType (juce::Justification::centredLeft);
    verdictLabel.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    addAndMakeVisible (verdictLabel);
    detailLabel.setJustificationType (juce::Justification::centredLeft);
    detailLabel.setColour (juce::Label::textColourId, colours::textDim);
    detailLabel.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (detailLabel);

    resetButton.setButtonText (tr ("REINICIAR", "RESET"));
    resetButton.setTooltip (tr ("Empieza la medición de cero. Hazlo al inicio de la canción y déjala sonar completa.",
                                "Start measuring from zero. Do it at the start of the song and let it play through."));
    resetButton.onClick = [this] { processor.requestReset(); };
    addAndMakeVisible (resetButton);

    addAndMakeVisible (history);

    setResizable (true, true);
    setResizeLimits (baseWidth * 3 / 4, baseHeight * 3 / 4, baseWidth * 2, baseHeight * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) baseWidth / baseHeight);
    setSize (baseWidth, baseHeight);

    startTimerHz (30);
}

MasterCheckLiveEditor::~MasterCheckLiveEditor()
{
    setLookAndFeel (nullptr);
}

void MasterCheckLiveEditor::paint (juce::Graphics& g)
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

void MasterCheckLiveEditor::resized()
{
    const float s = (float) getWidth() / baseWidth;
    auto px = [s] (int v) { return juce::roundToInt ((float) v * s); };

    auto r = getLocalBounds();
    r.removeFromTop (px (48));
    r.reduce (px (16), px (12));

    // Top row: big integrated + six small readouts.
    auto top = r.removeFromTop (px (150));
    integrated.setBounds (top.removeFromLeft (px (230)));
    top.removeFromLeft (px (10));
    auto small1 = top.removeFromTop (top.getHeight() / 2 - px (4));
    top.removeFromTop (px (8));
    const int w = small1.getWidth() / 3;
    shortTerm.setBounds (small1.removeFromLeft (w).reduced (px (4), 0));
    momentary.setBounds (small1.removeFromLeft (w).reduced (px (4), 0));
    maxShortTerm.setBounds (small1.reduced (px (4), 0));
    lra.setBounds (top.removeFromLeft (w).reduced (px (4), 0));
    truePeak.setBounds (top.removeFromLeft (w).reduced (px (4), 0));
    plr.setBounds (top.reduced (px (4), 0));
    r.removeFromTop (px (10));

    // Platform row.
    auto row = r.removeFromTop (px (60));
    auto cell = row.removeFromLeft (px (200));
    platformLabel.setBounds (cell.removeFromTop (px (16)));
    platformBox.setBounds (cell.reduced (px (4), px (6)));
    resetButton.setBounds (row.removeFromRight (px (130)).reduced (px (4), px (14)));
    row.removeFromLeft (px (12));
    verdictLabel.setBounds (row.removeFromTop (row.getHeight() / 2));
    detailLabel.setBounds (row);
    r.removeFromTop (px (10));

    history.setBounds (r);
}

void MasterCheckLiveEditor::timerCallback()
{
    const float i = processor.integrated.load();
    const float tp = processor.truePeak.load();
    const auto& platform = processor.currentPlatform();
    const auto v = verdict (platform, i, tp);
    const juce::Colour vc = v == Verdict::fail ? colours::warn : v == Verdict::warn ? juce::Colour (0xffe0a84b) : v == Verdict::ok ? colours::good : colours::textDim;

    integrated.set (i, v == Verdict::info ? juce::Colour() : vc);
    shortTerm.set (processor.shortTerm.load());
    momentary.set (processor.momentary.load());
    maxShortTerm.set (processor.maxShortTerm.load());
    lra.set (processor.loudnessRange.load());
    const float ceilingDb = applicableTruePeakCeiling (platform, i);
    truePeak.set (tp, tp > ceilingDb + 0.05f ? colours::warn : juce::Colour());
    plr.set (processor.plr.load());
    processor.blockTruePeak.exchange (-120.0f);

    juce::String verdictText, detail;
    if (v == Verdict::info)
    {
        verdictText = tr ("Dale play y deja sonar la canción…", "Press play and let the song run…");
        detail = tr ("El integrado necesita unos segundos. Reinicia al empezar la canción.",
                     "Integrated loudness needs a few seconds. Reset when the song starts.");
    }
    else if (isClub (platform))
    {
        verdictText = tr ("En el club se escucha tal cual: ", "The club plays it as it is: ") + fmt (i) + " LUFS";
        detail = (i < platform.clubLowLufs - 1.0f) ? tr ("Un poco flojo para club; lo normal es −10 a −6 LUFS.", "A bit quiet for club; −10 to −6 LUFS is usual.")
               : (i > platform.clubHighLufs + 1.0f) ? tr ("Más fuerte que lo normal (−10 a −6 LUFS): revisa que no esté aplastado.", "Louder than usual (−10 to −6 LUFS): check it is not squashed.")
               : tr ("Dentro del rango normal para club.", "Within the usual club range.");
        if (tp > ceilingDb + 0.05f)
            detail = tr ("El true peak pasa de 0 dBTP: va a distorsionar al convertir a MP3/streaming.", "True peak is over 0 dBTP: it will distort on MP3/streaming conversion.");
    }
    else
    {
        const float gain = playbackGainDb (platform, i, tp);
        const juce::String name (platform.name);
        if (gain < -0.05f)
            verdictText = name + tr (" va a bajar tu canción ", " will turn your song down ") + juce::String (-gain, 1) + " dB";
        else if (gain > 0.05f)
            verdictText = name + tr (" va a subir tu canción ", " will turn your song up ") + juce::String (gain, 1) + " dB";
        else
            verdictText = name + tr (" la deja como está", " leaves it as it is");
        detail = tr ("Sonará a ", "It will play at ") + fmt (i + gain) + tr (" LUFS. Objetivo ", " LUFS. Target ") + fmt (platform.targetLufs) + tr (" LUFS, techo ", " LUFS, ceiling ") + signedFmt (ceilingDb) + " dBTP"
               + (platform.estimated ? tr (" (estimado).", " (estimated).") : ".");
        if (tp > ceilingDb + 0.05f)
            detail = tr ("El true peak (", "True peak (") + signedFmt (tp) + tr (" dBTP) pasa del techo de ", " dBTP) is over the ") + signedFmt (ceilingDb) + tr (" dBTP: baja el limitador.", " dBTP ceiling: lower the limiter.");
        else if (gain < -6.0f)
            detail += tr (" Está mucho más fuerte de lo necesario: pierdes pegada sin ganar nada.", " It is much louder than needed: you lose punch for nothing.");
    }
    verdictLabel.setText (verdictText, juce::dontSendNotification);
    verdictLabel.setColour (juce::Label::textColourId, v == Verdict::info ? colours::textDim : vc);
    detailLabel.setText (detail, juce::dontSendNotification);
    history.repaint();
}
