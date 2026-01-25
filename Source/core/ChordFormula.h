#pragma once

#include <array>
#include <bitset>
#include <cstdint>

/**
 * ChordFormula.h - Declarative chord formula definitions
 * 
 * Following spec: "Never hardcode chord logic inline – always use chord formulas."
 * 
 * This file defines the chord formula structure with:
 * - Required intervals (must be present)
 * - Optional intervals (extensions, can enhance score)
 * - Forbidden intervals (if present, disqualify this chord type)
 * 
 * Real-time safe: All data is constexpr, no allocations.
 */

namespace ChordDetection
{

// ============================================================================
// Chord Quality Enum
// ============================================================================

enum class ChordQuality : uint8_t
{
    // Tier 1 - MVP Jazz Chords (~80% of real jazz harmony)
    Major,
    Minor,
    Dominant7,
    Minor7,
    Major7,
    HalfDiminished,     // m7b5
    Diminished7,
    
    // Tier 2 - Early Expansion
    Major6,
    Minor6,
    Dominant9,
    Minor9,
    Major9,
    Dominant11,         // sus-based
    Dominant13,
    Sus2,
    Sus4,
    
    // Additional common types
    Augmented,
    AugmentedMajor7,
    MinorMajor7,
    Dominant7Sus4,
    Dominant7b9,
    Dominant7Sharp9,
    Dominant7b5,
    Dominant7Sharp5,
    PowerChord,
    
    // Count
    _Count
};

// ============================================================================
// Chord Formula Structure
// ============================================================================

constexpr int MAX_REQUIRED_INTERVALS = 6;
constexpr int MAX_OPTIONAL_INTERVALS = 6;
constexpr int MAX_FORBIDDEN_INTERVALS = 4;

/**
 * ChordFormula - Declarative chord definition
 * 
 * Intervals are semitones from root (0-11):
 *   0=Root, 1=b2, 2=M2/9, 3=m3, 4=M3, 5=P4/11, 6=b5/#11, 
 *   7=P5, 8=#5/b13, 9=M6/13, 10=b7, 11=M7
 */
struct ChordFormula
{
    ChordQuality quality;
    const char* symbol;           // "maj7", "m7", "7", etc.
    const char* fullName;         // "Major 7th", "Minor 7th", etc.
    
    // Required intervals - chord must contain these
    std::array<int8_t, MAX_REQUIRED_INTERVALS> requiredIntervals;
    int8_t requiredCount;
    
    // Optional intervals - extend/enhance chord if present
    std::array<int8_t, MAX_OPTIONAL_INTERVALS> optionalIntervals;
    int8_t optionalCount;
    
    // Forbidden intervals - if present, this quality is disqualified
    std::array<int8_t, MAX_FORBIDDEN_INTERVALS> forbiddenIntervals;
    int8_t forbiddenCount;
    
    // Complexity level (1 = simple triad, higher = more complex)
    // Used for complexity penalty in scoring
    int8_t complexity;
    
    // Priority for tie-breaking (higher = preferred)
    float basePriority;
    
    constexpr ChordFormula(
        ChordQuality q,
        const char* sym,
        const char* name,
        std::array<int8_t, MAX_REQUIRED_INTERVALS> req, int8_t reqCount,
        std::array<int8_t, MAX_OPTIONAL_INTERVALS> opt, int8_t optCount,
        std::array<int8_t, MAX_FORBIDDEN_INTERVALS> forb, int8_t forbCount,
        int8_t cmplx,
        float prio
    ) : quality(q), symbol(sym), fullName(name),
        requiredIntervals(req), requiredCount(reqCount),
        optionalIntervals(opt), optionalCount(optCount),
        forbiddenIntervals(forb), forbiddenCount(forbCount),
        complexity(cmplx), basePriority(prio)
    {}
    
    /**
     * Check if pitch class set contains a forbidden interval
     */
    constexpr bool hasForbiddenInterval(const std::bitset<12>& relativePCs) const
    {
        for (int i = 0; i < forbiddenCount; ++i)
        {
            if (forbiddenIntervals[i] >= 0 && relativePCs[forbiddenIntervals[i]])
                return true;
        }
        return false;
    }
    
