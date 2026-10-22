# Each plugin is versioned on its own: plugins/<Name>/VERSION holds the full version (e.g. 1.0.0-alpha.2)
# and the release tag is <slug>-v<that version>.
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION VOX_SYNTH_VERSION LIMIT_COUNT 1)

music_plugin(VoxSynth
    NAME "Vox Synth"
    CODE Vxsy
    VERSION ${VOX_SYNTH_VERSION}
    MIDI_INPUT
    CLAP_FEATURES audio-effect vocoder pitch-shifter
    VST3_CATEGORIES Fx Vocal
    SOURCES
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
