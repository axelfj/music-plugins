// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rommuser::tuneready;

namespace
{
juce::String dbString (float v, int) { return juce::String (v, 1) + " dB"; }
juce::String msString (float v, int) { return v <= 0.0f ? juce::String ("0 ms") : juce::String (juce::roundToInt (v)) + " ms"; }
juce::String percentString (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; }
juce::String centsString (float v, int) { return juce::String (juce::roundToInt (v)) + " c"; }
juce::String semiString (float v, int) { return (v > 0.0f ? "+" : "") + juce::String (v, 1) + " st"; }
} // namespace

TuneReadyProcessor::TuneReadyProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "TuneReady", createLayout())
{
    for (auto& v : detectedHistory) v.store (0.0f);
    for (auto& v : correctedHistory) v.store (0.0f);
}

juce::AudioProcessorValueTreeState::ParameterLayout TuneReadyProcessor::createLayout()
{
    using namespace juce;
    using P = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;

    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::mode, 1 }, "Mode", Choices::modes, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::key, 1 }, "Key", Choices::keys, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::scale, 1 }, "Scale", Choices::scales, 0));
    for (int i = 0; i < 12; ++i)
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::pitchClass[i], 1 }, "Note " + Choices::keys[i], true));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::speed, 1 }, "Speed",
                                     NormalisableRange<float> (0.0f, 400.0f, 1.0f, 0.5f), 20.0f, A().withLabel ("ms").withStringFromValueFunction (msString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::amount, 1 }, "Amount",
                                     NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::humanize, 1 }, "Humanize",
                                     NormalisableRange<float> (0.0f, 50.0f, 1.0f), 0.0f, A().withLabel ("c").withStringFromValueFunction (centsString)));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::range, 1 }, "Range", Choices::ranges, 1));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::fixedPitch, 1 }, "Pitch",
                                     NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f, A().withLabel ("st").withStringFromValueFunction (semiString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::formant, 1 }, "Formant",
                                     NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, A().withLabel ("st").withStringFromValueFunction (semiString)));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::midiPriority, 1 }, "MIDI", true));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::mix, 1 }, "Mix",
                                     NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::output, 1 }, "Output",
                                     NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    return layout;
}

TuneParams TuneReadyProcessor::readParams() const
{
    TuneParams q;
    auto choice = [this] (const char* id, int count) { return juce::jlimit (0, count - 1, juce::roundToInt (p (id)->load())); };
    q.mode = choice (ParamIDs::mode, 2) == 1 ? Mode::fixed : Mode::correct;
    q.key = choice (ParamIDs::key, 12);
    q.scale = (Scale) choice (ParamIDs::scale, 4);
    uint16_t mask = 0;
    for (int i = 0; i < 12; ++i)
        if (p (ParamIDs::pitchClass[i])->load() > 0.5f) mask |= (uint16_t) (1 << i);
    q.customMask = mask;
    q.speedMs = p (ParamIDs::speed)->load();
    q.amount = p (ParamIDs::amount)->load() * 0.01f;
    q.humanizeCents = p (ParamIDs::humanize)->load();
    q.minHz = Choices::rangeMinHz[choice (ParamIDs::range, 4)];
    q.fixedSemis = p (ParamIDs::fixedPitch)->load();
    q.formantSemis = p (ParamIDs::formant)->load();
    q.midiPriority = p (ParamIDs::midiPriority)->load() > 0.5f;
    q.mix = p (ParamIDs::mix)->load() * 0.01f;
    q.outputDb = p (ParamIDs::output)->load();
    return q;
}

void TuneReadyProcessor::applyPreset (const TunePreset& pr)
{
    auto setValue = [this] (const char* id, float value)
    {
        if (auto* param = state.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };
    setValue (ParamIDs::mode, (float) pr.mode);
    setValue (ParamIDs::speed, pr.speed);
    setValue (ParamIDs::amount, pr.amount);
    setValue (ParamIDs::humanize, pr.humanize);
    setValue (ParamIDs::fixedPitch, pr.fixedPitch);
    setValue (ParamIDs::formant, pr.formant);
    setValue (ParamIDs::mix, 100.0f);
}

void TuneReadyProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, std::max (16, std::min (samplesPerBlock, 8192)), getMainBusNumInputChannels());
    engine.setParams (readParams(), true);
    reportedLatency = engine.getLatencySamples();
    setLatencySamples (reportedLatency);
    entryInterval = std::max (64, (int) (sampleRate / 60.0));
    samplesSinceEntry = 0;
}

void TuneReadyProcessor::reset()
{
    engine.reset();
}

bool TuneReadyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void TuneReadyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int numIn = std::min (getMainBusNumInputChannels(), maxChannels);

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

    // Process in slices so the pitch history gets regular entries.
    int pos = 0;
    while (pos < n)
    {
        const int len = std::min ({ n - pos, 8192, entryInterval - samplesSinceEntry });
        float* ptrs[maxChannels] = {};
        for (int c = 0; c < numIn; ++c)
            ptrs[c] = buffer.getWritePointer (c) + pos;
        engine.process (ptrs, numIn, len);
        pos += len;
        samplesSinceEntry += len;
        if (samplesSinceEntry >= entryInterval)
        {
            samplesSinceEntry = 0;
            const auto& m = engine.getMeters();
            const int w = writeIndex.load();
            detectedHistory[(size_t) w].store (m.voiced ? m.detectedMidi : 0.0f);
            correctedHistory[(size_t) w].store (m.voiced ? m.detectedMidi + m.shiftSemis : 0.0f);
            writeIndex.store ((w + 1) % historyLength);
        }
    }

    for (int ch = numIn; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);

    const auto& m = engine.getMeters();
    inputHz.store (m.inputHz);
    shiftSemis.store (m.shiftSemis);
    voiced.store (m.voiced);
    midiHeld.store (engine.hasHeldNotes());
}

juce::AudioProcessorEditor* TuneReadyProcessor::createEditor()
{
    return rommuser::ui::createScalableEditor<TuneReadyEditor> (*this);
}

void TuneReadyProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void TuneReadyProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TuneReadyProcessor();
}
