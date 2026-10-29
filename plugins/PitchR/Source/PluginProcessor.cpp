// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rommuser::pitchr;

namespace
{
juce::String dbString (float v, int) { return juce::String (v, 1) + " dB"; }
juce::String percentString (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; }
juce::String signedString (float v, int decimals, const char* unit)
{
    const int steps = juce::roundToInt (v * (decimals > 0 ? 10.0f : 1.0f));
    const auto text = decimals > 0 ? juce::String ((float) steps / 10.0f, 1) : juce::String (steps);
    return (steps > 0 ? "+" : "") + text + unit;
}
juce::String semiString (float v, int) { return signedString (v, 0, " st"); }
juce::String centsString (float v, int) { return signedString (v, 0, " c"); }
juce::String formantString (float v, int) { return signedString (v, 1, " st"); }
} // namespace

PitchRProcessor::PitchRProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "PitchR", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout PitchRProcessor::createLayout()
{
    using namespace juce;
    using P = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;

    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::mode, 1 }, "Mode", Choices::modes, 0));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::semitones, 1 }, "Pitch",
                                     NormalisableRange<float> (-24.0f, 24.0f, 1.0f), 0.0f, A().withLabel ("st").withStringFromValueFunction (semiString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::cents, 1 }, "Fine",
                                     NormalisableRange<float> (-100.0f, 100.0f, 1.0f), 0.0f, A().withLabel ("c").withStringFromValueFunction (centsString)));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::keepFormants, 1 }, "Keep Formants", false));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::formant, 1 }, "Formant",
                                     NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, A().withLabel ("st").withStringFromValueFunction (formantString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::mix, 1 }, "Mix",
                                     NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::output, 1 }, "Output",
                                     NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::original, 1 }, "Original (A/B)", false));
    return layout;
}

PitchParams PitchRProcessor::readParams() const
{
    PitchParams q;
    q.mode = (Mode) juce::jlimit (0, numModes - 1, juce::roundToInt (p (ParamIDs::mode)->load()));
    q.semitones = p (ParamIDs::semitones)->load();
    q.cents = p (ParamIDs::cents)->load();
    q.keepFormants = p (ParamIDs::keepFormants)->load() > 0.5f;
    q.formantSemis = p (ParamIDs::formant)->load();
    q.mix = p (ParamIDs::mix)->load() * 0.01f;
    q.outputDb = p (ParamIDs::output)->load();
    q.original = p (ParamIDs::original)->load() > 0.5f;
    return q;
}

void PitchRProcessor::setParameter (const char* id, float value)
{
    if (auto* param = state.getParameter (id))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 (value));
        param->endChangeGesture();
    }
}

float PitchRProcessor::latencyMsFor (Mode mode) const
{
    const double sr = currentSampleRate.load();
    return (float) ((double) (1 << PitchEngine::fftOrderFor (mode, sr)) / sr * 1000.0);
}

void PitchRProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate.store (sampleRate);
    engine.prepare (sampleRate, std::max (16, std::min (samplesPerBlock, 8192)), getMainBusNumInputChannels());
    engine.setParams (readParams(), true);
    reportedLatency = engine.getLatencySamples();
    setLatencySamples (reportedLatency);
}

void PitchRProcessor::reset()
{
    engine.reset();
}

bool PitchRProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void PitchRProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int numIn = std::min (getMainBusNumInputChannels(), maxChannels);

    engine.setParams (readParams());

    inSpectrum.push (buffer.getArrayOfReadPointers(), numIn, n);
    engine.process (buffer.getArrayOfWritePointers(), numIn, n);
    outSpectrum.push (buffer.getArrayOfReadPointers(), numIn, n);

    // A mode change swaps the FFT size once the fade-out is done: tell the host the new delay.
    if (engine.getLatencySamples() != reportedLatency)
    {
        reportedLatency = engine.getLatencySamples();
        setLatencySamples (reportedLatency);
    }

    for (int ch = numIn; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);
}

juce::AudioProcessorEditor* PitchRProcessor::createEditor()
{
    return rommuser::ui::createScalableEditor<PitchREditor> (*this);
}

void PitchRProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void PitchRProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PitchRProcessor();
}
