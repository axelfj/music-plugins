// Renders a plugin's editor to a PNG for the README gallery.
//
//     <Plugin>_Screenshot <output.png> [scale] [paramID=value ...]
//
// Built once per plugin when MUSIC_PLUGINS_BUILD_SCREENSHOTS is ON (see cmake/MusicPlugin.cmake).
// It links the plugin's shared code, feeds it a short drum + bass loop (or a held chord for
// instruments) so meters and analysers show something, then snapshots the editor offscreen.
// paramID=value pairs (plain units, e.g. semitones=7) set parameters first, for plugins whose
// defaults would show nothing happening; render.sh reads them from plugins/<Name>/SCREENSHOT_PARAMS.
// On Linux run it under xvfb-run; tools/screenshot/render.sh does all of this for every plugin.
//
// Themes: ROMMUSER_THEME=<name> opens with that theme. ROMMUSER_SWITCH_THEME=<name> switches to
// it while the editor is open (checks live re-theming), and ROMMUSER_THEME_EDITOR=1 opens the
// theme editor before the snapshot.

#include <juce_audio_utils/juce_audio_utils.h>

#include "ui/ScalableEditor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

namespace
{
constexpr double sampleRate = 48000.0;
constexpr int blockSize = 480;           // 10 ms
constexpr int warmUpMs = 2500;           // audio + UI time before the snapshot

// A 120 BPM loop: kick on every beat, hat on the off-beats, a saw bass on the root.
float testSignal (int64_t n, int channel)
{
    const double t = (double) n / sampleRate;
    const double beat = 0.5;
    const double tb = std::fmod (t, beat);
    const double toff = std::fmod (t + beat * 0.5, beat);

    const double kickPhase = 2.0 * juce::MathConstants<double>::pi * (45.0 * tb + 90.0 * (1.0 - std::exp (-tb * 30.0)) / 30.0);
    const double kick = std::sin (kickPhase) * std::exp (-tb * 9.0);

    static juce::Random random (1234);
    const double hat = (random.nextDouble() * 2.0 - 1.0) * std::exp (-toff * 60.0) * 0.25;

    const double bassFreq = 55.0;
    const double saw = 2.0 * std::fmod (t * bassFreq + (channel == 0 ? 0.0 : 0.01), 1.0) - 1.0;
    const double bass = saw * 0.18 * (0.6 + 0.4 * std::exp (-tb * 4.0));

    return (float) (0.55 * kick + hat + bass);
}

class Renderer : private juce::Timer
{
public:
    Renderer (juce::File outputFile, float snapshotScale, const juce::StringArray& params) : output (outputFile), scale (snapshotScale)
    {
        processor.reset (createPluginFilter());
        for (const auto& pair : params)
            for (auto* param : processor->getParameters())
                if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
                    if (ranged->paramID == pair.upToFirstOccurrenceOf ("=", false, false))
                        ranged->setValueNotifyingHost (ranged->convertTo0to1 (pair.fromFirstOccurrenceOf ("=", false, false).getFloatValue()));
        processor->enableAllBuses();
        processor->setRateAndBufferSizeDetails (sampleRate, blockSize);
        processor->prepareToPlay (sampleRate, blockSize);

        buffer.setSize (juce::jmax (2, processor->getTotalNumInputChannels(), processor->getTotalNumOutputChannels()), blockSize);
        isInstrument = processor->getMainBusNumInputChannels() == 0;

        editor.reset (processor->createEditorIfNeeded());
        if (editor == nullptr)
        {
            std::cerr << "the plugin has no editor" << std::endl;
            finish (false);
            return;
        }

        startMs = juce::Time::getMillisecondCounter();
        startTimer (10);
    }

    ~Renderer() override
    {
        stopTimer();
        editor.reset();
        processor->releaseResources();
    }

    bool succeeded() const { return ok; }

private:
    void timerCallback() override
    {
        // Keep the audio roughly in real time so the editor's meters see a steady signal.
        const auto elapsed = juce::Time::getMillisecondCounter() - startMs;
        while (samplesDone < (int64_t) (elapsed * sampleRate / 1000.0))
            processBlock();

        if (! switched && elapsed >= (juce::uint32) warmUpMs / 2)
        {
            switched = true;
            const auto name = juce::SystemStats::getEnvironmentVariable ("ROMMUSER_SWITCH_THEME", {});
            juce::SharedResourcePointer<rommuser::ui::ThemeManager> themes;
            for (const auto& t : themes->allThemes())
                if (name.isNotEmpty() && t.name == name)
                    themes->preview (t);

            if (juce::SystemStats::getEnvironmentVariable ("ROMMUSER_THEME_EDITOR", {}) == "1")
                if (auto* scalable = dynamic_cast<rommuser::ui::ScalableEditor*> (editor.get()))
                    scalable->showThemeEditor();
        }

        if (elapsed >= (juce::uint32) warmUpMs)
        {
            stopTimer();
            finish (save());
        }
    }

    void processBlock()
    {
        buffer.clear();
        juce::MidiBuffer midi;

        if (isInstrument)
        {
            // Re-strike a C minor chord every two seconds so instruments and MIDI tools stay busy.
            if (samplesDone % (int64_t) (sampleRate * 2) == 0)
                for (int note : { 48, 60, 63, 67 })
                    midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
        }
        else
        {
            for (int ch = 0; ch < juce::jmin (2, processor->getMainBusNumInputChannels()); ++ch)
                for (int i = 0; i < blockSize; ++i)
                    buffer.setSample (ch, i, testSignal (samplesDone + i, ch));
        }

        processor->processBlock (buffer, midi);
        samplesDone += blockSize;
    }

    bool save()
    {
        auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, scale);
        output.getParentDirectory().createDirectory();
        output.deleteFile();

        juce::FileOutputStream stream (output);
        juce::PNGImageFormat png;
        if (! stream.openedOk() || ! png.writeImageToStream (image, stream))
        {
            std::cerr << "could not write " << output.getFullPathName() << std::endl;
            return false;
        }

        std::cout << output.getFullPathName() << " (" << image.getWidth() << "x" << image.getHeight() << ")" << std::endl;
        return true;
    }

    void finish (bool success)
    {
        ok = success;
        juce::MessageManager::getInstance()->stopDispatchLoop();
    }

    std::unique_ptr<juce::AudioProcessor> processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor;
    juce::AudioBuffer<float> buffer;
    juce::File output;
    float scale = 1.0f;
    bool isInstrument = false, ok = false, switched = false;
    int64_t samplesDone = 0;
    juce::uint32 startMs = 0;
};
} // namespace

// The plugin's shared code forbids START_JUCE_APPLICATION, so this runs the message loop itself.
int main (int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "usage: " << argv[0] << " <output.png> [scale] [paramID=value ...]" << std::endl;
        return 1;
    }

    const auto output = juce::File::getCurrentWorkingDirectory().getChildFile (juce::String (argv[1]));
    const float scale = argc > 2 ? juce::jlimit (0.5f, 4.0f, juce::String (argv[2]).getFloatValue()) : 1.0f;

    juce::StringArray params;
    for (int i = 3; i < argc; ++i)
        params.add (juce::String (argv[i]));

    juce::ScopedJuceInitialiser_GUI juce;
    Renderer renderer (output, scale, params);
    juce::MessageManager::getInstance()->runDispatchLoop();
    return renderer.succeeded() ? 0 : 1;
}
