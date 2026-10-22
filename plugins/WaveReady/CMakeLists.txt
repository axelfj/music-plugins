# Each plugin is versioned on its own: plugins/<Name>/VERSION holds the full version (e.g. 1.0.0-alpha.2)
# and the release tag is <slug>-v<that version>.
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION WAVE_READY_VERSION LIMIT_COUNT 1)

music_plugin(WaveReady
    NAME "Wave Ready"
    CODE Wvrd
    VERSION ${WAVE_READY_VERSION}
    IS_SYNTH
    CLAP_FEATURES instrument synthesizer stereo
    VST3_CATEGORIES Instrument Synth
    SOURCES
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
