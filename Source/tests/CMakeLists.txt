# CMakeLists.txt for Chord Detection Tests
cmake_minimum_required(VERSION 3.15)
project(ChordDetectionTests VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files for the core chord detection (no JUCE dependency for tests)
set(CORE_SOURCES
    ../core/MidiNoteState.cpp
    ../core/CandidateGenerator.cpp
    ../core/ChordDetectorEngine.cpp
    ../core/ChordResolver.cpp
    ../core/ConfidenceScorer.cpp
    ../core/HarmonicMemory.cpp
)

# Test executable
add_executable(ChordDetectionTests
    ChordDetectionTests.cpp
    ${CORE_SOURCES}
)

# Include directories
target_include_directories(ChordDetectionTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../core
)

# On Windows, link with necessary libraries
if(WIN32)
    target_compile_definitions(ChordDetectionTests PRIVATE _USE_MATH_DEFINES)
endif()
