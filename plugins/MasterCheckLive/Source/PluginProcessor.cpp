// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rommuser::mastercheck;

MasterCheckLiveProcessor::MasterCheckLiveProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "MasterCheckLive", createLayout())
{
    pPlatform = state.getRawParameterValue (ParamIDs::platform);
    for (auto& v : shortTermHistory)
        v.store (-120.0f);
}

juce::AudioProcessorValueTreeState::ParameterLayout MasterCheckLiveProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { ParamIDs::platform, 1 }, "Platform", Choices::platform, 0));
    return layout;
}

const Platform& MasterCheckLiveProcessor::currentPlatform() const
{
    return platforms[juce::jlimit (0, numPlatforms - 1, juce::roundToInt (pPlatform->load()))];
}

void MasterCheckLiveProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, std::max (16, samplesPerBlock), getMainBusNumInputChannels());
}

void MasterCheckLiveProcessor::reset()
{
    engine.reset();
}

bool MasterCheckLiveProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void MasterCheckLiveProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int numIn = std::min (getMainBusNumInputChannels(), maxChannels);

    if (resetRequested.exchange (false))
    {
        engine.reset();
        for (auto& v : shortTermHistory) v.store (-120.0f);
        blockTruePeak.store (-120.0f);
    }

    const auto r = engine.process (buffer.getArrayOfReadPointers(), numIn, n);

    for (int ch = numIn; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);

    momentary.store (r.momentaryLufs);
    shortTerm.store (r.shortTermLufs);
    integrated.store (r.integratedLufs);
    maxMomentary.store (r.maxMomentaryLufs);
    maxShortTerm.store (r.maxShortTermLufs);
    loudnessRange.store (r.loudnessRangeLu);
    truePeak.store (r.truePeakDb);
    samplePeak.store (r.samplePeakDb);
    plr.store (r.plrDb);
    float prev = blockTruePeak.load();
    while (r.blockTruePeakDb > prev && ! blockTruePeak.compare_exchange_weak (prev, r.blockTruePeakDb)) {}

    if (r.newBlock)
    {
        const int h = historyHead.load();
        shortTermHistory[(size_t) h].store (r.shortTermLufs);
        historyHead.store ((h + 1) % historyLength);
    }
}

juce::AudioProcessorEditor* MasterCheckLiveProcessor::createEditor()
{
    return new MasterCheckLiveEditor (*this);
}

void MasterCheckLiveProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void MasterCheckLiveProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MasterCheckLiveProcessor();
}
