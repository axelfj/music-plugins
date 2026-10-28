# Each plugin is versioned on its own: plugins/<Name>/VERSION holds the full version (e.g. 1.0.0-alpha.2)
# and the release tag is <slug>-v<that version>.
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION LOW_MONO_VERSION LIMIT_COUNT 1)

music_plugin(LowMono
    NAME "Stereo-R"
    CODE Lwmn
    VERSION ${LOW_MONO_VERSION}
    CLAP_FEATURES audio-effect utility mastering
    VST3_CATEGORIES Fx Spatial Mastering
    SOURCES
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
