// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rommuser::arpready;

namespace
{
juce::String dbString (float v, int) { return juce::String (v, 1) + " dB"; }
juce::String percentString (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; }
juce::String intString (float v, int) { return juce::String (juce::roundToInt (v)); }
juce::String semiString (float v, int)
{
    const int i = juce::roundToInt (v);
    return (i > 0 ? "+" : "") + juce::String (i) + " st";
}
} // namespace

//==============================================================================
const std::vector<ArpPreset>& arpPresets()
{
    using namespace ParamIDs;
    static const std::vector<ArpPreset> presets = {
        { "CLASSIC UP", {} },
        { "TRANCE 1/16", { { octaves, 2 }, { gate, 45 }, { accent, 40 } } },
        { "UP-DOWN 2 OCT", { { mode, 2 }, { octaves, 2 }, { gate, 60 } } },
        { "PLUCK 1/8", { { division, 7 }, { gate, 30 }, { velocityMode, 1 }, { velocity, 110 } } },
        { "SWING HOUSE", { { division, 10 }, { swing, 55 }, { gate, 50 }, { accent, 30 } } },
        { "TRIPLET ROLL", { { division, 12 }, { octaves, 3 }, { gate, 70 } } },
        { "OFFBEAT", { { division, 7 }, { step[0], 0 }, { step[2], 0 }, { step[4], 0 }, { step[6], 0 }, { patternLength, 8 }, { gate, 40 } } },
        { "LATCH PAD", { { mode, 6 }, { division, 4 }, { gate, 100 }, { latch, 1 } } },
        { "RANDOM 3 OCT", { { mode, 5 }, { octaves, 3 }, { gate, 35 } } },
        { "SLOW 1/4 DOWN", { { mode, 1 }, { division, 4 }, { gate, 90 }, { octaves, 2 }, { octaveMode, 1 } } },
    };
    return presets;
}

//==============================================================================
ArpReadyProcessor::ArpReadyProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "ArpReady", createLayout())
{
    for (auto* param : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
            state.addParameterListener (ranged->getParameterID(), this);
}

ArpReadyProcessor::~ArpReadyProcessor()
{
    for (auto* param : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
            state.removeParameterListener (ranged->getParameterID(), this);
}

void ArpReadyProcessor::parameterChanged (const juce::String&, float)
{
    if (! applyingPreset.load())
        lastPreset.store (-1);
}

juce::AudioProcessorValueTreeState::ParameterLayout ArpReadyProcessor::createLayout()
{
    using namespace juce;
    using P = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;
    using namespace ParamIDs;

    AudioProcessorValueTreeState::ParameterLayout layout;
    auto f = [&] (const char* id, const char* name, NormalisableRange<float> range, float def, const char* label,
                  juce::String (*fmt) (float, int))
    {
        layout.add (std::make_unique<P> (ParameterID { id, 1 }, name, range, def, A().withLabel (label).withStringFromValueFunction (fmt)));
    };
    auto b = [&] (const char* id, const char* name, bool def)
    {
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def));
    };
    auto c = [&] (const char* id, const char* name, const StringArray& items, int def)
    {
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def));
    };
    auto lin = [] (float lo, float hi, float stepSize) { return NormalisableRange<float> (lo, hi, stepSize); };

    c (mode, "Mode", Choices::modes(), 0);
    c (division, "Rate", Choices::divisions(), defaultDivision);
    f (octaves, "Octaves", lin (1, 4, 1), 1, "", intString);
    c (octaveMode, "Octave Direction", Choices::octaveModes, 0);
    f (gate, "Gate", lin (5, 100, 1), 50, "%", percentString);
    f (swing, "Swing", lin (0, 100, 1), 0, "%", percentString);
    b (latch, "Latch", false);
    b (hostSync, "Sync To Host", true);
    f (transpose, "Transpose", lin (-24, 24, 1), 0, "st", semiString);
    f (patternLength, "Pattern Length", lin (1, 16, 1), 16, "", intString);
    for (int i = 0; i < 16; ++i)
        b (step[i], ("Step " + juce::String (i + 1)).toRawUTF8(), true);
    c (velocityMode, "Velocity Mode", Choices::velocityModes, 0);
    f (velocity, "Velocity", lin (1, 127, 1), 100, "", intString);
    f (accent, "Accent", lin (0, 100, 1), 0, "%", percentString);
    b (sound, "Built-in Sound", true);
    f (soundLevel, "Sound Level", lin (-30, 6, 0.1f), -6, "dB", dbString);
    return layout;
}

ArpParams ArpReadyProcessor::readParams() const
{
    using namespace ParamIDs;
    auto v = [this] (const char* id) { return p (id)->load(); };
    auto ix = [&] (const char* id) { return juce::roundToInt (v (id)); };
    auto on = [&] (const char* id) { return v (id) > 0.5f; };

    ArpParams q;
    q.mode = (ArpMode) juce::jlimit (0, numModes - 1, ix (mode));
    q.division = ix (division);
    q.octaves = ix (octaves);
    q.octaveMode = (OctaveMode) juce::jlimit (0, 2, ix (octaveMode));
    q.gate = v (gate) * 0.01f;
    q.swing = v (swing) * 0.01f;
    q.latch = on (latch);
    q.sync = on (hostSync);
    q.transpose = ix (transpose);
    q.patternLength = ix (patternLength);
    for (int i = 0; i < 16; ++i) q.pattern[(size_t) i] = on (step[i]);
    q.velocityMode = ix (velocityMode) == 1 ? VelocityMode::fixed : VelocityMode::played;
    q.velocity = ix (velocity);
    q.accent = v (accent) * 0.01f;
    return q;
}

