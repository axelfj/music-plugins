# Each plugin is versioned on its own: plugins/<Name>/VERSION holds the full version (e.g. 1.0.0-alpha.2)
# and the release tag is <slug>-v<that version>.
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION PITCH_R_VERSION LIMIT_COUNT 1)

music_plugin(PitchR
    NAME "Pitch-R"
    CODE Ptcr
    VERSION ${PITCH_R_VERSION}
    CLAP_FEATURES audio-effect pitch-shifter
    VST3_CATEGORIES Fx Pitch\ Shift
    SOURCES
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)

# The display uses JUCE's FFT.
target_link_libraries(PitchR PRIVATE juce::juce_dsp)
