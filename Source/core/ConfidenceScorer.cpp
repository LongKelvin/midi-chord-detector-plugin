#include "ConfidenceScorer.h"
#include <algorithm>
#include <cmath>

namespace ChordDetection
{

ConfidenceScorer::ConfidenceScorer()
    : minimumConfidence_(0.4f)
{
}

int ConfidenceScorer::scoreAllCandidates(
    const ReinforcedCandidate* reinforcedCandidates,
    int candidateCount,
    const ScoringContext& context,
    ScoredCandidate* outScored
) const
{
    int scoredCount = 0;
    
    for (int i = 0; i < candidateCount && scoredCount < MAX_SCORED_CANDIDATES; ++i)
    {
        const ReinforcedCandidate& rc = reinforcedCandidates[i];
        
        if (!rc.hypothesis.isValid())
            continue;
        
        ScoredCandidate scored = scoreCandidate(rc, context);
        
        if (scored.confidence >= minimumConfidence_)
        {
            outScored[scoredCount++] = scored;
        }
    }
    
    // Sort by confidence (descending)
    std::sort(outScored, outScored + scoredCount,
        [](const ScoredCandidate& a, const ScoredCandidate& b) {
            return a.confidence > b.confidence;
        });
    
    return scoredCount;
}

ScoredCandidate ConfidenceScorer::scoreCandidate(
    const ReinforcedCandidate& candidate,
    const ScoringContext& context
) const
{
    ScoredCandidate result;
    result.hypothesis = candidate.hypothesis;
    
    const ChordHypothesis& hyp = candidate.hypothesis;
    
    // Calculate individual factors
    result.intervalCoverageScore = calculateIntervalCoverage(hyp);
    result.missingCoreScore = calculateMissingCorePenalty(hyp);
    result.bassAlignmentScore = calculateBassAlignment(hyp, context.bassPitchClass);
    result.temporalStabilityScore = calculateTemporalStability(candidate);
    result.complexityPenaltyScore = calculateComplexityPenalty(hyp);
    result.velocityWeightScore = calculateVelocityWeight(hyp, context.averageVelocity);
    
    // Combine scores (as per spec pseudocode)
    float score = 0.0f;
    
    score += result.intervalCoverageScore * weights_.intervalCoverage;
    score -= result.missingCoreScore * weights_.missingCoreTones;
    score += result.bassAlignmentScore * weights_.bassAlignment;
    score += result.temporalStabilityScore * weights_.temporalStability;
    score -= result.complexityPenaltyScore * weights_.complexityPenalty;
    score += result.velocityWeightScore * weights_.velocityWeight;
    
    // Clamp to [0, 1]
    result.confidence = std::max(0.0f, std::min(1.0f, score));
    
    return result;
}

float ConfidenceScorer::calculateIntervalCoverage(
    const ChordHypothesis& hyp
) const
{
    if (hyp.requiredTotal <= 0)
        return 0.0f;
    
    // Full credit for all required intervals, proportional otherwise
    float ratio = static_cast<float>(hyp.requiredMatched) / static_cast<float>(hyp.requiredTotal);
    
    // Add small bonus for optional intervals
    float optionalBonus = (hyp.optionalMatched > 0) ? 0.1f : 0.0f;
    
    return std::min(ratio + optionalBonus, 1.0f);
}

float ConfidenceScorer::calculateMissingCorePenalty(
    const ChordHypothesis& hyp
) const
{
    // Penalty for missing core tones
    // The hypothesis already tracks this
    if (hyp.requiredTotal <= 0)
        return 0.0f;
    
    int missing = hyp.requiredTotal - hyp.requiredMatched;
    
    // Scale penalty based on how critical the missing notes are
    // More missing = higher penalty
    return std::min(static_cast<float>(missing) * 0.25f, 1.0f);
}

float ConfidenceScorer::calculateBassAlignment(
    const ChordHypothesis& hyp,
    int bassPitchClass
) const
{
    if (bassPitchClass < 0)
        return 0.5f;  // No bass info - neutral
    
    // Root position = highest score
    if (hyp.bassPitchClass == hyp.rootPitchClass)
        return 1.0f;
    
    // Bass is a chord tone (inversion) = medium score
    const ChordFormula& formula = CHORD_FORMULAS[hyp.formulaIndex];
    int bassInterval = (bassPitchClass - hyp.rootPitchClass + 12) % 12;
    
    // Check if bass is in required intervals
    for (int i = 0; i < formula.requiredCount; ++i)
    {
        if (formula.requiredIntervals[i] == bassInterval)
            return 0.7f;  // Inversion
    }
    
    // Check if bass is in optional intervals
    for (int i = 0; i < formula.optionalCount; ++i)
    {
        if (formula.optionalIntervals[i] == bassInterval)
            return 0.5f;  // Extension in bass
    }
    
    // Slash chord (bass not in chord) = lower score
    return 0.3f;
}

float ConfidenceScorer::calculateTemporalStability(
    const ReinforcedCandidate& rc
) const
{
    // Temporal stability is very important (spec: "Very High" weight)
    // Based on:
    // 1. Reinforcement score (accumulated over frames)
    // 2. Frame count (how many frames it appeared in)
    // 3. Duration (how long it's been present)
    
    // Normalize reinforcement score (typical range 0-3)
    float normalizedReinforcement = std::min(rc.reinforcementScore / 2.0f, 1.0f);
    
    // Frame count factor (more frames = more stable)
    float frameCountFactor = std::min(static_cast<float>(rc.frameCount) / 5.0f, 1.0f);
    
    // Stability metric from duration
    float stability = rc.getStability();
    float stabilityFactor = std::min(stability / 10.0f, 1.0f);
    
    // Combine factors
    return (normalizedReinforcement * 0.5f + frameCountFactor * 0.3f + stabilityFactor * 0.2f);
}

float ConfidenceScorer::calculateComplexityPenalty(
    const ChordHypothesis& hyp
) const
{
    const ChordFormula& formula = CHORD_FORMULAS[hyp.formulaIndex];
    
    // Complexity 1 = no penalty, higher = more penalty
    // Scale: complexity 1 = 0.0, complexity 4 = 0.3
    return (formula.complexity - 1) * 0.1f;
}

float ConfidenceScorer::calculateVelocityWeight(
    const ChordHypothesis& hyp,
    float avgVelocity
) const
{
    (void)hyp;  // Future: could weight by note velocities
    
    // For now, just use average velocity as a factor
    // Higher velocity = more confidence (player is more decisive)
    return avgVelocity * 0.5f + 0.25f;
}

} // namespace ChordDetection
