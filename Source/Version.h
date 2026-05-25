/*
  ==============================================================================

    Version.h
    
    Central version information for MIDI Chord Detector.
    This file defines version constants used throughout the application.
    
    Note: Keep this in sync with PROJECT_VERSION in CMakeLists.txt

  ==============================================================================
*/

#pragma once

namespace MidiChordDetector
{
    namespace Version
    {
        // Version string without prefix (e.g., "3.0.0")
        constexpr const char* VERSION_STRING = "3.1.0";
        
        // Version string with 'v' prefix (e.g., "v3.0.0")
        // Commonly used in UI displays and log messages
        constexpr const char* VERSION_STRING_WITH_V = "v3.1.0";

        // Author credentials
        constexpr const char* AUTHOR_NAME = "Long Kelvin";
        
        // Individual version components for programmatic version checks
        constexpr int MAJOR = 3;
        constexpr int MINOR = 1;
        constexpr int PATCH = 0;
    }
}

// Compile-time Developer Debug Configuration flag
// Set to 1 to unlock raw score debug screens. 
// Set to 0 to compile a clean, minimalist Cubase production layout hiding Confidence/Scoring metrics.
#ifndef DEVELOPER_DEBUG_MODE
#define DEVELOPER_DEBUG_MODE 0
#endif
