// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rommuser::punchshape;

namespace
{
juce::String dbString (float v, int) { return (v > 0.0f ? "+" : "") + juce::String (v, 1) + " dB"; }
juce::String msString (float v, int) { return juce::String (v, v < 10.0f ? 1 : 0) + " ms"; }

juce::NormalisableRange<float> logRange (float lo, float hi)
{
    return juce::NormalisableRange<float> (lo, hi,
        [] (float s, float e, float n) { return s * std::pow (e / s, n); },
        [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); });
}
} // namespace

PunchShapeProcessor::PunchShapeProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "PunchShape", createLayout())
{
    pAttack = state.getRawParameterValue (ParamIDs::attack);
    pSustain = state.getRawParameterValue (ParamIDs::sustain);
    pSpeed = state.getRawParameterValue (ParamIDs::speed);
    pTail = state.getRawParameterValue (ParamIDs::tail);
    pDetector = state.getRawParameterValue (ParamIDs::detector);
    pClip = state.getRawParameterValue (ParamIDs::clip);
    pOutput = state.getRawParameterValue (ParamIDs::output);
}

juce::AudioProcessorValueTreeState::ParameterLayout PunchShapeProcessor::createLayout()
{
    using namespace juce;
    using P = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;

    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::attack, 1 }, "Attack",
                                     NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::sustain, 1 }, "Sustain",
                                     NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::speed, 1 }, "Speed",
                                     logRange (5.0f, 100.0f), 20.0f, A().withLabel ("ms").withStringFromValueFunction (msString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::tail, 1 }, "Tail",
                                     logRange (50.0f, 2000.0f), 300.0f, A().withLabel ("ms").withStringFromValueFunction (msString)));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::detector, 1 }, "Detector", Choices::detector, 0));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::clip, 1 }, "Clip", false));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::output, 1 }, "Output",
                                     NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    return layout;
}

PunchParams PunchShapeProcessor::readParams() const
{
    PunchParams p;
    p.attackDb = pAttack->load();
    p.sustainDb = pSustain->load();
    p.speedMs = pSpeed->load();
    p.tailMs = pTail->load();
    p.detector = (Detector) juce::jlimit (0, Choices::detector.size() - 1, juce::roundToInt (pDetector->load()));
    p.clip = pClip->load() > 0.5f;
    p.outputDb = pOutput->load();
    return p;
}

void PunchShapeProcessor::applyPreset (const PunchPreset& preset)
{
    auto setValue = [this] (const char* id, float value)
    {
        if (auto* param = state.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };
    setValue (ParamIDs::attack, preset.attackDb);
    setValue (ParamIDs::sustain, preset.sustainDb);
    setValue (ParamIDs::speed, preset.speedMs);
    setValue (ParamIDs::tail, preset.tailMs);
    setValue (ParamIDs::detector, (float) preset.detector);
}

void PunchShapeProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate, getMainBusNumInputChannels());
    engine.setParams (readParams(), true);
}

void PunchShapeProcessor::reset()
{
    engine.reset();
}

bool PunchShapeProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void PunchShapeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int numIn = std::min (getMainBusNumInputChannels(), maxChannels);

    engine.setParams (readParams());
    const auto m = engine.process (buffer.getArrayOfWritePointers(), numIn, n);

    for (int ch = numIn; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);

    auto storeMax = [] (std::atomic<float>& a, float v)
    {
        float prev = a.load();
        while (v > prev && ! a.compare_exchange_weak (prev, v)) {}
    };
    auto storeMaxAbs = [] (std::atomic<float>& a, float v)
    {
        float prev = a.load();
        while (std::fabs (v) > std::fabs (prev) && ! a.compare_exchange_weak (prev, v)) {}
    };
    storeMax (inputPeak, m.inputPeak);
    storeMax (outputPeak, m.outputPeak);
    storeMaxAbs (attackGainDb, m.attackGainDb);
    storeMaxAbs (sustainGainDb, m.sustainGainDb);
    levelDb.store (m.levelDb);
    if (m.clipped)
        clipped.store (true);
}

juce::AudioProcessorEditor* PunchShapeProcessor::createEditor()
{
    return rommuser::ui::createScalableEditor<PunchShapeEditor> (*this);
}

void PunchShapeProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void PunchShapeProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PunchShapeProcessor();
}
