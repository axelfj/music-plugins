// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rommuser::peakready;

namespace
{
juce::String dbString (float v, int) { return (v > 0.0f ? "+" : "") + juce::String (v, 1) + " dB"; }
juce::String msString (float v, int) { return juce::String (v, v < 100.0f ? 1 : 0) + " ms"; }
juce::String lufsString (float v, int) { return juce::String (v, 1) + " LUFS"; }

juce::NormalisableRange<float> logRange (float lo, float hi)
{
    return juce::NormalisableRange<float> (lo, hi,
        [] (float s, float e, float n) { return s * std::pow (e / s, n); },
        [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); });
}
} // namespace

PeakReadyProcessor::PeakReadyProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "PeakReady", createLayout())
{
    pInput = state.getRawParameterValue (ParamIDs::input);
    pCeiling = state.getRawParameterValue (ParamIDs::ceiling);
    pRelease = state.getRawParameterValue (ParamIDs::release);
    pAuto = state.getRawParameterValue (ParamIDs::autoRelease);
    pStyle = state.getRawParameterValue (ParamIDs::style);
    pTruePeak = state.getRawParameterValue (ParamIDs::truePeak);
    pTarget = state.getRawParameterValue (ParamIDs::target);
}

juce::AudioProcessorValueTreeState::ParameterLayout PeakReadyProcessor::createLayout()
{
    using namespace juce;
    using P = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;

    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::input, 1 }, "Input",
                                     NormalisableRange<float> (-12.0f, 24.0f, 0.1f), 0.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::ceiling, 1 }, "Ceiling",
                                     NormalisableRange<float> (-12.0f, 0.0f, 0.1f), -1.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::release, 1 }, "Release",
                                     logRange (10.0f, 1000.0f), 200.0f, A().withLabel ("ms").withStringFromValueFunction (msString)));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::autoRelease, 1 }, "Auto Release", true));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::style, 1 }, "Style", Choices::style, 0));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::truePeak, 1 }, "True Peak", true));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::target, 1 }, "Target Loudness",
                                     NormalisableRange<float> (-24.0f, -4.0f, 0.5f), -9.0f, A().withLabel ("LUFS").withStringFromValueFunction (lufsString)));
    return layout;
}

PeakParams PeakReadyProcessor::readParams() const
{
    PeakParams p;
    p.inputDb = pInput->load();
    p.ceilingDb = pCeiling->load();
    p.releaseMs = pRelease->load();
    p.autoRelease = pAuto->load() > 0.5f;
    p.style = (Style) juce::jlimit (0, Choices::style.size() - 1, juce::roundToInt (pStyle->load()));
    p.truePeak = pTruePeak->load() > 0.5f;
    return p;
}

void PeakReadyProcessor::applyPreset (const PeakPreset& preset)
{
    auto setValue = [this] (const char* id, float value)
    {
        if (auto* param = state.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };
    setValue (ParamIDs::ceiling, preset.ceilingDb);
    setValue (ParamIDs::target, preset.targetLufs);
    setValue (ParamIDs::release, preset.releaseMs);
    setValue (ParamIDs::style, (float) preset.style);
    setValue (ParamIDs::autoRelease, 1.0f);
    setValue (ParamIDs::truePeak, 1.0f);
}

void PeakReadyProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, std::max (16, samplesPerBlock), getMainBusNumInputChannels());
    engine.setParams (readParams(), true);
    reportedLatency = engine.getLatencySamples();
    setLatencySamples (reportedLatency);
}

void PeakReadyProcessor::reset()
{
    engine.reset();
}

bool PeakReadyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void PeakReadyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int numIn = std::min (getMainBusNumInputChannels(), maxChannels);

    if (loudnessResetRequested.exchange (false))
        engine.resetLoudness();

    engine.setParams (readParams());
    const int latency = engine.getLatencySamples();
    if (latency != reportedLatency)
    {
        reportedLatency = latency;
        setLatencySamples (latency);
    }

    const auto m = engine.process (buffer.getArrayOfWritePointers(), numIn, n);

    for (int ch = numIn; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);

    auto storeMax = [] (std::atomic<float>& a, float v)
    {
        float prev = a.load();
        while (v > prev && ! a.compare_exchange_weak (prev, v)) {}
    };
    storeMax (inputPeak, m.inputPeak);
    storeMax (outputPeak, m.outputPeak);
    storeMax (gainReductionDb, m.gainReductionDb);
    momentaryLufs.store (m.momentaryLufs);
    shortTermLufs.store (m.shortTermLufs);
    integratedLufs.store (m.integratedLufs);
}

juce::AudioProcessorEditor* PeakReadyProcessor::createEditor()
{
    return new PeakReadyEditor (*this);
}

void PeakReadyProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void PeakReadyProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PeakReadyProcessor();
}
