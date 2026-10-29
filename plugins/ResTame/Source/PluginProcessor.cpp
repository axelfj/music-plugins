// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rommuser::restame;

namespace
{
juce::String dbString (float v, int) { return juce::String (v, 1) + " dB"; }
juce::String percentString (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; }
juce::String msString (float v, int) { return juce::String (v, v < 10.0f ? 1 : 0) + " ms"; }
juce::String hzString (float v, int)
{
    return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz";
}

juce::NormalisableRange<float> logRange (float lo, float hi)
{
    return juce::NormalisableRange<float> (lo, hi,
        [] (float s, float e, float n) { return s * std::pow (e / s, n); },
        [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); });
}
} // namespace

ResTameProcessor::ResTameProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                          .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      state (*this, nullptr, "ResTame", createLayout())
{
    pDepth = state.getRawParameterValue (ParamIDs::depth);
    pSharpness = state.getRawParameterValue (ParamIDs::sharpness);
    pSelectivity = state.getRawParameterValue (ParamIDs::selectivity);
    pThreshold = state.getRawParameterValue (ParamIDs::threshold);
    pAttack = state.getRawParameterValue (ParamIDs::attack);
    pRelease = state.getRawParameterValue (ParamIDs::release);
    pLow = state.getRawParameterValue (ParamIDs::low);
    pHigh = state.getRawParameterValue (ParamIDs::high);
    pPlacement = state.getRawParameterValue (ParamIDs::placement);
    pExternal = state.getRawParameterValue (ParamIDs::external);
    pDelta = state.getRawParameterValue (ParamIDs::delta);
}

juce::AudioProcessorValueTreeState::ParameterLayout ResTameProcessor::createLayout()
{
    using namespace juce;
    using P = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;

    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::depth, 1 }, "Depth",
                                     NormalisableRange<float> (0.0f, 24.0f, 0.1f), 8.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::sharpness, 1 }, "Sharpness",
                                     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 70.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::selectivity, 1 }, "Selectivity",
                                     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::threshold, 1 }, "Threshold",
                                     NormalisableRange<float> (2.0f, 12.0f, 0.1f), 6.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::attack, 1 }, "Attack",
                                     logRange (1.0f, 200.0f), 20.0f, A().withLabel ("ms").withStringFromValueFunction (msString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::release, 1 }, "Release",
                                     logRange (20.0f, 2000.0f), 200.0f, A().withLabel ("ms").withStringFromValueFunction (msString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::low, 1 }, "Low",
                                     logRange (20.0f, 2000.0f), 80.0f, A().withLabel ("Hz").withStringFromValueFunction (hzString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::high, 1 }, "High",
                                     logRange (1000.0f, 20000.0f), 16000.0f, A().withLabel ("Hz").withStringFromValueFunction (hzString)));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::placement, 1 }, "Placement", Choices::placements, 0));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::external, 1 }, "External Sidechain", false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::delta, 1 }, "Delta", false));
    return layout;
}

TameParams ResTameProcessor::readParams() const
{
    TameParams p;
    p.depthDb = pDepth->load();
    p.sharpness = pSharpness->load() / 100.0f;
    p.selectivity = pSelectivity->load() / 100.0f;
    p.thresholdDb = pThreshold->load();
    p.attackMs = pAttack->load();
    p.releaseMs = pRelease->load();
    p.lowHz = pLow->load();
    p.highHz = std::max (pHigh->load(), p.lowHz + 10.0f);
    p.placement = (Placement) juce::jlimit (0, 2, juce::roundToInt (pPlacement->load()));
    p.externalSidechain = pExternal->load() > 0.5f;
    p.delta = pDelta->load() > 0.5f;
    return p;
}

void ResTameProcessor::applyPreset (const TamePreset& preset)
{
    auto setValue = [this] (const char* id, float value)
    {
        if (auto* param = state.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };
    setValue (ParamIDs::depth, preset.depthDb);
    setValue (ParamIDs::sharpness, preset.sharpness * 100.0f);
    setValue (ParamIDs::selectivity, preset.selectivity * 100.0f);
    setValue (ParamIDs::threshold, preset.thresholdDb);
    setValue (ParamIDs::attack, preset.attackMs);
    setValue (ParamIDs::release, preset.releaseMs);
    setValue (ParamIDs::low, preset.lowHz);
    setValue (ParamIDs::high, preset.highHz);
    setValue (ParamIDs::placement, (float) preset.placement);
    setValue (ParamIDs::external, preset.external ? 1.0f : 0.0f);
    setValue (ParamIDs::delta, 0.0f);
}

void ResTameProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate, getMainBusNumInputChannels());
    engine.setParams (readParams());
    setLatencySamples (engine.getLatencySamples());
}

void ResTameProcessor::reset()
{
    engine.reset();
}

bool ResTameProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet() != out)
        return false;
    const auto sc = layouts.getChannelSet (true, 1);
    return sc.isDisabled() || sc == juce::AudioChannelSet::mono() || sc == juce::AudioChannelSet::stereo();
}

void ResTameProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    auto main = getBusBuffer (buffer, true, 0);
    const int numIn = main.getNumChannels();

    const float* scPtrs[TameEngine::maxChannels] = { nullptr, nullptr };
    bool haveSidechain = false;
    if (getBusCount (true) > 1 && getBus (true, 1)->isEnabled())
    {
        auto scBuffer = getBusBuffer (buffer, true, 1);
        if (scBuffer.getNumChannels() > 0 && numIn <= TameEngine::maxChannels)
        {
            for (int c = 0; c < numIn; ++c)
                scPtrs[c] = scBuffer.getReadPointer (std::min (c, scBuffer.getNumChannels() - 1));
            haveSidechain = true;
        }
    }

    auto p = readParams();
    if (! haveSidechain) p.externalSidechain = false;
    engine.setParams (p);
    const auto m = engine.process (main.getArrayOfWritePointers(), haveSidechain ? scPtrs : nullptr, numIn, n);

    for (int ch = numIn; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);

    maxReductionDb.store (m.maxReductionDb);
    meanReductionDb.store (m.meanReductionDb);
    const double sr = getSampleRate();
    const int bins = engine.getNumBins();
    for (int i = 0; i < displayPoints; ++i)
    {
        const double f = 20.0 * std::pow (1000.0, (double) i / (displayPoints - 1));
        const int k = juce::jlimit (0, bins - 1, (int) std::lround (f / engine.getBinFrequency (1)));
        // Take the deepest cut among the bins that fall into this display point.
        const double fNext = 20.0 * std::pow (1000.0, (double) (i + 1) / (displayPoints - 1));
        const int kNext = juce::jlimit (k, bins - 1, (int) std::lround (fNext / engine.getBinFrequency (1)));
        float red = 0.0f;
        for (int b = k; b <= kNext; ++b) red = std::max (red, -engine.getBinGainDb (b));
        displayReduction[(size_t) i].store (red);
    }
    juce::ignoreUnused (sr);
}

juce::AudioProcessorEditor* ResTameProcessor::createEditor()
{
    return rommuser::ui::createScalableEditor<ResTameEditor> (*this);
}

void ResTameProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void ResTameProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ResTameProcessor();
}
