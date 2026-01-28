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
    
    // Include baseScore from CandidateGenerator (has extension bonus and priority)
    // baseScore typically ranges 0.8-1.2, normalize to add as bonus
    if (hyp.baseScore > 0.0f)
    {
        // Add as small differentiating factor
        float baseBonus = (hyp.baseScore - 0.9f) * 0.3f;  // e.g., 1.1 → +0.06
        score += baseBonus;
    }
    
    // Clamp to [0, 1]
    result.confidence = std::max(0.0f, std::min(1.0f, score));
    
    return result;
}

float ConfidenceScorer::calculateIntervalCoverage(
    const ChordHypothesis& hyp
) const
{
    // H-WCTM path: use cosineSimilarity directly
    if (hyp.cosineSimilarity > 0.0f)
    {
        // Cosine similarity is already 0-1, just return it
        return hyp.cosineSimilarity;
    }
    
    // Legacy path: use requiredMatched / requiredTotal
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
    // H-WCTM path: no penalty if we have high cosine similarity
    if (hyp.cosineSimilarity > 0.0f)
    {
        // Invert similarity: high similarity = low penalty
        // 0.9 similarity -> 0.1 * 0.5 = 0.05 penalty
        // 0.5 similarity -> 0.5 * 0.5 = 0.25 penalty
        return (1.0f - hyp.cosineSimilarity) * 0.5f;
    }
    
    // Legacy path: penalty for missing core tones
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
        return 0.80f;  // No bass info - neutral
    
    // ROOT POSITION - Strong preference for bass = root
    // Critical for disambiguating enharmonic equivalents:
    // - Csus2 (root=C, bass=C) vs D7sus4/C (root=D, bass=C) - same notes!
    // - C6 (root=C, bass=C) vs Am7/C (root=A, bass=C) - same notes!
    if (hyp.bassPitchClass == hyp.rootPitchClass)
    {
        // Scale bonus by template match quality
        // High similarity = strong bonus, low similarity = weak bonus
        float matchBonus = 0.0f;
        if (hyp.cosineSimilarity >= 0.70f)
        {
            // Good template match + root position = definitive
            matchBonus = 0.12f;
        }
        else if (hyp.cosineSimilarity >= 0.55f)
        {
            // Moderate match + root position = prefer
            matchBonus = 0.06f;
        }
        // Complexity bonus (simpler root position chords are more likely intended)
        if (hyp.complexity == 1)
            matchBonus += 0.04f;
        else if (hyp.complexity == 2)
            matchBonus += 0.02f;
        
        return 1.0f + matchBonus;
    }
    
    // Calculate bass interval relative to root
    int bassInterval = (bassPitchClass - hyp.rootPitchClass + 12) % 12;
    
    // Standard inversions (bass is a chord tone) - good but not as preferred as root
    // 3 = minor 3rd, 4 = major 3rd, 7 = perfect 5th
    if (bassInterval == 3 || bassInterval == 4 || bassInterval == 7)
        return 0.94f;  // 1st/2nd inversion
    
    // 7th in bass (3rd inversion)
    if (bassInterval == 10 || bassInterval == 11)
        return 0.88f;
    
    // Extension in bass (9th or 6th)
    if (bassInterval == 2 || bassInterval == 9)
        return 0.80f;
    
    // Non-chord-tone bass (true slash chord) - penalize
    return 0.55f;
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
    // Use hypothesis.complexity directly (set from template in H-WCTM path)
    // Complexity 1 = no penalty, higher = more penalty
    // Scale: complexity 1 = 0.0, complexity 4 = 0.3
    return (hyp.complexity - 1) * 0.1f;
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
