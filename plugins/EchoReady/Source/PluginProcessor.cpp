// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rommuser::echoready;

namespace
{
juce::String dbString (float v, int) { return juce::String (v, 1) + " dB"; }
juce::String percentString (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; }
juce::String msString (float v, int) { return juce::String (v, v < 10.0f ? 1 : 0) + " ms"; }
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

EchoReadyProcessor::EchoReadyProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "EchoReady", createLayout())
{
    pSync = state.getRawParameterValue (ParamIDs::sync);
    pDivision = state.getRawParameterValue (ParamIDs::division);
    pTime = state.getRawParameterValue (ParamIDs::time);
    pMode = state.getRawParameterValue (ParamIDs::mode);
    pStyle = state.getRawParameterValue (ParamIDs::style);
    pTimeChange = state.getRawParameterValue (ParamIDs::timeChange);
    pFeedback = state.getRawParameterValue (ParamIDs::feedback);
    pLowCut = state.getRawParameterValue (ParamIDs::lowCut);
    pHighCut = state.getRawParameterValue (ParamIDs::highCut);
    pDiffusion = state.getRawParameterValue (ParamIDs::diffusion);
    pWow = state.getRawParameterValue (ParamIDs::wow);
    pDuck = state.getRawParameterValue (ParamIDs::duck);
    pMix = state.getRawParameterValue (ParamIDs::mix);
    pOutput = state.getRawParameterValue (ParamIDs::output);
}

juce::AudioProcessorValueTreeState::ParameterLayout EchoReadyProcessor::createLayout()
{
    using namespace juce;
    using P = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;

    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::sync, 1 }, "Sync", true));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::division, 1 }, "Division", Choices::divisions, 7));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::time, 1 }, "Time",
                                     logRange (1.0f, 2000.0f), 375.0f, A().withLabel ("ms").withStringFromValueFunction (msString)));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::mode, 1 }, "Mode", Choices::modes, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::style, 1 }, "Style", Choices::styles, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::timeChange, 1 }, "Time Change", Choices::timeChanges, 0));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::feedback, 1 }, "Feedback",
                                     NormalisableRange<float> (0.0f, 110.0f, 0.1f), 40.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::lowCut, 1 }, "Low Cut",
                                     logRange (20.0f, 2000.0f), 20.0f,
                                     A().withLabel ("Hz").withStringFromValueFunction ([] (float v, int n) { return hzOffString (v, n, true); })));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::highCut, 1 }, "High Cut",
                                     logRange (500.0f, 20000.0f), 20000.0f,
                                     A().withLabel ("Hz").withStringFromValueFunction ([] (float v, int n) { return hzOffString (v, n, false); })));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::diffusion, 1 }, "Diffusion",
                                     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::wow, 1 }, "Wow",
                                     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::duck, 1 }, "Ducking",
                                     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::mix, 1 }, "Mix",
                                     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 35.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::output, 1 }, "Output",
                                     NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    return layout;
}

EchoParams EchoReadyProcessor::readParams() const
{
    EchoParams p;
    p.sync = pSync->load() > 0.5f;
    p.beats = Choices::divisionBeats[juce::jlimit (0, Choices::divisions.size() - 1, juce::roundToInt (pDivision->load()))];
    p.timeMs = pTime->load();
    p.bpm = hostBpm.load();
    p.mode = (Mode) juce::jlimit (0, 1, juce::roundToInt (pMode->load()));
    p.style = (Style) juce::jlimit (0, 2, juce::roundToInt (pStyle->load()));
    p.timeChange = (TimeChange) juce::jlimit (0, 1, juce::roundToInt (pTimeChange->load()));
    p.feedback = pFeedback->load() / 100.0f;
    const float lc = pLowCut->load(), hc = pHighCut->load();
    p.lowCutHz = lc <= 20.5f ? 0.0f : lc;
    p.highCutHz = hc >= 19999.0f ? 0.0f : hc;
    p.diffusion = pDiffusion->load() / 100.0f;
    p.wow = pWow->load() / 100.0f;
    p.duck = pDuck->load() / 100.0f;
    p.mix = pMix->load() / 100.0f;
    p.outputDb = pOutput->load();
    return p;
}

void EchoReadyProcessor::applyPreset (const EchoPreset& preset)
{
    auto setValue = [this] (const char* id, float value)
    {
        if (auto* param = state.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };
    setValue (ParamIDs::sync, preset.sync ? 1.0f : 0.0f);
    setValue (ParamIDs::division, (float) preset.division);
    setValue (ParamIDs::time, preset.timeMs);
    setValue (ParamIDs::mode, (float) preset.mode);
    setValue (ParamIDs::style, (float) preset.style);
    setValue (ParamIDs::feedback, preset.feedback * 100.0f);
    setValue (ParamIDs::lowCut, preset.lowCutHz);
    setValue (ParamIDs::highCut, preset.highCutHz);
    setValue (ParamIDs::diffusion, preset.diffusion * 100.0f);
    setValue (ParamIDs::wow, preset.wow * 100.0f);
    setValue (ParamIDs::duck, preset.duck * 100.0f);
    setValue (ParamIDs::mix, preset.mix * 100.0f);
}

void EchoReadyProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate, getMainBusNumInputChannels());
    engine.setParams (readParams());
}

void EchoReadyProcessor::reset()
{
    engine.reset();
}

bool EchoReadyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void EchoReadyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels();

    if (auto* head = getPlayHead())
        if (const auto pos = head->getPosition())
            if (const auto bpm = pos->getBpm())
                if (*bpm > 20.0)
                    hostBpm.store ((float) *bpm);

    engine.setParams (readParams());
    const auto m = engine.process (buffer.getArrayOfWritePointers(), numIn, n);

    for (int ch = numIn; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);

    float prev = wetPeak.load();
    while (m.wetPeak > prev && ! wetPeak.compare_exchange_weak (prev, m.wetPeak)) {}
    duckGain.store (m.duckGain);
    currentDelayMs.store (engine.getCurrentDelaySamples() * 1000.0f / (float) getSampleRate());
}

juce::AudioProcessorEditor* EchoReadyProcessor::createEditor()
{
    return rommuser::ui::createScalableEditor<EchoReadyEditor> (*this);
}

void EchoReadyProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void EchoReadyProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EchoReadyProcessor();
}
