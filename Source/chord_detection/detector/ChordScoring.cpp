// ChordScoring.cpp
// Chord detection scoring implementation

#include "ChordScoring.h"
#include <algorithm>
#include <set>
#include <cmath>

namespace ChordDetection {
namespace ChordScoring {

// ============================================================================
// CONTEXTUAL BONUS (KEY DISAMBIGUATION FUNCTION)
// ============================================================================

float computeContextualBonus(
    const std::vector<int>& intervals,
    const ChordPattern& pattern,
    int bassPitchClass,
    int potentialRoot,
    const std::string& chordType)
{
    float bonus = 0.0f;
    
    // Build interval set for fast lookup
    std::set<int> intervalSet(intervals.begin(), intervals.end());
    
    // ========================================================================
    // RULE 1: Defining Interval Combinations
    // These interval combinations STRONGLY suggest specific chords
    // ========================================================================
    
    // Minor 3rd (3) + Major 6th (9) = Definitely m6, NOT m7♭5
    if (intervalSet.count(3) && intervalSet.count(9)) {
        if (chordType == "minor6") {
            bonus += 35.0f;  // Strong bonus - this is THE defining combination
        }
    }
    
    // Minor 3rd (3) + Dim 5th (6) + Minor 7th (10) = Half-diminished
    if (intervalSet.count(3) && intervalSet.count(6) && intervalSet.count(10)) {
        if (chordType == "half-diminished7" || chordType == "minor7b5") {
            bonus += 30.0f;
        }
    }
    
    // Major 3rd (4) + Major 7th (11) + 13th (21 or 9) = Major 13
    // Accept octave-reduced: 13th interval can be 21 or 9
    if (intervalSet.count(4) && intervalSet.count(11)) {
        if (intervalSet.count(21) || intervalSet.count(9)) {
            if (chordType.find("major13") != std::string::npos) {
                bonus += 40.0f;  // Very specific combination
            }
        }
    }
    
    // Major 3rd (4) + ♯5 (8) + ♭7 (10) + ♯9 (15) = Altered dominant 7♯5♯9
    if (intervalSet.count(4) && intervalSet.count(8) && 
        intervalSet.count(10) && intervalSet.count(15)) {
        if (chordType == "dominant7#5#9" || chordType == "dominant7♯5♯9") {
            bonus += 45.0f;  // Highly specific altered chord
        }
    }
    
    // Major 3rd (4) + ♯5 (8) + ♭7 (10) = Aug7 base
    if (intervalSet.count(4) && intervalSet.count(8) && intervalSet.count(10)) {
        if (chordType.find("7#5") != std::string::npos || 
            chordType.find("7♯5") != std::string::npos ||
            chordType == "augmented7") {
            bonus += 25.0f;
        }
    }
    
    // Major 3rd (4) + Major 6th (9) no 7th = Definitely major 6, NOT m7
    if (intervalSet.count(4) && intervalSet.count(9) && 
        !intervalSet.count(10) && !intervalSet.count(11)) {
        if (chordType == "major6") {
            bonus += 30.0f;  // Clear 6th chord, not ambiguous with m7
        }
    }
    
    // ========================================================================
    // RULE 2: Bass = Root Bonus (Enhanced)
    // ========================================================================
    
    if (bassPitchClass == potentialRoot) {
        bonus += 18.0f;  // Base bonus for root position
        
        // Extra bonus for complex chords (more likely in root position)
        if (intervals.size() >= 5) {
            bonus += 10.0f;  // Extended chords get extra root bonus
        }
        if (intervals.size() >= 6) {
            bonus += 5.0f;  // Even more for very extended chords
        }
    }
    
    // ========================================================================
    // RULE 3: Simplicity Preference
    // When ambiguous, prefer simpler chord names
    // ========================================================================
    
    int complexity = 0;
    
    // Count alterations/accidentals in chord type name
    for (char c : chordType) {
        if (c == '#' || c == 'b') complexity++;
    }
    // Unicode accidentals
    if (chordType.find("♭") != std::string::npos) complexity++;
    if (chordType.find("♯") != std::string::npos) complexity++;
    
    // m6 is simpler than m7♭5
    if (chordType == "minor6") complexity -= 2;  // Prefer m6
    if (chordType == "half-diminished7" || chordType == "minor7b5") complexity += 1;
    
    // 6 is simpler than m7
    if (chordType == "major6") complexity -= 1;
    
    // Penalize complexity
    bonus -= complexity * 4.0f;
    
    // ========================================================================
    // RULE 4: Extended Chord Completeness
    // Reward having the characteristic extensions
    // ========================================================================
    
    if (chordType.find("13") != std::string::npos) {
        // Has actual 13th present?
        if (intervalSet.count(21) || intervalSet.count(9)) {
            bonus += 20.0f;  // Has the 13th that defines it
        }
        // Also has 9th? Even better
        if (intervalSet.count(14) || intervalSet.count(2)) {
            bonus += 10.0f;
        }
    }
    
    if (chordType.find("11") != std::string::npos) {
        // Has actual 11th present?
        if (intervalSet.count(17) || intervalSet.count(5)) {
            bonus += 15.0f;
        }
    }
    
    if (chordType.find("9") != std::string::npos && chordType.find("13") == std::string::npos) {
        // 9 chord that's not a 13
        if (intervalSet.count(14) || intervalSet.count(2)) {
            bonus += 12.0f;
        }
    }
    
    // ========================================================================
    // RULE 5: Altered Dominant Bonus
    // Reward multiple alterations (jazz chords!)
    // ========================================================================
    
    if (chordType.find("dominant") != std::string::npos || 
        (chordType.find("7") != std::string::npos && 
         chordType.find("maj7") == std::string::npos &&
         chordType.find("m7") == std::string::npos &&
         chordType.find("dim7") == std::string::npos)) {
        
        int alterations = 0;
        
        if (intervalSet.count(13)) alterations++;  // ♭9
        if (intervalSet.count(15)) alterations++;  // ♯9
        if (intervalSet.count(6))  alterations++;  // ♭5
        if (intervalSet.count(8))  alterations++;  // ♯5
        if (intervalSet.count(18)) alterations++;  // ♯11
        if (intervalSet.count(20)) alterations++;  // ♭13
        
        // Reward multiple alterations - this is sophisticated jazz harmony
        if (alterations >= 2) {
            bonus += alterations * 15.0f;
        } else if (alterations == 1) {
            bonus += 8.0f;
        }
    }
    
    // ========================================================================
    // RULE 6: Voicing-specific bonuses
    // ========================================================================
    
    // Shell voicings: if pattern only has 3-4 notes, give bonus if played notes match
    if (pattern.intervals.size() <= 4 && intervals.size() <= 4) {
        bonus += 5.0f;  // Compact voicing match
    }
    
    return bonus;
}

// ============================================================================
// ALTERNATIVE FILTERING
// ============================================================================

bool shouldIncludeAlternative(
    const std::string& primaryType,
    const std::string& alternativeType,
    float scoreRatio,
    bool sameRoot)
{
    // Don't include if score is too low
    if (scoreRatio < 0.55f) {
        return false;
    }
    
    // Don't include shell/rootless variants of same chord
    if (sameRoot) {
        bool primaryIsShell = primaryType.find("shell") != std::string::npos;
        bool altIsShell = alternativeType.find("shell") != std::string::npos;
        bool primaryIsRootless = primaryType.find("rootless") != std::string::npos;
        bool altIsRootless = alternativeType.find("rootless") != std::string::npos;
        
        // If one is shell/rootless version of similar chord, skip
        if ((primaryIsShell && !altIsShell) || (!primaryIsShell && altIsShell) ||
            (primaryIsRootless && !altIsRootless) || (!primaryIsRootless && altIsRootless)) {
            // Check if they're the same base chord
            std::string primaryBase = primaryType;
            std::string altBase = alternativeType;
            
            auto stripVariant = [](std::string& s) {
                size_t pos = s.find("-shell");
                if (pos != std::string::npos) s = s.substr(0, pos);
                pos = s.find("-partial");
                if (pos != std::string::npos) s = s.substr(0, pos);
                pos = s.find("-flex");
                if (pos != std::string::npos) s = s.substr(0, pos);
                pos = s.find("rootless-");
                if (pos != std::string::npos) s = s.substr(pos + 9);
            };
            
            stripVariant(primaryBase);
            stripVariant(altBase);
            
            if (primaryBase == altBase) {
                return false;  // Same base chord, different voicing
            }
        }
    }
    
    // Always include known ambiguous pairs
    bool isKnownPair = 
        (primaryType == "minor6" && alternativeType == "half-diminished7") ||
        (primaryType == "half-diminished7" && alternativeType == "minor6") ||
        (primaryType == "major6" && alternativeType == "minor7") ||
        (primaryType == "minor7" && alternativeType == "major6") ||
        (primaryType == "sus4" && alternativeType == "sus2") ||
        (primaryType == "sus2" && alternativeType == "sus4") ||
        (primaryType == "minor6" && alternativeType == "minor7b5") ||
        (primaryType == "minor7b5" && alternativeType == "minor6");
    
    if (isKnownPair) {
        return true;
    }
    
    // Include alternatives with good scores that are different enough
    return scoreRatio >= 0.60f;
}

// ============================================================================
// MAIN SCORING FUNCTION
// ============================================================================

float computeScore(
    const std::vector<int>& intervals,
    const ChordPattern& pattern,
    int bassPitchClass,
    int potentialRoot,
    VoicingType voicingType,
    const std::string& chordType)
{
    float score = static_cast<float>(pattern.baseScore);
    
    // Check required intervals - must all be present
    bool allRequiredPresent = true;
    for (int req : pattern.required) {
        // For extended chords, also check octave-reduced versions
        bool found = std::find(intervals.begin(), intervals.end(), req) != intervals.end();
        if (!found && req > 11) {
            // Check octave-reduced version (e.g., 14 -> 2 for 9th)
            int reduced = req % 12;
            found = std::find(intervals.begin(), intervals.end(), reduced) != intervals.end();
        }
        if (!found) {
            allRequiredPresent = false;
            break;
        }
    }
    
    if (!allRequiredPresent) {
        return 0.0f;
    }
    
    // Convert to sets for easier comparison
    std::set<int> intervalSet(intervals.begin(), intervals.end());
    std::set<int> patternSet(pattern.intervals.begin(), pattern.intervals.end());
    
    // MASSIVE bonus for exact match
    if (intervalSet == patternSet) {
        score += 150.0f;
    }
    
    // Required intervals bonus
    int requiredCount = 0;
    for (int interval : intervals) {
        if (std::find(pattern.required.begin(), pattern.required.end(), interval) 
            != pattern.required.end()) {
            requiredCount++;
        }
        // Also check extended version
        int extended = interval + 12;
        if (std::find(pattern.required.begin(), pattern.required.end(), extended) 
            != pattern.required.end()) {
            requiredCount++;
        }
    }
    score += static_cast<float>(requiredCount) * 30.0f;
    
    // Optional intervals bonus - increased weight for extended chords
    int optionalCount = 0;
    for (int interval : intervals) {
        if (std::find(pattern.optional.begin(), pattern.optional.end(), interval) 
            != pattern.optional.end()) {
            optionalCount++;
        }
    }
    score += static_cast<float>(optionalCount) * 15.0f;  // Increased from 10
    
    // Root position bonus - enhanced for disambiguation
    if (bassPitchClass == potentialRoot) {
        score += 20.0f;  // Increased from 15
        
        // Extra bonus if bass is clearly the lowest note and root
        // This helps with C6 vs Am7 disambiguation
        score += 8.0f;
    }
    
    // Important interval bonus (3rd and 7th define chord quality)
    int importantPresent = 0;
    for (int interval : intervals) {
        if (std::find(pattern.importantIntervals.begin(), 
                     pattern.importantIntervals.end(), interval) 
            != pattern.importantIntervals.end()) {
            importantPresent++;
        }
    }
    score += static_cast<float>(importantPresent) * 30.0f;
    
    // Match ratio bonus - improved for extended chords
    int matchedCount = 0;
    for (int interval : intervals) {
        if (patternSet.find(interval) != patternSet.end()) {
            matchedCount++;
        }
        // Also check if interval matches an extended version in pattern
        int extended = interval + 12;
        if (extended <= 24 && patternSet.find(extended) != patternSet.end()) {
            matchedCount++;
        }
    }
    
    if (!pattern.intervals.empty()) {
        float matchRatio = static_cast<float>(matchedCount) / 
                          static_cast<float>(pattern.intervals.size());
        score += matchRatio * 80.0f;
        
        // Bonus for high match ratio with extended chords
        if (matchRatio > 0.8f && pattern.intervals.size() >= 5) {
            score += 25.0f;  // Extended chord match bonus
        }
    }
    
    // Penalty for extra intervals not in pattern or optional - reduced for extended
    int extraCount = 0;
    for (int interval : intervals) {
        bool inPattern = patternSet.find(interval) != patternSet.end();
        bool inOptional = std::find(pattern.optional.begin(), pattern.optional.end(), interval) 
                          != pattern.optional.end();
        
        // Also check extended/reduced versions
        int extended = interval + 12;
        int reduced = interval > 11 ? interval - 12 : interval;
        bool extendedInPattern = patternSet.find(extended) != patternSet.end() ||
                                 patternSet.find(reduced) != patternSet.end();
        
        if (!inPattern && !inOptional && !extendedInPattern) {
            extraCount++;
        }
    }
    score -= static_cast<float>(extraCount) * 6.0f;  // Reduced penalty from 8
    
    // Voicing considerations
    if (voicingType == VoicingType::Rootless) {
        score += 15.0f;  // Increased for rootless voicings
    } else if (voicingType == VoicingType::Close) {
        score += 5.0f;
    } else if (voicingType == VoicingType::Drop2 || voicingType == VoicingType::Drop3) {
        score += 8.0f;  // Bonus for drop voicings (common in jazz)
    }
    
    // Root handling - more flexible for jazz voicings
    bool hasRoot = std::find(intervals.begin(), intervals.end(), 0) != intervals.end();
    if (!hasRoot) {
        if (voicingType == VoicingType::Rootless) {
            // No penalty for rootless voicings
        } else if (pattern.intervals.size() >= 4) {
            // Reduced penalty for extended chords (root often omitted)
            score -= 20.0f;
        } else {
            score -= 40.0f;
        }
    }
    
    // Prefer chords with 3rd or sus interval
    bool hasThirdOrSus = false;
    for (int interval : {2, 3, 4, 5}) {  // sus2=2, m3=3, M3=4, sus4=5
        if (std::find(intervals.begin(), intervals.end(), interval) != intervals.end()) {
            hasThirdOrSus = true;
            break;
        }
    }
    if (!hasThirdOrSus) {
        score -= 25.0f;
    }
    
    // Special handling for 6th chord vs m7 ambiguity
    // If we have intervals 0, 3/4, 7, 9 - this could be X6 or Ym7
    bool has6th = std::find(intervals.begin(), intervals.end(), 9) != intervals.end();
    bool hasMinor7th = std::find(intervals.begin(), intervals.end(), 10) != intervals.end();
    bool hasMajor7th = std::find(intervals.begin(), intervals.end(), 11) != intervals.end();
    
    if (has6th && !hasMinor7th && !hasMajor7th) {
        // This is ambiguous - could be 6th chord
        // Give small bonus to 6th chord interpretation when bass = root
        if (chordType == "major6" || chordType == "minor6") {
            if (bassPitchClass == potentialRoot) {
                score += 12.0f;  // Prefer 6th when bass is the root
            }
        }
    }
    
    // ========================================================================
    // APPLY CONTEXTUAL BONUS (KEY DISAMBIGUATION LOGIC)
    // This is where the magic happens for Cm6 vs Am7♭5, E7♯5♯9, Cmaj13, etc.
    // ========================================================================
    score += computeContextualBonus(intervals, pattern, bassPitchClass, potentialRoot, chordType);
    
    return score;
}

float computeConfidence(
    float bestScore,
    float secondBestScore,
    int noteCount,
    bool exactMatch) noexcept
{
    // Score margin component - more nuanced
    float scoreMargin = bestScore - secondBestScore;
    float marginConfidence;
    if (scoreMargin > 80.0f) {
        marginConfidence = 1.0f;
    } else if (scoreMargin > 40.0f) {
        marginConfidence = 0.7f + (scoreMargin - 40.0f) / 133.0f;
    } else {
        marginConfidence = scoreMargin / 80.0f;  // Low confidence for close scores
    }
    
    // Absolute score component
    float absoluteConfidence = std::min(bestScore / 250.0f, 1.0f);
    
    // Note count component (more notes = more confident, but also more complex)
    float noteConfidence;
    if (noteCount <= 3) {
        noteConfidence = 0.6f + static_cast<float>(noteCount) * 0.1f;
    } else if (noteCount <= 5) {
        noteConfidence = 0.85f + static_cast<float>(noteCount - 3) * 0.05f;
    } else {
        noteConfidence = 0.95f;  // Extended chords have high note confidence
    }
    
    // Exact match component
    float exactConfidence = exactMatch ? 1.0f : 0.5f;
    
    // Weighted combination - adjusted weights
    float confidence = 0.30f * marginConfidence +
                      0.25f * absoluteConfidence +
                      0.20f * noteConfidence +
                      0.25f * exactConfidence;
    
    return confidence;
}

float computeEnhancedConfidence(
    float bestScore,
    float secondBestScore,
    int noteCount,
    bool exactMatch,
    const std::string& bestType,
    const std::string& secondType) noexcept
{
    // Start with base confidence
    float confidence = computeConfidence(bestScore, secondBestScore, noteCount, exactMatch);
    
    // Apply ambiguity penalty for known difficult pairs
    bool isKnownAmbiguity = 
        (bestType == "minor6" && secondType == "half-diminished7") ||
        (bestType == "half-diminished7" && secondType == "minor6") ||
        (bestType == "minor6" && secondType == "minor7b5") ||
        (bestType == "minor7b5" && secondType == "minor6") ||
        (bestType == "major6" && secondType == "minor7") ||
        (bestType == "minor7" && secondType == "major6") ||
        (bestType == "sus4" && secondType == "sus2") ||
        (bestType == "sus2" && secondType == "sus4");
    
    if (isKnownAmbiguity) {
        // Reduce confidence by 15% for known ambiguous pairs
        confidence -= 0.15f;
    } else {
        // Check score ratio for other ambiguities
        float scoreRatio = (bestScore > 0) ? secondBestScore / bestScore : 0.0f;
        if (scoreRatio > 0.90f) {
            // Very close scores - slight penalty
            confidence -= 0.08f;
        } else if (scoreRatio > 0.80f) {
            confidence -= 0.04f;
        }
    }
    
    return std::max(0.0f, std::min(1.0f, confidence));
}

float computeAmbiguityScore(
    float bestScore,
    float secondBestScore,
    const std::string& bestType,
    const std::string& secondType) noexcept
{
    // Calculate how ambiguous the detection is
    // Returns 0.0 for unambiguous, 1.0 for highly ambiguous
    
    float scoreRatio = (bestScore > 0) ? secondBestScore / bestScore : 0.0f;
    
    // If scores are very close (within 15%), it's ambiguous
    if (scoreRatio > 0.85f) {
        return std::min((scoreRatio - 0.85f) / 0.15f + 0.5f, 1.0f);
    }
    
    // Check for known ambiguous pairs
    bool isKnownAmbiguous = false;
    
    // 6th chord vs m7 ambiguity
    if ((bestType == "major6" && secondType == "minor7") ||
        (bestType == "minor7" && secondType == "major6") ||
        (bestType == "minor6" && secondType == "half-diminished7") ||
        (bestType == "half-diminished7" && secondType == "minor6")) {
        isKnownAmbiguous = true;
    }
    
    // Sus chord ambiguity
    if ((bestType == "sus2" && secondType == "sus4") ||
        (bestType == "sus4" && secondType == "sus2")) {
        isKnownAmbiguous = true;
    }
    
    // Inversion ambiguity (slash chord vs standalone)
    if (bestType.find("slash") != std::string::npos ||
        secondType.find("slash") != std::string::npos) {
        isKnownAmbiguous = true;
    }
    
    if (isKnownAmbiguous) {
        return std::max(0.4f, scoreRatio);  // At least 40% ambiguity
    }
    
    return scoreRatio * 0.5f;  // Base ambiguity from score ratio
}

} // namespace ChordScoring
} // namespace ChordDetection
