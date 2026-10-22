// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rommuser::bandpush;

namespace
{
juce::String dbString (float v, int) { return juce::String (v, 1) + " dB"; }
juce::String percentString (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; }
juce::String hzString (float v, int) { return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz"; }
juce::String timeString (float v, int) { return juce::String (v, 2) + "x"; }

juce::NormalisableRange<float> logRange (float lo, float hi)
{
    return juce::NormalisableRange<float> (lo, hi,
        [] (float s, float e, float n) { return s * std::pow (e / s, n); },
        [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); });
}
} // namespace

BandPushProcessor::BandPushProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "BandPush", createLayout())
{
    pDepth = state.getRawParameterValue (ParamIDs::depth);
    pTime = state.getRawParameterValue (ParamIDs::time);
    pThreshold = state.getRawParameterValue (ParamIDs::threshold);
    pInput = state.getRawParameterValue (ParamIDs::input);
    pOutput = state.getRawParameterValue (ParamIDs::output);
    pMix = state.getRawParameterValue (ParamIDs::mix);
    pLowXover = state.getRawParameterValue (ParamIDs::lowXover);
    pHighXover = state.getRawParameterValue (ParamIDs::highXover);
    for (int b = 0; b < numBands; ++b)
    {
        pUp[b] = state.getRawParameterValue (ParamIDs::band (b, "up"));
        pDown[b] = state.getRawParameterValue (ParamIDs::band (b, "down"));
        pGain[b] = state.getRawParameterValue (ParamIDs::band (b, "gain"));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout BandPushProcessor::createLayout()
{
    using namespace juce;
    using P = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;

    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::depth, 1 }, "Depth",
                                     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f,
                                     A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::time, 1 }, "Time",
                                     logRange (0.1f, 10.0f), 1.0f,
                                     A().withStringFromValueFunction (timeString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::threshold, 1 }, "Threshold",
                                     NormalisableRange<float> (-48.0f, 0.0f, 0.1f), -24.0f,
                                     A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::input, 1 }, "Input",
                                     NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f,
                                     A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::output, 1 }, "Output",
                                     NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f,
                                     A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::mix, 1 }, "Mix",
                                     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
                                     A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::lowXover, 1 }, "Low Crossover",
                                     logRange (40.0f, 1000.0f), 120.0f,
                                     A().withLabel ("Hz").withStringFromValueFunction (hzString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::highXover, 1 }, "High Crossover",
                                     logRange (500.0f, 12000.0f), 2500.0f,
                                     A().withLabel ("Hz").withStringFromValueFunction (hzString)));

    const char* bandNames[] = { "Low", "Mid", "High" };
    for (int b = 0; b < numBands; ++b)
    {
        const juce::String name (bandNames[b]);
        layout.add (std::make_unique<P> (ParameterID { ParamIDs::band (b, "up"), 1 }, name + " Up",
                                         NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
                                         A().withLabel ("%").withStringFromValueFunction (percentString)));
        layout.add (std::make_unique<P> (ParameterID { ParamIDs::band (b, "down"), 1 }, name + " Down",
                                         NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
                                         A().withLabel ("%").withStringFromValueFunction (percentString)));
        layout.add (std::make_unique<P> (ParameterID { ParamIDs::band (b, "gain"), 1 }, name + " Gain",
                                         NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f,
                                         A().withLabel ("dB").withStringFromValueFunction (dbString)));
    }
    return layout;
}

PushParams BandPushProcessor::readParams() const
{
    PushParams p;
    p.depth = pDepth->load() / 100.0f;
    p.time = pTime->load();
    p.thresholdDb = pThreshold->load();
    p.inDb = pInput->load();
    p.outDb = pOutput->load();
    p.mix = pMix->load() / 100.0f;
    p.lowXoverHz = pLowXover->load();
    p.highXoverHz = pHighXover->load();
    for (int b = 0; b < numBands; ++b)
    {
        p.bands[(size_t) b].up = pUp[b]->load() / 100.0f;
        p.bands[(size_t) b].down = pDown[b]->load() / 100.0f;
        p.bands[(size_t) b].gainDb = pGain[b]->load();
    }
    return p;
}

void BandPushProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate, getMainBusNumInputChannels());
    engine.setParams (readParams(), true);
}

void BandPushProcessor::reset()
{
    engine.reset();
}

bool BandPushProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void BandPushProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels();

    engine.setParams (readParams());
    const auto m = engine.process (buffer.getArrayOfWritePointers(), numIn, n);

    for (int ch = numIn; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);

    for (int b = 0; b < numBands; ++b)
        bandGainDb[b].store (m.bandGainDb[(size_t) b]);
    float prev = outputPeak.load();
    while (m.outputPeak > prev && ! outputPeak.compare_exchange_weak (prev, m.outputPeak)) {}
}

juce::AudioProcessorEditor* BandPushProcessor::createEditor()
{
    return new BandPushEditor (*this);
}

void BandPushProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void BandPushProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BandPushProcessor();
}
