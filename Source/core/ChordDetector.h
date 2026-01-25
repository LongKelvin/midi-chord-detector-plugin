#pragma once

#include "ChordTypes.h"
#include "ChordCandidate.h"
#include "MidiNoteState.h"
#include "TimeWindowBuffer.h"
#include <bitset>
#include <array>

namespace ChordDetection
{

/**
 * ChordDetector - Main chord recognition engine
 * 
 * Implements the chord detection algorithm from the Python version:
 * 1. Voice separation (bass, chord tones, melody)
 * 2. Pitch class normalization
 * 3. Candidate generation (all 12 roots × all chord types)
 * 4. Weighted scoring
 * 5. Best match selection
 * 
 * Real-time safe:
 * - No heap allocations
 * - Fixed-size buffers
 * - Stack-only operations
 * - Deterministic timing
 */
class ChordDetector
{
public:
    ChordDetector();
    ~ChordDetector() = default;
    
    /**
     * Detect chord from current MIDI note state
     * 
     * @param noteState Current active notes
     * @param currentTimeMs Current time in milliseconds (for time window)
     * @param useTimeWindow Whether to use rolled chord detection
     * @param minNotes Minimum number of notes required for chord detection
     * @return ChordCandidate result (isValid=false if no chord detected)
     */
    ChordCandidate detectChord(
        const MidiNoteState& noteState,
        double currentTimeMs,
        bool useTimeWindow = true,
        int minNotes = 2
    );
    
    /**
     * Update time window buffer with note events
     * Call this when notes change, before detectChord
     */
    void noteOn(int midiNote, double currentTimeMs);
    void noteOff(int midiNote, double currentTimeMs);
    
    /**
     * Clear all state
     */
    void reset();
    
    /**
     * Configuration
     */
    void setTimeWindow(double windowMs);
    void setMinimumScore(float score);
    
    double getTimeWindow() const { return timeWindow_.getWindowSize(); }
    float getMinimumScore() const { return minimumScore_; }
    
private:
    TimeWindowBuffer timeWindow_;
    float minimumScore_;
    
    // Temporary buffers (reused to avoid allocations)
    mutable std::array<int, 128> noteBuffer_;
    mutable std::bitset<12> pitchClassSet_;
    mutable std::array<float, 12> pitchClassWeights_;
    
    /**
     * Voice separation: split notes into bass, chord, melody
     */
    void separateVoices(
        const int* midiNotes,
        int noteCount,
        VoiceBuckets& outVoices
    ) const;
    
    /**
     * Convert MIDI notes to pitch class set (0-11)
     * Returns pitch class bitset
     */
    std::bitset<12> getPitchClassSet(
        const int* midiNotes,
        int noteCount
    ) const;
    
    /**
     * Score a chord candidate
     * 
     * @param playedPCs Pitch classes being played (relative to root)
     * @param def Chord definition to test
     * @return Weighted score (0.0 - 1.0+)
     */
    float scoreChordCandidate(
        const std::bitset<12>& playedPCs,
        const ChordDefinition& def
    ) const;
    
    /**
     * Determine inversion type from bass interval
     */
    int8_t determineInversion(
        int bassIntervalFromRoot,
        const ChordDefinition& def
    ) const;
    
    /**
     * Context-based score adjustment (for jazz progressions, etc.)
     */
    float adjustScoreForContext(
        const char* chordType,
        float baseScore
    ) const;
};

} // namespace ChordDetection
