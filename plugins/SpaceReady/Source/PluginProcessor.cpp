// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rommuser::spaceready;

namespace
{
juce::String dbString (float v, int) { return juce::String (v, 1) + " dB"; }
juce::String percentString (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; }
juce::String msString (float v, int) { return juce::String (v, v < 10.0f ? 1 : 0) + " ms"; }
juce::String secondsString (float v, int) { return juce::String (v, v < 10.0f ? 2 : 1) + " s"; }
juce::String sizeString (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; }
juce::String hzOffString (float v, int, bool lowCut)
{
    if (lowCut ? v <= 20.5f : v >= 19999.0f) return "Off";
    return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz";
}

juce::NormalisableRange<float> logRange (float lo, float hi)
{
    return juce::NormalisableRange<float> (lo, hi,
        [] (float s, float e, float n) { return s * std::pow (e / s, n); },
        [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); });
}
} // namespace

SpaceReadyProcessor::SpaceReadyProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "SpaceReady", createLayout())
{
    pMode = state.getRawParameterValue (ParamIDs::mode);
    pSize = state.getRawParameterValue (ParamIDs::size);
    pDecay = state.getRawParameterValue (ParamIDs::decay);
    pPreDelay = state.getRawParameterValue (ParamIDs::preDelay);
    pDamping = state.getRawParameterValue (ParamIDs::damping);
    pDiffusion = state.getRawParameterValue (ParamIDs::diffusion);
    pModulation = state.getRawParameterValue (ParamIDs::modulation);
    pWidth = state.getRawParameterValue (ParamIDs::width);
    pLowCut = state.getRawParameterValue (ParamIDs::lowCut);
    pHighCut = state.getRawParameterValue (ParamIDs::highCut);
    pDuck = state.getRawParameterValue (ParamIDs::duck);
    pMix = state.getRawParameterValue (ParamIDs::mix);
    pOutput = state.getRawParameterValue (ParamIDs::output);
    pFreeze = state.getRawParameterValue (ParamIDs::freeze);
}

juce::AudioProcessorValueTreeState::ParameterLayout SpaceReadyProcessor::createLayout()
{
    using namespace juce;
    using P = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;

    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::mode, 1 }, "Mode", Choices::modes, 2));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::size, 1 }, "Size",
                                     NormalisableRange<float> (0.5f, 2.0f, 0.01f), 1.0f, A().withLabel ("%").withStringFromValueFunction (sizeString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::decay, 1 }, "Decay",
                                     logRange (0.1f, 30.0f), 2.0f, A().withLabel ("s").withStringFromValueFunction (secondsString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::preDelay, 1 }, "Pre-delay",
                                     NormalisableRange<float> (0.0f, 200.0f, 0.1f), 10.0f, A().withLabel ("ms").withStringFromValueFunction (msString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::damping, 1 }, "Damping",
                                     logRange (1000.0f, 20000.0f), 6000.0f,
                                     A().withLabel ("Hz").withStringFromValueFunction ([] (float v, int n) { return hzOffString (v, n, false); })));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::diffusion, 1 }, "Diffusion",
                                     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 60.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::modulation, 1 }, "Modulation",
                                     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::width, 1 }, "Width",
                                     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::lowCut, 1 }, "Low Cut",
                                     logRange (20.0f, 1000.0f), 20.0f,
                                     A().withLabel ("Hz").withStringFromValueFunction ([] (float v, int n) { return hzOffString (v, n, true); })));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::highCut, 1 }, "High Cut",
                                     logRange (1000.0f, 20000.0f), 20000.0f,
                                     A().withLabel ("Hz").withStringFromValueFunction ([] (float v, int n) { return hzOffString (v, n, false); })));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::duck, 1 }, "Ducking",
                                     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::mix, 1 }, "Mix",
                                     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 30.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::output, 1 }, "Output",
                                     NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::freeze, 1 }, "Freeze", false));
    return layout;
}

SpaceParams SpaceReadyProcessor::readParams() const
{
    SpaceParams p;
    p.mode = (Mode) juce::jlimit (0, 3, juce::roundToInt (pMode->load()));
    p.size = pSize->load();
    p.decaySeconds = pDecay->load();
    p.preDelayMs = pPreDelay->load();
    p.dampingHz = pDamping->load();
    p.diffusion = pDiffusion->load() / 100.0f;
    p.modulation = pModulation->load() / 100.0f;
    p.width = pWidth->load() / 100.0f;
    const float lc = pLowCut->load(), hc = pHighCut->load();
    p.lowCutHz = lc <= 20.5f ? 0.0f : lc;
    p.highCutHz = hc >= 19999.0f ? 0.0f : hc;
    p.duck = pDuck->load() / 100.0f;
    p.mix = pMix->load() / 100.0f;
    p.outputDb = pOutput->load();
    p.freeze = pFreeze->load() > 0.5f;
    return p;
}

void SpaceReadyProcessor::applyPreset (const SpacePreset& preset)
{
    auto setValue = [this] (const char* id, float value)
    {
        if (auto* param = state.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };
    setValue (ParamIDs::mode, (float) preset.mode);
    setValue (ParamIDs::size, preset.size);
    setValue (ParamIDs::decay, preset.decay);
    setValue (ParamIDs::preDelay, preset.preDelayMs);
    setValue (ParamIDs::damping, preset.dampingHz);
    setValue (ParamIDs::diffusion, preset.diffusion * 100.0f);
    setValue (ParamIDs::modulation, preset.modulation * 100.0f);
    setValue (ParamIDs::width, preset.width * 100.0f);
    setValue (ParamIDs::lowCut, preset.lowCutHz);
    setValue (ParamIDs::highCut, preset.highCutHz);
    setValue (ParamIDs::duck, preset.duck * 100.0f);
    setValue (ParamIDs::mix, preset.mix * 100.0f);
    setValue (ParamIDs::freeze, 0.0f);
}

void SpaceReadyProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate, getMainBusNumInputChannels());
    engine.setParams (readParams());
}

void SpaceReadyProcessor::reset()
{
    engine.reset();
}

bool SpaceReadyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void SpaceReadyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels();

    engine.setParams (readParams());
    const auto m = engine.process (buffer.getArrayOfWritePointers(), numIn, n);

    for (int ch = numIn; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);

    float prev = outputPeak.load();
    while (m.outputPeak > prev && ! outputPeak.compare_exchange_weak (prev, m.outputPeak)) {}
    wetRms.store (m.wetRms);
    duckGain.store (m.duckGain);
}

juce::AudioProcessorEditor* SpaceReadyProcessor::createEditor()
{
    return new SpaceReadyEditor (*this);
}

void SpaceReadyProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void SpaceReadyProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpaceReadyProcessor();
}
