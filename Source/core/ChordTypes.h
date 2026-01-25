#pragma once

#include <array>
#include <cstdint>

/**
 * ChordTypes.h - Compile-time chord definitions
 * 
 * This file contains all chord type definitions as constexpr data.
 * No heap allocation, real-time safe, suitable for audio thread use.
 */

namespace ChordDetection
{

// Maximum number of intervals in any chord definition
constexpr int MAX_CORE_INTERVALS = 6;
constexpr int MAX_OPTIONAL_INTERVALS = 8;

// Note names for display
constexpr const char* NOTE_NAMES[12] = {
    "C", "C#", "D", "D#", "E", "F", 
    "F#", "G", "G#", "A", "A#", "B"
};

// Interval weights for scoring (matches Python implementation)
constexpr float INTERVAL_WEIGHTS[12] = {
    0.8f,  // 0: Root (P1)
    0.3f,  // 1: b9
    0.3f,  // 2: 9 (M2)
    1.0f,  // 3: m3
    1.0f,  // 4: M3
    0.3f,  // 5: 11 (P4)
    0.3f,  // 6: #11/b5
    0.5f,  // 7: P5
    0.3f,  // 8: #5/b13
    0.3f,  // 9: 13 (M6)
    1.0f,  // 10: b7 (m7)
    1.0f   // 11: M7
};

/**
 * Chord definition structure
 * Designed for stack allocation and cache efficiency
 */
struct ChordDefinition
{
    const char* symbol;           // "maj7", "min9", etc.
    const char* fullName;         // "Major 7th", etc.
    
    // Core intervals (required for chord identification)
    std::array<int8_t, MAX_CORE_INTERVALS> coreIntervals;
    int8_t coreCount;
    
    // Optional intervals (extensions, can be omitted)
    std::array<int8_t, MAX_OPTIONAL_INTERVALS> optionalIntervals;
    int8_t optionalCount;
    
    float priority;  // For tie-breaking between equally scored chords
    
