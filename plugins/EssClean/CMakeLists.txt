# Each plugin is versioned on its own: plugins/<Name>/VERSION holds the full version (e.g. 1.0.0-alpha.2)
# and the release tag is <slug>-v<that version>.
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION ESS_CLEAN_VERSION LIMIT_COUNT 1)

music_plugin(EssClean
    NAME "Ess Clean"
    CODE Escl
    VERSION ${ESS_CLEAN_VERSION}
    CLAP_FEATURES audio-effect de-esser mixing
    VST3_CATEGORIES Fx Dynamics
    SOURCES
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
