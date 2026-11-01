// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rommuser::voxsynth;

namespace
{
juce::String dbString (float v, int) { return juce::String (v, 1) + " dB"; }
juce::String dryDbString (float v, int) { return v < -59.0f ? juce::String ("off") : juce::String (v, 1) + " dB"; }
juce::String msString (float v, int) { return juce::String (v, 1) + " ms"; }
juce::String percentString (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; }
juce::String centsString (float v, int) { return (v > 0.0f ? "+" : "") + juce::String (juce::roundToInt (v)) + " c"; }
juce::String semiString (float v, int) { return (v > 0.0f ? "+" : "") + juce::String (v, 1) + " st"; }
juce::String panString (float v, int)
{
    const int pc = juce::roundToInt (v * 100.0f);
    return pc == 0 ? juce::String ("C") : pc < 0 ? juce::String (-pc) + " L" : juce::String (pc) + " R";
}
} // namespace

VoxSynthProcessor::VoxSynthProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                          .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      state (*this, nullptr, "VoxSynth", createLayout())
{
    for (auto& v : detectedHistory) v.store (0.0f);
    for (auto& voice : voiceHistory)
        for (auto& v : voice) v.store (0.0f);
    for (auto& v : bandLevels) v.store (0.0f);
}

juce::AudioProcessorValueTreeState::ParameterLayout VoxSynthProcessor::createLayout()
{
    using namespace juce;
    using P = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;

    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::mode, 1 }, "Mode", Choices::modes, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::range, 1 }, "Range", Choices::ranges, 1));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::key, 1 }, "Key", Choices::keys, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::scale, 1 }, "Scale", Choices::scales, 1));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::intervalMode, 1 }, "Interval mode", Choices::intervalModes, 0));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::glide, 1 }, "Glide",
                                     NormalisableRange<float> (0.0f, 200.0f, 1.0f, 0.5f), 20.0f, A().withLabel ("ms").withStringFromValueFunction (msString)));

    const int defaultDegree[4] = { 2, 4, -3, 0 };
    const float defaultSemi[4] = { 4.0f, 7.0f, -5.0f, 12.0f };
    const float defaultPan[4] = { -0.5f, 0.5f, -0.25f, 0.25f };
    for (int v = 0; v < maxVoices; ++v)
    {
        const String name = "Voice " + String (v + 1) + " ";
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::voiceOn[v], 1 }, name + "On", v < 2));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::voiceDegrees[v], 1 }, name + "Degree", Choices::degrees, defaultDegree[v] + 7));
        layout.add (std::make_unique<P> (ParameterID { ParamIDs::voiceSemis[v], 1 }, name + "Semitones",
                                         NormalisableRange<float> (-24.0f, 24.0f, 1.0f), defaultSemi[v], A().withLabel ("st").withStringFromValueFunction (semiString)));
        layout.add (std::make_unique<P> (ParameterID { ParamIDs::voiceDetune[v], 1 }, name + "Detune",
                                         NormalisableRange<float> (-50.0f, 50.0f, 1.0f), 0.0f, A().withLabel ("c").withStringFromValueFunction (centsString)));
        layout.add (std::make_unique<P> (ParameterID { ParamIDs::voicePan[v], 1 }, name + "Pan",
                                         NormalisableRange<float> (-1.0f, 1.0f, 0.01f), defaultPan[v], A().withStringFromValueFunction (panString)));
        layout.add (std::make_unique<P> (ParameterID { ParamIDs::voiceLevel[v], 1 }, name + "Level",
                                         NormalisableRange<float> (-36.0f, 6.0f, 0.1f), -6.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
        layout.add (std::make_unique<P> (ParameterID { ParamIDs::voiceFormant[v], 1 }, name + "Formant",
                                         NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, A().withLabel ("st").withStringFromValueFunction (semiString)));
    }

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::carrier, 1 }, "Carrier", Choices::carriers, 0));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::carrierFollow, 1 }, "Carrier follows voice", true));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::carrierNote, 1 }, "Carrier note", Choices::noteNames(), 48 - Choices::firstNote));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::bands, 1 }, "Bands",
                                     NormalisableRange<float> (8.0f, 32.0f, 1.0f), 20.0f));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::attack, 1 }, "Attack",
                                     NormalisableRange<float> (1.0f, 50.0f, 0.1f, 0.5f), 10.0f, A().withLabel ("ms").withStringFromValueFunction (msString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::release, 1 }, "Release",
                                     NormalisableRange<float> (5.0f, 500.0f, 1.0f, 0.5f), 60.0f, A().withLabel ("ms").withStringFromValueFunction (msString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::sibilance, 1 }, "Sibilance",
                                     NormalisableRange<float> (0.0f, 100.0f, 1.0f), 30.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::carrierDetune, 1 }, "Carrier detune",
                                     NormalisableRange<float> (-50.0f, 50.0f, 1.0f), 0.0f, A().withLabel ("c").withStringFromValueFunction (centsString)));

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::robotNote, 1 }, "Robot note", Choices::noteNames(), 48 - Choices::firstNote));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::robotFormant, 1 }, "Robot formant",
                                     NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, A().withLabel ("st").withStringFromValueFunction (semiString)));

    layout.add (std::make_unique<P> (ParameterID { ParamIDs::dry, 1 }, "Dry",
                                     NormalisableRange<float> (-60.0f, 6.0f, 0.1f), 0.0f, A().withLabel ("dB").withStringFromValueFunction (dryDbString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::mix, 1 }, "Mix",
                                     NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::output, 1 }, "Output",
                                     NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    return layout;
}