    constexpr ChordDefinition(
        const char* sym,
        const char* name,
        std::array<int8_t, MAX_CORE_INTERVALS> core,
        int8_t coreLen,
        std::array<int8_t, MAX_OPTIONAL_INTERVALS> optional,
        int8_t optLen,
        float prio = 1.0f
    ) : symbol(sym), fullName(name), coreIntervals(core), coreCount(coreLen),
        optionalIntervals(optional), optionalCount(optLen), priority(prio)
    {}
};

// Helper macros for readable chord definitions
// These expand variadic args and pad with -1 to fill the array
#define CORE_INTERVALS(...) std::array<int8_t, MAX_CORE_INTERVALS>{{__VA_ARGS__}}
#define OPTIONAL_INTERVALS(...) std::array<int8_t, MAX_OPTIONAL_INTERVALS>{{__VA_ARGS__}}
#define NO_OPTIONAL std::array<int8_t, MAX_OPTIONAL_INTERVALS>{{}}

/**
 * Complete chord definition table
 * Ordered by priority (simpler chords first for better matching)
 * 
 * Based on Python chord_definitions.json
 */
constexpr ChordDefinition CHORD_DEFINITIONS[] = {
    // ====== MAJOR CHORDS ======
    {"maj", "Major Triad", CORE_INTERVALS(0, 4, 7), 3, NO_OPTIONAL, 0, 1.0f},
    {"maj7", "Major 7th", CORE_INTERVALS(0, 4, 7, 11), 4, NO_OPTIONAL, 0, 1.1f},
    {"maj9", "Major 9th", CORE_INTERVALS(0, 4, 7, 11), 4, OPTIONAL_INTERVALS(2), 1, 1.2f},
    {"maj13", "Major 13th", CORE_INTERVALS(0, 4, 7, 11), 4, OPTIONAL_INTERVALS(2, 5, 9), 3, 1.3f},
    {"maj7#11", "Major 7 #11", CORE_INTERVALS(0, 4, 7, 11), 4, OPTIONAL_INTERVALS(6), 1, 1.25f},
    {"6", "Major Sixth", CORE_INTERVALS(0, 4, 7), 3, OPTIONAL_INTERVALS(9), 1, 1.0f},
    {"6/9", "Major 6/9", CORE_INTERVALS(0, 4, 7), 3, OPTIONAL_INTERVALS(2, 9), 2, 1.1f},
    {"add4", "Major Add4", CORE_INTERVALS(0, 4, 7), 3, OPTIONAL_INTERVALS(5), 1, 0.9f},
    {"majb5", "Major b5", CORE_INTERVALS(0, 4, 6), 3, NO_OPTIONAL, 0, 0.8f},
    
    // ====== MINOR CHORDS ======
    {"min", "Minor Triad", CORE_INTERVALS(0, 3, 7), 3, NO_OPTIONAL, 0, 1.0f},
    {"min7", "Minor 7th", CORE_INTERVALS(0, 3, 7, 10), 4, NO_OPTIONAL, 0, 1.1f},
    {"min9", "Minor 9th", CORE_INTERVALS(0, 3, 7, 10), 4, OPTIONAL_INTERVALS(2), 1, 1.2f},
    {"min11", "Minor 11th", CORE_INTERVALS(0, 3, 7, 10), 4, OPTIONAL_INTERVALS(2, 5), 2, 1.25f},
    {"min13", "Minor 13th", CORE_INTERVALS(0, 3, 7, 10), 4, OPTIONAL_INTERVALS(2, 5, 9), 3, 1.3f},
    {"minMaj7", "Minor Major 7", CORE_INTERVALS(0, 3, 7, 11), 4, NO_OPTIONAL, 0, 1.05f},
    {"minMaj9", "Minor Major 9", CORE_INTERVALS(0, 3, 7, 11), 4, OPTIONAL_INTERVALS(2), 1, 1.15f},
    {"min6", "Minor Sixth", CORE_INTERVALS(0, 3, 7), 3, OPTIONAL_INTERVALS(9), 1, 1.0f},
    {"m6/9", "Minor 6/9", CORE_INTERVALS(0, 3, 7), 3, OPTIONAL_INTERVALS(2, 9), 2, 1.1f},
    {"madd9", "Minor Add9", CORE_INTERVALS(0, 3, 7), 3, OPTIONAL_INTERVALS(2), 1, 0.95f},
    {"madd4", "Minor Add4", CORE_INTERVALS(0, 3, 7), 3, OPTIONAL_INTERVALS(5), 1, 0.9f},
    {"min7b5", "Half Diminished", CORE_INTERVALS(0, 3, 6, 10), 4, NO_OPTIONAL, 0, 1.05f},
    
    // ====== DOMINANT CHORDS ======
    {"7", "Dominant 7th", CORE_INTERVALS(0, 4, 10), 3, OPTIONAL_INTERVALS(7), 1, 1.1f},
    {"9", "Dominant 9th", CORE_INTERVALS(0, 4, 10), 3, OPTIONAL_INTERVALS(2, 7), 2, 1.2f},
    {"13", "Dominant 13th", CORE_INTERVALS(0, 4, 10), 3, OPTIONAL_INTERVALS(2, 7, 9), 3, 1.3f},
    {"11", "Dominant 11th", CORE_INTERVALS(0, 4, 10), 3, OPTIONAL_INTERVALS(2, 5, 7), 3, 1.25f},
    {"7#5", "Dominant 7 #5", CORE_INTERVALS(0, 4, 10), 3, OPTIONAL_INTERVALS(8), 1, 1.1f},
    {"7b9", "Dominant 7 b9", CORE_INTERVALS(0, 4, 10), 3, OPTIONAL_INTERVALS(1, 7), 2, 1.2f},
    {"7#9", "Dominant 7 #9", CORE_INTERVALS(0, 4, 10), 3, OPTIONAL_INTERVALS(3, 7), 2, 1.2f},
    {"7b13", "Dominant 7 b13", CORE_INTERVALS(0, 4, 10), 3, OPTIONAL_INTERVALS(7, 8), 2, 1.15f},
    {"7#5b9", "Dominant 7#5b9", CORE_INTERVALS(0, 4, 10), 3, OPTIONAL_INTERVALS(1, 8), 2, 1.25f},
    {"9#5", "Dominant 9 #5", CORE_INTERVALS(0, 4, 10), 3, OPTIONAL_INTERVALS(2, 8), 2, 1.2f},
    {"7sus4", "Dominant 7 sus4", CORE_INTERVALS(0, 5, 10), 3, OPTIONAL_INTERVALS(7), 1, 1.0f},
    {"9sus4", "Dominant 9 sus4", CORE_INTERVALS(0, 5, 10), 3, OPTIONAL_INTERVALS(2, 7), 2, 1.1f},
    
    // ====== SUSPENDED & OTHER ======
    {"sus4", "Suspended 4th", CORE_INTERVALS(0, 5, 7), 3, NO_OPTIONAL, 0, 0.95f},
    {"sus2", "Suspended 2nd", CORE_INTERVALS(0, 2, 7), 3, NO_OPTIONAL, 0, 0.95f},
    
    // ====== DIMINISHED & AUGMENTED ======
    {"dim", "Diminished", CORE_INTERVALS(0, 3, 6), 3, NO_OPTIONAL, 0, 0.9f},
    {"dim7", "Diminished 7th", CORE_INTERVALS(0, 3, 6, 9), 4, NO_OPTIONAL, 0, 1.0f},
    {"aug", "Augmented", CORE_INTERVALS(0, 4, 8), 3, NO_OPTIONAL, 0, 0.9f},
    {"aug7", "Augmented 7th", CORE_INTERVALS(0, 4, 8, 10), 4, NO_OPTIONAL, 0, 1.0f},
    {"augMaj7", "Augmented Maj7", CORE_INTERVALS(0, 4, 8, 11), 4, NO_OPTIONAL, 0, 1.0f},
    
    // ====== POWER CHORDS & INTERVALS ======
    {"5", "Power Chord", CORE_INTERVALS(0, 7), 2, NO_OPTIONAL, 0, 0.7f},
};

constexpr int CHORD_DEFINITION_COUNT = sizeof(CHORD_DEFINITIONS) / sizeof(ChordDefinition);

// Scoring constants (from Python implementation)
constexpr float MIN_ACCEPTABLE_SCORE = 0.6f;
constexpr float EXTRA_NOTE_PENALTY_PER_NOTE = 0.05f;
constexpr float MAX_EXTRA_NOTE_PENALTY = 0.15f;
constexpr float OPTIONAL_INTERVAL_WEIGHT_FACTOR = 0.5f;

// Voice separation thresholds (MIDI note numbers)
constexpr int BASS_MAX_NOTE = 52;  // E3 (below this is bass)
constexpr int MELODY_MIN_NOTE = 72; // C5 (above this is melody)

// Time window for rolled chord detection (milliseconds)
// Short window to catch notes played slightly apart in rolled/arpeggiated chords
// Event-driven: this only affects detection, not display hold time
constexpr double DEFAULT_TIME_WINDOW_MS = 150.0;
constexpr int MAX_TIME_WINDOW_NOTES = 32;

} // namespace ChordDetection
