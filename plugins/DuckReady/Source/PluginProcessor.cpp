// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rommuser::duckready;

namespace
{
juce::String percentString (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; }
juce::String percentString1 (float v, int) { return juce::String (v, 1) + " %"; }
juce::String dbString (float v, int) { return juce::String (v, 1) + " dB"; }
juce::String hzString (float v, int) { return juce::String (juce::roundToInt (v)) + " Hz"; }
juce::String curveString (float v, int)
{
    if (std::fabs (v) < 0.05f)
        return "0";
    return juce::String (v, 2);
}

juce::NormalisableRange<float> logRange (float lo, float hi)
{
    return juce::NormalisableRange<float> (lo, hi,
        [] (float s, float e, float n) { return s * std::pow (e / s, n); },
        [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); });
}
} // namespace

DuckReadyProcessor::DuckReadyProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                          .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      state (*this, nullptr, "DuckReady", createLayout())
{
    pDepth = state.getRawParameterValue (ParamIDs::depth);
    pAttack = state.getRawParameterValue (ParamIDs::attack);
    pHold = state.getRawParameterValue (ParamIDs::hold);
    pRelease = state.getRawParameterValue (ParamIDs::release);
    pCurve = state.getRawParameterValue (ParamIDs::curve);
    pOffset = state.getRawParameterValue (ParamIDs::offset);
    pRate = state.getRawParameterValue (ParamIDs::rate);
    pTrigger = state.getRawParameterValue (ParamIDs::trigger);
    pScThreshold = state.getRawParameterValue (ParamIDs::scThreshold);
    pBand = state.getRawParameterValue (ParamIDs::band);
    pCrossover = state.getRawParameterValue (ParamIDs::crossover);
}

juce::AudioProcessorValueTreeState::ParameterLayout DuckReadyProcessor::createLayout()
{
    using namespace juce;
    using P = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;

    const auto& d = curvePresets[0];

    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::depth, 1 }, "Depth",
                                     NormalisableRange<float> (0.0f, 100.0f, 0.1f), d.depth * 100.0f,
                                     A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::attack, 1 }, "Attack",
                                     logRange (0.5f, 50.0f), d.attack * 100.0f,
                                     A().withLabel ("%").withStringFromValueFunction (percentString1)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::hold, 1 }, "Hold",
                                     NormalisableRange<float> (0.0f, 90.0f, 0.1f), d.hold * 100.0f,
                                     A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::release, 1 }, "Release",
                                     NormalisableRange<float> (1.0f, 100.0f, 0.1f), d.release * 100.0f,
                                     A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::curve, 1 }, "Curve",
                                     NormalisableRange<float> (-1.0f, 1.0f, 0.01f), d.curve,
                                     A().withStringFromValueFunction (curveString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::offset, 1 }, "Offset",
                                     NormalisableRange<float> (-50.0f, 50.0f, 0.1f), 0.0f,
                                     A().withLabel ("%").withStringFromValueFunction (percentString1)));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::rate, 1 }, "Rate", Choices::rates, 2));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::trigger, 1 }, "Trigger", Choices::triggers, 0));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::scThreshold, 1 }, "Sidechain Threshold",
                                     NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -24.0f,
                                     A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::band, 1 }, "Band", Choices::bands, 0));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::crossover, 1 }, "Crossover",
                                     logRange (40.0f, 2000.0f), 200.0f,
                                     A().withLabel ("Hz").withStringFromValueFunction (hzString)));
    return layout;
}

DuckParams DuckReadyProcessor::readParams() const
{
    DuckParams p;
    p.depth = pDepth->load() / 100.0f;
    p.attack = pAttack->load() / 100.0f;
    p.hold = pHold->load() / 100.0f;
    p.release = pRelease->load() / 100.0f;
    p.curve = pCurve->load();
    p.offset = pOffset->load() / 100.0f;
    p.beatsPerCycle = Choices::rateBeats[juce::jlimit (0, Choices::rates.size() - 1, juce::roundToInt (pRate->load()))];
    p.trigger = (Trigger) juce::jlimit (0, Choices::triggers.size() - 1, juce::roundToInt (pTrigger->load()));
    p.sidechainThresholdDb = pScThreshold->load();
    p.band = (Band) juce::jlimit (0, Choices::bands.size() - 1, juce::roundToInt (pBand->load()));
    p.crossoverHz = pCrossover->load();
    return p;
}

void DuckReadyProcessor::applyPreset (const CurvePreset& preset)
{
    struct Item { const char* id; float value; };
    const Item items[] = {
        { ParamIDs::depth, preset.depth * 100.0f },
        { ParamIDs::attack, preset.attack * 100.0f },
        { ParamIDs::hold, preset.hold * 100.0f },
        { ParamIDs::release, preset.release * 100.0f },
        { ParamIDs::curve, preset.curve },
    };
    for (const auto& it : items)
        if (auto* param = state.getParameter (it.id))
            param->setValueNotifyingHost (param->convertTo0to1 (it.value));
}

void DuckReadyProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate, getMainBusNumInputChannels());
    engine.setParams (readParams(), true);
}

void DuckReadyProcessor::reset()
{
    engine.reset();
}

bool DuckReadyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void DuckReadyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    auto main = getBusBuffer (buffer, true, 0);
    const int numIn = main.getNumChannels();

    const float* scPtrs[DuckEngine::maxChannels] = {};
    const float* const* sc = nullptr;
    if (getBusCount (true) > 1 && getBus (true, 1)->isEnabled())
    {
        auto scBuffer = getBusBuffer (buffer, true, 1);
        if (scBuffer.getNumChannels() > 0 && numIn <= DuckEngine::maxChannels)
        {
            for (int c = 0; c < numIn; ++c)
                scPtrs[c] = scBuffer.getReadPointer (std::min (c, scBuffer.getNumChannels() - 1));
            sc = scPtrs;
        }
    }

    int numMidi = 0;
    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn() && numMidi < maxMidiPerBlock)
            midiOffsets[numMidi++] = juce::jlimit (0, n - 1, metadata.samplePosition);
    }

    Transport t;
    if (auto* head = getPlayHead())
    {
        if (const auto pos = head->getPosition())
        {
            t.playing = pos->getIsPlaying();
            if (const auto bpm = pos->getBpm())
                t.bpm = *bpm;
            if (const auto ppq = pos->getPpqPosition())
            {
                t.ppqPosition = *ppq;
                t.valid = true;
            }
        }
    }
    hostPlaying.store (t.valid && t.playing);

    engine.setParams (readParams());
    engine.process (main.getArrayOfWritePointers(), sc, numIn, n, t, midiOffsets, numMidi);

    for (int ch = numIn; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);

    currentPhase.store (engine.getCurrentPhase());
    currentGain.store (engine.getCurrentGain());
}

juce::AudioProcessorEditor* DuckReadyProcessor::createEditor()
{
    return new DuckReadyEditor (*this);
}

void DuckReadyProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void DuckReadyProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DuckReadyProcessor();
}
