# Each plugin is versioned on its own: plugins/<Name>/VERSION holds the full version (e.g. 1.0.0-alpha.2)
# and the release tag is <slug>-v<that version>.
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION SPACE_READY_VERSION LIMIT_COUNT 1)

music_plugin(SpaceReady
    NAME "Reverb-R"
    CODE Sprd
    VERSION ${SPACE_READY_VERSION}
    CLAP_FEATURES audio-effect reverb
    VST3_CATEGORIES Fx Reverb
    SOURCES
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
