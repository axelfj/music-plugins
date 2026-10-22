# Each plugin is versioned on its own: plugins/<Name>/VERSION holds the full version (e.g. 1.0.0-alpha.2)
# and the release tag is <slug>-v<that version>.
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION TUNE_READY_VERSION LIMIT_COUNT 1)

music_plugin(TuneReady
    NAME "Tune Ready"
    CODE Tnrd
    VERSION ${TUNE_READY_VERSION}
    MIDI_INPUT
    CLAP_FEATURES audio-effect pitch-correction pitch-shifter
    VST3_CATEGORIES Fx Pitch\ Shift
    SOURCES
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
