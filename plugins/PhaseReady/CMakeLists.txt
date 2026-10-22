# Each plugin is versioned on its own: plugins/<Name>/VERSION holds the full version (e.g. 1.0.0-alpha.2)
# and the release tag is <slug>-v<that version>.
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION PHASE_READY_VERSION LIMIT_COUNT 1)

music_plugin(PhaseReady
    NAME "Phase Ready"
    CODE Phrd
    VERSION ${PHASE_READY_VERSION}
    CLAP_FEATURES audio-effect phaser modulation
    VST3_CATEGORIES Fx Modulation
    SOURCES
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
