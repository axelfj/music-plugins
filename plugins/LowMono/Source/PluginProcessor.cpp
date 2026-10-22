// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rommuser::lowmono;

namespace
{
juce::String percentString (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; }
juce::String hzOffString (float v, int)
{
    if (v < 20.0f) return "Off";
    return juce::String (juce::roundToInt (v)) + " Hz";
}
} // namespace

LowMonoProcessor::LowMonoProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "LowMono", createLayout())
{
    pWidth = state.getRawParameterValue (ParamIDs::width);
    pMonoBelow = state.getRawParameterValue (ParamIDs::monoBelow);
    pSteep = state.getRawParameterValue (ParamIDs::steep);
    pLowWidth = state.getRawParameterValue (ParamIDs::lowWidth);
}

juce::AudioProcessorValueTreeState::ParameterLayout LowMonoProcessor::createLayout()
{
    using namespace juce;
    using P = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;

    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::width, 1 }, "Width",
                                     NormalisableRange<float> (0.0f, 200.0f, 0.1f), 100.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    // 0 .. 500 Hz, with everything below 20 Hz meaning "off"; skewed so the useful 60-200 Hz area is wide.
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::monoBelow, 1 }, "Mono Below",
                                     NormalisableRange<float> (0.0f, 500.0f, 1.0f, 0.5f), 120.0f, A().withLabel ("Hz").withStringFromValueFunction (hzOffString)));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::steep, 1 }, "Steep (48 dB/oct)", false));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::lowWidth, 1 }, "Low Width",
                                     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    return layout;
}

MonoParams LowMonoProcessor::readParams() const
{
    MonoParams p;
    p.width = pWidth->load() / 100.0f;
    const float hz = pMonoBelow->load();
    p.monoBelowHz = hz < 20.0f ? 0.0f : hz;
    p.steep = pSteep->load() > 0.5f;
    p.lowWidth = pLowWidth->load() / 100.0f;
    return p;
}

void LowMonoProcessor::applyPreset (const MonoPreset& preset)
{
    auto setValue = [this] (const char* id, float value)
    {
        if (auto* param = state.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };
    setValue (ParamIDs::width, preset.width * 100.0f);
    setValue (ParamIDs::monoBelow, preset.monoBelowHz);
    setValue (ParamIDs::steep, preset.steep ? 1.0f : 0.0f);
    setValue (ParamIDs::lowWidth, preset.lowWidth * 100.0f);
}

void LowMonoProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
    engine.setParams (readParams());
}

void LowMonoProcessor::reset()
{
    engine.reset();
}

bool LowMonoProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void LowMonoProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels();

    engine.setParams (readParams());
    const auto m = engine.process (buffer.getArrayOfWritePointers(), numIn, n);

    for (int ch = numIn; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);

    correlation.store (m.correlation);
    float prev = midPeak.load();
    while (m.midPeak > prev && ! midPeak.compare_exchange_weak (prev, m.midPeak)) {}
    prev = sidePeak.load();
    while (m.sidePeak > prev && ! sidePeak.compare_exchange_weak (prev, m.sidePeak)) {}

    const float* sl = engine.getScopeL();
    const float* sr = engine.getScopeR();
    const int start = engine.getScopeWritePosition();
    for (int i = 0; i < scopePoints; ++i)
    {
        const int idx = (start + i) % scopePoints;
        scopeL[(size_t) i].store (sl[idx]);
        scopeR[(size_t) i].store (sr[idx]);
    }
}

juce::AudioProcessorEditor* LowMonoProcessor::createEditor()
{
    return new LowMonoEditor (*this);
}

void LowMonoProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void LowMonoProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LowMonoProcessor();
}
