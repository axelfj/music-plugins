// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rommuser::stutterready;

namespace
{
juce::String dbString (float v, int) { return juce::String (v, 1) + " dB"; }
juce::String percentString (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; }
juce::String semiString (float v, int) { return (v > 0.0f ? "+" : "") + juce::String (v, 1) + " st"; }
juce::String beatsString (float v, int)
{
    if (v < 0.99f) return "1/" + juce::String (juce::roundToInt (4.0f / v));
    const bool whole = std::fabs (v - std::round (v)) < 0.01f;
    return juce::String (v, whole ? 0 : 2) + (v > 1.5f ? " beats" : " beat");
}

const juce::String stepsProperty = "steps";
} // namespace

StutterReadyProcessor::StutterReadyProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "StutterReady", createLayout())
{
    for (auto& s : steps) s.store (-1);
    for (auto& w : waveform) w.store (0.0f);
    storeStepsToState();
}

juce::AudioProcessorValueTreeState::ParameterLayout StutterReadyProcessor::createLayout()
{
    using namespace juce;
    using P = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;

    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::ramp, 1 }, "Ramp",
                                     NormalisableRange<float> (0.25f, 16.0f, 0.25f, 0.5f), 2.0f, A().withStringFromValueFunction (beatsString)));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::quantize, 1 }, "Quantize", Choices::quantize, 2));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::seqOn, 1 }, "Sequencer", false));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::seqBars, 1 }, "Bars", Choices::bars, 0));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::mix, 1 }, "Mix",
                                     NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f, A().withLabel ("%").withStringFromValueFunction (percentString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::output, 1 }, "Output",
                                     NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));

    for (int g = 0; g < numGestures; ++g)
    {
        const Gesture& d = builtInGesture (g);
        const String name = String (Choices::gestureNamesEn[g]) + " ";
        auto id = [g] (const char* suffix) { return ParameterID { ParamIDs::gesture (g, suffix), 1 }; };
        layout.add (std::make_unique<AudioParameterChoice> (id ("divstart"), name + "Division", Choices::divisions, d.divStart));
        layout.add (std::make_unique<AudioParameterChoice> (id ("divend"), name + "Division end", Choices::divisions, d.divEnd));
        layout.add (std::make_unique<P> (id ("pitch"), name + "Pitch", NormalisableRange<float> (-24.0f, 24.0f, 1.0f), d.pitchSemis,
                                         A().withLabel ("st").withStringFromValueFunction (semiString)));
        layout.add (std::make_unique<AudioParameterBool> (id ("reverse"), name + "Reverse", d.reverse));
        layout.add (std::make_unique<AudioParameterBool> (id ("tapestop"), name + "Tape stop", d.tapeStop));
        layout.add (std::make_unique<P> (id ("gate"), name + "Gate", NormalisableRange<float> (0.0f, 100.0f, 1.0f), d.gate * 100.0f,
                                         A().withLabel ("%").withStringFromValueFunction (percentString)));
        layout.add (std::make_unique<AudioParameterChoice> (id ("filter"), name + "Filter", Choices::filters, (int) d.filter));
        layout.add (std::make_unique<P> (id ("filteramt"), name + "Filter amount", NormalisableRange<float> (0.0f, 100.0f, 1.0f), d.filterAmount * 100.0f,
                                         A().withLabel ("%").withStringFromValueFunction (percentString)));
        layout.add (std::make_unique<P> (id ("pan"), name + "Pan", NormalisableRange<float> (0.0f, 100.0f, 1.0f), d.pan * 100.0f,
                                         A().withLabel ("%").withStringFromValueFunction (percentString)));
        layout.add (std::make_unique<P> (id ("crush"), name + "Crush", NormalisableRange<float> (0.0f, 100.0f, 1.0f), d.crush * 100.0f,
                                         A().withLabel ("%").withStringFromValueFunction (percentString)));
        layout.add (std::make_unique<P> (id ("echo"), name + "Echo", NormalisableRange<float> (0.0f, 95.0f, 1.0f), d.echo * 100.0f,
                                         A().withLabel ("%").withStringFromValueFunction (percentString)));
    }
    return layout;
}

