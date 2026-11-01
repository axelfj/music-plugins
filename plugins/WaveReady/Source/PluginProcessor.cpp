// SPDX-License-Identifier: GPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rommuser::waveready;

namespace
{
juce::String dbString (float v, int) { return juce::String (v, 1) + " dB"; }
juce::String msString (float v, int)
{
    if (v >= 1000.0f) return juce::String (v / 1000.0f, 2) + " s";
    return juce::String (v, v < 10.0f ? 1 : 0) + " ms";
}
juce::String percentString (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; }
juce::String semiString (float v, int) { return juce::String (v, 1) + " st"; }
juce::String centString (float v, int) { return juce::String (juce::roundToInt (v)) + " ct"; }
juce::String octString (float v, int) { return juce::String (v, 2) + " oct"; }
juce::String secString (float v, int) { return juce::String (v, 1) + " s"; }
juce::String hzString (float v, int)
{
    if (v >= 1000.0f) return juce::String (v / 1000.0f, 2) + " kHz";
    return v < 10.0f ? juce::String (v, 2) + " Hz" : juce::String (juce::roundToInt (v)) + " Hz";
}
juce::String intString (float v, int) { return juce::String (juce::roundToInt (v)); }
juce::String panString (float v, int)
{
    const int i = juce::roundToInt (v);
    if (i == 0) return "C";
    return i < 0 ? juce::String (-i) + " L" : juce::String (i) + " R";
}

juce::NormalisableRange<float> logRange (float lo, float hi)
{
    return juce::NormalisableRange<float> (lo, hi,
        [] (float s, float e, float n) { return s * std::pow (e / s, n); },
        [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); });
}
} // namespace

