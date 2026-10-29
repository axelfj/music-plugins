// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rommuser::phaseready;

namespace
{
juce::String dbString (float v, int) { return juce::String (v, 1) + " dB"; }
juce::String percentString (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; }
juce::String hzString (float v, int)
{
    return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz";
}
juce::String rateString (float v, int) { return juce::String (v, v < 1.0f ? 2 : 1) + " Hz"; }

juce::NormalisableRange<float> logRange (float lo, float hi)
{
    return juce::NormalisableRange<float> (lo, hi,
        [] (float s, float e, float n) { return s * std::pow (e / s, n); },
        [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); });
}
} // namespace

PhaseReadyProcessor::PhaseReadyProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "PhaseReady", createLayout())
{
    pStages = state.getRawParameterValue (ParamIDs::stages);
    pSync = state.getRawParameterValue (ParamIDs::sync);
    pDivision = state.getRawParameterValue (ParamIDs::division);
    pRate = state.getRawParameterValue (ParamIDs::rate);
    pDepth = state.getRawParameterValue (ParamIDs::depth);
    pFeedback = state.getRawParameterValue (ParamIDs::feedback);
    pCenter = state.getRawParameterValue (ParamIDs::center);
    pSpread = state.getRawParameterValue (ParamIDs::spread);
    pShape = state.getRawParameterValue (ParamIDs::shape);
    pMix = state.getRawParameterValue (ParamIDs::mix);
    pOutput = state.getRawParameterValue (ParamIDs::output);
}

juce::AudioProcessorValueTreeState::ParameterLayout PhaseReadyProcessor::createLayout()
{
    using namespace juce;
    using P = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;

    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::stages, 1 }, "Stages", Choices::stages, 1));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::sync, 1 }, "Sync", false));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::division, 1 }, "Division", Choices::divisions, 2));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::rate, 1 }, "Rate",
                                     logRange (0.01f, 10.0f), 0.5f, A().withLabel ("Hz").withStringFromValueFunction (rateString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::depth, 1 }, "Depth",
                                     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 70.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::feedback, 1 }, "Feedback",
                                     NormalisableRange<float> (-95.0f, 95.0f, 0.1f), 30.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::center, 1 }, "Center",
                                     logRange (100.0f, 5000.0f), 800.0f, A().withLabel ("Hz").withStringFromValueFunction (hzString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::spread, 1 }, "Stereo Spread",
                                     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::shape, 1 }, "LFO Shape", Choices::shapes, 0));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::mix, 1 }, "Mix",
                                     NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::output, 1 }, "Output",
                                     NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    return layout;
}

PhaseParams PhaseReadyProcessor::readParams() const
{
    PhaseParams p;
    p.stages = Choices::stageCounts[juce::jlimit (0, Choices::stages.size() - 1, juce::roundToInt (pStages->load()))];
    p.sync = pSync->load() > 0.5f;
    p.beats = Choices::divisionBeats[juce::jlimit (0, Choices::divisions.size() - 1, juce::roundToInt (pDivision->load()))];
    p.rateHz = pRate->load();
    p.bpm = hostBpm.load();
    p.depth = pDepth->load() / 100.0f;
    p.feedback = pFeedback->load() / 100.0f;
    p.centerHz = pCenter->load();
    p.spread = pSpread->load() / 100.0f;
    p.shape = (LfoShape) juce::jlimit (0, 2, juce::roundToInt (pShape->load()));
    p.mix = pMix->load() / 100.0f;
    p.outputDb = pOutput->load();
    return p;
}

void PhaseReadyProcessor::applyPreset (const PhasePreset& preset)
{
    auto setValue = [this] (const char* id, float value)
    {
        if (auto* param = state.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };
    setValue (ParamIDs::stages, (float) preset.stagesIndex);
    setValue (ParamIDs::sync, preset.sync ? 1.0f : 0.0f);
    setValue (ParamIDs::division, (float) preset.division);
    setValue (ParamIDs::rate, preset.rateHz);
    setValue (ParamIDs::depth, preset.depth * 100.0f);
    setValue (ParamIDs::feedback, preset.feedback * 100.0f);
    setValue (ParamIDs::center, preset.centerHz);
    setValue (ParamIDs::spread, preset.spread * 100.0f);
    setValue (ParamIDs::shape, (float) preset.shape);
    setValue (ParamIDs::mix, preset.mix * 100.0f);
}

void PhaseReadyProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate, getMainBusNumInputChannels());
    engine.setParams (readParams());
}

void PhaseReadyProcessor::reset()
{
    engine.reset();
}

bool PhaseReadyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void PhaseReadyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels();

    Transport t;
    if (auto* head = getPlayHead())
    {
        if (const auto pos = head->getPosition())
        {
            t.valid = true;
            t.playing = pos->getIsPlaying();
            if (const auto ppq = pos->getPpqPosition()) t.ppqPosition = *ppq;
            if (const auto bpm = pos->getBpm())
            {
                t.bpm = *bpm;
                if (*bpm > 20.0) hostBpm.store ((float) *bpm);
            }
        }
    }

    engine.setParams (readParams());
    const auto m = engine.process (buffer.getArrayOfWritePointers(), numIn, n, t);

    for (int ch = numIn; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);

    float prev = outputPeak.load();
    while (m.outputPeak > prev && ! outputPeak.compare_exchange_weak (prev, m.outputPeak)) {}
    lfoL.store (m.lfoL);
    lfoR.store (m.lfoR);

    for (int ch = 0; ch < 2; ++ch)
    {
        float coeffs[maxStages];
        int count = 0;
        engine.getStageCoefficients (std::min (ch, numIn - 1), coeffs, count);
        for (int k = 0; k < count; ++k) displayCoeffs[(size_t) ch][(size_t) k].store (coeffs[k]);
        displayStages.store (count);
    }
}

juce::AudioProcessorEditor* PhaseReadyProcessor::createEditor()
{
    return rommuser::ui::createScalableEditor<PhaseReadyEditor> (*this);
}

void PhaseReadyProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void PhaseReadyProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PhaseReadyProcessor();
}
