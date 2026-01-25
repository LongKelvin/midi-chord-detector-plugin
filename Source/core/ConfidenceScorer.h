#pragma once

#include "HarmonicMemory.h"
#include "MidiNoteState.h"
#include <array>

namespace ChordDetection
{

/**
 * ScoringContext - Additional context for confidence calculation
 */
struct ScoringContext
{
    // Note state info
    int bassPitchClass;
    int totalNoteCount;
    float averageVelocity;
    
    // Temporal info
    double currentTimeMs;
    
    // Previous chord (for transition scoring)
    ChordHypothesis previousChord;
    bool hasPreviousChord;
    
    ScoringContext()
        : bassPitchClass(-1)
        , totalNoteCount(0)
        , averageVelocity(0.5f)
        , currentTimeMs(0.0)
        , hasPreviousChord(false)
    {}
};

/**
 * ScoredCandidate - A chord hypothesis with final confidence score
 */
struct ScoredCandidate
{
    ChordHypothesis hypothesis;
    
    // Individual scoring factors (for debugging/tuning)
    float intervalCoverageScore;    // Required intervals present
    float missingCoreScore;         // Penalty for missing 3rd/7th
    float bassAlignmentScore;       // Root matches bass
    float temporalStabilityScore;   // Persists across frames
    float complexityPenaltyScore;   // Penalty for exotic chords
    float velocityWeightScore;      // Louder notes matter more
    
    // Final combined confidence
    float confidence;
    
    ScoredCandidate()
        : intervalCoverageScore(0.0f)
        , missingCoreScore(0.0f)
        , bassAlignmentScore(0.0f)
        , temporalStabilityScore(0.0f)
        , complexityPenaltyScore(0.0f)
        , velocityWeightScore(0.0f)
        , confidence(0.0f)
    {}
};

// Maximum scored candidates
constexpr int MAX_SCORED_CANDIDATES = 16;

/**
 * Scoring weights (from spec)
 * Can be tuned for different genres/preferences
 */
struct ScoringWeights
{
    float intervalCoverage;     // High
    float missingCoreTones;     // High (penalty)
    float bassAlignment;        // Medium
    float temporalStability;    // Very High
    float complexityPenalty;    // Medium
    float velocityWeight;       // Low
    
    ScoringWeights()
        : intervalCoverage(0.25f)
        , missingCoreTones(0.20f)
        , bassAlignment(0.15f)
        , temporalStability(0.30f)
        , complexityPenalty(0.05f)
        , velocityWeight(0.05f)
    {}
};

/**
 * ConfidenceScorer - Multi-factor confidence calculation
 * 
 * As per spec: "Confidence is evidence-based, not binary."
 * 
 * Calculates confidence as weighted sum of:
 * - Interval coverage (required tones present)
 * - Missing core tones penalty (3rd, 7th)
 * - Bass alignment (root matches bass)
 * - Temporal stability (persists across frames)
 * - Complexity penalty (prefer simpler chords)
 * - Velocity weighting (louder notes matter more)
 * 
 * Real-time safe: No allocations.
 */
class ConfidenceScorer
{
public:
    ConfidenceScorer();
    ~ConfidenceScorer() = default;
    
    /**
     * Score a set of reinforced candidates
     * 
     * @param reinforcedCandidates From HarmonicMemory
     * @param candidateCount Number of candidates
     * @param context Scoring context (bass note, velocity, etc.)
     * @param outScored Output array (must have space for MAX_SCORED_CANDIDATES)
     * @return Number of scored candidates
     */
    int scoreAllCandidates(
        const ReinforcedCandidate* reinforcedCandidates,
        int candidateCount,
        const ScoringContext& context,
        ScoredCandidate* outScored
    ) const;
    
    /**
     * Calculate confidence for a single candidate
     */
    ScoredCandidate scoreCandidate(
        const ReinforcedCandidate& candidate,
        const ScoringContext& context
    ) const;
    
    /**
     * Get/set scoring weights
     */
    const ScoringWeights& getWeights() const { return weights_; }
    void setWeights(const ScoringWeights& weights) { weights_ = weights; }
    
    /**
     * Minimum confidence threshold
     */
    float getMinimumConfidence() const { return minimumConfidence_; }
    void setMinimumConfidence(float conf) { minimumConfidence_ = conf; }
    
private:
    ScoringWeights weights_;
    float minimumConfidence_;
    
    // Individual scoring functions
    float calculateIntervalCoverage(const ChordHypothesis& hyp) const;
    float calculateMissingCorePenalty(const ChordHypothesis& hyp) const;
    float calculateBassAlignment(const ChordHypothesis& hyp, int bassPitchClass) const;
    float calculateTemporalStability(const ReinforcedCandidate& rc) const;
    float calculateComplexityPenalty(const ChordHypothesis& hyp) const;
    float calculateVelocityWeight(const ChordHypothesis& hyp, float avgVelocity) const;
};

} // namespace ChordDetection