//==============================================================================
const std::vector<WavePreset>& wavePresets()
{
    using namespace ParamIDs;
    static const std::vector<WavePreset> presets = {
        { "INIT", {} },
        { "SUPERSAW LEAD", { { oscWave[0], 2 }, { oscUnison[0], 7 }, { oscDetune[0], 28 }, { oscWidth[0], 90 }, { oscLevel[0], 70 },
                             { oscOn[1], 1 }, { oscWave[1], 2 }, { oscOct[1], 1 }, { oscUnison[1], 5 }, { oscDetune[1], 22 }, { oscLevel[1], 35 },
                             { cutoff, 12000 }, { resonance, 10 }, { envAttack[0], 8 }, { envRelease[0], 400 },
                             { chorusMix, 30 }, { reverbMix, 22 }, { reverbDecay, 2.5f }, { delayMix, 18 }, { output, -9 } } },
        { "PLUCK", { { oscWave[0], 2 }, { oscMorph[0], 0 }, { oscUnison[0], 3 }, { oscDetune[0], 12 }, { oscLevel[0], 80 },
                     { cutoff, 600 }, { resonance, 25 }, { envToCutoff, 3.2f }, { velToCutoff, 1.0f }, { keyTrack, 50 },
                     { envAttack[0], 1 }, { envDecay[0], 300 }, { envSustain[0], 0 }, { envRelease[0], 300 },
                     { envAttack[1], 0.5f }, { envDecay[1], 120 }, { envSustain[1], 0 }, { envRelease[1], 120 },
                     { delayMix, 25 }, { delayFeedback, 35 }, { reverbMix, 15 } } },
        { "REESE BASS", { { oscWave[0], 2 }, { oscOct[0], -1 }, { oscUnison[0], 2 }, { oscDetune[0], 18 }, { oscWidth[0], 0 }, { oscLevel[0], 80 },
                          { oscOn[1], 1 }, { oscWave[1], 2 }, { oscOct[1], -1 }, { oscFine[1], 9 }, { oscLevel[1], 70 }, { oscWidth[1], 0 },
                          { filterMode, 1 }, { cutoff, 900 }, { resonance, 5 }, { drive, 8 },
                          { envAttack[0], 5 }, { envRelease[0], 120 }, { mono, 1 }, { glide, 60 }, { output, -8 } } },
        { "SUB 808", { { oscWave[0], 0 }, { oscOct[0], -2 }, { oscLevel[0], 90 }, { oscRetrig[0], 1 },
                       { subLevel, 0 }, { envToPitch, 14 }, { envAttack[1], 0.5f }, { envDecay[1], 60 }, { envSustain[1], 0 }, { envRelease[1], 60 },
                       { envAttack[0], 1 }, { envDecay[0], 1500 }, { envSustain[0], 60 }, { envRelease[0], 300 },
                       { drive, 12 }, { cutoff, 2500 }, { mono, 1 }, { glide, 40 }, { output, -4 } } },
        { "FUTURE CHORD", { { oscWave[0], 2 }, { oscUnison[0], 5 }, { oscDetune[0], 16 }, { oscWidth[0], 80 }, { oscLevel[0], 60 },
                            { oscOn[1], 1 }, { oscWave[1], 3 }, { oscMorph[1], 40 }, { oscLevel[1], 40 }, { oscOct[1], -1 },
                            { cutoff, 500 }, { resonance, 15 }, { envToCutoff, 3.0f }, { keyTrack, 30 },
                            { envAttack[0], 2 }, { envDecay[0], 600 }, { envSustain[0], 20 }, { envRelease[0], 500 },
                            { envAttack[1], 1 }, { envDecay[1], 250 }, { envSustain[1], 0 }, { envRelease[1], 250 },
                            { chorusMix, 25 }, { reverbMix, 28 }, { reverbDecay, 3.0f }, { output, -9 } } },
        { "PAD", { { oscWave[0], 2 }, { oscMorph[0], 50 }, { oscUnison[0], 5 }, { oscDetune[0], 14 }, { oscWidth[0], 100 }, { oscLevel[0], 60 },
                   { oscOn[1], 1 }, { oscWave[1], 1 }, { oscOct[1], -1 }, { oscLevel[1], 50 },
                   { cutoff, 1800 }, { resonance, 10 }, { lfo1ToCutoff, 0.6f }, { lfoRate[0], 0.15f },
                   { envAttack[0], 900 }, { envDecay[0], 2000 }, { envSustain[0], 80 }, { envRelease[0], 1800 },
                   { chorusMix, 45 }, { reverbMix, 45 }, { reverbDecay, 5.0f }, { output, -10 } } },
        { "ORGAN", { { oscWave[0], 6 }, { oscLevel[0], 80 }, { oscOn[1], 1 }, { oscWave[1], 0 }, { oscOct[1], 1 }, { oscLevel[1], 35 },
                     { cutoff, 9000 }, { envAttack[0], 2 }, { envDecay[0], 100 }, { envSustain[0], 100 }, { envRelease[0], 60 },
                     { lfo1ToPitch, 0.08f }, { lfoRate[0], 6.5f }, { lfo1ToAmp, 12 }, { chorusMix, 35 }, { reverbMix, 18 } } },
        { "ARP PLUCK", { { oscWave[0], 3 }, { oscMorph[0], 100 }, { oscLevel[0], 80 }, { oscOn[1], 1 }, { oscWave[1], 0 }, { oscOct[1], 1 }, { oscLevel[1], 30 },
                         { cutoff, 400 }, { resonance, 30 }, { envToCutoff, 3.8f }, { velToCutoff, 1.5f },
                         { envAttack[0], 0.5f }, { envDecay[0], 200 }, { envSustain[0], 0 }, { envRelease[0], 200 },
                         { envAttack[1], 0.5f }, { envDecay[1], 90 }, { envSustain[1], 0 }, { envRelease[1], 90 },
                         { delayMix, 35 }, { delayDiv, 6 }, { delayFeedback, 45 }, { reverbMix, 20 } } },
        { "ACID", { { oscWave[0], 2 }, { oscLevel[0], 85 }, { filterMode, 1 }, { cutoff, 300 }, { resonance, 70 }, { filterDrive, 40 },
                    { envToCutoff, 3.5f }, { velToCutoff, 1.5f }, { envAttack[1], 0.5f }, { envDecay[1], 220 }, { envSustain[1], 0 }, { envRelease[1], 220 },
                    { envAttack[0], 1 }, { envDecay[0], 300 }, { envSustain[0], 50 }, { envRelease[0], 80 },
                    { mono, 1 }, { legato, 1 }, { glide, 70 }, { drive, 10 }, { delayMix, 15 } } },
        { "HOOVER", { { oscWave[0], 2 }, { oscMorph[0], 40 }, { oscUnison[0], 7 }, { oscDetune[0], 45 }, { oscWidth[0], 70 }, { oscLevel[0], 70 },
                      { oscOn[1], 1 }, { oscWave[1], 4 }, { oscOct[1], -1 }, { oscUnison[1], 3 }, { oscDetune[1], 30 }, { oscLevel[1], 50 },
                      { envToPitch, -7 }, { envAttack[1], 1 }, { envDecay[1], 350 }, { envSustain[1], 0 }, { envRelease[1], 350 },
                      { cutoff, 6000 }, { resonance, 10 }, { envAttack[0], 10 }, { envRelease[0], 500 }, { chorusMix, 40 }, { reverbMix, 20 }, { output, -9 } } },
        { "BELL", { { oscWave[0], 8 }, { oscLevel[0], 70 }, { oscOn[1], 1 }, { oscWave[1], 0 }, { oscOct[1], 2 }, { oscLevel[1], 25 },
                    { cutoff, 7000 }, { envToCutoff, 2.0f }, { envAttack[0], 1 }, { envDecay[0], 1800 }, { envSustain[0], 0 }, { envRelease[0], 1200 },
                    { envAttack[1], 1 }, { envDecay[1], 900 }, { envSustain[1], 0 }, { envRelease[1], 900 },
                    { delayMix, 25 }, { delayDiv, 6 }, { reverbMix, 40 }, { reverbDecay, 4.0f } } },
        { "WOBBLE", { { oscWave[0], 2 }, { oscOct[0], -1 }, { oscLevel[0], 80 }, { oscOn[1], 1 }, { oscWave[1], 3 }, { oscOct[1], -2 }, { oscLevel[1], 60 },
                      { filterMode, 1 }, { cutoff, 250 }, { resonance, 35 }, { lfoSync[0], 1 }, { lfoDiv[0], 5 }, { lfoShape[0], 1 }, { lfo1ToCutoff, 3.5f },
                      { envAttack[0], 2 }, { envSustain[0], 100 }, { envRelease[0], 80 }, { mono, 1 }, { drive, 6 }, { output, -6 } } },
    };
    return presets;
}

