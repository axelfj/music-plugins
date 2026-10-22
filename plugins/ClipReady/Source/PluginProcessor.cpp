// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
const juce::StringArray oversamplingChoices { "1x", "2x", "4x", "8x" };
constexpr int oversamplingFactors[] = { 1, 2, 4, 8 };

juce::String dbString (float v, int) { return juce::String (v, 1) + " dB"; }
juce::String percentString (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; }
} // namespace

ClipReadyProcessor::ClipReadyProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "ClipReady", createLayout())
{
    pInput = state.getRawParameterValue (ParamIDs::input);
    pCeiling = state.getRawParameterValue (ParamIDs::ceiling);
    pSoftness = state.getRawParameterValue (ParamIDs::softness);
    pOutput = state.getRawParameterValue (ParamIDs::output);
    pMix = state.getRawParameterValue (ParamIDs::mix);
    pOversampling = state.getRawParameterValue (ParamIDs::oversampling);
    pDelta = state.getRawParameterValue (ParamIDs::delta);
    pLink = state.getRawParameterValue (ParamIDs::link);
    pStrict = state.getRawParameterValue (ParamIDs::strict);
}

juce::AudioProcessorValueTreeState::ParameterLayout ClipReadyProcessor::createLayout()
{
    using namespace juce;
    using P = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;

    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::input, 1 }, "Input",
                                     NormalisableRange<float> (-12.0f, 24.0f, 0.01f), 0.0f,
                                     A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::ceiling, 1 }, "Ceiling",
                                     NormalisableRange<float> (-24.0f, 0.0f, 0.01f), -0.3f,
                                     A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::softness, 1 }, "Softness",
                                     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 35.0f,
                                     A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::output, 1 }, "Output",
                                     NormalisableRange<float> (-24.0f, 12.0f, 0.01f), 0.0f,
                                     A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::mix, 1 }, "Mix",
                                     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
                                     A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::oversampling, 1 }, "Oversampling",
                                                        oversamplingChoices, 2));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::delta, 1 }, "Delta", false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::link, 1 }, "Gain Link", false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::strict, 1 }, "Strict Ceiling", true));
    return layout;
}

rommuser::clipready::ClipParams ClipReadyProcessor::readParams() const
{
    rommuser::clipready::ClipParams p;
    p.inputDb = pInput->load();
    p.ceilingDb = pCeiling->load();
    p.softness = pSoftness->load() / 100.0f;
    p.outputDb = pOutput->load();
    p.mix = pMix->load() / 100.0f;
    p.oversampling = oversamplingFactors[juce::jlimit (0, 3, juce::roundToInt (pOversampling->load()))];
    p.delta = pDelta->load() > 0.5f;
    p.gainLink = pLink->load() > 0.5f;
    p.strictCeiling = pStrict->load() > 0.5f;
    return p;
}

void ClipReadyProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, juce::jmax (32, samplesPerBlock), getTotalNumInputChannels());
    engine.setParams (readParams(), true);
    reportedLatency = engine.getLatencySamples();
    setLatencySamples (reportedLatency);
}

void ClipReadyProcessor::reset()
{
    engine.reset();
}

bool ClipReadyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void ClipReadyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numIn = getTotalNumInputChannels();
    for (int ch = numIn; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    engine.setParams (readParams());

    // Changing oversampling changes latency; tell the host so it can re-align tracks.
    const int latency = engine.getLatencySamples();
    if (latency != reportedLatency)
    {
        reportedLatency = latency;
        setLatencySamples (latency);
    }

    const auto m = engine.process (buffer.getArrayOfWritePointers(), numIn, buffer.getNumSamples());

    // Keep the highest value until the editor reads and resets it.
    auto storeMax = [] (std::atomic<float>& a, float v)
    {
        float prev = a.load();
        while (v > prev && ! a.compare_exchange_weak (prev, v)) {}
    };
    storeMax (inputPeak, m.inputPeak);
    storeMax (outputPeak, m.outputPeak);
    storeMax (clipDb, m.clipDb);
}

juce::AudioProcessorEditor* ClipReadyProcessor::createEditor()
{
    return new ClipReadyEditor (*this);
}

void ClipReadyProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void ClipReadyProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ClipReadyProcessor();
}
