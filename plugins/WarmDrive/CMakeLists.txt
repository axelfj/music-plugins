# Each plugin is versioned on its own: plugins/<Name>/VERSION holds the full version (e.g. 1.0.0-alpha.2)
# and the release tag is <slug>-v<that version>.
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION WARM_DRIVE_VERSION LIMIT_COUNT 1)

music_plugin(WarmDrive
    NAME "Warm Drive"
    CODE Wmdr
    VERSION ${WARM_DRIVE_VERSION}
    CLAP_FEATURES audio-effect distortion mixing
    VST3_CATEGORIES Fx Distortion
    SOURCES
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
