// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rommuser::warmdrive;

namespace
{
juce::String dbString (float v, int) { return juce::String (v, 1) + " dB"; }
juce::String percentString (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; }
juce::String hzString (float v, int) { return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz"; }
juce::String toneString (float v, int) { return v >= 19999.0f ? juce::String ("Off") : hzString (v, 0); }

juce::NormalisableRange<float> logRange (float lo, float hi)
{
    return juce::NormalisableRange<float> (lo, hi,
        [] (float s, float e, float n) { return s * std::pow (e / s, n); },
        [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); });
}
} // namespace

WarmDriveProcessor::WarmDriveProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "WarmDrive", createLayout())
{
    pBands = state.getRawParameterValue (ParamIDs::bands);
    pXover[0] = state.getRawParameterValue (ParamIDs::xover1);
    pXover[1] = state.getRawParameterValue (ParamIDs::xover2);
    pXover[2] = state.getRawParameterValue (ParamIDs::xover3);
    pAutoGain = state.getRawParameterValue (ParamIDs::autoGain);
    pInput = state.getRawParameterValue (ParamIDs::input);
    pOutput = state.getRawParameterValue (ParamIDs::output);
    pMix = state.getRawParameterValue (ParamIDs::mix);
    pOversampling = state.getRawParameterValue (ParamIDs::oversampling);
    for (int b = 0; b < maxBands; ++b)
    {
        pType[b] = state.getRawParameterValue (ParamIDs::band (b, "type"));
        pDrive[b] = state.getRawParameterValue (ParamIDs::band (b, "drive"));
        pTone[b] = state.getRawParameterValue (ParamIDs::band (b, "tone"));
        pBandMix[b] = state.getRawParameterValue (ParamIDs::band (b, "mix"));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout WarmDriveProcessor::createLayout()
{
    using namespace juce;
    using P = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;

    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::bands, 1 }, "Bands", Choices::bands, 0));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::xover1, 1 }, "Crossover 1",
                                     logRange (40.0f, 2000.0f), 150.0f, A().withLabel ("Hz").withStringFromValueFunction (hzString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::xover2, 1 }, "Crossover 2",
                                     logRange (200.0f, 8000.0f), 800.0f, A().withLabel ("Hz").withStringFromValueFunction (hzString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::xover3, 1 }, "Crossover 3",
                                     logRange (1000.0f, 16000.0f), 4000.0f, A().withLabel ("Hz").withStringFromValueFunction (hzString)));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::autoGain, 1 }, "Auto Gain", true));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::input, 1 }, "Input",
                                     NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::output, 1 }, "Output",
                                     NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::mix, 1 }, "Mix",
                                     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::oversampling, 1 }, "Oversampling", Choices::oversampling, 1));

    for (int b = 0; b < maxBands; ++b)
    {
        const juce::String name = "Band " + juce::String (b + 1) + " ";
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::band (b, "type"), 1 }, name + "Type", Choices::types, 0));
        layout.add (std::make_unique<P> (ParameterID { ParamIDs::band (b, "drive"), 1 }, name + "Drive",
                                         NormalisableRange<float> (0.0f, 36.0f, 0.1f), 6.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
        layout.add (std::make_unique<P> (ParameterID { ParamIDs::band (b, "tone"), 1 }, name + "Tone",
                                         logRange (1000.0f, 20000.0f), 20000.0f, A().withLabel ("Hz").withStringFromValueFunction (toneString)));
        layout.add (std::make_unique<P> (ParameterID { ParamIDs::band (b, "mix"), 1 }, name + "Mix",
                                         NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    }
    return layout;
}

DriveParams WarmDriveProcessor::readParams() const
{
    DriveParams p;
    p.numBands = juce::jlimit (0, 3, juce::roundToInt (pBands->load())) + 1;
    for (int i = 0; i < 3; ++i)
        p.crossoverHz[i] = pXover[i]->load();
    p.autoGain = pAutoGain->load() > 0.5f;
    p.inDb = pInput->load();
    p.outDb = pOutput->load();
    p.mix = pMix->load() / 100.0f;
    p.oversampling = Choices::oversamplingValues[juce::jlimit (0, 2, juce::roundToInt (pOversampling->load()))];
    for (int b = 0; b < maxBands; ++b)
    {
        auto& bs = p.bands[(size_t) b];
        bs.type = (Saturation) juce::jlimit (0, numSaturations - 1, juce::roundToInt (pType[b]->load()));
        bs.driveDb = pDrive[b]->load();
        bs.toneHz = pTone[b]->load();
        bs.mix = pBandMix[b]->load() / 100.0f;
    }
    return p;
}

void WarmDriveProcessor::applyPreset (const DrivePreset& preset)
{
    auto setValue = [this] (const juce::String& id, float value)
    {
        if (auto* param = state.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };
    setValue (ParamIDs::bands, (float) (preset.numBands - 1));
    for (int b = 0; b < maxBands; ++b)
    {
        setValue (ParamIDs::band (b, "type"), (float) preset.types[b]);
        setValue (ParamIDs::band (b, "drive"), preset.driveDb[b]);
        setValue (ParamIDs::band (b, "tone"), 20000.0f);
        setValue (ParamIDs::band (b, "mix"), 100.0f);
    }
    setValue (ParamIDs::autoGain, 1.0f);
}

void WarmDriveProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, juce::jmax (32, samplesPerBlock), getMainBusNumInputChannels());
    engine.setParams (readParams(), true);
    reportedLatency = engine.getLatencySamples();
    setLatencySamples (reportedLatency);
}

void WarmDriveProcessor::reset()
{
    engine.reset();
}

bool WarmDriveProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void WarmDriveProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels();

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
    for (int b = 0; b < maxBands; ++b)
        storeMax (bandDriveDb[b], m.bandDriveDb[(size_t) b]);
    storeMax (outputPeak, m.outputPeak);
}

juce::AudioProcessorEditor* WarmDriveProcessor::createEditor()
{
    return new WarmDriveEditor (*this);
}

void WarmDriveProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void WarmDriveProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WarmDriveProcessor();
}