//==============================================================================
WaveReadyProcessor::WaveReadyProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "WaveReady", createLayout())
{
    for (auto& s : scope) s.store (0.0f);
    for (auto* param : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
            state.addParameterListener (ranged->getParameterID(), this);
}

WaveReadyProcessor::~WaveReadyProcessor()
{
    for (auto* param : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
            state.removeParameterListener (ranged->getParameterID(), this);
}

// Any edit by the user turns the preset display into "(edited)".
void WaveReadyProcessor::parameterChanged (const juce::String&, float)
{
    if (! applyingPreset.load())
        lastPreset.store (-1);
}

juce::AudioProcessorValueTreeState::ParameterLayout WaveReadyProcessor::createLayout()
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

    for (int o = 0; o < 2; ++o)
    {
        const juce::String n = "Osc " + juce::String (o + 1) + " ";
        b (oscOn[o], (n + "On").toRawUTF8(), o == 0);
        c (oscWave[o], (n + "Wave").toRawUTF8(), Choices::waves(), 2);
        f (oscMorph[o], (n + "Morph").toRawUTF8(), lin (0, 100, 1), 0, "%", percentString);
        c (oscOct[o], (n + "Octave").toRawUTF8(), Choices::octaves, 2);
        f (oscSemi[o], (n + "Semi").toRawUTF8(), lin (-12, 12, 1), 0, "st", intString);
        f (oscFine[o], (n + "Fine").toRawUTF8(), lin (-100, 100, 1), 0, "ct", centString);
        f (oscLevel[o], (n + "Level").toRawUTF8(), lin (0, 100, 1), o == 0 ? 80 : 60, "%", percentString);
        f (oscPan[o], (n + "Pan").toRawUTF8(), lin (-100, 100, 1), 0, "", panString);
        f (oscUnison[o], (n + "Unison").toRawUTF8(), lin (1, 7, 1), 1, "", intString);
        f (oscDetune[o], (n + "Detune").toRawUTF8(), lin (0, 100, 1), 20, "ct", centString);
        f (oscWidth[o], (n + "Width").toRawUTF8(), lin (0, 100, 1), 70, "%", percentString);
        f (oscPhase[o], (n + "Phase").toRawUTF8(), lin (0, 100, 1), 0, "%", percentString);
        b (oscRetrig[o], (n + "Retrig").toRawUTF8(), false);
    }

    f (subLevel, "Sub Level", lin (0, 100, 1), 0, "%", percentString);
    c (subOct, "Sub Octave", Choices::subOctaves, 0);
    b (subSquare, "Sub Square", false);
    f (noiseLevel, "Noise Level", lin (0, 100, 1), 0, "%", percentString);
    c (noiseColour, "Noise Colour", Choices::noiseColours, 0);

    c (filterMode, "Filter Mode", Choices::filterModes, 1);
    f (cutoff, "Cutoff", logRange (20, 20000), 8000, "Hz", hzString);
    f (resonance, "Resonance", lin (0, 100, 1), 20, "%", percentString);
    f (filterDrive, "Filter Drive", lin (0, 100, 1), 0, "%", percentString);
    f (keyTrack, "Key Track", lin (0, 100, 1), 0, "%", percentString);
    f (envToCutoff, "Env > Cutoff", lin (-5, 5, 0.01f), 0, "oct", octString);
    f (velToCutoff, "Vel > Cutoff", lin (0, 5, 0.01f), 0, "oct", octString);
    f (envToPitch, "Env > Pitch", lin (-24, 24, 0.1f), 0, "st", semiString);

    for (int e = 0; e < 2; ++e)
    {
        const juce::String n = e == 0 ? "Amp " : "Mod ";
        f (envAttack[e], (n + "Attack").toRawUTF8(), logRange (0.1f, 10000), 5, "ms", msString);
        f (envDecay[e], (n + "Decay").toRawUTF8(), logRange (1, 10000), 200, "ms", msString);
        f (envSustain[e], (n + "Sustain").toRawUTF8(), lin (0, 100, 1), 80, "%", percentString);
        f (envRelease[e], (n + "Release").toRawUTF8(), logRange (1, 10000), 200, "ms", msString);
    }
    f (velToAmp, "Vel > Amp", lin (0, 100, 1), 50, "%", percentString);

    for (int l = 0; l < 2; ++l)
    {
        const juce::String n = "LFO " + juce::String (l + 1) + " ";
        c (lfoShape[l], (n + "Shape").toRawUTF8(), Choices::lfoShapes, 0);
        b (lfoSync[l], (n + "Sync").toRawUTF8(), false);
        f (lfoRate[l], (n + "Rate").toRawUTF8(), logRange (0.01f, 40), l == 0 ? 5.0f : 0.5f, "Hz", hzString);
        c (lfoDiv[l], (n + "Division").toRawUTF8(), Choices::divisions(), 4);
        f (lfoFade[l], (n + "Fade").toRawUTF8(), lin (0, 5000, 1), 0, "ms", msString);
        b (lfoRetrig[l], (n + "Retrig").toRawUTF8(), true);
    }
    f (lfo1ToPitch, "LFO1 > Pitch", lin (0, 12, 0.01f), 0, "st", semiString);
    f (lfo1ToCutoff, "LFO1 > Cutoff", lin (-5, 5, 0.01f), 0, "oct", octString);
    f (lfo1ToAmp, "LFO1 > Amp", lin (0, 100, 1), 0, "%", percentString);
    f (lfo2ToMorph, "LFO2 > Morph", lin (0, 100, 1), 0, "%", percentString);
    f (lfo2ToCutoff, "LFO2 > Cutoff", lin (-5, 5, 0.01f), 0, "oct", octString);
    f (lfo2ToPitch, "LFO2 > Pitch", lin (0, 12, 0.01f), 0, "st", semiString);
    f (wheelToCutoff, "Wheel > Cutoff", lin (0, 5, 0.01f), 0, "oct", octString);
    f (wheelToVibrato, "Wheel > Vibrato", lin (0, 2, 0.01f), 0.5f, "st", semiString);

    b (mono, "Mono", false);
    b (legato, "Legato", true);
    f (glide, "Glide", lin (0, 2000, 1), 0, "ms", msString);
    f (bendRange, "Bend Range", lin (0, 24, 1), 2, "st", intString);
    f (tune, "Tune", lin (-100, 100, 1), 0, "ct", centString);

    f (drive, "Drive", lin (0, 36, 0.1f), 0, "dB", dbString);
    f (chorusMix, "Chorus Mix", lin (0, 100, 1), 0, "%", percentString);
    f (chorusRate, "Chorus Rate", logRange (0.05f, 10), 0.6f, "Hz", hzString);
    f (chorusDepth, "Chorus Depth", lin (0, 100, 1), 50, "%", percentString);
    f (delayMix, "Delay Mix", lin (0, 100, 1), 0, "%", percentString);
    b (delaySync, "Delay Sync", true);
    c (delayDiv, "Delay Division", Choices::divisions(), 5);
    f (delayTime, "Delay Time", logRange (1, 4000), 375, "ms", msString);
    f (delayFeedback, "Delay Feedback", lin (0, 95, 1), 40, "%", percentString);
    b (delayPingPong, "Delay Ping-Pong", true);
    f (delayTone, "Delay Tone", logRange (200, 20000), 6000, "Hz", hzString);
    f (reverbMix, "Reverb Mix", lin (0, 100, 1), 0, "%", percentString);
    f (reverbDecay, "Reverb Decay", logRange (0.1f, 30), 2, "s", secString);
    b (reverbPlate, "Reverb Plate", false);
    f (output, "Output", lin (-24, 12, 0.1f), -6, "dB", dbString);
    return layout;
}

WaveParams WaveReadyProcessor::readParams() const
{
    using namespace ParamIDs;
    WaveParams q;
    auto v = [this] (const char* id) { return p (id)->load(); };
    auto on = [this] (const char* id) { return p (id)->load() > 0.5f; };
    auto ix = [this] (const char* id) { return juce::roundToInt (p (id)->load()); };

    for (int o = 0; o < 2; ++o)
    {
        OscParams& s = o == 0 ? q.osc1 : q.osc2;
        s.on = on (oscOn[o]);
        s.wave = ix (oscWave[o]);
        s.morph = v (oscMorph[o]) * 0.01f;
        s.octave = ix (oscOct[o]) - 2;
        s.semi = ix (oscSemi[o]);
        s.fine = v (oscFine[o]);
        s.level = v (oscLevel[o]) * 0.01f;
        s.pan = v (oscPan[o]) * 0.01f;
        s.unison = std::clamp (ix (oscUnison[o]), 1, maxUnison);
        s.detune = v (oscDetune[o]);
        s.width = v (oscWidth[o]) * 0.01f;
        s.phase = v (oscPhase[o]) * 0.01f;
        s.retrig = on (oscRetrig[o]);
    }
    q.subLevel = v (subLevel) * 0.01f;
    q.subOctave = ix (subOct) + 1;
    q.subSquare = on (subSquare);
    q.noiseLevel = v (noiseLevel) * 0.01f;
    q.noiseColour = ix (noiseColour) == 1 ? NoiseColour::pink : NoiseColour::white;

    q.filterMode = static_cast<FilterMode> (std::clamp (ix (filterMode), 0, 3));
    q.cutoffHz = v (cutoff);
    q.resonance = v (resonance) * 0.01f;
    q.filterDrive = v (filterDrive) * 0.01f;
    q.keyTrack = v (keyTrack) * 0.01f;
    q.envToCutoff = v (envToCutoff);
    q.velToCutoff = v (velToCutoff);
    q.envToPitch = v (envToPitch);

    for (int e = 0; e < 2; ++e)
    {
        EnvParams& env = e == 0 ? q.ampEnv : q.modEnv;
        env.attackMs = v (envAttack[e]);
        env.decayMs = v (envDecay[e]);
        env.sustain = v (envSustain[e]) * 0.01f;
        env.releaseMs = v (envRelease[e]);
    }
    q.velToAmp = v (velToAmp) * 0.01f;

    for (int l = 0; l < 2; ++l)
    {
        LfoParams& lfo = l == 0 ? q.lfo1 : q.lfo2;
        lfo.shape = static_cast<LfoShape> (std::clamp (ix (lfoShape[l]), 0, 4));
        lfo.sync = on (lfoSync[l]);
        lfo.rateHz = v (lfoRate[l]);
        lfo.division = ix (lfoDiv[l]);
        lfo.fadeMs = v (lfoFade[l]);
        lfo.retrig = on (lfoRetrig[l]);
    }
    q.lfo1ToPitch = v (lfo1ToPitch);
    q.lfo1ToCutoff = v (lfo1ToCutoff);
    q.lfo1ToAmp = v (lfo1ToAmp) * 0.01f;
    q.lfo2ToMorph = v (lfo2ToMorph) * 0.01f;
    q.lfo2ToCutoff = v (lfo2ToCutoff);
    q.lfo2ToPitch = v (lfo2ToPitch);
    q.wheelToCutoff = v (wheelToCutoff);
    q.wheelToVibrato = v (wheelToVibrato);

    q.mono = on (mono);
    q.legato = on (legato);
    q.glideMs = v (glide);
    q.bendRange = v (bendRange);
    q.tune = v (tune);

    q.driveDb = v (drive);
    q.chorusMix = v (chorusMix) * 0.01f;
    q.chorusRateHz = v (chorusRate);
    q.chorusDepth = v (chorusDepth) * 0.01f;
    q.delayMix = v (delayMix) * 0.01f;
    q.delaySync = on (delaySync);
    q.delayDivision = ix (delayDiv);
    q.delayMs = v (delayTime);
    q.delayFeedback = v (delayFeedback) * 0.01f;
    q.delayPingPong = on (delayPingPong);
    q.delayToneHz = v (delayTone);
    q.reverbMix = v (reverbMix) * 0.01f;
    q.reverbDecay = v (reverbDecay);
    q.reverbPlate = on (reverbPlate);
    q.outputDb = v (output);
    q.bpm = lastBpm;
    return q;
}

void WaveReadyProcessor::applyPreset (const WavePreset& pr)
{
    // Start from the defaults, then apply the preset's values.
    applyingPreset.store (true);
    for (auto* param : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
            ranged->setValueNotifyingHost (ranged->getDefaultValue());
    for (const auto& [id, value] : pr.values)
        if (auto* param = state.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    applyingPreset.store (false);
}

void WaveReadyProcessor::applyPreset (int index)
{
    const auto& list = wavePresets();
    if (index < 0 || index >= (int) list.size()) return;
    applyPreset (list[(size_t) index]);
    lastPreset.store (index);
}

void WaveReadyProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    const int block = std::max (16, samplesPerBlock);
    engine.prepare (sampleRate, block);
    engine.setParams (readParams(), true);
    left.assign ((size_t) block, 0.0f);
    right.assign ((size_t) block, 0.0f);
    keyboardState.reset();
}

void WaveReadyProcessor::reset()
{
    engine.reset();
}

bool WaveReadyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void WaveReadyProcessor::handleMidi (const juce::MidiMessage& msg)
{
    if (msg.isNoteOn())
        engine.noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
    else if (msg.isNoteOff())
        engine.noteOff (msg.getNoteNumber());
    else if (msg.isPitchWheel())
        engine.setPitchBend ((float) (msg.getPitchWheelValue() - 8192) / 8192.0f * engine.getParams().bendRange);
    else if (msg.isController())
    {
        const int cc = msg.getControllerNumber();
        if (cc == 1) engine.setModWheel ((float) msg.getControllerValue() / 127.0f);
        else if (cc == 64) engine.setSustainPedal (msg.getControllerValue() >= 64);
        else if (cc == 120) engine.allSoundOff();
        else if (cc == 123) engine.allNotesOff();
    }
    else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        engine.allSoundOff();
}

void WaveReadyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    if ((int) left.size() < n)
    {
        left.assign ((size_t) n, 0.0f);
        right.assign ((size_t) n, 0.0f);
    }

    if (auto* head = getPlayHead())
        if (auto pos = head->getPosition())
            if (auto bpm = pos->getBpm())
                if (*bpm > 20.0 && *bpm < 999.0)
                    lastBpm = (float) *bpm;

    keyboardState.processNextMidiBuffer (midi, 0, n, true);
    engine.setParams (readParams());

    // Render between MIDI events so every note starts on its exact sample.
    int pos = 0;
    for (const auto metadata : midi)
    {
        const int at = juce::jlimit (pos, n, metadata.samplePosition);
        if (at > pos)
        {
            engine.render (left.data() + pos, right.data() + pos, at - pos);
            pos = at;
        }
        handleMidi (metadata.getMessage());
    }
    if (pos < n)
        engine.render (left.data() + pos, right.data() + pos, n - pos);

    float peak = 0.0f;
    int w = scopeWrite.load (std::memory_order_relaxed);
    for (int i = 0; i < n; ++i)
    {
        const float l = left[(size_t) i], r = right[(size_t) i];
        peak = std::max (peak, std::max (std::fabs (l), std::fabs (r)));
        scope[(size_t) w].store (0.5f * (l + r), std::memory_order_relaxed);
        w = (w + 1) & (scopeSize - 1);
    }
    scopeWrite.store (w, std::memory_order_release);

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

    const auto& m = engine.getMeters();
    activeVoices.store (m.activeVoices);
    displayCutoff.store (m.cutoffHz);
    float prev = outputPeak.load();
    while (peak > prev && ! outputPeak.compare_exchange_weak (prev, peak)) {}
}

juce::AudioProcessorEditor* WaveReadyProcessor::createEditor()
{
    return rommuser::ui::createScalableEditor<WaveReadyEditor> (*this);
}

void WaveReadyProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void WaveReadyProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WaveReadyProcessor();
}
