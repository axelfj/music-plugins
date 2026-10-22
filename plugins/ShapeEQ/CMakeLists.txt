# Each plugin is versioned on its own (plugins/<Name>/VERSION) and released with a <slug>-vX.Y.Z tag.
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION SHAPE_EQ_VERSION LIMIT_COUNT 1)

music_plugin(ShapeEQ
    NAME "Shape EQ"
    CODE Shpq
    VERSION ${SHAPE_EQ_VERSION}
    CLAP_FEATURES audio-effect equalizer mixing mastering
    VST3_CATEGORIES Fx EQ
    SOURCES
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)

# The analyzer uses JUCE's FFT.
target_link_libraries(ShapeEQ PRIVATE juce::juce_dsp)
