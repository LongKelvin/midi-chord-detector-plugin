// ChordPatterns.h
// Chord pattern database initialization
//
// Contains all chord pattern definitions for the detector.

#pragma once

#include "ChordPattern.h"
#include <map>
#include <string>
#include <vector>

namespace ChordDetection {
namespace ChordPatterns {

/**
 * Initialize the complete chord pattern database.
 * 
 * This function populates the pattern map with 60+ chord types including:
 * - Triads (major, minor, diminished, augmented, sus2, sus4)
 * - Seventh chords (maj7, m7, dom7, dim7, m7b5, etc.)
 * - Sixth chords (6, m6, 6/9)
 * - Extended chords (9, 11, 13 and variants)
 * - Altered dominants
 * - Add chords
 * - Special voicings (quartal, etc.)
 * 
 * @param patterns Map to populate with chord patterns
 */
void initializePatterns(std::map<std::string, ChordPattern>& patterns);

/**
 * Build interval-based index for fast pattern lookup.
 * 
 * Creates a reverse index mapping interval signatures to chord types,
 * enabling O(1) lookup for exact interval matches.
 * 
 * @param patterns Source pattern database
 * @param index Map to populate with interval-to-type mappings
 */
void buildIntervalIndex(
    const std::map<std::string, ChordPattern>& patterns,
    std::map<std::vector<int>, std::vector<std::string>>& index);

} // namespace ChordPatterns
} // namespace ChordDetection
