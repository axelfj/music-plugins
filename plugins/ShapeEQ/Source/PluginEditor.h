// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PluginProcessor.h"

#include "ui/RommuserLookAndFeel.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <functional>

// The big interactive graph: grid, pre/post spectrum, the EQ curve and one draggable node per band.
//   drag node        -> frequency + gain (or Q, for types without gain)
//   mouse wheel      -> Q of the band under the mouse (or the selected one)
//   double-click     -> on empty space: new band here; on a node: enable / disable it
//   right-click node -> type, slope, placement, remove
class EqDisplay : public juce::Component
{
public:
    explicit EqDisplay (ShapeEQProcessor&);

    // Called from the editor timer: re-reads parameters and pulls analyzer data.
    void refresh();

    void setSelectedBand (int band);
    int getSelectedBand() const { return selected; }
    std::function<void (int)> onSelectionChanged;

    void setRangeDb (float r) { rangeDb = r; repaint(); }
    float getRangeDb() const { return rangeDb; }

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    static juce::Colour bandColour (int band);

private:
    juce::Rectangle<float> graphArea() const;
    float freqToX (double f) const;
    double xToFreq (float x) const;
    float dbToY (double db) const;
    double yToDb (float y) const;
    juce::Point<float> nodePosition (int band) const;
    int bandAt (juce::Point<float> p) const;
    int firstFreeBand() const;

    void setParam (int band, const char* suffix, float value);
    void gesture (int band, const char* suffix, bool begin);
    void showBandMenu (int band, juce::Point<int> where);
    void drawSpectrum (juce::Graphics&, const rommuser::shapeeq::SpectrumAnalyzer&, juce::Colour, bool fill) const;
    void drawCurve (juce::Graphics&, int onlyBand, juce::Colour, float thickness) const;
    double curveDb (double f, int onlyBand) const;

    ShapeEQProcessor& processor;
    rommuser::shapeeq::EqParams params;
    std::array<rommuser::shapeeq::BandCoeffs, rommuser::shapeeq::maxBands> coeffs {};
    rommuser::shapeeq::SpectrumAnalyzer preAnalyzer, postAnalyzer;
    int analyzerMode = 3;
    double sampleRate = 44100.0;

    int selected = 0, hovered = -1, dragging = -1;
    double dragStartQ = 1.0;
    float dragStartY = 0.0f;
    float rangeDb = 30.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EqDisplay)
};

class ShapeEQEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ShapeEQEditor (ShapeEQProcessor&);
    ~ShapeEQEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void bindBand (int band);
    void updateBandControls();

    ShapeEQProcessor& processor;
    rommuser::ui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    EqDisplay display;

    // Selected band controls.
    juce::Label bandTitle;
    juce::TextButton bandOn;
    juce::ComboBox typeBox, slopeBox, placeBox;
    rommuser::ui::Knob freq, gain, q;

    // Global controls.
    rommuser::ui::Knob output;
    juce::TextButton autoGain, bypass, range;
    juce::ComboBox analyzerBox;
    juce::Label analyzerLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> bandSliders;
    std::vector<std::unique_ptr<ComboAttachment>> bandCombos;
    std::unique_ptr<ButtonAttachment> bandOnAttachment;
    std::unique_ptr<SliderAttachment> outputAttachment;
    std::unique_ptr<ButtonAttachment> autoGainAttachment, bypassAttachment;
    std::unique_ptr<ComboAttachment> analyzerAttachment;
    int boundBand = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShapeEQEditor)
};
