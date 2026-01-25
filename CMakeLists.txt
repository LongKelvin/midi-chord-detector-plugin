cmake_minimum_required(VERSION 3.15)

project(MidiChordDetector VERSION 1.0.0)

# JUCE Framework Setup
# Priority order:
# 1. JUCE_DIR from command line (-DJUCE_DIR=...)
# 2. Environment variable JUCE_DIR
# 3. Project Resources folder (./Resources/JUCE)
# 4. Legacy: C:\Audio\juce-8.0.12-windows\JUCE
# 5. Legacy: C:\Audio\JUCE

if(NOT DEFINED JUCE_DIR)
    # Try environment variable
    if(DEFINED ENV{JUCE_DIR})
        set(JUCE_DIR "$ENV{JUCE_DIR}" CACHE PATH "Path to JUCE framework")
        message(STATUS "Using JUCE from environment variable: ${JUCE_DIR}")
    # Try Resources folder (same directory as CMakeLists.txt)
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/JUCE/CMakeLists.txt")
        set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Resources/JUCE" CACHE PATH "Path to JUCE framework")
        get_filename_component(JUCE_DIR "${JUCE_DIR}" ABSOLUTE)
        message(STATUS "Found JUCE in project Resources folder: ${JUCE_DIR}")
    # Try default Windows location (version specific) - legacy
    elseif(EXISTS "C:/Audio/juce-8.0.12-windows/JUCE/CMakeLists.txt")
        set(JUCE_DIR "C:/Audio/juce-8.0.12-windows/JUCE" CACHE PATH "Path to JUCE framework")
        message(STATUS "Found JUCE at legacy location: ${JUCE_DIR}")
    # Try default Windows location (generic) - legacy
    elseif(EXISTS "C:/Audio/JUCE/CMakeLists.txt")
        set(JUCE_DIR "C:/Audio/JUCE" CACHE PATH "Path to JUCE framework")
        message(STATUS "Found JUCE at legacy location: ${JUCE_DIR}")
    else()
        message(FATAL_ERROR 
            "JUCE not found! Please specify JUCE_DIR:\n"
            "  cmake -DJUCE_DIR=\"C:/Path/To/JUCE\" ..\n"
            "Or set environment variable:\n"
            "  set JUCE_DIR=C:\\Path\\To\\JUCE\n"
            "Or run the build script which will auto-download JUCE to Resources/JUCE\n"
            "Download from: https://github.com/juce-framework/JUCE")
    endif()
endif()

# Verify JUCE installation
if(NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR 
        "JUCE CMakeLists.txt not found at: ${JUCE_DIR}\n"
        "Please ensure JUCE is properly installed at this location.")
endif()

message(STATUS "Using JUCE from: ${JUCE_DIR}")
add_subdirectory(${JUCE_DIR} JUCE EXCLUDE_FROM_ALL)

# VST3 SDK is included with JUCE
juce_add_plugin(MidiChordDetector
    COMPANY_NAME "MusicProgrammings"
    PLUGIN_MANUFACTURER_CODE Mprg
    PLUGIN_CODE Mcdt
    FORMATS VST3
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PRODUCT_NAME "MIDI Chord Detector"
    VST3_CATEGORIES "Instrument"
)

# Source files
target_sources(MidiChordDetector
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/core/MidiNoteState.cpp
        Source/core/TimeWindowBuffer.cpp
        Source/core/ChordDetector.cpp
        Source/core/CandidateGenerator.cpp
        Source/core/HarmonicMemory.cpp
        Source/core/ConfidenceScorer.cpp
        Source/core/ChordResolver.cpp
        Source/core/ChordDetectorEngine.cpp
        Source/ui/ChordDisplayComponent.cpp
)

# Compile definitions
target_compile_definitions(MidiChordDetector
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
)

# Link JUCE modules
target_link_libraries(MidiChordDetector
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# C++17 required
target_compile_features(MidiChordDetector PRIVATE cxx_std_17)

# Include directories
target_include_directories(MidiChordDetector
    PRIVATE
        Source
)
