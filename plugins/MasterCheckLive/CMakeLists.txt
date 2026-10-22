# Each plugin is versioned on its own: plugins/<Name>/VERSION holds the full version (e.g. 1.0.0-alpha.2)
# and the release tag is <slug>-v<that version>.
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION MASTER_CHECK_LIVE_VERSION LIMIT_COUNT 1)

music_plugin(MasterCheckLive
    NAME "Master Check Live"
    CODE Mckl
    VERSION ${MASTER_CHECK_LIVE_VERSION}
    CLAP_FEATURES audio-effect analyzer mastering
    VST3_CATEGORIES Fx Analyzer Mastering
    SOURCES
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
