// ChordCandidate.h
// Chord detection result data structure
//
// Contains all information about a detected chord.

#pragma once

#include "ChordTypes.h"
#include <vector>
#include <string>

namespace ChordDetection {

/**
 * Result of chord detection - contains all analysis data.
 * 
 * This structure holds the complete information about a detected chord,
 * including the chord name, root, intervals, confidence, and note data.
 */
struct ChordCandidate {
    // ========================================================================
    // PRIMARY IDENTIFICATION
    // ========================================================================
    
    /// Full display name (e.g., "Cmaj7/E")
    std::string chordName;
    
    /// Root pitch class (0-11, where 0=C)
    int root = 0;
    
    /// Root note name (e.g., "C", "F♯")
    std::string rootName;
    
    /// Pattern type key (e.g., "major7", "minor", "dominant9")
    std::string chordType;
    
    // ========================================================================
    // ANALYSIS DATA
    // ========================================================================
    
    /// Matched pattern intervals (from pattern definition)
    std::vector<int> pattern;
    
    /// Detected intervals from root (actual notes played)
    std::vector<int> intervals;
    
    /// Raw detection score (higher = better match)
    float score = 0.0f;
    
    /// Normalized confidence (0.0 - 1.0)
    float confidence = 0.0f;
    
    /// Position description (e.g., "Root Position", "1st Inversion", "Slash/E")
    std::string position;
    
    /// Voicing classification
    VoicingType voicingType = VoicingType::Unknown;
    
    // ========================================================================
    // NOTE INFORMATION
    // ========================================================================
    
    /// Degree labels (e.g., "R", "3", "5", "7", "♭9")
    std::vector<std::string> degrees;
    
    /// Pitch class names for each note (e.g., "C", "E", "G")
    std::vector<std::string> noteNames;
    
    /// Original MIDI note numbers as played
    std::vector<int> noteNumbers;
    
    /// Unique pitch classes (0-11)
    std::vector<int> pitchClasses;
    
    // ========================================================================
    // CONSTRUCTORS
    // ========================================================================
    
    ChordCandidate() = default;
    
    // ========================================================================
    // QUERIES
    // ========================================================================
    
    /// Check if this is a valid detection result
    [[nodiscard]] bool isValid() const noexcept { 
        return confidence > 0.0f && !chordName.empty(); 
    }
    
    /// Check if chord is in root position
    [[nodiscard]] bool isRootPosition() const noexcept {
        return position == "Root Position";
    }
    
    /// Get the bass pitch class (lowest note)
    [[nodiscard]] int getBassPitchClass() const noexcept {
        return pitchClasses.empty() ? 0 : pitchClasses.front();
    }
    
    /// Get number of unique pitch classes
    [[nodiscard]] size_t noteCount() const noexcept {
        return pitchClasses.size();
    }
    
    /// Check if confidence is high (> 80%)
    [[nodiscard]] bool isHighConfidence() const noexcept {
        return confidence > 0.8f;
    }
    
    /// Check if confidence is medium (60-80%)
    [[nodiscard]] bool isMediumConfidence() const noexcept {
        return confidence > 0.6f && confidence <= 0.8f;
    }
    
    /// Check if confidence is low (<= 60%)
    [[nodiscard]] bool isLowConfidence() const noexcept {
        return confidence <= 0.6f;
    }
};

} // namespace ChordDetection
