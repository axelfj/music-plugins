// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rommuser::shapeeq;

namespace
{
juce::String dbString (float v, int) { return juce::String (v, 1) + " dB"; }
juce::String qString (float v, int) { return juce::String (v, v < 10.0f ? 2 : 1); }
juce::String hzString (float v, int)
{
    return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz" : juce::String (v, v < 100.0f ? 1 : 0) + " Hz";
}

// Logarithmic ranges feel right for frequency and Q.
juce::NormalisableRange<float> logRange (float lo, float hi)
{
    return juce::NormalisableRange<float> (lo, hi,
        [] (float s, float e, float n) { return s * std::pow (e / s, n); },
        [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); });
}

// Where a freshly enabled band lands: spread over the spectrum so the 12 nodes don't stack up.
float defaultFreq (int band) { return 30.0f * std::pow (16000.0f / 30.0f, (float) band / (float) (maxBands - 1)); }
} // namespace

ShapeEQProcessor::ShapeEQProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "ShapeEQ", createLayout())
{
    for (int b = 0; b < maxBands; ++b)
    {
        auto& p = bands[(size_t) b];
        p.on = state.getRawParameterValue (ParamIDs::band (b, ParamIDs::on));
        p.type = state.getRawParameterValue (ParamIDs::band (b, ParamIDs::type));
        p.freq = state.getRawParameterValue (ParamIDs::band (b, ParamIDs::freq));
        p.gain = state.getRawParameterValue (ParamIDs::band (b, ParamIDs::gain));
        p.q = state.getRawParameterValue (ParamIDs::band (b, ParamIDs::q));
        p.slope = state.getRawParameterValue (ParamIDs::band (b, ParamIDs::slope));
        p.place = state.getRawParameterValue (ParamIDs::band (b, ParamIDs::place));
    }
    pOutput = state.getRawParameterValue (ParamIDs::output);
    pAutoGain = state.getRawParameterValue (ParamIDs::autoGain);
    pBypass = state.getRawParameterValue (ParamIDs::bypass);
    pAnalyzer = state.getRawParameterValue (ParamIDs::analyzer);
}

juce::AudioProcessorValueTreeState::ParameterLayout ShapeEQProcessor::createLayout()
{
    using namespace juce;
    using P = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;

    AudioProcessorValueTreeState::ParameterLayout layout;

    for (int b = 0; b < maxBands; ++b)
    {
        const String n = "Band " + String (b + 1) + " ";
        auto group = std::make_unique<AudioProcessorParameterGroup> ("band" + String (b + 1), "Band " + String (b + 1), " | ");
        group->addChild (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::band (b, ParamIDs::on), 1 }, n + "On", false));
        group->addChild (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::band (b, ParamIDs::type), 1 }, n + "Type", Choices::types, 0));
        group->addChild (std::make_unique<P> (ParameterID { ParamIDs::band (b, ParamIDs::freq), 1 }, n + "Freq",
                                              logRange (10.0f, 30000.0f), defaultFreq (b),
                                              A().withLabel ("Hz").withStringFromValueFunction (hzString)));
        group->addChild (std::make_unique<P> (ParameterID { ParamIDs::band (b, ParamIDs::gain), 1 }, n + "Gain",
                                              NormalisableRange<float> (-30.0f, 30.0f, 0.01f), 0.0f,
                                              A().withLabel ("dB").withStringFromValueFunction (dbString)));
        group->addChild (std::make_unique<P> (ParameterID { ParamIDs::band (b, ParamIDs::q), 1 }, n + "Q",
                                              logRange (0.025f, 40.0f), 1.0f,
                                              A().withStringFromValueFunction (qString)));
        group->addChild (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::band (b, ParamIDs::slope), 1 }, n + "Slope", Choices::slopes, 1));
        group->addChild (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::band (b, ParamIDs::place), 1 }, n + "Placement", Choices::placements, 0));
        layout.add (std::move (group));
    }

    layout.add (std::make_unique<P> (ParameterID { ParamIDs::output, 1 }, "Output",
                                     NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f,
                                     A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::autoGain, 1 }, "Auto Gain", false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::bypass, 1 }, "Bypass", false));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::analyzer, 1 }, "Analyzer", Choices::analyzer, 3));
    return layout;
}

EqParams ShapeEQProcessor::readParams() const
{
    EqParams p;
    for (int b = 0; b < maxBands; ++b)
    {
        const auto& src = bands[(size_t) b];
        auto& dst = p.bands[(size_t) b];
        dst.enabled = src.on->load() > 0.5f;
        dst.type = (rommuser::dsp::FilterType) juce::jlimit (0, Choices::types.size() - 1, juce::roundToInt (src.type->load()));
        dst.freq = src.freq->load();
        dst.gainDb = src.gain->load();
        dst.q = src.q->load();
        dst.slopeDb = Choices::slopeValues[juce::jlimit (0, Choices::slopes.size() - 1, juce::roundToInt (src.slope->load()))];
        dst.placement = (Placement) juce::jlimit (0, Choices::placements.size() - 1, juce::roundToInt (src.place->load()));
    }
    p.outputDb = pOutput->load();
    p.autoGain = pAutoGain->load() > 0.5f;
    p.bypass = pBypass->load() > 0.5f;
    return p;
}

void ShapeEQProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate.store (sampleRate);
    engine.prepare (sampleRate, juce::jmax (32, samplesPerBlock), getTotalNumInputChannels());
    engine.setParams (readParams(), true);
    setLatencySamples (engine.getLatencySamples());
}

void ShapeEQProcessor::reset()
{
    engine.reset();
}

bool ShapeEQProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void ShapeEQProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numIn = getTotalNumInputChannels();
    const int n = buffer.getNumSamples();
    for (int ch = numIn; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);

    const int analyzerMode = juce::roundToInt (pAnalyzer->load());
    if (analyzerMode == 1 || analyzerMode == 3)
        preSpectrum.push (buffer.getArrayOfReadPointers(), numIn, n);

    engine.setParams (readParams());
    engine.process (buffer.getArrayOfWritePointers(), numIn, n);

    if (analyzerMode >= 2)
        postSpectrum.push (buffer.getArrayOfReadPointers(), numIn, n);
}

juce::AudioProcessorEditor* ShapeEQProcessor::createEditor()
{
    return rommuser::ui::createScalableEditor<ShapeEQEditor> (*this);
}

void ShapeEQProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void ShapeEQProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ShapeEQProcessor();
}
