# =============================================================================
# CMakeLists.txt for H-WCTM Chord Detection Tests
# =============================================================================
# Features:
# - Automatic test generation from JSON using Python
# - Graceful fallback if Python unavailable
# - Standalone tests without JUCE dependency
# =============================================================================

cmake_minimum_required(VERSION 3.15)
project(ChordDetectionTests VERSION 1.0.0)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================================
# Core Source Files (Standalone - No JUCE Dependencies)
# =============================================================================
set(CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../core")

set(CORE_SOURCES
    ${CORE_DIR}/MidiNoteState.cpp
    ${CORE_DIR}/CandidateGenerator.cpp
    ${CORE_DIR}/ChordDetectorEngine.cpp
    ${CORE_DIR}/ChordResolver.cpp
    ${CORE_DIR}/ConfidenceScorer.cpp
    ${CORE_DIR}/HarmonicMemory.cpp
)

# =============================================================================
# Python-Based Test Generation (Optional)
# =============================================================================
# If Python 3 is available, automatically generates C++ tests from JSON

find_package(Python3 COMPONENTS Interpreter)

# Path configuration
set(JSON_TEST_FILE "${CMAKE_CURRENT_SOURCE_DIR}/chord_tests.json")
set(GENERATED_TESTS_FILE "${CMAKE_CURRENT_BINARY_DIR}/generated_tests_from_json.h")
set(GENERATOR_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/json_to_cpp_tests.py")

# Generate C++ tests from JSON if Python is available
if(Python3_FOUND AND EXISTS "${JSON_TEST_FILE}" AND EXISTS "${GENERATOR_SCRIPT}")
    message(STATUS "Python 3 found - Enabling JSON test generation")
    message(STATUS "  Test JSON: ${JSON_TEST_FILE}")
    message(STATUS "  Generator: ${GENERATOR_SCRIPT}")
    message(STATUS "  Output: ${GENERATED_TESTS_FILE}")
    
    # Custom command to generate tests at build time
    add_custom_command(
        OUTPUT ${GENERATED_TESTS_FILE}
        COMMAND ${Python3_EXECUTABLE} ${GENERATOR_SCRIPT} ${JSON_TEST_FILE} > ${GENERATED_TESTS_FILE}
        DEPENDS ${JSON_TEST_FILE} ${GENERATOR_SCRIPT}
        COMMENT "Generating C++ tests from chord_tests.json..."
        VERBATIM
    )
    
    # Create a custom target for generation
    add_custom_target(GenerateTests DEPENDS ${GENERATED_TESTS_FILE})
    
    set(USE_GENERATED_TESTS TRUE)
    message(STATUS "JSON test generation: ENABLED (168 additional tests)")
else()
    if(NOT Python3_FOUND)
        message(WARNING "Python 3 not found - JSON test generation DISABLED")
    elseif(NOT EXISTS "${JSON_TEST_FILE}")
        message(WARNING "chord_tests.json not found - JSON test generation DISABLED")
    elseif(NOT EXISTS "${GENERATOR_SCRIPT}")
        message(WARNING "json_to_cpp_tests.py not found - JSON test generation DISABLED")
    endif()
    
    set(USE_GENERATED_TESTS FALSE)
    message(STATUS "JSON test generation: DISABLED (manual tests only - 20 tests)")
endif()

# =============================================================================
# Test Executable Configuration
# =============================================================================

# Include directories
include_directories(
    ${CORE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}  # For generated header
)

# Create test executable
add_executable(ChordDetectionTests
    ChordDetectionTests.cpp
    ${CORE_SOURCES}
)

# Link generated tests if available
if(USE_GENERATED_TESTS)
    add_dependencies(ChordDetectionTests GenerateTests)
    target_compile_definitions(ChordDetectionTests PRIVATE USE_JSON_GENERATED_TESTS=1)
    message(STATUS "Test executable configured with JSON-generated tests")
endif()

# =============================================================================
# Compiler Configuration
# =============================================================================

# Windows-specific settings
if(WIN32)
    target_compile_definitions(ChordDetectionTests PRIVATE 
        _USE_MATH_DEFINES
        _CRT_SECURE_NO_WARNINGS
    )
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(ChordDetectionTests PRIVATE /W4)
else()
    target_compile_options(ChordDetectionTests PRIVATE -Wall -Wextra)
endif()

# =============================================================================
# Build Output Configuration  
# =============================================================================

# Set output directory
set_target_properties(ChordDetectionTests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release"
)

# =============================================================================
# Build Summary
# =============================================================================

message(STATUS "")
message(STATUS "========================================")
message(STATUS "Chord Detection Tests Configuration")
message(STATUS "========================================")
message(STATUS "Build Type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard:      C++${CMAKE_CXX_STANDARD}")
message(STATUS "Python 3:          ${Python3_FOUND}")
message(STATUS "JSON Tests:        ${USE_GENERATED_TESTS}")
message(STATUS "Manual Tests:      20")
if(USE_GENERATED_TESTS)
    message(STATUS "Generated Tests:   168")
    message(STATUS "Total Tests:       188")
else()
    message(STATUS "Total Tests:       20")
endif()
message(STATUS "Output Directory:  ${CMAKE_BINARY_DIR}")
message(STATUS "========================================")
message(STATUS "")
