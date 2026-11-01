// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <juce_audio_formats/juce_audio_formats.h>

using namespace rommuser::kickready;

namespace
{
juce::String dbString (float v, int) { return juce::String (v, 1) + " dB"; }
juce::String msString (float v, int) { return juce::String (v, v < 10.0f ? 1 : 0) + " ms"; }
juce::String percentString (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; }
juce::String semiString (float v, int) { return juce::String (v, 1) + " st"; }
juce::String hzString (float v, int)
{
    return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz";
}

juce::NormalisableRange<float> logRange (float lo, float hi)
{
    return juce::NormalisableRange<float> (lo, hi,
        [] (float s, float e, float n) { return s * std::pow (e / s, n); },
        [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); });
}
} // namespace

KickReadyProcessor::KickReadyProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "KickReady", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout KickReadyProcessor::createLayout()
{
    using namespace juce;
    using P = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;

    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::follow, 1 }, "Follow MIDI", true));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::baseNote, 1 }, "Note", Choices::noteNames(), 36 - Choices::firstNote));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::tune, 1 }, "Tune",
                                     NormalisableRange<float> (-12.0f, 12.0f, 0.01f), 0.0f, A().withLabel ("st").withStringFromValueFunction (semiString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::glide, 1 }, "Glide",
                                     NormalisableRange<float> (0.0f, 500.0f, 1.0f), 0.0f, A().withLabel ("ms").withStringFromValueFunction (msString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::velocity, 1 }, "Velocity",
                                     NormalisableRange<float> (0.0f, 100.0f, 1.0f), 50.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));

    layout.add (std::make_unique<P> (ParameterID { ParamIDs::pitchStart, 1 }, "Pitch Start",
                                     NormalisableRange<float> (0.0f, 72.0f, 0.1f), 36.0f, A().withLabel ("st").withStringFromValueFunction (semiString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::pitchDecay, 1 }, "Pitch Decay",
                                     logRange (5.0f, 1000.0f), 60.0f, A().withLabel ("ms").withStringFromValueFunction (msString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::pitchCurve, 1 }, "Pitch Curve",
                                     NormalisableRange<float> (0.0f, 100.0f, 1.0f), 50.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));

    layout.add (std::make_unique<P> (ParameterID { ParamIDs::attack, 1 }, "Attack",
                                     NormalisableRange<float> (0.0f, 20.0f, 0.1f), 0.1f, A().withLabel ("ms").withStringFromValueFunction (msString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::hold, 1 }, "Hold",
                                     NormalisableRange<float> (0.0f, 300.0f, 1.0f), 0.0f, A().withLabel ("ms").withStringFromValueFunction (msString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::decay, 1 }, "Decay",
                                     logRange (20.0f, 3000.0f), 400.0f, A().withLabel ("ms").withStringFromValueFunction (msString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::ampCurve, 1 }, "Decay Curve",
                                     NormalisableRange<float> (0.0f, 100.0f, 1.0f), 70.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::clickMode, 1 }, "Click Mode", Choices::clickModes, 0));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::clickLevel, 1 }, "Click",
                                     NormalisableRange<float> (0.0f, 100.0f, 1.0f), 30.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::clickDecay, 1 }, "Click Decay",
                                     logRange (1.0f, 60.0f), 6.0f, A().withLabel ("ms").withStringFromValueFunction (msString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::clickTone, 1 }, "Click Tone",
                                     logRange (200.0f, 12000.0f), 2500.0f, A().withLabel ("Hz").withStringFromValueFunction (hzString)));

    layout.add (std::make_unique<P> (ParameterID { ParamIDs::drive, 1 }, "Drive",
                                     NormalisableRange<float> (0.0f, 36.0f, 0.1f), 0.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::clip, 1 }, "Clip",
                                     NormalisableRange<float> (0.0f, 100.0f, 1.0f), 0.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::sub, 1 }, "Sub",
                                     NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::highCut, 1 }, "High Cut",
                                     logRange (200.0f, 20000.0f), 20000.0f, A().withLabel ("Hz").withStringFromValueFunction (hzString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::output, 1 }, "Output",
                                     NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    return layout;
}

int KickReadyProcessor::getBaseNote() const
{
    return Choices::firstNote + juce::jlimit (0, Choices::numNotes - 1, juce::roundToInt (p (ParamIDs::baseNote)->load()));
}

