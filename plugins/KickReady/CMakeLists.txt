# Each plugin is versioned on its own: plugins/<Name>/VERSION holds the full version (e.g. 1.0.0-alpha.2)
# and the release tag is <slug>-v<that version>.
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION KICK_READY_VERSION LIMIT_COUNT 1)

music_plugin(KickReady
    NAME "Kick-R"
    CODE Kkrd
    VERSION ${KICK_READY_VERSION}
    IS_SYNTH
    CLAP_FEATURES instrument drum synthesizer
    VST3_CATEGORIES Instrument Drum Synth
    SOURCES
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
