FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.16.1
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(Catch2)

add_executable(dsp_tests
    test_oversampler.cpp
    test_clipready.cpp
    test_shapeeq.cpp
    test_pressready.cpp
    test_duckready.cpp
    test_bandpush.cpp
    test_warmdrive.cpp
    test_gateready.cpp
    test_echoready.cpp
    test_phaseready.cpp
    test_wideready.cpp
    test_spaceready.cpp
    test_restame.cpp)

target_link_libraries(dsp_tests PRIVATE rommuser_dsp Catch2::Catch2WithMain)

if(MSVC)
    target_compile_options(dsp_tests PRIVATE /W4 /fp:precise)
else()
    target_compile_options(dsp_tests PRIVATE -Wall -Wextra)
endif()

include(${catch2_SOURCE_DIR}/extras/Catch.cmake)
catch_discover_tests(dsp_tests)
