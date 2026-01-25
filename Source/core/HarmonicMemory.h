#pragma once

#include "CandidateGenerator.h"
#include <array>
#include <cstdint>
#include <cmath>

namespace ChordDetection
{

/**
 * HarmonicFrame - A snapshot of chord hypotheses at a moment in time
 * 
 * As per spec: "Maintain a sliding time window of chord hypotheses"
 */
struct HarmonicFrame
{
    double timestamp;
    std::array<ChordHypothesis, MAX_CANDIDATES_PER_FRAME> candidates;
    int candidateCount;
    
    HarmonicFrame()
        : timestamp(0.0)
        , candidateCount(0)
    {}
    
    void clear()
    {
        timestamp = 0.0;
        candidateCount = 0;
    }
    
    bool isEmpty() const { return candidateCount == 0; }
};

// Maximum frames in memory window
constexpr int MAX_HARMONIC_FRAMES = 16;

// Default memory window size (milliseconds)
constexpr double DEFAULT_MEMORY_WINDOW_MS = 200.0;

// Decay half-life (milliseconds)
constexpr double DEFAULT_DECAY_HALF_LIFE_MS = 100.0;

/**
 * ReinforcedCandidate - A chord hypothesis with temporal reinforcement score
 * 
 * Candidates gain score by persisting across frames
 */
struct ReinforcedCandidate
{
    ChordHypothesis hypothesis;
    float reinforcementScore;   // Accumulated temporal score
    int frameCount;             // How many frames this chord appeared in
    double firstSeenTime;       // When this chord first appeared
    double lastSeenTime;        // Most recent frame time
    
    ReinforcedCandidate()
        : reinforcementScore(0.0f)
        , frameCount(0)
        , firstSeenTime(0.0)
        , lastSeenTime(0.0)
    {}
    
    /**
     * Get stability metric (higher = more stable over time)
     */
    float getStability() const
    {
        if (frameCount < 2)
            return 0.0f;
        
        double duration = lastSeenTime - firstSeenTime;
        if (duration <= 0.0)
            return 0.0f;
        
        // Stability = how consistently this chord appeared per unit time
        return static_cast<float>(frameCount / duration) * 100.0f;
    }
};

// Maximum reinforced candidates to track
constexpr int MAX_REINFORCED_CANDIDATES = 24;

/**
 * HarmonicMemory - Temporal reasoning layer for chord detection
 * 
 * Key innovation from spec:
 * "Chords are hypotheses evolving over time, not instant detections."
 * 
 * This layer:
 * 1. Stores recent harmonic frames (sliding window)
 * 2. Applies exponential time decay to old frames
 * 3. Reinforces candidates that persist across frames
 * 4. Provides temporally stable chord hypotheses
 * 
 * Real-time safe: Fixed-size arrays, no heap allocations.
 */
class HarmonicMemory
{
public:
    HarmonicMemory();
    ~HarmonicMemory() = default;
    
    /**
     * Add a new harmonic frame
     * 
     * @param currentTimeMs Current timestamp
     * @param candidates Array of chord hypotheses from CandidateGenerator
     * @param candidateCount Number of candidates
     */
    void addFrame(
        double currentTimeMs,
        const ChordHypothesis* candidates,
        int candidateCount
    );
    
    /**
     * Get temporally reinforced candidates
     * 
     * Applies decay weighting to historical frames and aggregates
     * scores for each unique chord interpretation.
     * 
     * @param outCandidates Output array (must have space for MAX_REINFORCED_CANDIDATES)
     * @return Number of reinforced candidates
     */
    int getReinforcedCandidates(ReinforcedCandidate* outCandidates) const;
    
    /**
     * Get the most stable chord hypothesis
     * 
     * Returns the chord with highest temporal stability
     * (persisted longest/most consistently)
     */
    ChordHypothesis getMostStableCandidate() const;
    
    /**
     * Clear all memory
     */
    void clear();
    
    /**
     * Prune old frames outside the memory window
     */
    void pruneOldFrames(double currentTimeMs);
    
    // Configuration
    void setMemoryWindowMs(double windowMs) { memoryWindowMs_ = windowMs; }
    double getMemoryWindowMs() const { return memoryWindowMs_; }
    
    void setDecayHalfLifeMs(double halfLifeMs) { decayHalfLifeMs_ = halfLifeMs; }
    double getDecayHalfLifeMs() const { return decayHalfLifeMs_; }
    
    // Debug/info
    int getFrameCount() const;
    double getOldestFrameTime() const;
    double getNewestFrameTime() const;
    
private:
    // Circular buffer of frames
    std::array<HarmonicFrame, MAX_HARMONIC_FRAMES> frames_;
    int writeIndex_;
    int frameCount_;
    
    // Configuration
    double memoryWindowMs_;
    double decayHalfLifeMs_;
    
    /**
     * Calculate time decay weight for a frame
     * 
     * Uses exponential decay: weight = exp(-Δt / halfLife)
     */
    float calculateDecayWeight(double frameTime, double currentTime) const;
    
    /**
     * Find or add a candidate to the reinforcement map
     */
    int findOrAddCandidate(
        const ChordHypothesis& hyp,
        ReinforcedCandidate* candidates,
        int& count
    ) const;
};

} // namespace ChordDetection