    /**
     * Count required intervals present
     */
    constexpr int countRequiredPresent(const std::bitset<12>& relativePCs) const
    {
        int count = 0;
        for (int i = 0; i < requiredCount; ++i)
        {
            if (requiredIntervals[i] >= 0 && relativePCs[requiredIntervals[i]])
                ++count;
        }
        return count;
    }
    
    /**
     * Check if all required intervals are present
     */
    constexpr bool allRequiredPresent(const std::bitset<12>& relativePCs) const
    {
        return countRequiredPresent(relativePCs) == requiredCount;
    }
    
    /**
     * Count optional intervals present
     */
    constexpr int countOptionalPresent(const std::bitset<12>& relativePCs) const
    {
        int count = 0;
        for (int i = 0; i < optionalCount; ++i)
        {
            if (optionalIntervals[i] >= 0 && relativePCs[optionalIntervals[i]])
                ++count;
        }
        return count;
    }
};

// ============================================================================
// Helper Macros for Readable Definitions
// ============================================================================

#define REQ_INTERVALS(...) std::array<int8_t, MAX_REQUIRED_INTERVALS>{{__VA_ARGS__}}
#define OPT_INTERVALS(...) std::array<int8_t, MAX_OPTIONAL_INTERVALS>{{__VA_ARGS__}}
#define FORB_INTERVALS(...) std::array<int8_t, MAX_FORBIDDEN_INTERVALS>{{__VA_ARGS__}}
#define NO_REQ std::array<int8_t, MAX_REQUIRED_INTERVALS>{{}}
#define NO_OPT std::array<int8_t, MAX_OPTIONAL_INTERVALS>{{}}
#define NO_FORB std::array<int8_t, MAX_FORBIDDEN_INTERVALS>{{}}

// ============================================================================
// Complete Chord Formula Table
// ============================================================================

/**
 * CHORD_FORMULAS - Complete chord dictionary
 * 
 * Tier 1 (MVP) chords listed first for priority
 * Ordered by: simplicity, then commonality
 */
constexpr ChordFormula CHORD_FORMULAS[] = {
    // ========== TIER 1: MVP Jazz Chords ==========
    
    // Major Triad: 0, 4, 7
    {ChordQuality::Major, "maj", "Major",
     REQ_INTERVALS(0, 4, 7), 3,
     OPT_INTERVALS(2, 9), 2,          // 9th, 6th are common additions
     FORB_INTERVALS(3, 10), 2,        // No minor 3rd or b7 (would be dominant)
     1, 1.0f},
    
    // Minor Triad: 0, 3, 7
    {ChordQuality::Minor, "m", "Minor",
     REQ_INTERVALS(0, 3, 7), 3,
     OPT_INTERVALS(2, 9), 2,
     FORB_INTERVALS(4), 1,            // No major 3rd
     1, 1.0f},
    
    // Dominant 7: 0, 4, (7), 10 - fifth is often omitted
    {ChordQuality::Dominant7, "7", "Dominant 7th",
     REQ_INTERVALS(0, 4, 10), 3,
     OPT_INTERVALS(7, 2), 2,          // 5th optional, 9th extension
     FORB_INTERVALS(3, 11), 2,        // No minor 3rd or major 7th
     2, 1.1f},
    
    // Minor 7: 0, 3, (7), 10
    {ChordQuality::Minor7, "m7", "Minor 7th",
     REQ_INTERVALS(0, 3, 10), 3,
     OPT_INTERVALS(7, 2, 5), 3,       // 5th, 9th, 11th
     FORB_INTERVALS(4, 11), 2,        // No major 3rd or major 7th
     2, 1.1f},
    
    // Major 7: 0, 4, (7), 11
    {ChordQuality::Major7, "maj7", "Major 7th",
     REQ_INTERVALS(0, 4, 11), 3,
     OPT_INTERVALS(7, 2, 9), 3,       // 5th, 9th, 6th/13
     FORB_INTERVALS(3, 10), 2,        // No minor 3rd or b7
     2, 1.1f},
    
    // Half-Diminished (m7b5): 0, 3, 6, 10
    {ChordQuality::HalfDiminished, "m7b5", "Half Diminished",
     REQ_INTERVALS(0, 3, 6, 10), 4,
     OPT_INTERVALS(2), 1,             // 9th extension
     FORB_INTERVALS(4, 7), 2,         // No major 3rd or natural 5th
     3, 1.05f},
    
    // Diminished 7: 0, 3, 6, 9
    {ChordQuality::Diminished7, "dim7", "Diminished 7th",
     REQ_INTERVALS(0, 3, 6, 9), 4,
     NO_OPT, 0,
     FORB_INTERVALS(4, 7, 10, 11), 4, // Very specific intervals
     3, 1.0f},
    
    // ========== TIER 2: Early Expansion ==========
    
    // Major 6: 0, 4, 7, 9
    {ChordQuality::Major6, "6", "Major 6th",
     REQ_INTERVALS(0, 4, 9), 3,
     OPT_INTERVALS(7, 2), 2,
     FORB_INTERVALS(3, 10, 11), 3,    // Distinguish from other types
     2, 0.95f},
    
    // Minor 6: 0, 3, 7, 9
    {ChordQuality::Minor6, "m6", "Minor 6th",
     REQ_INTERVALS(0, 3, 9), 3,
     OPT_INTERVALS(7), 1,
     FORB_INTERVALS(4, 10, 11), 3,
     2, 0.95f},
    
    // Dominant 9: 0, 4, 10, plus 2
    {ChordQuality::Dominant9, "9", "Dominant 9th",
     REQ_INTERVALS(0, 4, 10, 2), 4,
     OPT_INTERVALS(7), 1,
     FORB_INTERVALS(3, 11), 2,
     3, 1.15f},
    
    // Minor 9: 0, 3, 10, plus 2
    {ChordQuality::Minor9, "m9", "Minor 9th",
     REQ_INTERVALS(0, 3, 10, 2), 4,
     OPT_INTERVALS(7, 5), 2,          // 5th, 11th
     FORB_INTERVALS(4, 11), 2,
     3, 1.15f},
    
    // Major 9: 0, 4, 11, plus 2
    {ChordQuality::Major9, "maj9", "Major 9th",
     REQ_INTERVALS(0, 4, 11, 2), 4,
     OPT_INTERVALS(7), 1,
     FORB_INTERVALS(3, 10), 2,
     3, 1.15f},
    
    // Dominant 11 (sus-based): 0, 4/5, 10, 2, 5
    {ChordQuality::Dominant11, "11", "Dominant 11th",
     REQ_INTERVALS(0, 10, 5), 3,      // Root, b7, 11
     OPT_INTERVALS(7, 2, 4), 3,       // 5th, 9th, 3rd optional
     FORB_INTERVALS(11), 1,
     4, 1.2f},
    
    // Dominant 13: 0, 4, 10, 9
    {ChordQuality::Dominant13, "13", "Dominant 13th",
     REQ_INTERVALS(0, 4, 10, 9), 4,
     OPT_INTERVALS(7, 2, 5), 3,
     FORB_INTERVALS(3, 11), 2,
     4, 1.25f},
    
    // Sus2: 0, 2, 7
    {ChordQuality::Sus2, "sus2", "Suspended 2nd",
     REQ_INTERVALS(0, 2, 7), 3,
     NO_OPT, 0,
     FORB_INTERVALS(3, 4), 2,         // No 3rd (suspended means no 3rd)
     1, 0.9f},
    
    // Sus4: 0, 5, 7
    {ChordQuality::Sus4, "sus4", "Suspended 4th",
     REQ_INTERVALS(0, 5, 7), 3,
     NO_OPT, 0,
     FORB_INTERVALS(3, 4), 2,
     1, 0.9f},
    
    // ========== Additional Common Types ==========
    
    // Augmented: 0, 4, 8
    {ChordQuality::Augmented, "aug", "Augmented",
     REQ_INTERVALS(0, 4, 8), 3,
     NO_OPT, 0,
     FORB_INTERVALS(3, 7), 2,         // No minor 3rd or natural 5th
     2, 0.85f},
    
    // Augmented Major 7: 0, 4, 8, 11
    {ChordQuality::AugmentedMajor7, "augMaj7", "Augmented Major 7th",
     REQ_INTERVALS(0, 4, 8, 11), 4,
     NO_OPT, 0,
     FORB_INTERVALS(3, 7, 10), 3,
     3, 0.9f},
    
    // Minor Major 7: 0, 3, 7, 11
    {ChordQuality::MinorMajor7, "mMaj7", "Minor Major 7th",
     REQ_INTERVALS(0, 3, 7, 11), 4,
     OPT_INTERVALS(2), 1,
     FORB_INTERVALS(4, 10), 2,
     3, 1.0f},
    
    // Dominant 7 sus4: 0, 5, 7, 10
    {ChordQuality::Dominant7Sus4, "7sus4", "Dominant 7th Sus4",
     REQ_INTERVALS(0, 5, 10), 3,
     OPT_INTERVALS(7, 2), 2,
     FORB_INTERVALS(3, 4, 11), 3,
     2, 0.95f},
    
    // Dominant 7 b9: 0, 4, 10, 1
    {ChordQuality::Dominant7b9, "7b9", "Dominant 7th b9",
     REQ_INTERVALS(0, 4, 10, 1), 4,
     OPT_INTERVALS(7), 1,
     FORB_INTERVALS(3, 2, 11), 3,     // No minor 3rd, natural 9, or maj7
     3, 1.1f},
    
    // Dominant 7 #9: 0, 4, 10, 3 (yes, has both 3 and 4 - the #9 sounds like m3)
    {ChordQuality::Dominant7Sharp9, "7#9", "Dominant 7th #9",
     REQ_INTERVALS(0, 4, 10, 3), 4,   // M3 + #9 (enharmonic m3)
     OPT_INTERVALS(7), 1,
     FORB_INTERVALS(11), 1,
     3, 1.1f},
    
    // Dominant 7 b5: 0, 4, 6, 10
    {ChordQuality::Dominant7b5, "7b5", "Dominant 7th b5",
     REQ_INTERVALS(0, 4, 6, 10), 4,
     OPT_INTERVALS(2, 1), 2,
     FORB_INTERVALS(3, 7, 11), 3,
     3, 1.0f},
    
    // Dominant 7 #5: 0, 4, 8, 10
    {ChordQuality::Dominant7Sharp5, "7#5", "Dominant 7th #5",
     REQ_INTERVALS(0, 4, 8, 10), 4,
     OPT_INTERVALS(2), 1,
     FORB_INTERVALS(3, 7, 11), 3,
     3, 1.0f},
    
    // Power Chord: 0, 7 (no 3rd)
    {ChordQuality::PowerChord, "5", "Power Chord",
     REQ_INTERVALS(0, 7), 2,
     NO_OPT, 0,
     FORB_INTERVALS(3, 4), 2,         // Ambiguous - no 3rd
     1, 0.7f},
};

constexpr int CHORD_FORMULA_COUNT = sizeof(CHORD_FORMULAS) / sizeof(ChordFormula);

// ============================================================================
// Interval Weights for Scoring
// ============================================================================

/**
 * Weight of each interval for scoring purposes
 * Higher = more important for chord identity
 */
constexpr float INTERVAL_IMPORTANCE[12] = {
    0.9f,   // 0: Root - very important
    0.5f,   // 1: b9 - extension
    0.4f,   // 2: 9/M2 - extension
    1.0f,   // 3: m3 - critical (defines minor quality)
    1.0f,   // 4: M3 - critical (defines major quality)
    0.5f,   // 5: P4/11 - sus/extension
    0.6f,   // 6: b5/#11 - altered
    0.7f,   // 7: P5 - foundation (often omitted)
    0.6f,   // 8: #5/b13 - altered
    0.5f,   // 9: 6/13 - extension
    0.9f,   // 10: b7 - critical (defines dominant/minor 7)
    0.9f,   // 11: M7 - critical (defines major 7)
};

// ============================================================================
// Note Names
// ============================================================================

constexpr const char* PITCH_CLASS_NAMES[12] = {
    "C", "C#", "D", "D#", "E", "F",
    "F#", "G", "G#", "A", "A#", "B"
};

// Flat names for display variety
constexpr const char* PITCH_CLASS_NAMES_FLAT[12] = {
    "C", "Db", "D", "Eb", "E", "F",
    "Gb", "G", "Ab", "A", "Bb", "B"
};

} // namespace ChordDetection