VoxParams VoxSynthProcessor::readParams() const
{
    VoxParams q;
    auto choice = [this] (const char* id, int count) { return juce::jlimit (0, count - 1, juce::roundToInt (p (id)->load())); };
    q.mode = (Mode) choice (ParamIDs::mode, 3);
    q.minHz = Choices::rangeMinHz[choice (ParamIDs::range, 4)];
    q.key = choice (ParamIDs::key, 12);
    q.scale = (Scale) choice (ParamIDs::scale, 3);
    q.customMask = 0x0FFF;
    q.intervalMode = (IntervalMode) choice (ParamIDs::intervalMode, 3);
    q.glideMs = p (ParamIDs::glide)->load();
    for (int v = 0; v < maxVoices; ++v)
    {
        auto& voice = q.voices[(size_t) v];
        voice.enabled = p (ParamIDs::voiceOn[v])->load() > 0.5f;
        voice.degrees = choice (ParamIDs::voiceDegrees[v], 15) - 7;
        voice.semitones = p (ParamIDs::voiceSemis[v])->load();
        voice.detuneCents = p (ParamIDs::voiceDetune[v])->load();
        voice.pan = p (ParamIDs::voicePan[v])->load();
        voice.levelDb = p (ParamIDs::voiceLevel[v])->load();
        voice.formantSemis = p (ParamIDs::voiceFormant[v])->load();
    }
    q.carrier = (Carrier) choice (ParamIDs::carrier, 4);
    q.carrierFollowsPitch = p (ParamIDs::carrierFollow)->load() > 0.5f;
    q.carrierNote = Choices::firstNote + choice (ParamIDs::carrierNote, Choices::numNotes);
    q.bands = juce::jlimit (8, maxBands, juce::roundToInt (p (ParamIDs::bands)->load()));
    q.attackMs = p (ParamIDs::attack)->load();
    q.releaseMs = p (ParamIDs::release)->load();
    q.sibilance = p (ParamIDs::sibilance)->load() * 0.01f;
    q.carrierDetuneCents = p (ParamIDs::carrierDetune)->load();
    q.robotNote = Choices::firstNote + choice (ParamIDs::robotNote, Choices::numNotes);
    q.robotFormantSemis = p (ParamIDs::robotFormant)->load();
    q.dryDb = p (ParamIDs::dry)->load();
    q.mix = p (ParamIDs::mix)->load() * 0.01f;
    q.outputDb = p (ParamIDs::output)->load();
    return q;
}

