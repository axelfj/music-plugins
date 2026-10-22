// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rommuser::gateready;

namespace
{
juce::String dbString (float v, int) { return juce::String (v, 1) + " dB"; }
juce::String msString (float v, int) { return juce::String (v, v < 1.0f ? 2 : v < 10.0f ? 1 : 0) + " ms"; }
juce::String ratioString (float v, int)
{
    return v >= infiniteRatio ? juce::String ("Gate") : "1:" + juce::String (v, v < 10.0f ? 1 : 0);
}

juce::NormalisableRange<float> logRange (float lo, float hi)
{
    return juce::NormalisableRange<float> (lo, hi,
        [] (float s, float e, float n) { return s * std::pow (e / s, n); },
        [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); });
}
} // namespace

GateReadyProcessor::GateReadyProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                          .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      state (*this, nullptr, "GateReady", createLayout())
{
    pThreshold = state.getRawParameterValue (ParamIDs::threshold);
    pRatio = state.getRawParameterValue (ParamIDs::ratio);
    pAttack = state.getRawParameterValue (ParamIDs::attack);
    pHold = state.getRawParameterValue (ParamIDs::hold);
    pRelease = state.getRawParameterValue (ParamIDs::release);
    pRange = state.getRawParameterValue (ParamIDs::range);
    pKnee = state.getRawParameterValue (ParamIDs::knee);
    pHysteresis = state.getRawParameterValue (ParamIDs::hysteresis);
    pLookahead = state.getRawParameterValue (ParamIDs::lookahead);
    pScHpf = state.getRawParameterValue (ParamIDs::scHpf);
    pScLpf = state.getRawParameterValue (ParamIDs::scLpf);
    pExternal = state.getRawParameterValue (ParamIDs::external);
    pListen = state.getRawParameterValue (ParamIDs::listen);
}

juce::AudioProcessorValueTreeState::ParameterLayout GateReadyProcessor::createLayout()
{
    using namespace juce;
    using P = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;

    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::threshold, 1 }, "Threshold",
                                     NormalisableRange<float> (-80.0f, 0.0f, 0.1f), -40.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::ratio, 1 }, "Ratio",
                                     logRange (1.0f, infiniteRatio), infiniteRatio, A().withStringFromValueFunction (ratioString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::attack, 1 }, "Attack",
                                     logRange (0.1f, 100.0f), 1.0f, A().withLabel ("ms").withStringFromValueFunction (msString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::hold, 1 }, "Hold",
                                     NormalisableRange<float> (0.0f, 1000.0f, 1.0f, 0.5f), 50.0f, A().withLabel ("ms").withStringFromValueFunction (msString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::release, 1 }, "Release",
                                     logRange (1.0f, 3000.0f), 100.0f, A().withLabel ("ms").withStringFromValueFunction (msString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::range, 1 }, "Range",
                                     NormalisableRange<float> (-100.0f, 0.0f, 0.1f), -80.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::knee, 1 }, "Knee",
                                     NormalisableRange<float> (0.0f, 24.0f, 0.1f), 0.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::hysteresis, 1 }, "Hysteresis",
                                     NormalisableRange<float> (0.0f, 12.0f, 0.1f), 3.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::lookahead, 1 }, "Lookahead", Choices::lookahead, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::scHpf, 1 }, "Sidechain HPF", Choices::scHpf, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::scLpf, 1 }, "Sidechain LPF", Choices::scLpf, 0));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::external, 1 }, "External Sidechain", false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::listen, 1 }, "Listen", false));
    return layout;
}

GateParams GateReadyProcessor::readParams() const
{
    GateParams p;
    p.thresholdDb = pThreshold->load();
    p.ratio = pRatio->load();
    p.attackMs = pAttack->load();
    p.holdMs = pHold->load();
    p.releaseMs = pRelease->load();
    p.rangeDb = pRange->load();
    p.kneeDb = pKnee->load();
    p.hysteresisDb = pHysteresis->load();
    p.lookaheadMs = Choices::lookaheadValues[juce::jlimit (0, Choices::lookahead.size() - 1, juce::roundToInt (pLookahead->load()))];
    p.sidechainHpfHz = Choices::scHpfValues[juce::jlimit (0, Choices::scHpf.size() - 1, juce::roundToInt (pScHpf->load()))];
    p.sidechainLpfHz = Choices::scLpfValues[juce::jlimit (0, Choices::scLpf.size() - 1, juce::roundToInt (pScLpf->load()))];
    p.externalSidechain = pExternal->load() > 0.5f;
    p.listen = pListen->load() > 0.5f;
    return p;
}

void GateReadyProcessor::applyPreset (const GatePreset& preset)
{
    auto setValue = [this] (const char* id, float value)
    {
        if (auto* param = state.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };
    setValue (ParamIDs::threshold, preset.thresholdDb);
    setValue (ParamIDs::ratio, infiniteRatio);
    setValue (ParamIDs::attack, preset.attackMs);
    setValue (ParamIDs::hold, preset.holdMs);
    setValue (ParamIDs::release, preset.releaseMs);
    setValue (ParamIDs::range, preset.rangeDb);
    setValue (ParamIDs::scHpf, (float) preset.scHpf);
    setValue (ParamIDs::scLpf, (float) preset.scLpf);
    setValue (ParamIDs::listen, 0.0f);
}

void GateReadyProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate, getMainBusNumInputChannels());
    engine.setParams (readParams(), true);
    reportedLatency = engine.getLatencySamples();
    setLatencySamples (reportedLatency);
}

void GateReadyProcessor::reset()
{
    engine.reset();
}

bool GateReadyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void GateReadyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    auto main = getBusBuffer (buffer, true, 0);
    const int numIn = main.getNumChannels();

    const float* scPtrs[GateEngine::maxChannels] = {};
    const float* const* sc = nullptr;
    if (getBusCount (true) > 1 && getBus (true, 1)->isEnabled())
    {
        auto scBuffer = getBusBuffer (buffer, true, 1);
        if (scBuffer.getNumChannels() > 0 && numIn <= GateEngine::maxChannels)
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
    levelDb.store (m.levelDb);
    gateOpen.store (m.open);
}

juce::AudioProcessorEditor* GateReadyProcessor::createEditor()
{
    return new GateReadyEditor (*this);
}

void GateReadyProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void GateReadyProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GateReadyProcessor();
}