void ArpReadyProcessor::applyPreset (const ArpPreset& pr)
{
    applyingPreset.store (true);
    for (auto* param : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
        {
            const auto id = ranged->getParameterID();
            if (id == ParamIDs::sound || id == ParamIDs::soundLevel) continue;
            ranged->setValueNotifyingHost (ranged->getDefaultValue());
        }
    for (const auto& [id, value] : pr.values)
        if (auto* param = state.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    applyingPreset.store (false);
}

void ArpReadyProcessor::applyPreset (int index)
{
    const auto& list = arpPresets();
    if (index < 0 || index >= (int) list.size()) return;
    applyPreset (list[(size_t) index]);
    lastPreset.store (index);
}

//==============================================================================
void ArpReadyProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    const int block = std::max (16, samplesPerBlock);
    engine.prepare (sampleRate);
    engine.setParams (readParams());
    synth.prepare (sampleRate);
    inEvents.reserve (256);
    outEvents.reserve (1024);
    outMidi.ensureSize (4096);
    left.assign ((size_t) block, 0.0f);
    right.assign ((size_t) block, 0.0f);
    keyboardState.reset();
    outputKeys.reset();
}

void ArpReadyProcessor::reset()
{
    engine.reset();
    synth.reset();
    outputKeys.reset();
    currentStep.store (-1);
    heldCount.store (0);
}

bool ArpReadyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void ArpReadyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    if ((int) left.size() < n)
    {
        left.assign ((size_t) n, 0.0f);
        right.assign ((size_t) n, 0.0f);
    }
    std::fill (left.begin(), left.begin() + n, 0.0f);
    std::fill (right.begin(), right.begin() + n, 0.0f);

    keyboardState.processNextMidiBuffer (midi, 0, n, true);
    engine.setParams (readParams());

    // Host tempo and position.
    transport.playing = false;
    if (auto* head = getPlayHead())
        if (auto pos = head->getPosition())
        {
            if (auto bpm = pos->getBpm())
                if (*bpm > 20.0 && *bpm < 999.0) transport.bpm = *bpm;
            if (pos->getIsPlaying())
                if (auto ppq = pos->getPpqPosition())
                {
                    transport.playing = true;
                    transport.ppq = *ppq;
                }
        }
    engine.setTransport (transport);
    displayBpm.store ((float) transport.bpm);
    transportPlaying.store (transport.playing);

    const bool useSound = p (ParamIDs::sound)->load() > 0.5f;
    synth.setLevel (juce::Decibels::decibelsToGain (p (ParamIDs::soundLevel)->load()));

    inEvents.clear();
    outEvents.clear();
    outMidi.clear();
    bool panic = false;
    int panicAt = 0;
    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();
        const int at = juce::jlimit (0, std::max (0, n - 1), metadata.samplePosition);
        if (msg.isNoteOn())
            inEvents.push_back ({ at, true, msg.getNoteNumber(), msg.getVelocity(), msg.getChannel() });
        else if (msg.isNoteOff())
            inEvents.push_back ({ at, false, msg.getNoteNumber(), 0, msg.getChannel() });
        else
        {
            if (msg.isAllNotesOff() || msg.isAllSoundOff()) { panic = true; panicAt = at; }
            outMidi.addEvent (msg, at);
        }
    }
    if (panic)
    {
        inEvents.clear();
        engine.allNotesOff (panicAt, outEvents);
        synth.allNotesOff();
    }
    else
        engine.process (inEvents, n, outEvents);

    int pos = 0;
    for (const auto& e : outEvents)
    {
        const int at = juce::jlimit (pos, n, e.sample);
        if (useSound && at > pos)
        {
            synth.render (left.data() + pos, right.data() + pos, at - pos);
            pos = at;
        }
        if (e.on)
        {
            outMidi.addEvent (juce::MidiMessage::noteOn (e.channel, e.note, (juce::uint8) e.velocity), e.sample);
            outputKeys.noteOn (e.channel, e.note, (float) e.velocity / 127.0f);
            if (useSound) synth.noteOn (e.note, (float) e.velocity / 127.0f);
        }
        else
        {
            outMidi.addEvent (juce::MidiMessage::noteOff (e.channel, e.note), e.sample);
            outputKeys.noteOff (e.channel, e.note, 0.0f);
            synth.noteOff (e.note);
        }
    }
    if (useSound && pos < n)
        synth.render (left.data() + pos, right.data() + pos, n - pos);
    if (! useSound && synth.isSounding())
    {
        synth.allNotesOff();
        synth.render (left.data(), right.data(), n);
    }
    midi.swapWith (outMidi);

    currentStep.store (engine.currentPatternStep());
    heldCount.store (engine.numHeld());

    float peak = 0.0f;
    for (int i = 0; i < n; ++i)
        peak = std::max (peak, std::max (std::fabs (left[(size_t) i]), std::fabs (right[(size_t) i])));

    if (buffer.getNumChannels() >= 2)
    {
        buffer.copyFrom (0, 0, left.data(), n);
        buffer.copyFrom (1, 0, right.data(), n);
        for (int ch = 2; ch < buffer.getNumChannels(); ++ch)
            buffer.clear (ch, 0, n);
    }
    else if (buffer.getNumChannels() == 1)
    {
        buffer.copyFrom (0, 0, left.data(), n);
        buffer.addFrom (0, 0, right.data(), n);
        buffer.applyGain (0, 0, n, 0.5f);
    }

    float prev = outputPeak.load();
    while (peak > prev && ! outputPeak.compare_exchange_weak (prev, peak)) {}
}

//==============================================================================
juce::AudioProcessorEditor* ArpReadyProcessor::createEditor()
{
    return rommuser::ui::createScalableEditor<ArpReadyEditor> (*this);
}

void ArpReadyProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void ArpReadyProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ArpReadyProcessor();
}
