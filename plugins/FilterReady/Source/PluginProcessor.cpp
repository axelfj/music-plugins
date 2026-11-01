// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rommuser::filterready;

namespace
{
juce::String dbString (float v, int) { return (v > 0.0f ? "+" : "") + juce::String (v, 1) + " dB"; }
juce::String msString (float v, int) { return juce::String (v, v < 10.0f ? 1 : 0) + " ms"; }
juce::String hzString (float v, int)
{
    return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz";
}
juce::String pctString (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; }
juce::String octString (float v, int) { return (v > 0.0f ? "+" : "") + juce::String (v, 2) + " oct"; }
juce::String rateString (float v, int) { return juce::String (v, v < 1.0f ? 2 : 1) + " Hz"; }

juce::NormalisableRange<float> logRange (float lo, float hi)
{
    return juce::NormalisableRange<float> (lo, hi,
        [] (float s, float e, float n) { return s * std::pow (e / s, n); },
        [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); });
}
} // namespace

FilterReadyProcessor::FilterReadyProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "FilterReady", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout FilterReadyProcessor::createLayout()
{
    using namespace juce;
    using P = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;

    AudioProcessorValueTreeState::ParameterLayout layout;
    auto stage = [&] (const char* prefix, const char* on, const char* type, const char* slope, const char* cutoff, const char* res,
                      const char* lfo, const char* env, bool defOn, int defType, int defSlope, float defCut, float defRes)
    {
        const String pre (prefix);
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { on, 1 }, pre + " On", defOn));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { type, 1 }, pre + " Type", Choices::types, defType));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { slope, 1 }, pre + " Slope", Choices::slopes, defSlope));
        layout.add (std::make_unique<P> (ParameterID { cutoff, 1 }, pre + " Cutoff", logRange (20.0f, 20000.0f), defCut, A().withLabel ("Hz").withStringFromValueFunction (hzString)));
        layout.add (std::make_unique<P> (ParameterID { res, 1 }, pre + " Resonance", NormalisableRange<float> (0.0f, 1.0f, 0.001f), defRes, A().withStringFromValueFunction (pctString)));
        layout.add (std::make_unique<P> (ParameterID { lfo, 1 }, pre + " LFO Amount", NormalisableRange<float> (-5.0f, 5.0f, 0.01f), 0.0f, A().withStringFromValueFunction (octString)));
        layout.add (std::make_unique<P> (ParameterID { env, 1 }, pre + " Env Amount", NormalisableRange<float> (-5.0f, 5.0f, 0.01f), 0.0f, A().withStringFromValueFunction (octString)));
    };
    stage ("A", ParamIDs::aOn, ParamIDs::aType, ParamIDs::aSlope, ParamIDs::aCutoff, ParamIDs::aRes, ParamIDs::aLfo, ParamIDs::aEnv, true, 0, 1, 1200.0f, 0.4f);
    stage ("B", ParamIDs::bOn, ParamIDs::bType, ParamIDs::bSlope, ParamIDs::bCutoff, ParamIDs::bRes, ParamIDs::bLfo, ParamIDs::bEnv, false, 1, 0, 150.0f, 0.2f);

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::routing, 1 }, "Routing", Choices::routing, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::character, 1 }, "Character", Choices::character, 0));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::drive, 1 }, "Drive", NormalisableRange<float> (0.0f, 36.0f, 0.1f), 0.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::lfoSync, 1 }, "LFO Sync", true));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::lfoRate, 1 }, "LFO Rate", logRange (0.02f, 20.0f), 0.5f, A().withLabel ("Hz").withStringFromValueFunction (rateString)));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::lfoDivision, 1 }, "LFO Division", Choices::divisions, 2));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::lfoShape, 1 }, "LFO Shape", Choices::shapes, 0));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::envAttack, 1 }, "Env Attack", logRange (1.0f, 200.0f), 10.0f, A().withLabel ("ms").withStringFromValueFunction (msString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::envRelease, 1 }, "Env Release", logRange (10.0f, 2000.0f), 200.0f, A().withLabel ("ms").withStringFromValueFunction (msString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::envSens, 1 }, "Env Sensitivity", NormalisableRange<float> (0.0f, 36.0f, 0.1f), 12.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::mix, 1 }, "Mix", NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f, A().withStringFromValueFunction (pctString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::output, 1 }, "Output", NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    return layout;
}

