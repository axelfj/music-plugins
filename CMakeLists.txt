cmake_minimum_required(VERSION 3.22)

project(MusicPlugins VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
endif()

option(MUSIC_PLUGINS_BUILD_PLUGINS "Build the plugins (needs JUCE, fetched automatically)" ON)
option(MUSIC_PLUGINS_BUILD_TESTS "Build the headless DSP tests" ON)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# Header-only DSP shared by every plugin. Pure C++, no JUCE, so it can be tested on its own.
add_library(rommuser_dsp INTERFACE)
target_include_directories(rommuser_dsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/shared)

if(MUSIC_PLUGINS_BUILD_PLUGINS)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.15
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)

    FetchContent_Declare(clap-juce-extensions
        GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
        GIT_TAG 7adee3a1bd4684d4caa5601100e364abccff4b4d
        GIT_SUBMODULES_RECURSE ON)
    FetchContent_MakeAvailable(clap-juce-extensions)

    include(cmake/MusicPlugin.cmake)
    add_subdirectory(plugins/ClipReady)
    add_subdirectory(plugins/ShapeEQ)
    add_subdirectory(plugins/PressReady)
    add_subdirectory(plugins/DuckReady)
    add_subdirectory(plugins/BandPush)
    add_subdirectory(plugins/WarmDrive)
    add_subdirectory(plugins/GateReady)
    add_subdirectory(plugins/EchoReady)
    add_subdirectory(plugins/PhaseReady)
endif()

if(MUSIC_PLUGINS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
