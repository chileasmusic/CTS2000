cmake_minimum_required(VERSION 3.22)

# Runtime Visual C++ lié statiquement : le .vst3 n'a pas besoin des redistribuables Microsoft.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")

project(CTS2000 VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(CTS2000
    COMPANY_NAME                "CTS2000Emu"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    PLUGIN_MANUFACTURER_CODE    Ctse
    PLUGIN_CODE                 Cts2
    FORMATS                     VST3 Standalone
    VST3_CATEGORIES             Instrument Synth
    PRODUCT_NAME                "CTS2000")

target_sources(CTS2000 PRIVATE
    Source/Engine.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(CTS2000 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(CTS2000
    PRIVATE juce::juce_audio_utils
    PUBLIC  juce::juce_recommended_config_flags)

# Test du moteur sans JUCE (facultatif) : cmake -DCTS_BUILD_TESTS=ON
option(CTS_BUILD_TESTS "Build engine test" OFF)
if(CTS_BUILD_TESTS)
    add_executable(engine_test tests/engine_test.cpp Source/Engine.cpp)
    target_include_directories(engine_test PRIVATE Source)
endif()
