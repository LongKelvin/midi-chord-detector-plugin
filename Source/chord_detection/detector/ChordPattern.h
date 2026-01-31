// ChordPattern.h
// Chord pattern data structure definition
//
// Defines the template structure used to match and identify chords.

#pragma once

#include <vector>
#include <string>

namespace ChordDetection {

/**
 * Definition of a chord pattern for matching.
 * 
 * Each pattern describes the interval structure and scoring characteristics
 * of a specific chord type (e.g., major7, minor, dominant9).
 */
struct ChordPattern {
    // ========================================================================
    // INTERVAL DATA
    // ========================================================================
    
    /// All intervals in the chord (semitones from root, including 0)
    std::vector<int> intervals;
    
    /// Base scoring weight (higher = preferred when ambiguous)
    int baseScore = 100;
    
    /// Required intervals for positive identification
    std::vector<int> required;
    
    /// Optional extensions that don't disqualify the chord
    std::vector<int> optional;
    
    /// Quality-defining intervals (typically 3rd and 7th)
    std::vector<int> importantIntervals;
    
    // ========================================================================
    // DISPLAY DATA
    // ========================================================================
    
    /// Display template with {root} placeholder (e.g., "{root}maj7")
    std::string display;
    
    /// Chord quality category (e.g., "major", "minor", "dominant")
    std::string quality;
    
    /// Chord type key (e.g., "major6", "minor7") - used for disambiguation
    std::string chordType;
    
    // ========================================================================
    // CONSTRUCTORS
    // ========================================================================
    
    ChordPattern() = default;
    
    ChordPattern(std::vector<int> intervals_,
                 int baseScore_,
                 std::vector<int> required_,
                 std::vector<int> optional_,
                 std::vector<int> important_,
                 std::string display_,
                 std::string quality_,
                 std::string type_ = "")
        : intervals(std::move(intervals_))
        , baseScore(baseScore_)
        , required(std::move(required_))
        , optional(std::move(optional_))
        , importantIntervals(std::move(important_))
        , display(std::move(display_))
        , quality(std::move(quality_))
        , chordType(std::move(type_))
    {}
    
    // ========================================================================
    // QUERIES
    // ========================================================================
    
    /// Check if interval is required for this pattern
    [[nodiscard]] bool isRequired(int interval) const noexcept {
        for (int req : required) {
            if (req == interval) return true;
        }
        return false;
    }
    
    /// Check if interval is optional for this pattern
    [[nodiscard]] bool isOptional(int interval) const noexcept {
        for (int opt : optional) {
            if (opt == interval) return true;
        }
        return false;
    }
    
    /// Check if interval is important (quality-defining)
    [[nodiscard]] bool isImportant(int interval) const noexcept {
        for (int imp : importantIntervals) {
            if (imp == interval) return true;
        }
        return false;
    }
    
    /// Check if interval exists in the pattern
    [[nodiscard]] bool hasInterval(int interval) const noexcept {
        for (int i : intervals) {
            if (i == interval) return true;
        }
        return false;
    }
};

} // namespace ChordDetection
