// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rommuser::wideready;

namespace
{
juce::String dbString (float v, int) { return juce::String (v, 1) + " dB"; }
juce::String percentString (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; }
juce::String msString (float v, int) { return juce::String (v, v < 10.0f ? 2 : 1) + " ms"; }
juce::String rateString (float v, int) { return juce::String (v, v < 1.0f ? 2 : 1) + " Hz"; }
juce::String centsString (float v, int) { return juce::String (v, 1) + " ct"; }

juce::NormalisableRange<float> logRange (float lo, float hi)
{
    return juce::NormalisableRange<float> (lo, hi,
        [] (float s, float e, float n) { return s * std::pow (e / s, n); },
        [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); });
}
} // namespace

WideReadyProcessor::WideReadyProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "WideReady", createLayout())
{
    pMode = state.getRawParameterValue (ParamIDs::mode);
    pVoices = state.getRawParameterValue (ParamIDs::voices);
    pEnsemble = state.getRawParameterValue (ParamIDs::ensemble);
    pRate = state.getRawParameterValue (ParamIDs::rate);
    pDepth = state.getRawParameterValue (ParamIDs::depth);
    pDelay = state.getRawParameterValue (ParamIDs::delay);
    pFeedback = state.getRawParameterValue (ParamIDs::feedback);
    pThroughZero = state.getRawParameterValue (ParamIDs::throughZero);
    pInvert = state.getRawParameterValue (ParamIDs::invert);
    pSpread = state.getRawParameterValue (ParamIDs::spread);
    pCents = state.getRawParameterValue (ParamIDs::cents);
    pMix = state.getRawParameterValue (ParamIDs::mix);
    pOutput = state.getRawParameterValue (ParamIDs::output);
}

juce::AudioProcessorValueTreeState::ParameterLayout WideReadyProcessor::createLayout()
{
    using namespace juce;
    using P = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;

    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::mode, 1 }, "Mode", Choices::modes, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::voices, 1 }, "Voices", Choices::voices, 1));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::ensemble, 1 }, "Ensemble", false));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::rate, 1 }, "Rate",
                                     logRange (0.05f, 10.0f), 0.4f, A().withLabel ("Hz").withStringFromValueFunction (rateString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::depth, 1 }, "Depth",
                                     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::delay, 1 }, "Delay",
                                     logRange (0.1f, 40.0f), 15.0f, A().withLabel ("ms").withStringFromValueFunction (msString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::feedback, 1 }, "Feedback",
                                     NormalisableRange<float> (-95.0f, 95.0f, 0.1f), 0.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::throughZero, 1 }, "Through Zero", false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::invert, 1 }, "Invert", false));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::spread, 1 }, "Stereo Spread",
                                     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::cents, 1 }, "Detune",
                                     NormalisableRange<float> (0.0f, 50.0f, 0.1f), 12.0f, A().withLabel ("ct").withStringFromValueFunction (centsString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::mix, 1 }, "Mix",
                                     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::output, 1 }, "Output",
                                     NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    return layout;
}

WideParams WideReadyProcessor::readParams() const
{
    WideParams p;
    p.mode = (Mode) juce::jlimit (0, 2, juce::roundToInt (pMode->load()));
    p.voices = juce::jlimit (0, 2, juce::roundToInt (pVoices->load())) + 1;
    p.ensemble = pEnsemble->load() > 0.5f;
    p.rateHz = pRate->load();
    p.depth = pDepth->load() / 100.0f;
    p.delayMs = pDelay->load();
    p.feedback = pFeedback->load() / 100.0f;
    p.throughZero = pThroughZero->load() > 0.5f;
    p.invert = pInvert->load() > 0.5f;
    p.spread = pSpread->load() / 100.0f;
    p.cents = pCents->load();
    p.mix = pMix->load() / 100.0f;
    p.outputDb = pOutput->load();
    return p;
}

void WideReadyProcessor::applyPreset (const WidePreset& preset)
{
    auto setValue = [this] (const char* id, float value)
    {
        if (auto* param = state.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };
    setValue (ParamIDs::mode, (float) preset.mode);
    setValue (ParamIDs::voices, (float) (preset.voices - 1));
    setValue (ParamIDs::ensemble, preset.ensemble ? 1.0f : 0.0f);
    setValue (ParamIDs::rate, preset.rateHz);
    setValue (ParamIDs::depth, preset.depth * 100.0f);
    setValue (ParamIDs::delay, preset.delayMs);
    setValue (ParamIDs::feedback, preset.feedback * 100.0f);
    setValue (ParamIDs::throughZero, preset.throughZero ? 1.0f : 0.0f);
    setValue (ParamIDs::invert, preset.invert ? 1.0f : 0.0f);
    setValue (ParamIDs::spread, preset.spread * 100.0f);
    setValue (ParamIDs::cents, preset.cents);
    setValue (ParamIDs::mix, preset.mix * 100.0f);
}

void WideReadyProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate, getMainBusNumInputChannels());
    engine.setParams (readParams());
}

void WideReadyProcessor::reset()
{
    engine.reset();
}

bool WideReadyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void WideReadyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
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
    activeVoices.store (m.activeVoices);
    for (int ch = 0; ch < 2; ++ch)
        for (int v = 0; v < maxVoices; ++v)
            voiceDelayMs[(size_t) ch][(size_t) v].store (m.voiceDelayMs[std::min (ch, numIn - 1)][v]);
}

juce::AudioProcessorEditor* WideReadyProcessor::createEditor()
{
    return rommuser::ui::createScalableEditor<WideReadyEditor> (*this);
}

void WideReadyProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void WideReadyProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WideReadyProcessor();
}
