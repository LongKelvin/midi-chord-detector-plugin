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
 * @param chordType The chord type key (e.g., "major6", "minor7") for disambiguation
 * @return Score value (higher = better match)
 */
[[nodiscard]] float computeScore(
    const std::vector<int>& intervals,
    const ChordPattern& pattern,
    int bassPitchClass,
    int potentialRoot,
    VoicingType voicingType,
    const std::string& chordType = "");

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

/**
 * Compute enhanced confidence with ambiguity penalty.
 * 
 * This version applies a penalty for known ambiguous chord pairs.
 * 
 * @param bestScore Score of best candidate
 * @param secondBestScore Score of second-best candidate
 * @param noteCount Number of unique pitch classes
 * @param exactMatch Whether intervals exactly match pattern
 * @param bestType Chord type of best candidate
 * @param secondType Chord type of second-best candidate
 * @return Confidence value (0.0 - 1.0) with ambiguity penalty applied
 */
[[nodiscard]] float computeEnhancedConfidence(
    float bestScore,
    float secondBestScore,
    int noteCount,
    bool exactMatch,
    const std::string& bestType,
    const std::string& secondType) noexcept;

/**
 * Compute ambiguity score between two chord interpretations.
 * 
 * Used to determine if alternative interpretations should be shown.
 * 
 * @param bestScore Score of best candidate
 * @param secondBestScore Score of second-best candidate
 * @param bestType Chord type of best candidate
 * @param secondType Chord type of second-best candidate
 * @return Ambiguity score (0.0 = unambiguous, 1.0 = highly ambiguous)
 */
[[nodiscard]] float computeAmbiguityScore(
    float bestScore,
    float secondBestScore,
    const std::string& bestType,
    const std::string& secondType) noexcept;

/**
 * Compute contextual bonus based on defining interval combinations.
 * 
 * This is the KEY function for disambiguation. It adds bonuses for:
 * - Defining interval combinations (e.g., ♭3 + 6 = definitely m6)
 * - Bass = root position with extra for complex chords
 * - Simplicity preference (penalize complex chord names)
 * - Extended chord completeness (13th present for 13 chord, etc.)
 * - Altered dominant bonuses for multiple alterations
 * 
 * @param intervals Detected intervals from potential root
 * @param pattern Chord pattern being matched
 * @param bassPitchClass Pitch class of bass note
 * @param potentialRoot Pitch class of potential root
 * @param chordType The chord type string (e.g., "minor6", "dominant7#5#9")
 * @return Contextual bonus to add to base score
 */
[[nodiscard]] float computeContextualBonus(
    const std::vector<int>& intervals,
    const ChordPattern& pattern,
    int bassPitchClass,
    int potentialRoot,
    const std::string& chordType);

/**
 * Check if an alternative chord should be included in the results.
 * 
 * Filters out alternatives that are:
 * - Scored too low compared to primary
 * - Just different voicings of the same chord
 * - Too similar or too different to be meaningful
 * 
 * @param primaryType Chord type of best candidate
 * @param alternativeType Chord type of alternative
 * @param scoreRatio Alternative score / primary score
 * @param sameRoot Whether both have the same root
 * @return True if alternative should be shown
 */
[[nodiscard]] bool shouldIncludeAlternative(
    const std::string& primaryType,
    const std::string& alternativeType,
    float scoreRatio,
    bool sameRoot);

} // namespace ChordScoring
} // namespace ChordDetection