void VoxSynthProcessor::applyPreset (const VoxPreset& pr)
{
    auto setValue = [this] (const char* id, float value)
    {
        if (auto* param = state.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };
    setValue (ParamIDs::mode, (float) pr.mode);
    setValue (ParamIDs::intervalMode, (float) pr.intervalMode);
    for (int v = 0; v < maxVoices; ++v)
    {
        setValue (ParamIDs::voiceOn[v], pr.on[v] ? 1.0f : 0.0f);
        setValue (ParamIDs::voiceDegrees[v], (float) (pr.degrees[v] + 7));
        setValue (ParamIDs::voiceSemis[v], pr.semis[v]);
        setValue (ParamIDs::voiceDetune[v], 0.0f);
        setValue (ParamIDs::voicePan[v], pr.pan[v]);
        setValue (ParamIDs::voiceLevel[v], pr.level[v]);
        setValue (ParamIDs::voiceFormant[v], 0.0f);
    }
    setValue (ParamIDs::carrier, (float) pr.carrier);
    setValue (ParamIDs::carrierFollow, pr.follow ? 1.0f : 0.0f);
    setValue (ParamIDs::robotNote, (float) (pr.robotNote - Choices::firstNote));
    setValue (ParamIDs::robotFormant, pr.robotFormant);
    setValue (ParamIDs::dry, pr.dry);
    setValue (ParamIDs::mix, 100.0f);
}

void VoxSynthProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, std::max (16, std::min (samplesPerBlock, 8192)), getMainBusNumInputChannels());
    engine.setParams (readParams(), true);
    reportedLatency = engine.getLatencySamples();
    setLatencySamples (reportedLatency);
    entryInterval = std::max (64, (int) (sampleRate / 60.0));
    samplesSinceEntry = 0;
}

void VoxSynthProcessor::reset()
{
    engine.reset();
}

bool VoxSynthProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet() != out)
        return false;
    const auto sc = layouts.getChannelSet (true, 1);
    return sc.isDisabled() || sc == juce::AudioChannelSet::mono() || sc == juce::AudioChannelSet::stereo();
}

void VoxSynthProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int numIn = std::min (getMainBusNumInputChannels(), rommuser::dsp::psola::maxChannels);

    engine.setParams (readParams());
    if (engine.getLatencySamples() != reportedLatency)
    {
        reportedLatency = engine.getLatencySamples();
        setLatencySamples (reportedLatency);
    }

    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn()) engine.noteOn (msg.getNoteNumber());
        else if (msg.isNoteOff()) engine.noteOff (msg.getNoteNumber());
        else if (msg.isAllNotesOff() || msg.isAllSoundOff()) engine.allNotesOff();
    }

    // The sidechain (external carrier) may be absent, mono or stereo.
    const float* scPtrs[2] = { nullptr, nullptr };
    int scChannels = 0;
    if (getBusCount (true) > 1 && getBus (true, 1)->isEnabled())
    {
        auto sc = getBusBuffer (buffer, true, 1);
        scChannels = std::min (sc.getNumChannels(), 2);
        for (int c = 0; c < scChannels; ++c)
            scPtrs[c] = sc.getReadPointer (c);
    }

    auto main = getBusBuffer (buffer, true, 0);

    int pos = 0;
    while (pos < n)
    {
        const int len = std::min ({ n - pos, 8192, entryInterval - samplesSinceEntry });
        float* ptrs[2] = {};
        for (int c = 0; c < numIn; ++c)
            ptrs[c] = main.getWritePointer (c) + pos;
        const float* scOff[2] = {};
        for (int c = 0; c < scChannels; ++c)
            scOff[c] = scPtrs[c] + pos;
        engine.process (ptrs, numIn, len, scChannels > 0 ? scOff : nullptr, scChannels);
        pos += len;
        samplesSinceEntry += len;
        if (samplesSinceEntry >= entryInterval)
        {
            samplesSinceEntry = 0;
            const auto& m = engine.getMeters();
            const int w = writeIndex.load();
            detectedHistory[(size_t) w].store (m.voiced ? m.detectedMidi : 0.0f);
            for (int v = 0; v < maxVoices; ++v)
                voiceHistory[(size_t) v][(size_t) w].store (m.voiced ? m.voiceMidi[(size_t) v] : 0.0f);
            writeIndex.store ((w + 1) % historyLength);
        }
    }

    for (int ch = numIn; ch < getMainBusNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);

    const auto& m = engine.getMeters();
    inputHz.store (m.inputHz);
    carrierHz.store (m.carrierHz);
    outputPeak.store (m.outputPeak);
    voiced.store (m.voiced);
    midiHeld.store (engine.hasHeldNotes());
    for (int b = 0; b < maxBands; ++b)
        bandLevels[(size_t) b].store (m.bandLevels[(size_t) b]);
}

juce::AudioProcessorEditor* VoxSynthProcessor::createEditor()
{
    return rommuser::ui::createScalableEditor<VoxSynthEditor> (*this);
}

void VoxSynthProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void VoxSynthProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoxSynthProcessor();
}