StutterParams StutterReadyProcessor::readParams() const
{
    StutterParams q;
    auto choice = [this] (const juce::String& id, int count) { return juce::jlimit (0, count - 1, juce::roundToInt (p (id)->load())); };
    q.rampBeats = p (ParamIDs::ramp)->load();
    q.quantize = choice (ParamIDs::quantize, 4);
    q.seqEnabled = p (ParamIDs::seqOn)->load() > 0.5f;
    q.seqBars = Choices::barsValues[choice (ParamIDs::seqBars, 3)];
    q.mix = p (ParamIDs::mix)->load() * 0.01f;
    q.outputDb = p (ParamIDs::output)->load();
    for (int s = 0; s < maxSteps; ++s)
        q.steps[(size_t) s] = (int8_t) steps[(size_t) s].load();
    for (int g = 0; g < numGestures; ++g)
    {
        auto& ge = q.gestures[(size_t) g];
        ge.divStart = choice (ParamIDs::gesture (g, "divstart"), numDivisions);
        ge.divEnd = choice (ParamIDs::gesture (g, "divend"), numDivisions);
        ge.pitchSemis = p (ParamIDs::gesture (g, "pitch"))->load();
        ge.reverse = p (ParamIDs::gesture (g, "reverse"))->load() > 0.5f;
        ge.tapeStop = p (ParamIDs::gesture (g, "tapestop"))->load() > 0.5f;
        ge.gate = p (ParamIDs::gesture (g, "gate"))->load() * 0.01f;
        ge.filter = (FilterType) choice (ParamIDs::gesture (g, "filter"), 3);
        ge.filterAmount = p (ParamIDs::gesture (g, "filteramt"))->load() * 0.01f;
        ge.pan = p (ParamIDs::gesture (g, "pan"))->load() * 0.01f;
        ge.crush = p (ParamIDs::gesture (g, "crush"))->load() * 0.01f;
        ge.echo = p (ParamIDs::gesture (g, "echo"))->load() * 0.01f;
    }
    return q;
}

void StutterReadyProcessor::applyPreset (const StutterPreset& pr)
{
    auto setValue = [this] (const char* id, float value)
    {
        if (auto* param = state.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };
    const int barsIndex = pr.bars >= 4 ? 2 : pr.bars == 2 ? 1 : 0;
    setValue (ParamIDs::seqBars, (float) barsIndex);
    setValue (ParamIDs::ramp, pr.rampBeats);
    const juce::String pattern (pr.pattern);
    const int total = stepsPerBar * pr.bars;
    for (int s = 0; s < maxSteps; ++s)
    {
        int g = -1;
        if (pattern.isNotEmpty() && s < total)
        {
            const auto ch = pattern[s % pattern.length()];
            if (ch >= '0' && ch <= '7') g = (int) (ch - '0');
        }
        steps[(size_t) s].store (g);
    }
    storeStepsToState();
    setValue (ParamIDs::seqOn, pattern.isNotEmpty() ? 1.0f : 0.0f);
}

void StutterReadyProcessor::setStep (int index, int gesture)
{
    if (index < 0 || index >= maxSteps) return;
    steps[(size_t) index].store (juce::jlimit (-1, numGestures - 1, gesture));
    storeStepsToState();
}

void StutterReadyProcessor::clearSteps()
{
    for (auto& s : steps) s.store (-1);
    storeStepsToState();
}

void StutterReadyProcessor::storeStepsToState()
{
    juce::String text;
    for (int s = 0; s < maxSteps; ++s)
    {
        const int g = steps[(size_t) s].load();
        text << (g < 0 ? '.' : (juce::juce_wchar) ('0' + g));
    }
    state.state.setProperty (stepsProperty, text, nullptr);
}

void StutterReadyProcessor::loadStepsFromState()
{
    const juce::String text = state.state.getProperty (stepsProperty, "").toString();
    for (int s = 0; s < maxSteps; ++s)
    {
        int g = -1;
        if (s < text.length())
        {
            const auto ch = text[s];
            if (ch >= '0' && ch <= '7') g = (int) (ch - '0');
        }
        steps[(size_t) s].store (g);
    }
}

void StutterReadyProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    engine.prepare (sampleRate, std::max (16, std::min (samplesPerBlock, 8192)), getMainBusNumInputChannels());
    engine.setParams (readParams());
    setLatencySamples (0);
}

