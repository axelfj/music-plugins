// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rommuser::chordready;

namespace
{
juce::String dbString (float v, int) { return juce::String (v, 1) + " dB"; }
juce::String msString (float v, int) { return juce::String (juce::roundToInt (v)) + " ms"; }
juce::String percentString (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; }
juce::String semiString (float v, int)
{
    const int i = juce::roundToInt (v);
    return (i > 0 ? "+" : "") + juce::String (i) + " st";
}
juce::String inversionString (float v, int)
{
    const int i = juce::roundToInt (v);
    if (i == 0) return "root";
    return (i > 0 ? "+" : "") + juce::String (i);
}
} // namespace

//==============================================================================
const std::vector<ChordPreset>& chordPresets()
{
    using namespace ParamIDs;
    static const std::vector<ChordPreset> presets = {
        { "TRIADS", {} },
        { "7THS", { { add7, 1 } } },
        { "9THS PAD", { { add7, 1 }, { add9, 1 }, { voicing, 2 }, { strum, 12 }, { spread, 30 } } },
        { "TRANCE STAB", { { mode, 1 }, { chordType, 1 }, { topOct, 1 }, { bassOct, 1 } } },
        { "POWER CHORD", { { mode, 1 }, { chordType, 6 }, { topOct, 1 } } },
        { "LOFI KEYS", { { add7, 1 }, { voicing, 1 }, { strum, 18 }, { spread, 40 }, { humanize, 35 } } },
        { "GUITAR STRUM", { { voicing, 3 }, { strum, 35 }, { strumDir, 2 }, { spread, 25 }, { humanize, 20 } } },
        { "HOUSE PIANO", { { add7, 1 }, { inversion, 1 }, { voicing, 1 }, { topOct, 1 } } },
        { "MINOR FIXED", { { mode, 1 }, { chordType, 1 } } },
        { "SUS WASH", { { mode, 1 }, { chordType, 5 }, { voicing, 2 }, { strum, 60 }, { spread, 40 } } },
    };
    return presets;
}

//==============================================================================
ChordReadyProcessor::ChordReadyProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "ChordReady", createLayout())
{
    for (auto& n : displayNotes) n.store (0);
    for (auto* param : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
            state.addParameterListener (ranged->getParameterID(), this);
}

ChordReadyProcessor::~ChordReadyProcessor()
{
    for (auto* param : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
            state.removeParameterListener (ranged->getParameterID(), this);
}

void ChordReadyProcessor::parameterChanged (const juce::String&, float)
{
    if (! applyingPreset.load())
        lastPreset.store (-1);
}

juce::AudioProcessorValueTreeState::ParameterLayout ChordReadyProcessor::createLayout()
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
    auto lin = [] (float lo, float hi, float step) { return NormalisableRange<float> (lo, hi, step); };

    c (mode, "Mode", Choices::modes, 0);
    c (key, "Key", Choices::keys(), 0);
    c (scale, "Scale", Choices::scales(), 0);
    b (snap, "Snap To Scale", true);
    b (add7, "Add 7th", false);
    b (add9, "Add 9th", false);
    c (chordType, "Chord Type", Choices::chordTypes(), 0);
    c (voicing, "Voicing", Choices::voicings, 0);
    f (inversion, "Inversion", lin (-2, 2, 1), 0, "", inversionString);
    b (bassOct, "Bass Octave", false);
    b (topOct, "Top Octave", false);
    f (transpose, "Transpose", lin (-24, 24, 1), 0, "st", semiString);
    f (strum, "Strum", lin (0, 300, 1), 0, "ms", msString);
    c (strumDir, "Strum Direction", Choices::strumDirs, 0);
    f (spread, "Velocity Spread", lin (0, 100, 1), 0, "%", percentString);
    f (humanize, "Humanize", lin (0, 100, 1), 0, "%", percentString);
    b (sound, "Built-in Sound", true);
    f (soundLevel, "Sound Level", lin (-30, 6, 0.1f), -6, "dB", dbString);
    return layout;
}

ChordParams ChordReadyProcessor::readParams() const
{
    using namespace ParamIDs;
    auto v = [this] (const char* id) { return p (id)->load(); };
    auto ix = [&] (const char* id) { return juce::roundToInt (v (id)); };
    auto on = [&] (const char* id) { return v (id) > 0.5f; };

    ChordParams q;
    q.mode = ix (mode) == 1 ? ChordMode::fixed : ChordMode::scale;
    q.key = ix (key);
    q.scale = ix (scale);
    q.snapInput = on (snap);
    q.add7 = on (add7);
    q.add9 = on (add9);
    q.chordType = ix (chordType);
    q.voicing = (Voicing) juce::jlimit (0, 3, ix (voicing));
    q.inversion = ix (inversion);
    q.bassOctave = on (bassOct);
    q.topOctave = on (topOct);
    q.transpose = ix (transpose);
    q.strumMs = v (strum);
    q.strumDir = (StrumDir) juce::jlimit (0, 3, ix (strumDir));
    q.velocitySpread = v (spread) * 0.01f;
    q.humanize = v (humanize) * 0.01f;
    return q;
}

void ChordReadyProcessor::applyPreset (const ChordPreset& pr)
{
    applyingPreset.store (true);
    for (auto* param : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
        {
            // The sound toggle and level are a listening aid, not part of the chord: leave them alone.
            const auto id = ranged->getParameterID();
            if (id == ParamIDs::sound || id == ParamIDs::soundLevel) continue;
            ranged->setValueNotifyingHost (ranged->getDefaultValue());
        }
    for (const auto& [id, value] : pr.values)
        if (auto* param = state.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    applyingPreset.store (false);
}

void ChordReadyProcessor::applyPreset (int index)
{
    const auto& list = chordPresets();
    if (index < 0 || index >= (int) list.size()) return;
    applyPreset (list[(size_t) index]);
    lastPreset.store (index);
}

//==============================================================================
void ChordReadyProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
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

void ChordReadyProcessor::reset()
{
    engine.reset();
    synth.reset();
    outputKeys.reset();
    displayInput.store (-1);
    displayCount.store (0);
    heldInputs.store (0);
}

bool ChordReadyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void ChordReadyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
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
    const bool useSound = p (ParamIDs::sound)->load() > 0.5f;
    synth.setLevel (juce::Decibels::decibelsToGain (p (ParamIDs::soundLevel)->load()));

    // Split the input: notes go to the engine, everything else passes straight through.
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
        // Drop the notes of this block and release everything at the panic point.
        inEvents.clear();
        engine.allNotesOff (panicAt, outEvents);
        synth.allNotesOff();
    }
    else
        engine.process (inEvents, n, outEvents);

    // Output events are in time order: render the preview synth between them.
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
        synth.render (left.data(), right.data(), n);   // short fade instead of a click
    }
    midi.swapWith (outMidi);

    // Display: last input note and its chord.
    const int last = engine.lastInputNote();
    if (last >= 0)
    {
        const auto notes = engine.chordFor (last);
        const int count = std::min ((int) notes.size(), (int) displayNotes.size());
        for (int i = 0; i < count; ++i) displayNotes[(size_t) i].store (notes[(size_t) i]);
        displayCount.store (count);
        displayInput.store (last);
    }
    heldInputs.store (engine.numHeldInputs());

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
juce::AudioProcessorEditor* ChordReadyProcessor::createEditor()
{
    return new ChordReadyEditor (*this);
}

void ChordReadyProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void ChordReadyProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChordReadyProcessor();
}
