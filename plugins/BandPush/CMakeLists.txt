# Each plugin is versioned on its own: plugins/<Name>/VERSION holds the full version (e.g. 1.0.0-alpha.2)
# and the release tag is <slug>-v<that version>.
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION BAND_PUSH_VERSION LIMIT_COUNT 1)

music_plugin(BandPush
    NAME "Multiband-R"
    CODE Bdph
    VERSION ${BAND_PUSH_VERSION}
    CLAP_FEATURES audio-effect compressor multi-effects mixing
    VST3_CATEGORIES Fx Dynamics
    SOURCES
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
