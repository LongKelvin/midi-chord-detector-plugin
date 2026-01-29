// ChordFormatter.h
// Chord result formatting and display
//
// Functions for formatting chord detection results for output.

#pragma once

#include "ChordCandidate.h"
#include "ChordPattern.h"
#include <string>
#include <memory>
#include <map>

namespace ChordDetection {
namespace ChordFormatter {

/**
 * Format a chord detection result for display.
 * 
 * Produces a detailed multi-line output including:
 * - Chord name and confidence
 * - Position/inversion
 * - MIDI notes and note names
 * - Intervals and degrees
 * 
 * @param result Detection result to format
 * @param patterns Pattern database (for quality lookup)
 * @return Formatted string
 */
[[nodiscard]] std::string formatDetailed(
    const std::shared_ptr<ChordCandidate>& result,
    const std::map<std::string, ChordPattern>& patterns);

/**
 * Format chord name only (single line).
 * 
 * @param result Detection result
 * @return Chord name string (e.g., "Cmaj7/E")
 */
[[nodiscard]] std::string formatChordName(
    const std::shared_ptr<ChordCandidate>& result);

/**
 * Format confidence as a string indicator.
 * 
 * @param confidence Confidence value (0.0 - 1.0)
 * @return Indicator string ("[HIGH]", "[MED]", or "[LOW]")
 */
[[nodiscard]] std::string formatConfidenceIndicator(float confidence);

/**
 * Format note list as space-separated string.
 * 
 * @param notes Vector of values to format
 * @return Space-separated string
 */
template<typename T>
[[nodiscard]] std::string formatNoteList(const std::vector<T>& notes) {
    std::string result;
    for (size_t i = 0; i < notes.size(); ++i) {
        if (i > 0) result += " ";
        if constexpr (std::is_same_v<T, std::string>) {
            result += notes[i];
        } else {
            result += std::to_string(notes[i]);
        }
    }
    return result;
}

} // namespace ChordFormatter
} // namespace ChordDetection
