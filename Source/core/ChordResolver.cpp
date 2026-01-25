#include "ChordResolver.h"
#include <cstring>
#include <cmath>

namespace ChordDetection
{

ChordResolver::ChordResolver()
    : lastChordChangeTime_(0.0)
{
}

ResolvedChord ChordResolver::resolve(
    const ScoredCandidate* scoredCandidates,
    int candidateCount,
    const ScoringContext& context,
    double currentTimeMs
)
{
    ResolvedChord result;
    result.timestamp = currentTimeMs;
    result.candidatesConsidered = candidateCount;
    
    // No candidates = no chord
    if (candidateCount == 0 || scoredCandidates == nullptr)
    {
        result.confidence = 0.0f;
        result.buildDisplayName();
        previousChord_ = result;
        return result;
    }
    
    // Start with the highest-confidence candidate (array is pre-sorted)
    const ScoredCandidate* best = &scoredCandidates[0];
    
    // Apply resolution rules
    for (int i = 1; i < candidateCount; ++i)
    {
        const ScoredCandidate& candidate = scoredCandidates[i];
        
        if (shouldPrefer(candidate, *best))
        {
            best = &candidate;
        }
    }
    
    // Check minimum confidence threshold
    if (best->confidence < config_.minimumConfidence)
    {
        // Below threshold - keep previous chord if we have one and it's recent
        double timeSinceChange = currentTimeMs - lastChordChangeTime_;
        if (previousChord_.isValid() && timeSinceChange < 300.0)
        {
            // Stick with previous chord (hysteresis)
            return previousChord_;
        }
        
        // No valid chord
        result.confidence = 0.0f;
        result.buildDisplayName();
        previousChord_ = result;
        return result;
    }
    
    // Build resolved chord
    result.chord = best->hypothesis;
    result.confidence = best->confidence;
    result.bassPitchClass = context.bassPitchClass;
    
    // Determine if slash chord
    result.isSlashChord = (result.chord.bassPitchClass != result.chord.rootPitchClass)
                        && (result.chord.bassPitchClass >= 0);
    
    // If slash chords not allowed, adjust bass to root
    if (result.isSlashChord && !config_.allowSlashChords)
    {
        result.chord.bassPitchClass = result.chord.rootPitchClass;
        result.isSlashChord = false;
    }
    
    // Calculate inversion
    result.inversionType = calculateInversionType(result.chord);
    
    // Build display name
    result.buildDisplayName();
    
    // Track chord change
    if (!previousChord_.isValid() || !result.chord.isSameChord(previousChord_.chord))
    {
        lastChordChangeTime_ = currentTimeMs;
    }
    
    previousChord_ = result;
    return result;
}

void ChordResolver::reset()
{
    previousChord_ = ResolvedChord();
    lastChordChangeTime_ = 0.0;
}

bool ChordResolver::shouldPrefer(
    const ScoredCandidate& a,
    const ScoredCandidate& b
) const
{
    // Rule 1: Prefer temporally stable chord (if within margin)
    float scoreDiff = b.confidence - a.confidence;
    
    // If A has significantly higher temporal stability, prefer it
    float stabilityDiff = a.temporalStabilityScore - b.temporalStabilityScore;
    if (stabilityDiff > 0.2f && scoreDiff < config_.stabilityMargin)
    {
        return true;
    }
    
    // Rule 2: Prefer simpler quality if scores are close
    const ChordFormula& formulaA = CHORD_FORMULAS[a.hypothesis.formulaIndex];
    const ChordFormula& formulaB = CHORD_FORMULAS[b.hypothesis.formulaIndex];
    
    if (formulaA.complexity < formulaB.complexity && scoreDiff < config_.simplicityMargin)
    {
        return true;
    }
    
    // Rule 3: Prefer previous chord for stability (hysteresis)
    if (matchesPreviousChord(a) && !matchesPreviousChord(b) && scoreDiff < config_.stabilityMargin)
    {
        return true;
    }
    
    // Default: higher confidence wins
    return a.confidence > b.confidence;
}

bool ChordResolver::matchesPreviousChord(const ScoredCandidate& candidate) const
{
    if (!previousChord_.isValid())
        return false;
    
    return candidate.hypothesis.isSameChord(previousChord_.chord);
}

int ChordResolver::calculateInversionType(const ChordHypothesis& hyp) const
{
    if (!hyp.isValid() || hyp.bassPitchClass < 0)
        return 0;
    
    // Root position
    if (hyp.bassPitchClass == hyp.rootPitchClass)
        return 0;
    
    const ChordFormula& formula = CHORD_FORMULAS[hyp.formulaIndex];
    int bassInterval = (hyp.bassPitchClass - hyp.rootPitchClass + 12) % 12;
    
    // Check required intervals for inversions
    for (int i = 1; i < formula.requiredCount; ++i)  // Start at 1 (skip root)
    {
        if (formula.requiredIntervals[i] == bassInterval)
        {
            return i;  // 1st, 2nd, 3rd inversion
        }
    }
    
    // Not a standard inversion - slash chord
    return -1;
}

} // namespace ChordDetection
