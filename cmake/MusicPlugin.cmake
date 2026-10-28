# music_plugin(<target>
#     NAME "<Product Name>"
#     CODE <4-char plugin code, unique per plugin, first letter uppercase>
#     VERSION <x.y.z[-prerelease]>   (full distribution version, as in plugins/<Name>/VERSION)
#     [MIDI_INPUT]                   (the plugin wants MIDI note input, e.g. as a trigger)
#     [IS_SYNTH]                     (instrument: MIDI input, no audio input, AU MusicDevice)
#     [MIDI_OUTPUT]                  (the plugin also sends MIDI, e.g. a chord or arpeggio tool;
#                                     combine with IS_SYNTH so FL Studio / Ableton load it as an
#                                     instrument whose MIDI output can be routed to another track)
#     CLAP_FEATURES <clap features...>
#     VST3_CATEGORIES <categories...>
#     SOURCES <files...>)
#
# One call per plugin: VST3, AU (macOS), CLAP and Standalone with the shared ROMMUSER settings.
function(music_plugin target)
    cmake_parse_arguments(ARG "MIDI_INPUT;IS_SYNTH;MIDI_OUTPUT" "NAME;CODE;VERSION" "CLAP_FEATURES;VST3_CATEGORIES;SOURCES" ${ARGN})

    # Hosts and installers want a numeric version; the GUI and file names show the full one.
    string(REGEX REPLACE "-.*$" "" numeric_version "${ARG_VERSION}")

    set(midi_input FALSE)
    set(is_synth FALSE)
    set(midi_output FALSE)
    set(au_type kAudioUnitType_Effect)
    if(ARG_MIDI_OUTPUT)
        set(midi_output TRUE)
    endif()
    if(ARG_MIDI_INPUT)
        set(midi_input TRUE)
    endif()
    if(ARG_IS_SYNTH)
        set(midi_input TRUE)
        set(is_synth TRUE)
        set(au_type kAudioUnitType_MusicDevice)
    endif()

    set(formats VST3 Standalone)
    if(APPLE)
        list(APPEND formats AU)
    endif()

    juce_add_plugin(${target}
        COMPANY_NAME "ROMMUSER"
        COMPANY_WEBSITE "https://rommuser.com"
        BUNDLE_ID "com.rommuser.${target}"
        PRODUCT_NAME "${ARG_NAME}"
        VERSION "${numeric_version}"
        PLUGIN_MANUFACTURER_CODE Rmmr
        PLUGIN_CODE ${ARG_CODE}
        FORMATS ${formats}
        IS_SYNTH ${is_synth}
        NEEDS_MIDI_INPUT ${midi_input}
        NEEDS_MIDI_OUTPUT ${midi_output}
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_CATEGORIES ${ARG_VST3_CATEGORIES}
        AU_MAIN_TYPE ${au_type})

    clap_juce_extensions_plugin(TARGET ${target}
        CLAP_ID "com.rommuser.${target}"
        CLAP_FEATURES ${ARG_CLAP_FEATURES})

    target_sources(${target} PRIVATE ${ARG_SOURCES})

    target_compile_definitions(${target} PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        PLUGIN_VERSION_STRING="${ARG_VERSION}")

    target_link_libraries(${target}
        PRIVATE
            rommuser_dsp
            juce::juce_audio_utils
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)

    # README gallery: a small app that renders this plugin's editor to a PNG (tools/screenshot).
    # It reuses the plugin's shared code, the same way JUCE's format wrappers do.
    if(MUSIC_PLUGINS_BUILD_SCREENSHOTS)
        add_executable(${target}_Screenshot ${PROJECT_SOURCE_DIR}/tools/screenshot/Screenshot.cpp)
        target_compile_definitions(${target}_Screenshot PRIVATE
            $<TARGET_PROPERTY:${target},COMPILE_DEFINITIONS>)
        target_include_directories(${target}_Screenshot PRIVATE
            $<TARGET_PROPERTY:${target},INCLUDE_DIRECTORIES>)
        target_compile_options(${target}_Screenshot PRIVATE
            $<TARGET_PROPERTY:${target},COMPILE_OPTIONS>)
        target_link_libraries(${target}_Screenshot PRIVATE ${target})
    endif()
endfunction()
