# Each plugin is versioned on its own (plugins/<Name>/VERSION) and released with a <slug>-vX.Y.Z tag.
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION CLIP_READY_VERSION LIMIT_COUNT 1)

music_plugin(ClipReady
    NAME "Clip Ready"
    CODE Clpr
    VERSION ${CLIP_READY_VERSION}
    CLAP_FEATURES audio-effect distortion mastering
    VST3_CATEGORIES Fx Dynamics
    SOURCES
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
