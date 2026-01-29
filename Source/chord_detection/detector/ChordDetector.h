// ChordDetector.h
// Pattern-based MIDI Chord Detection Engine
// 
// Main detector class that orchestrates all chord detection modules.
// This is a slim wrapper that delegates to specialized components.

#pragma once

// Include all module headers for convenience
#include "ChordTypes.h"
#include "ChordPattern.h"
#include "ChordCandidate.h"
#include "NoteUtils.h"
#include "ChordPatterns.h"
#include "ChordScoring.h"
#include "VoicingAnalyzer.h"
#include "ChordFormatter.h"

#include <vector>
#include <string>
#include <map>
#include <memory>

namespace ChordDetection {

/**
 * Pattern-based chord detection engine.
 * 
 * This class is the main entry point for chord detection. It orchestrates
 * the various detection modules:
 * - ChordPatterns: Pattern database initialization
 * - ChordScoring: Candidate scoring algorithms
 * - VoicingAnalyzer: Voicing classification
 * - ChordFormatter: Output formatting
 * - NoteUtils: Note name and conversion utilities
 * 
 * Thread-safety: NOT thread-safe. Use separate instances per thread or
 * synchronize externally.
 * 
 * Example usage:
 * @code
 *     ChordDetector detector;
 *     auto result = detector.detectChord({60, 64, 67});  // C major
 *     if (result) {
 *         std::cout << result->chordName << std::endl;
 *     }
 * @endcode
 */
class ChordDetector {
public:
    // ========================================================================
    // CONSTRUCTION
    // ========================================================================
    
    /**
     * Construct a chord detector.
     * 
     * @param enableContext Enable harmonic context tracking (default: false)
     * @param slashMode Slash chord display mode (default: Auto)
     */
    explicit ChordDetector(bool enableContext = false, 
                          SlashChordMode slashMode = SlashChordMode::Auto);
    
    ~ChordDetector();
    
    // Non-copyable (pattern initialization is expensive)
    ChordDetector(const ChordDetector&) = delete;
    ChordDetector& operator=(const ChordDetector&) = delete;
    
    // Movable
    ChordDetector(ChordDetector&&) noexcept = default;
    ChordDetector& operator=(ChordDetector&&) noexcept = default;
    
    // ========================================================================
    // DETECTION
    // ========================================================================
    
    /**
     * Detect chord from MIDI note numbers.
     * 
     * @param midiNotes Vector of MIDI note numbers (0-127)
     * @return Detected chord or nullptr if no valid chord found
     */
    [[nodiscard]] std::shared_ptr<ChordCandidate> detectChord(
        const std::vector<int>& midiNotes);
    
    // ========================================================================
    // CONFIGURATION
    // ========================================================================
    
    void setSlashChordMode(SlashChordMode mode) noexcept { slashChordMode_ = mode; }
    [[nodiscard]] SlashChordMode getSlashChordMode() const noexcept { return slashChordMode_; }
    
    void setEnableContext(bool enable) noexcept { enableContext_ = enable; }
    [[nodiscard]] bool isContextEnabled() const noexcept { return enableContext_; }
    
    // ========================================================================
    // UTILITIES
    // ========================================================================
    
    /**
     * Parse note string to MIDI number (e.g., "C4" -> 60)
     * @throws std::runtime_error on invalid input
     */
    [[nodiscard]] static int parseNoteInput(const std::string& noteStr) {
        return NoteUtils::parseNoteString(noteStr);
    }
    
    /**
     * Get note name for pitch class
     */
    [[nodiscard]] static std::string getNoteName(int pitchClass, bool preferSharp = true) {
        return NoteUtils::getNoteName(pitchClass, preferSharp);
    }
    
    /**
     * Format detection result for display/logging
     */
    [[nodiscard]] std::string formatOutput(const std::shared_ptr<ChordCandidate>& result) const {
        return ChordFormatter::formatDetailed(result, chordPatterns_);
    }
    
    /**
     * Access chord patterns (for quality lookup)
     */
    [[nodiscard]] const std::map<std::string, ChordPattern>& getChordPatterns() const noexcept {
        return chordPatterns_;
    }
    
    /**
     * Clear chord history (context tracking)
     */
    void clearHistory() noexcept { chordHistory_.clear(); }
    
private:
    // ========================================================================
    // DETECTION INTERNALS
    // ========================================================================
    
    [[nodiscard]] std::shared_ptr<ChordCandidate> resolveAmbiguity(
        const std::vector<std::shared_ptr<ChordCandidate>>& candidates,
        int bassPitchClass) const;
    
    // ========================================================================
    // MEMBER DATA
    // ========================================================================
    
    bool enableContext_;
    SlashChordMode slashChordMode_;
    
    // Pattern database
    std::map<std::string, ChordPattern> chordPatterns_;
    std::map<std::vector<int>, std::vector<std::string>> intervalIndex_;
    
    // Context tracking (optional)
    std::vector<std::shared_ptr<ChordCandidate>> chordHistory_;
};

// ============================================================================
// BACKWARD COMPATIBILITY ALIAS
// ============================================================================

// Alias for code that used OptimizedChordDetector
using OptimizedChordDetector = ChordDetector;

} // namespace ChordDetection
