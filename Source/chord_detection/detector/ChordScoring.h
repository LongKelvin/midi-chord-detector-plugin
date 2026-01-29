// ChordScoring.h
// Chord detection scoring algorithms
//
// Contains functions for scoring chord candidates and computing confidence.

#pragma once

#include "ChordTypes.h"
#include "ChordPattern.h"
#include <vector>

namespace ChordDetection {
namespace ChordScoring {

/**
 * Compute score for a chord candidate.
 * 
 * Scoring factors:
 * - Exact pattern match bonus
 * - Required intervals present
 * - Important intervals (3rd, 7th) present
 * - Optional intervals present
 * - Root position bonus
 * - Penalty for extra/missing intervals
 * 
 * @param intervals Detected intervals from potential root
 * @param pattern Chord pattern to match against
 * @param bassPitchClass Pitch class of the bass note
 * @param potentialRoot Pitch class of the potential root
 * @param voicingType Classified voicing type
 * @return Score value (higher = better match)
 */
[[nodiscard]] float computeScore(
    const std::vector<int>& intervals,
    const ChordPattern& pattern,
    int bassPitchClass,
    int potentialRoot,
    VoicingType voicingType);

/**
 * Compute normalized confidence value.
 * 
 * Confidence is based on:
 * - Score margin over second-best candidate
 * - Absolute score value
 * - Number of notes
 * - Exact pattern match
 * 
 * @param bestScore Score of best candidate
 * @param secondBestScore Score of second-best candidate
 * @param noteCount Number of unique pitch classes
 * @param exactMatch Whether intervals exactly match pattern
 * @return Confidence value (0.0 - 1.0)
 */
[[nodiscard]] float computeConfidence(
    float bestScore,
    float secondBestScore,
    int noteCount,
    bool exactMatch) noexcept;

} // namespace ChordScoring
} // namespace ChordDetection