FilterParams FilterReadyProcessor::readParams() const
{
    FilterParams q;
    auto choice = [this] (const char* id, int count) { return juce::jlimit (0, count - 1, juce::roundToInt (p (id)->load())); };
    auto stage = [&] (StageParams& s, const char* on, const char* type, const char* slope, const char* cutoff, const char* res, const char* lfoId, const char* envId)
    {
        s.enabled = p (on)->load() > 0.5f;
        s.type = (FilterType) choice (type, Choices::types.size());
        s.slope = (Slope) choice (slope, Choices::slopes.size());
        s.cutoffHz = p (cutoff)->load();
        s.resonance = p (res)->load();
        s.lfoOctaves = p (lfoId)->load();
        s.envOctaves = p (envId)->load();
    };
    stage (q.a, ParamIDs::aOn, ParamIDs::aType, ParamIDs::aSlope, ParamIDs::aCutoff, ParamIDs::aRes, ParamIDs::aLfo, ParamIDs::aEnv);
    stage (q.b, ParamIDs::bOn, ParamIDs::bType, ParamIDs::bSlope, ParamIDs::bCutoff, ParamIDs::bRes, ParamIDs::bLfo, ParamIDs::bEnv);
    q.routing = (Routing) choice (ParamIDs::routing, Choices::routing.size());
    q.character = (Character) choice (ParamIDs::character, Choices::character.size());
    q.driveDb = p (ParamIDs::drive)->load();
    q.lfoSync = p (ParamIDs::lfoSync)->load() > 0.5f;
    q.lfoRateHz = p (ParamIDs::lfoRate)->load();
    q.lfoBeats = Choices::divisionBeats[choice (ParamIDs::lfoDivision, Choices::divisions.size())];
    q.bpm = hostBpm.load();
    q.lfoShape = (LfoShape) choice (ParamIDs::lfoShape, Choices::shapes.size());
    q.envAttackMs = p (ParamIDs::envAttack)->load();
    q.envReleaseMs = p (ParamIDs::envRelease)->load();
    q.envSensitivityDb = p (ParamIDs::envSens)->load();
    q.mix = p (ParamIDs::mix)->load();
    q.outputDb = p (ParamIDs::output)->load();
    return q;
}

void FilterReadyProcessor::applyPreset (const FilterPreset& pr)
{
    auto setValue = [this] (const char* id, float value)
    {
        if (auto* param = state.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };
    setValue (ParamIDs::aOn, 1.0f);
    setValue (ParamIDs::aType, (float) pr.aType);
    setValue (ParamIDs::aSlope, (float) pr.aSlope);
    setValue (ParamIDs::aCutoff, pr.aCutoff);
    setValue (ParamIDs::aRes, pr.aRes);
    setValue (ParamIDs::aLfo, pr.aLfo);
    setValue (ParamIDs::aEnv, pr.aEnv);
    setValue (ParamIDs::bOn, pr.bOn ? 1.0f : 0.0f);
    setValue (ParamIDs::bType, (float) pr.bType);
    setValue (ParamIDs::bSlope, (float) pr.bSlope);
    setValue (ParamIDs::bCutoff, pr.bCutoff);
    setValue (ParamIDs::bRes, pr.bRes);
    setValue (ParamIDs::bLfo, 0.0f);
    setValue (ParamIDs::bEnv, 0.0f);
    setValue (ParamIDs::routing, (float) pr.routing);
    setValue (ParamIDs::character, (float) pr.character);
    setValue (ParamIDs::drive, pr.drive);
    setValue (ParamIDs::lfoSync, pr.lfoSync ? 1.0f : 0.0f);
    setValue (ParamIDs::lfoDivision, (float) pr.lfoDivision);
    setValue (ParamIDs::lfoRate, pr.lfoRate);
    setValue (ParamIDs::lfoShape, (float) pr.lfoShape);
    setValue (ParamIDs::mix, 1.0f);
}

void FilterReadyProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    engine.prepare (sampleRate, std::max (16, std::min (samplesPerBlock, 8192)), getMainBusNumInputChannels());
    engine.setParams (readParams(), true);
    reportedLatency = engine.getLatencySamples();
    setLatencySamples (reportedLatency);
}

void FilterReadyProcessor::reset()
{
    engine.reset();
}

bool FilterReadyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void FilterReadyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int numIn = std::min (getMainBusNumInputChannels(), maxChannels);

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
    // Hosts may hand us blocks larger than prepareToPlay promised: process in slices.
    const int maxSlice = 8192;
    FilterMeters m;
    for (int pos = 0; pos < n; pos += maxSlice)
    {
        float* ptrs[maxChannels] = {};
        for (int c = 0; c < numIn; ++c)
            ptrs[c] = buffer.getWritePointer (c) + pos;
        m = engine.process (ptrs, numIn, std::min (maxSlice, n - pos), t);
    }

    for (int ch = numIn; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);

    cutoffA.store (m.cutoffA);
    cutoffB.store (m.cutoffB);
    lfo.store (m.lfo);
    env.store (m.env);
    float prev = outputPeak.load();
    while (m.outputPeak > prev && ! outputPeak.compare_exchange_weak (prev, m.outputPeak)) {}
}

juce::AudioProcessorEditor* FilterReadyProcessor::createEditor()
{
    return rommuser::ui::createScalableEditor<FilterReadyEditor> (*this);
}

void FilterReadyProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void FilterReadyProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FilterReadyProcessor();
}