KickParams KickReadyProcessor::readParams() const
{
    KickParams q;
    q.followMidi = p (ParamIDs::follow)->load() > 0.5f;
    q.baseNote = getBaseNote();
    q.tuneSemitones = p (ParamIDs::tune)->load();
    q.glideMs = p (ParamIDs::glide)->load();
    q.velocitySens = p (ParamIDs::velocity)->load() * 0.01f;
    q.pitchStartSemis = p (ParamIDs::pitchStart)->load();
    q.pitchDecayMs = p (ParamIDs::pitchDecay)->load();
    q.pitchCurve = p (ParamIDs::pitchCurve)->load() * 0.01f;
    q.ampAttackMs = p (ParamIDs::attack)->load();
    q.ampHoldMs = p (ParamIDs::hold)->load();
    q.ampDecayMs = p (ParamIDs::decay)->load();
    q.ampCurve = p (ParamIDs::ampCurve)->load() * 0.01f;
    q.clickMode = p (ParamIDs::clickMode)->load() > 0.5f ? ClickMode::tick : ClickMode::noise;
    q.clickLevel = p (ParamIDs::clickLevel)->load() * 0.01f;
    q.clickDecayMs = p (ParamIDs::clickDecay)->load();
    q.clickToneHz = p (ParamIDs::clickTone)->load();
    q.driveDb = p (ParamIDs::drive)->load();
    q.clip = p (ParamIDs::clip)->load() * 0.01f;
    q.subDb = p (ParamIDs::sub)->load();
    q.highCutHz = p (ParamIDs::highCut)->load();
    q.outputDb = p (ParamIDs::output)->load();
    return q;
}

void KickReadyProcessor::applyPreset (const KickPreset& pr)
{
    auto setValue = [this] (const char* id, float value)
    {
        if (auto* param = state.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };
    setValue (ParamIDs::baseNote, (float) (pr.baseNote - Choices::firstNote));
    setValue (ParamIDs::tune, 0.0f);
    setValue (ParamIDs::pitchStart, pr.pitchStart);
    setValue (ParamIDs::pitchDecay, pr.pitchDecay);
    setValue (ParamIDs::pitchCurve, pr.pitchCurve * 100.0f);
    setValue (ParamIDs::attack, pr.attack);
    setValue (ParamIDs::hold, pr.hold);
    setValue (ParamIDs::decay, pr.decay);
    setValue (ParamIDs::ampCurve, pr.ampCurve * 100.0f);
    setValue (ParamIDs::clickMode, (float) pr.clickMode);
    setValue (ParamIDs::clickLevel, pr.clickLevel * 100.0f);
    setValue (ParamIDs::clickDecay, pr.clickDecay);
    setValue (ParamIDs::clickTone, pr.clickTone);
    setValue (ParamIDs::drive, pr.drive);
    setValue (ParamIDs::clip, pr.clip * 100.0f);
    setValue (ParamIDs::sub, pr.sub);
    setValue (ParamIDs::highCut, pr.highCut);
    setValue (ParamIDs::output, 0.0f);
}

void KickReadyProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    const int block = std::max (16, samplesPerBlock);
    engine.prepare (sampleRate, block);
    engine.setParams (readParams(), true);
    mono.assign ((size_t) block, 0.0f);
}

void KickReadyProcessor::reset()
{
    engine.reset();
}

bool KickReadyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void KickReadyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    if ((int) mono.size() < n)
        mono.assign ((size_t) n, 0.0f);

    engine.setParams (readParams());

    if (uiTrigger.exchange (false))
    {
        engine.noteOn (getBaseNote(), 1.0f);
        lastNote.store (getBaseNote());
    }

    // Render between MIDI events so every note starts on its exact sample.
    int pos = 0;
    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();
        const int at = juce::jlimit (pos, n, metadata.samplePosition);
        if (at > pos)
        {
            engine.render (mono.data() + pos, at - pos);
            pos = at;
        }
        if (msg.isNoteOn())
        {
            engine.noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
            lastNote.store (engine.getParams().followMidi ? msg.getNoteNumber() : getBaseNote());
        }
    }
    if (pos < n)
        engine.render (mono.data() + pos, n - pos);

    float peak = 0.0f;
    for (int i = 0; i < n; ++i)
        peak = std::max (peak, std::fabs (mono[(size_t) i]));
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, mono.data(), n);

    sounding.store (engine.isSounding());
    float prev = outputPeak.load();
    while (peak > prev && ! outputPeak.compare_exchange_weak (prev, peak)) {}
}

std::vector<float> KickReadyProcessor::renderPreview (double sampleRate)
{
    KickEngine preview;
    preview.prepare (sampleRate, 512);
    preview.setParams (readParams(), true);
    return preview.renderOneShot (getBaseNote(), 1.0f, 4.0);
}

bool KickReadyProcessor::exportWav (const juce::File& file)
{
    const double sr = currentSampleRate > 1000.0 ? currentSampleRate : 48000.0;
    auto samples = renderPreview (sr);
    if (samples.empty())
        return false;

    file.deleteFile();
    std::unique_ptr<juce::OutputStream> stream = file.createOutputStream();
    if (stream == nullptr)
        return false;

    juce::WavAudioFormat wav;
    auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions().withSampleRate (sr).withNumChannels (1).withBitsPerSample (24));
    if (writer == nullptr)
        return false;

    juce::AudioBuffer<float> buffer (1, (int) samples.size());
    buffer.copyFrom (0, 0, samples.data(), (int) samples.size());
    return writer->writeFromAudioSampleBuffer (buffer, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* KickReadyProcessor::createEditor()
{
    return rommuser::ui::createScalableEditor<KickReadyEditor> (*this);
}

void KickReadyProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void KickReadyProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KickReadyProcessor();
}
