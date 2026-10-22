# Each plugin is versioned on its own: plugins/<Name>/VERSION holds the full version (e.g. 1.0.0-alpha.2)
# and the release tag is <slug>-v<that version>.
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION DUCK_READY_VERSION LIMIT_COUNT 1)

music_plugin(DuckReady
    NAME "Duck Ready"
    CODE Dkrd
    VERSION ${DUCK_READY_VERSION}
    MIDI_INPUT
    CLAP_FEATURES audio-effect utility mixing
    VST3_CATEGORIES Fx Dynamics Modulation
    SOURCES
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
