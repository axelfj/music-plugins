// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rommuser::pressready;

namespace
{
juce::String dbString (float v, int) { return juce::String (v, 1) + " dB"; }
juce::String percentString (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; }
juce::String msString (float v, int) { return juce::String (v, v < 1.0f ? 2 : v < 10.0f ? 1 : 0) + " ms"; }
juce::String ratioString (float v, int)
{
    return v >= infiniteRatio ? juce::String::fromUTF8 ("\xe2\x88\x9e:1") : juce::String (v, v < 10.0f ? 1 : 0) + ":1";
}

juce::NormalisableRange<float> logRange (float lo, float hi)
{
    return juce::NormalisableRange<float> (lo, hi,
        [] (float s, float e, float n) { return s * std::pow (e / s, n); },
        [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); });
}
} // namespace

PressReadyProcessor::PressReadyProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                          .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      state (*this, nullptr, "PressReady", createLayout())
{
    pThreshold = state.getRawParameterValue (ParamIDs::threshold);
    pRatio = state.getRawParameterValue (ParamIDs::ratio);
    pAttack = state.getRawParameterValue (ParamIDs::attack);
    pRelease = state.getRawParameterValue (ParamIDs::release);
    pAutoRelease = state.getRawParameterValue (ParamIDs::autoRelease);
    pKnee = state.getRawParameterValue (ParamIDs::knee);
    pMakeup = state.getRawParameterValue (ParamIDs::makeup);
    pAutoMakeup = state.getRawParameterValue (ParamIDs::autoMakeup);
    pMix = state.getRawParameterValue (ParamIDs::mix);
    pDetector = state.getRawParameterValue (ParamIDs::detector);
    pScHpf = state.getRawParameterValue (ParamIDs::scHpf);
    pExternal = state.getRawParameterValue (ParamIDs::external);
    pCharacter = state.getRawParameterValue (ParamIDs::character);
    pLookahead = state.getRawParameterValue (ParamIDs::lookahead);
}

juce::AudioProcessorValueTreeState::ParameterLayout PressReadyProcessor::createLayout()
{
    using namespace juce;
    using P = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;

    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::threshold, 1 }, "Threshold",
                                     NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -18.0f,
                                     A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::ratio, 1 }, "Ratio",
                                     logRange (1.0f, infiniteRatio), 4.0f,
                                     A().withStringFromValueFunction (ratioString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::attack, 1 }, "Attack",
                                     logRange (0.05f, 300.0f), 10.0f,
                                     A().withLabel ("ms").withStringFromValueFunction (msString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::release, 1 }, "Release",
                                     logRange (5.0f, 3000.0f), 120.0f,
                                     A().withLabel ("ms").withStringFromValueFunction (msString)));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::autoRelease, 1 }, "Auto Release", false));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::knee, 1 }, "Knee",
                                     NormalisableRange<float> (0.0f, 24.0f, 0.1f), 6.0f,
                                     A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::makeup, 1 }, "Makeup",
                                     NormalisableRange<float> (-12.0f, 24.0f, 0.1f), 0.0f,
                                     A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::autoMakeup, 1 }, "Auto Gain", false));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::mix, 1 }, "Mix",
                                     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
                                     A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::detector, 1 }, "Detector", Choices::detectors, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::scHpf, 1 }, "Sidechain HPF", Choices::scHpf, 0));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::external, 1 }, "External Sidechain", false));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::character, 1 }, "Character", Choices::characters, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::lookahead, 1 }, "Lookahead", Choices::lookahead, 0));
    return layout;
}

CompParams PressReadyProcessor::readParams() const
{
    CompParams p;
    p.thresholdDb = pThreshold->load();
    p.ratio = pRatio->load();
    p.attackMs = pAttack->load();
    p.releaseMs = pRelease->load();
    p.autoRelease = pAutoRelease->load() > 0.5f;
    p.kneeDb = pKnee->load();
    p.makeupDb = pMakeup->load();
    p.autoMakeup = pAutoMakeup->load() > 0.5f;
    p.mix = pMix->load() / 100.0f;
    p.detector = pDetector->load() > 0.5f ? Detector::rms : Detector::peak;
    p.sidechainHpfHz = Choices::scHpfValues[juce::jlimit (0, Choices::scHpf.size() - 1, juce::roundToInt (pScHpf->load()))];
    p.externalSidechain = pExternal->load() > 0.5f;
    p.character = (Character) juce::jlimit (0, Choices::characters.size() - 1, juce::roundToInt (pCharacter->load()));
    p.lookaheadMs = Choices::lookaheadValues[juce::jlimit (0, Choices::lookahead.size() - 1, juce::roundToInt (pLookahead->load()))];
    return p;
}

void PressReadyProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, juce::jmax (32, samplesPerBlock), getMainBusNumInputChannels());
    engine.setParams (readParams(), true);
    reportedLatency = engine.getLatencySamples();
    setLatencySamples (reportedLatency);
}

void PressReadyProcessor::reset()
{
    engine.reset();
}

bool PressReadyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet() != out)
        return false;
    const auto sc = layouts.getChannelSet (true, 1);
    // The sidechain may be absent, mono or stereo regardless of the main bus (auval tries
    // every combination; a mono sidechain is mapped onto both channels in processBlock).
    return sc.isDisabled() || sc == juce::AudioChannelSet::mono() || sc == juce::AudioChannelSet::stereo();
}

void PressReadyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    auto main = getBusBuffer (buffer, true, 0);
    const int numIn = main.getNumChannels();

    // Pointers into the host's sidechain channels (no copy, no allocation on the audio thread).
    const float* scPtrs[rommuser::pressready::CompEngine::maxChannels] = {};
    const float* const* sc = nullptr;
    if (getBusCount (true) > 1 && getBus (true, 1)->isEnabled())
    {
        auto scBuffer = getBusBuffer (buffer, true, 1);
        if (scBuffer.getNumChannels() > 0 && numIn <= rommuser::pressready::CompEngine::maxChannels)
        {
            for (int c = 0; c < numIn; ++c)
                scPtrs[c] = scBuffer.getReadPointer (std::min (c, scBuffer.getNumChannels() - 1));
            sc = scPtrs;
        }
    }

    engine.setParams (readParams());

    const int latency = engine.getLatencySamples();
    if (latency != reportedLatency)
    {
        reportedLatency = latency;
        setLatencySamples (latency);
    }

    const auto m = engine.process (main.getArrayOfWritePointers(), sc, numIn, n);

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
    levelDb.store (juce::Decibels::gainToDecibels (m.inputPeak, -100.0f));
}

juce::AudioProcessorEditor* PressReadyProcessor::createEditor()
{
    return new PressReadyEditor (*this);
}

void PressReadyProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void PressReadyProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PressReadyProcessor();
}
