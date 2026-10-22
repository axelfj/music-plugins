# music_plugin(<target>
#     NAME "<Product Name>"
#     CODE <4-char plugin code, unique per plugin, first letter uppercase>
#     VERSION <x.y.z[-prerelease]>   (full distribution version, as in plugins/<Name>/VERSION)
#     [MIDI_INPUT]                   (the plugin wants MIDI note input, e.g. as a trigger)
#     CLAP_FEATURES <clap features...>
#     VST3_CATEGORIES <categories...>
#     SOURCES <files...>)
#
# One call per plugin: VST3, AU (macOS), CLAP and Standalone with the shared ROMMUSER settings.
function(music_plugin target)
    cmake_parse_arguments(ARG "MIDI_INPUT" "NAME;CODE;VERSION" "CLAP_FEATURES;VST3_CATEGORIES;SOURCES" ${ARGN})

    # Hosts and installers want a numeric version; the GUI and file names show the full one.
    string(REGEX REPLACE "-.*$" "" numeric_version "${ARG_VERSION}")

    set(midi_input FALSE)
    if(ARG_MIDI_INPUT)
        set(midi_input TRUE)
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
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT ${midi_input}
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_CATEGORIES ${ARG_VST3_CATEGORIES}
        AU_MAIN_TYPE kAudioUnitType_Effect)

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
endfunction()
