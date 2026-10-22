// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rommuser::shimmerready;

namespace
{
juce::String dbString (float v, int) { return juce::String (v, 1) + " dB"; }
juce::String msString (float v, int) { return juce::String (juce::roundToInt (v)) + " ms"; }
juce::String percentString (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; }
juce::String semiString (float v, int) { return (v > 0.0f ? "+" : "") + juce::String (juce::roundToInt (v)) + " st"; }
juce::String secondsString (float v, int) { return juce::String (v, 1) + " s"; }
juce::String hzOffString (float v, int)
{
    if (v < 1.0f) return "off";
    return v >= 1000.0f ? juce::String (v / 1000.0f, 1) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz";
}
juce::String highCutString (float v, int) { return v >= 19999.0f ? juce::String ("off") : hzOffString (v, 0); }
} // namespace

ShimmerReadyProcessor::ShimmerReadyProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "ShimmerReady", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout ShimmerReadyProcessor::createLayout()
{
    using namespace juce;
    using P = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;

    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::sync, 1 }, "Sync", true));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::division, 1 }, "Division", Choices::divisions, 5));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::time, 1 }, "Time",
                                     NormalisableRange<float> (10.0f, 4000.0f, 1.0f, 0.4f), 375.0f, A().withLabel ("ms").withStringFromValueFunction (msString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::pitch, 1 }, "Pitch",
                                     NormalisableRange<float> (-24.0f, 24.0f, 1.0f), 12.0f, A().withLabel ("st").withStringFromValueFunction (semiString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::grain, 1 }, "Grain",
                                     NormalisableRange<float> (20.0f, 200.0f, 1.0f), 80.0f, A().withLabel ("ms").withStringFromValueFunction (msString)));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::reverse, 1 }, "Reverse", false));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::feedback, 1 }, "Feedback",
                                     NormalisableRange<float> (0.0f, 110.0f, 1.0f), 50.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::lowCut, 1 }, "Low cut",
                                     NormalisableRange<float> (0.0f, 2000.0f, 1.0f, 0.4f), 0.0f, A().withLabel ("Hz").withStringFromValueFunction (hzOffString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::highCut, 1 }, "High cut",
                                     NormalisableRange<float> (500.0f, 20000.0f, 1.0f, 0.4f), 20000.0f, A().withLabel ("Hz").withStringFromValueFunction (highCutString)));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::freeze, 1 }, "Freeze", false));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::shimmer, 1 }, "Shimmer",
                                     NormalisableRange<float> (0.0f, 100.0f, 1.0f), 50.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::decay, 1 }, "Decay",
                                     NormalisableRange<float> (0.5f, 30.0f, 0.1f, 0.5f), 4.0f, A().withLabel ("s").withStringFromValueFunction (secondsString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::width, 1 }, "Width",
                                     NormalisableRange<float> (0.0f, 100.0f, 1.0f), 50.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::mix, 1 }, "Mix",
                                     NormalisableRange<float> (0.0f, 100.0f, 1.0f), 40.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::output, 1 }, "Output",
                                     NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    return layout;
}

ShimmerParams ShimmerReadyProcessor::readParams() const
{
    ShimmerParams q;
    q.sync = p (ParamIDs::sync)->load() > 0.5f;
    q.division = juce::jlimit (0, numDivisions - 1, juce::roundToInt (p (ParamIDs::division)->load()));
    q.timeMs = p (ParamIDs::time)->load();
    q.bpm = hostBpm.load();
    q.pitchSemis = p (ParamIDs::pitch)->load();
    q.grainMs = p (ParamIDs::grain)->load();
    q.reverse = p (ParamIDs::reverse)->load() > 0.5f;
    q.feedback = p (ParamIDs::feedback)->load() * 0.01f;
    q.lowCutHz = p (ParamIDs::lowCut)->load();
    q.highCutHz = p (ParamIDs::highCut)->load();
    q.freeze = p (ParamIDs::freeze)->load() > 0.5f;
    q.shimmer = p (ParamIDs::shimmer)->load() * 0.01f;
    q.decaySeconds = p (ParamIDs::decay)->load();
    q.width = p (ParamIDs::width)->load() * 0.01f;
    q.mix = p (ParamIDs::mix)->load() * 0.01f;
    q.outputDb = p (ParamIDs::output)->load();
    return q;
}

void ShimmerReadyProcessor::applyPreset (const ShimmerPreset& pr)
{
    auto setValue = [this] (const char* id, float value)
    {
        if (auto* param = state.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };
    setValue (ParamIDs::sync, 1.0f);
    setValue (ParamIDs::division, (float) pr.division);
    setValue (ParamIDs::pitch, pr.pitch);
    setValue (ParamIDs::feedback, pr.feedback);
    setValue (ParamIDs::shimmer, pr.shimmer);
    setValue (ParamIDs::decay, pr.decay);
    setValue (ParamIDs::highCut, pr.highCut);
    setValue (ParamIDs::reverse, pr.reverse ? 1.0f : 0.0f);
    setValue (ParamIDs::freeze, 0.0f);
    setValue (ParamIDs::mix, pr.mix);
}

void ShimmerReadyProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, std::max (16, std::min (samplesPerBlock, 8192)), getMainBusNumInputChannels());
    engine.setParams (readParams());
    setLatencySamples (0);
}

void ShimmerReadyProcessor::reset()
{
    engine.reset();
}

bool ShimmerReadyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void ShimmerReadyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int numIn = std::min (getMainBusNumInputChannels(), maxChannels);

    if (auto* head = getPlayHead())
        if (const auto pos = head->getPosition())
            if (const auto bpm = pos->getBpm())
                if (*bpm > 20.0) hostBpm.store ((float) *bpm);

    engine.setParams (readParams());

    for (int pos = 0; pos < n; pos += 8192)
    {
        const int len = std::min (8192, n - pos);
        float* ptrs[maxChannels] = {};
        for (int c = 0; c < numIn; ++c)
            ptrs[c] = buffer.getWritePointer (c) + pos;
        engine.process (ptrs, numIn, len);
    }

    for (int ch = numIn; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);

    const auto& m = engine.getMeters();
    delayMs.store (m.delayMs);
    wetPeak.store (m.wetPeak);
    feedbackPeak.store (m.feedbackPeak);
    inputHz.store (m.inputHz);
}

juce::AudioProcessorEditor* ShimmerReadyProcessor::createEditor()
{
    return new ShimmerReadyEditor (*this);
}

void ShimmerReadyProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void ShimmerReadyProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ShimmerReadyProcessor();
}