void StutterReadyProcessor::reset()
{
    engine.reset();
}

bool StutterReadyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void StutterReadyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
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
    playing.store (t.valid && t.playing);

    engine.setParams (readParams());

    const int req = uiRequest.exchange (0);
    if (req > 0) engine.uiPress (req - 1);
    else if (req < 0) engine.uiRelease();

    // MIDI notes trigger gestures (note % 12 < 8); render between events so timing is exact.
    int pos = 0;
    auto renderUpTo = [&] (int end)
    {
        while (pos < end)
        {
            const int len = std::min (end - pos, 8192);
            float* ptrs[maxChannels] = {};
            for (int c = 0; c < numIn; ++c)
                ptrs[c] = buffer.getWritePointer (c) + pos;
            Transport slice = t;
            slice.ppqPosition = t.ppqPosition + (double) pos * t.bpm / 60.0 / currentSampleRate;
            engine.process (ptrs, numIn, len, slice);
            pos += len;
        }
    };
    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();
        renderUpTo (juce::jlimit (0, n, metadata.samplePosition));
        if (msg.isNoteOn()) engine.noteOn (msg.getNoteNumber());
        else if (msg.isNoteOff()) engine.noteOff (msg.getNoteNumber());
        else if (msg.isAllNotesOff() || msg.isAllSoundOff()) engine.allNotesOff();
    }
    renderUpTo (n);

    for (int ch = numIn; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);

    const auto& m = engine.getMeters();
    active.store (m.active);
    currentGesture.store (m.gesture);
    currentStep.store (m.step);
    progress.store (m.progress);
    divisionNow.store (m.divisionBeatsNow);
    wetPeak.store (m.wetPeak);

    // Waveform of the last two beats, peak per bin (coarse scan, ~30 times per second).
    waveformCountdown -= n;
    if (waveformCountdown <= 0)
    {
        waveformCountdown = (int) (currentSampleRate / 30.0);
        const double bpm = t.valid && t.bpm > 20.0 ? t.bpm : 120.0;
        const int64_t window = (int64_t) (2.0 * currentSampleRate * 60.0 / bpm);
        const int64_t end = engine.getInputCount();
        const int64_t start = end - window;
        const int stride = std::max (1, (int) (window / (waveformLength * 8)));
        for (int b = 0; b < waveformLength; ++b)
        {
            const int64_t from = start + window * b / waveformLength;
            const int64_t to = start + window * (b + 1) / waveformLength;
            float peak = 0.0f;
            for (int64_t i = from; i < to; i += stride)
                peak = std::max (peak, std::fabs (engine.readMono (i)));
            waveform[(size_t) b].store (peak);
        }
        const int64_t sliceStart = engine.getSliceStart();
        sliceStartFraction.store (sliceStart < 0 ? -1.0f : (float) ((double) (sliceStart - start) / (double) window));
        sliceLengthFraction.store ((float) (engine.getSliceLength() / (double) window));
    }
}

juce::AudioProcessorEditor* StutterReadyProcessor::createEditor()
{
    return new StutterReadyEditor (*this);
}

void StutterReadyProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void StutterReadyProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
        {
            state.replaceState (juce::ValueTree::fromXml (*xml));
            loadStepsFromState();
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StutterReadyProcessor();
}
