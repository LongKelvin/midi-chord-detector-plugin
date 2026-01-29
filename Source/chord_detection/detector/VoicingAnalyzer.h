// VoicingAnalyzer.h
// Voicing classification for chord analysis
//
// Analyzes chord voicings to determine close/open/drop voicing types.

#pragma once

#include "ChordTypes.h"
#include <vector>

namespace ChordDetection {
namespace VoicingAnalyzer {

/**
 * Classify the voicing type of a chord.
 * 
 * Analyzes the distribution of MIDI notes to determine:
 * - Close voicing: all notes within one octave
 * - Open voicing: notes spread beyond one octave
 * - Drop2: second voice from top dropped an octave
 * - Drop3: third voice from top dropped an octave
 * 
 * @param midiNotes MIDI note numbers (will be sorted internally)
 * @return Classified voicing type
 */
[[nodiscard]] VoicingType classifyVoicing(const std::vector<int>& midiNotes);

/**
 * Calculate the span (range) of a set of notes.
 * 
 * @param midiNotes MIDI note numbers
 * @return Difference between highest and lowest notes (semitones)
 */
[[nodiscard]] int calculateSpan(const std::vector<int>& midiNotes);

/**
 * Check if voicing is within one octave.
 * 
 * @param midiNotes MIDI note numbers
 * @return true if all notes fit within 12 semitones
 */
[[nodiscard]] bool isCloseVoicing(const std::vector<int>& midiNotes);

/**
 * Get intervals between adjacent notes.
 * 
 * @param midiNotes MIDI note numbers (assumed sorted)
 * @return Vector of intervals between consecutive notes
 */
[[nodiscard]] std::vector<int> getAdjacentIntervals(const std::vector<int>& midiNotes);

} // namespace VoicingAnalyzer
} // namespace ChordDetection
