#pragma once

#include "MidiNoteState.h"
#include "CandidateGenerator.h"
#include "HarmonicMemory.h"
#include "ConfidenceScorer.h"
#include "ChordResolver.h"

namespace ChordDetection
{

/**
 * EngineConfig - Configuration for the chord detection engine
 */
struct EngineConfig
{
    // Minimum notes to detect a chord
    int minimumNotes;
    
    // Harmonic memory window (ms)
    double memoryWindowMs;
    
    // Time decay half-life (ms)
    double decayHalfLifeMs;
    
    // Minimum confidence for chord output
    float minimumConfidence;
    
    // Allow slash chords
    bool allowSlashChords;
    
    EngineConfig()
        : minimumNotes(2)
        , memoryWindowMs(200.0)
        , decayHalfLifeMs(100.0)
        , minimumConfidence(0.45f)
        , allowSlashChords(true)
    {}
};

/**
 * ChordDetectorEngine - Main orchestrator for chord detection
 * 
 * As per spec architecture:
 * 
 * MIDI Input
 *    ↓
 * Note Tracker (MidiNoteState) - stateful
 *    ↓
 * Pitch-Class Normalizer (integrated)
 *    ↓
 * Candidate Chord Generator (CandidateGenerator)
 *    ↓
 * Harmonic Memory Layer (HarmonicMemory) - temporal reasoning
 *    ↓
 * Confidence Scoring Engine (ConfidenceScorer)
 *    ↓
 * Chord Resolver (ChordResolver)
 *    ↓
 * UI / MIDI Output
 * 
 * Key design principle:
 * "Chords are hypotheses evolving over time, not instant detections."
 * 
 * Real-time safe:
 * - No heap allocations
 * - Fixed-size buffers
 * - Unit-testable without JUCE UI
 */
class ChordDetectorEngine
{
public:
    ChordDetectorEngine();
    ~ChordDetectorEngine() = default;
    
    /**
     * Process incoming MIDI note events
     * Call these from the audio thread on MIDI messages
     */
    void noteOn(int noteNumber, float velocity, double currentTimeMs);
    void noteOff(int noteNumber, double currentTimeMs);
    void setSustainPedal(bool isPressed, double currentTimeMs);
    void allNotesOff(double currentTimeMs);
    
    /**
     * Detect chord based on current note state
     * 
     * Call this after processing MIDI events, or periodically.
     * Returns the resolved chord with confidence score.
     * 
     * @param currentTimeMs Current timestamp in milliseconds
     * @return Resolved chord result
     */
    ResolvedChord detectChord(double currentTimeMs);
    
    /**
     * Get the most recent resolved chord without re-processing
     */
    const ResolvedChord& getCurrentChord() const { return currentChord_; }
    
    /**
     * Get read-only access to the note state (for UI display)
     */
    const MidiNoteState& getNoteState() const { return noteState_; }
    
    /**
     * Reset all state (call on playback stop, etc.)
     */
    void reset();
    
    /**
     * Configure the engine
     */
    void setConfig(const EngineConfig& config);
    EngineConfig getConfig() const;
    
    /**
     * Access to subsystems for advanced configuration
     */
    CandidateGenerator& getCandidateGenerator() { return candidateGenerator_; }
    HarmonicMemory& getHarmonicMemory() { return harmonicMemory_; }
    ConfidenceScorer& getConfidenceScorer() { return confidenceScorer_; }
    ChordResolver& getChordResolver() { return chordResolver_; }
    
private:
    // Core components
    MidiNoteState noteState_;
    CandidateGenerator candidateGenerator_;
    HarmonicMemory harmonicMemory_;
    ConfidenceScorer confidenceScorer_;
    ChordResolver chordResolver_;
    
    // Current state
    ResolvedChord currentChord_;
    double lastProcessTime_;
    
    // Temporary buffers (pre-allocated, reused)
    std::array<ChordHypothesis, MAX_CANDIDATES_PER_FRAME> candidateBuffer_;
    std::array<ReinforcedCandidate, MAX_REINFORCED_CANDIDATES> reinforcedBuffer_;
    std::array<ScoredCandidate, MAX_SCORED_CANDIDATES> scoredBuffer_;
    
    // Configuration
    int minimumNotes_;
    
    /**
     * Build scoring context from current state
     */
    ScoringContext buildScoringContext(double currentTimeMs) const;
};

} // namespace ChordDetection
