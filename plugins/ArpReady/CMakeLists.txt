# Each plugin is versioned on its own: plugins/<Name>/VERSION holds the full version (e.g. 1.0.0-alpha.2)
# and the release tag is <slug>-v<that version>.
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION ARP_READY_VERSION LIMIT_COUNT 1)

music_plugin(ArpReady
    NAME "Arp Ready"
    CODE Arpr
    VERSION ${ARP_READY_VERSION}
    IS_SYNTH
    MIDI_OUTPUT
    CLAP_FEATURES instrument note-effect stereo
    VST3_CATEGORIES Instrument Synth
    SOURCES
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
