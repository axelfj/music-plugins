// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rommuser::essclean;

namespace
{
juce::String dbString (float v, int) { return juce::String (v, 1) + " dB"; }
juce::String msString (float v, int) { return juce::String (v, v < 1.0f ? 2 : v < 10.0f ? 1 : 0) + " ms"; }
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

EssCleanProcessor::EssCleanProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "EssClean", createLayout())
{
    pLow = state.getRawParameterValue (ParamIDs::low);
    pHigh = state.getRawParameterValue (ParamIDs::high);
    pThreshold = state.getRawParameterValue (ParamIDs::threshold);
    pRange = state.getRawParameterValue (ParamIDs::range);
    pAttack = state.getRawParameterValue (ParamIDs::attack);
    pRelease = state.getRawParameterValue (ParamIDs::release);
    pMode = state.getRawParameterValue (ParamIDs::mode);
    pLookahead = state.getRawParameterValue (ParamIDs::lookahead);
    pListen = state.getRawParameterValue (ParamIDs::listen);

    fft.prepare (fftOrder);
    for (int i = 0; i < fftSize; ++i)
        window[(size_t) i] = 0.5f - 0.5f * std::cos (2.0f * juce::MathConstants<float>::pi * (float) i / (float) fftSize);
    for (auto& b : spectrumDb)
        b.store (-120.0f);
}

juce::AudioProcessorValueTreeState::ParameterLayout EssCleanProcessor::createLayout()
{
    using namespace juce;
    using P = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;

    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::low, 1 }, "Low",
                                     logRange (2000.0f, 12000.0f), 4500.0f, A().withLabel ("Hz").withStringFromValueFunction (hzString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::high, 1 }, "High",
                                     logRange (3000.0f, 16000.0f), 9000.0f, A().withLabel ("Hz").withStringFromValueFunction (hzString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::threshold, 1 }, "Threshold",
                                     NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -24.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::range, 1 }, "Range",
                                     NormalisableRange<float> (0.0f, 24.0f, 0.1f), 10.0f, A().withLabel ("dB").withStringFromValueFunction (dbString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::attack, 1 }, "Attack",
                                     logRange (0.1f, 20.0f), 1.0f, A().withLabel ("ms").withStringFromValueFunction (msString)));
    layout.add (std::make_unique<P> (ParameterID { ParamIDs::release, 1 }, "Release",
                                     logRange (10.0f, 500.0f), 80.0f, A().withLabel ("ms").withStringFromValueFunction (msString)));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::mode, 1 }, "Mode", Choices::mode, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::lookahead, 1 }, "Lookahead", Choices::lookahead, 0));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::listen, 1 }, "Listen", false));
    return layout;
}

EssParams EssCleanProcessor::readParams() const
{
    EssParams p;
    p.lowHz = pLow->load();
    p.highHz = pHigh->load();
    p.thresholdDb = pThreshold->load();
    p.rangeDb = pRange->load();
    p.attackMs = pAttack->load();
    p.releaseMs = pRelease->load();
    p.mode = (Mode) juce::jlimit (0, Choices::mode.size() - 1, juce::roundToInt (pMode->load()));
    p.lookaheadMs = Choices::lookaheadValues[juce::jlimit (0, Choices::lookahead.size() - 1, juce::roundToInt (pLookahead->load()))];
    p.listen = pListen->load() > 0.5f;
    return p;
}

void EssCleanProcessor::applyPreset (const EssPreset& preset)
{
    auto setValue = [this] (const char* id, float value)
    {
        if (auto* param = state.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };
    setValue (ParamIDs::low, preset.lowHz);
    setValue (ParamIDs::high, preset.highHz);
    setValue (ParamIDs::threshold, preset.thresholdDb);
    setValue (ParamIDs::range, preset.rangeDb);
    setValue (ParamIDs::listen, 0.0f);
}

void EssCleanProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    engine.prepare (sampleRate, getMainBusNumInputChannels());
    engine.setParams (readParams(), true);
    reportedLatency = engine.getLatencySamples();
    setLatencySamples (reportedLatency);
    fifoPos = 0;
}

void EssCleanProcessor::reset()
{
    engine.reset();
}

bool EssCleanProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void EssCleanProcessor::pushSpectrumSample (float x)
{
    fifo[(size_t) fifoPos++] = x;
    if (fifoPos < fftSize)
        return;
    fifoPos = 0;
    for (int i = 0; i < fftSize; ++i)
        fftData[(size_t) i] = { fifo[(size_t) i] * window[(size_t) i], 0.0f };
    fft.forward (fftData.data());
    const float norm = 2.0f / ((float) fftSize * 0.5f);   // Hann coherent gain 0.5
    for (int k = 0; k < numBins; ++k)
    {
        const float mag = std::abs (fftData[(size_t) k]) * norm;
        const float db = mag > 1.0e-6f ? 20.0f * std::log10 (mag) : -120.0f;
        // A little ballistics so the display does not flicker.
        const float prev = spectrumDb[(size_t) k].load();
        spectrumDb[(size_t) k].store (db > prev ? db : std::max (db, prev - 6.0f));
    }
}

void EssCleanProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int numIn = std::min (getMainBusNumInputChannels(), maxChannels);

    // Spectrum of the input (mono sum) for the display.
    if (numIn > 0)
    {
        const float scale = 1.0f / (float) numIn;
        for (int i = 0; i < n; ++i)
        {
            float s = 0.0f;
            for (int c = 0; c < numIn; ++c)
                s += buffer.getReadPointer (c)[i];
            pushSpectrumSample (s * scale);
        }
    }

    engine.setParams (readParams());
    const int latency = engine.getLatencySamples();
    if (latency != reportedLatency)
    {
        reportedLatency = latency;
        setLatencySamples (latency);
    }

    const auto m = engine.process (buffer.getArrayOfWritePointers(), numIn, n);

    for (int ch = numIn; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);

    auto storeMax = [] (std::atomic<float>& a, float v)
    {
        float prev = a.load();
        while (v > prev && ! a.compare_exchange_weak (prev, v)) {}
    };
    storeMax (reductionDb, m.reductionDb);
    storeMax (inputPeak, m.inputPeak);
    storeMax (outputPeak, m.outputPeak);
    levelDb.store (m.levelDb);
}

juce::AudioProcessorEditor* EssCleanProcessor::createEditor()
{
    return rommuser::ui::createScalableEditor<EssCleanEditor> (*this);
}

void EssCleanProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void EssCleanProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EssCleanProcessor();
}
