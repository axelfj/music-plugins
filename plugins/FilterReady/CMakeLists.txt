# Each plugin is versioned on its own: plugins/<Name>/VERSION holds the full version (e.g. 1.0.0-alpha.2)
# and the release tag is <slug>-v<that version>.
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION FILTER_READY_VERSION LIMIT_COUNT 1)

music_plugin(FilterReady
    NAME "Filter-R"
    CODE Fltr
    VERSION ${FILTER_READY_VERSION}
    CLAP_FEATURES audio-effect filter modulation
    VST3_CATEGORIES Fx Filter
    SOURCES
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
