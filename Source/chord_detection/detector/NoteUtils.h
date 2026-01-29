// NoteUtils.h
// Note name utilities and conversion functions
//
// Provides functions for working with note names, pitch classes,
// and MIDI note parsing.

#pragma once

#include "ChordTypes.h"
#include <string>
#include <vector>
#include <map>

namespace ChordDetection {
namespace NoteUtils {

// ============================================================================
// NOTE NAME LOOKUP TABLES
// ============================================================================

/// Sharp note names (C, C♯, D, D♯, E, F, F♯, G, G♯, A, A♯, B)
inline const std::vector<std::string> kNoteNamesSharp = {
    "C", "C♯", "D", "D♯", "E", "F", "F♯", "G", "G♯", "A", "A♯", "B"
};

/// Flat note names (C, D♭, D, E♭, E, F, G♭, G, A♭, A, B♭, B)
inline const std::vector<std::string> kNoteNamesFlat = {
    "C", "D♭", "D", "E♭", "E", "F", "G♭", "G", "A♭", "A", "B♭", "B"
};

/// Enharmonic equivalents for input normalization
inline const std::map<std::string, std::string> kEnharmonicMap = {
    {"C#", "C♯"}, {"Db", "D♭"}, {"D#", "D♯"}, {"Eb", "E♭"},
    {"E#", "F"},  {"Fb", "E"},  {"F#", "F♯"}, {"Gb", "G♭"},
    {"G#", "G♯"}, {"Ab", "A♭"}, {"A#", "A♯"}, {"Bb", "B♭"},
    {"B#", "C"},  {"Cb", "B"}
};

/// Degree names for interval display
inline const std::map<int, std::string> kDegreeMap = {
    {0, "R"}, {1, "♭9"}, {2, "9"}, {3, "♭3/♯9"}, {4, "3"}, {5, "11"}, 
    {6, "♭5/♯11"}, {7, "5"}, {8, "♯5/♭13"}, {9, "6/13"}, {10, "♭7"}, 
    {11, "7"}, {12, "R"}, {13, "♭9"}, {14, "9"}, {15, "♯9"}, {16, "m10"}, 
    {17, "11"}, {18, "♯11"}, {19, "m12"}, {20, "♭13"}, {21, "13"}, 
    {22, "m14"}, {23, "m15"}
};

// ============================================================================
// PITCH CLASS FUNCTIONS
// ============================================================================

/**
 * Convert MIDI note number to pitch class (0-11).
 * 
 * @param midiNote MIDI note number (0-127)
 * @return Pitch class (0=C, 1=C#, ..., 11=B)
 */
[[nodiscard]] inline constexpr int midiToPitchClass(int midiNote) noexcept {
    return midiNote % kPitchClassCount;
}

/**
 * Normalize pitch class to 0-11 range (handles negatives).
 * 
 * @param pitchClass Any integer pitch class
 * @return Normalized pitch class (0-11)
 */
[[nodiscard]] inline constexpr int normalizePitchClass(int pitchClass) noexcept {
    return ((pitchClass % kPitchClassCount) + kPitchClassCount) % kPitchClassCount;
}

/**
 * Calculate interval between two pitch classes.
 * 
 * @param from Starting pitch class
 * @param to Ending pitch class
 * @return Interval in semitones (0-11)
 */
[[nodiscard]] inline constexpr int intervalBetween(int from, int to) noexcept {
    return (to - from + kPitchClassCount) % kPitchClassCount;
}

// ============================================================================
// NOTE NAME FUNCTIONS
// ============================================================================

/**
 * Get note name for a pitch class.
 * 
 * @param pitchClass Pitch class (0-11)
 * @param preferSharp If true, use sharps; if false, use flats
 * @return Note name string (e.g., "C", "C♯", "D♭")
 */
[[nodiscard]] inline std::string getNoteName(int pitchClass, bool preferSharp = true) {
    const int pc = normalizePitchClass(pitchClass);
    return preferSharp 
        ? kNoteNamesSharp[static_cast<size_t>(pc)] 
        : kNoteNamesFlat[static_cast<size_t>(pc)];
}

/**
 * Get degree name for an interval.
 * 
 * @param interval Interval in semitones
 * @return Degree name (e.g., "R", "3", "♭7") or interval number as string
 */
[[nodiscard]] inline std::string getDegreeName(int interval) {
    auto it = kDegreeMap.find(interval);
    return (it != kDegreeMap.end()) ? it->second : std::to_string(interval);
}

/**
 * Parse a note string to MIDI number.
 * 
 * Supports formats: "C4", "F#3", "Bb5", "C♯4", "D♭2"
 * Default octave is 4 if not specified.
 * 
 * @param noteStr Note string to parse
 * @return MIDI note number (0-127)
 * @throws std::runtime_error on invalid input
 */
[[nodiscard]] int parseNoteString(const std::string& noteStr);

/**
 * Replace {root} placeholder in a display template.
 * 
 * @param pattern Display pattern with {root} placeholder
 * @param rootName Root note name to substitute
 * @return Pattern with {root} replaced
 */
[[nodiscard]] inline std::string replaceRootTemplate(
    const std::string& pattern, 
    const std::string& rootName) 
{
    std::string result = pattern;
    size_t pos = result.find("{root}");
    if (pos != std::string::npos) {
        result.replace(pos, 6, rootName);
    }
    return result;
}

} // namespace NoteUtils
} // namespace ChordDetection
