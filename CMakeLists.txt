cmake_minimum_required(VERSION 3.22)

# These must be set BEFORE project()
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures")
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
# Static MSVC runtime so users don't need the VC++ redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(GrannyDelay VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Path to the JUCE repository root (the folder that contains CMakeLists.txt, modules/, ...).
# CI clones it to ./JUCE
set(JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/JUCE" CACHE PATH "Path to the JUCE repository root")

if(NOT EXISTS "${JUCE_PATH}/CMakeLists.txt")
    message(FATAL_ERROR
        "JUCE not found at '${JUCE_PATH}'.\n"
        "Clone it:  git clone --depth 1 --branch 7.0.12 https://github.com/juce-framework/JUCE.git JUCE\n"
        "or pass -DJUCE_PATH=/path/to/JUCE")
endif()

add_subdirectory(${JUCE_PATH} JUCE EXCLUDE_FROM_ALL)

set(GD_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND GD_FORMATS AU)
endif()

juce_add_plugin(GrannyDelay
    COMPANY_NAME "Aron H"
    PRODUCT_NAME "GrannyDelay"
    PLUGIN_MANUFACTURER_CODE ArnH
    PLUGIN_CODE GrDy
    FORMATS ${GD_FORMATS}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Delay
    AU_MAIN_TYPE kAudioUnitType_Effect
    BUNDLE_ID com.aronh.grannydelay)

target_sources(GrannyDelay PRIVATE
    source/PluginProcessor.cpp
    source/PluginEditor.cpp
    source/GranularEngine.cpp
    source/UI/GranularVisualizer.cpp
    source/UI/PresetBar.cpp
    source/PresetManager.cpp
    source/FactoryPresets.cpp
    source/ModMatrix.cpp
    source/UI/ModSourceHandle.cpp
    source/UI/ModernKnob.cpp
    source/UI/AnimatedBackground.cpp)

target_include_directories(GrannyDelay PRIVATE source)

target_compile_definitions(GrannyDelay PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(GrannyDelay
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags)
