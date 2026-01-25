#include "CandidateGenerator.h"
#include <cstring>
#include <cstdio>
#include <cmath>

namespace ChordDetection
{

void ChordHypothesis::getChordName(char* buffer, int bufferSize) const
{
    if (!isValid() || bufferSize < 16)
    {
        snprintf(buffer, bufferSize, "N.C.");
        return;
    }
    
    const ChordFormula& formula = CHORD_FORMULAS[formulaIndex];
    const char* rootName = PITCH_CLASS_NAMES[rootPitchClass];
    
    if (bassPitchClass == rootPitchClass || bassPitchClass < 0)
    {
        // Root position
        snprintf(buffer, bufferSize, "%s%s", rootName, formula.symbol);
    }
    else
    {
        // Slash chord
        const char* bassName = PITCH_CLASS_NAMES[bassPitchClass];
        snprintf(buffer, bufferSize, "%s%s/%s", rootName, formula.symbol, bassName);
    }
}

// ============================================================================
// CandidateGenerator Implementation
// ============================================================================

CandidateGenerator::CandidateGenerator()
    : minimumNoteCount_(2)
    , minimumBaseScore_(0.5f)
{
}

int CandidateGenerator::generateCandidates(
    const std::bitset<12>& pitchClasses,
    int bassPitchClass,
    double currentTimeMs,
    ChordHypothesis* outCandidates
) const
{
    int candidateCount = 0;
    int noteCount = static_cast<int>(pitchClasses.count());
    
    // Must have minimum notes
    if (noteCount < minimumNoteCount_)
        return 0;
    
    // Test all 12 possible roots
    for (int rootPC = 0; rootPC < 12; ++rootPC)
    {
        // Skip if root pitch class is not present (except for rootless voicings)
        // For now, prefer root in the voicing
        bool rootPresent = pitchClasses[rootPC];
        
        // Normalize pitch classes relative to this root
        std::bitset<12> relativePCs;
        for (int pc = 0; pc < 12; ++pc)
        {
            if (pitchClasses[pc])
            {
                int relativePC = (pc - rootPC + 12) % 12;
                relativePCs[relativePC] = true;
            }
        }
        
        // Test against all chord formulas
        for (int fIdx = 0; fIdx < CHORD_FORMULA_COUNT; ++fIdx)
        {
            const ChordFormula& formula = CHORD_FORMULAS[fIdx];
            
            ChordHypothesis hyp;
            hyp.rootPitchClass = rootPC;
            hyp.bassPitchClass = bassPitchClass;
            hyp.formulaIndex = fIdx;
            hyp.timestamp = currentTimeMs;
            
            float score = scoreFormula(relativePCs, formula, hyp);
            
            // Apply root presence bonus
            if (rootPresent)
                score *= 1.1f;
            else
                score *= 0.85f;  // Rootless voicing penalty
            
            // Apply formula's base priority
            score *= formula.basePriority;
            
            hyp.baseScore = score;
            
            // Only include if meets threshold and not forbidden
            if (score >= minimumBaseScore_ && !hyp.hasForbiddenInterval)
            {
                if (candidateCount < MAX_CANDIDATES_PER_FRAME)
                {
                    outCandidates[candidateCount++] = hyp;
                }
            }
        }
    }
    
    return candidateCount;
}

int CandidateGenerator::generateCandidatesFromNoteState(
    const MidiNoteState& noteState,
    double currentTimeMs,
    ChordHypothesis* outCandidates
) const
{
    // Get pitch class set
    std::bitset<12> pitchClasses = noteState.getPitchClassSet();
    
    // Get bass pitch class
    int lowestNote = noteState.getLowestNote();
    int bassPitchClass = (lowestNote >= 0) ? (lowestNote % 12) : -1;
    
    return generateCandidates(pitchClasses, bassPitchClass, currentTimeMs, outCandidates);
}

float CandidateGenerator::scoreFormula(
    const std::bitset<12>& relativePCs,
    const ChordFormula& formula,
    ChordHypothesis& hyp
) const
{
    // Check for forbidden intervals first
    if (formula.hasForbiddenInterval(relativePCs))
    {
        hyp.hasForbiddenInterval = true;
        return 0.0f;
    }
    
    // Count required intervals
    int requiredPresent = 0;
    float requiredScore = 0.0f;
    float totalRequiredWeight = 0.0f;
    
    for (int i = 0; i < formula.requiredCount; ++i)
    {
        int interval = formula.requiredIntervals[i];
        if (interval < 0) break;
        
        float weight = INTERVAL_IMPORTANCE[interval];
        totalRequiredWeight += weight;
        
        if (relativePCs[interval])
        {
            requiredScore += weight;
            ++requiredPresent;
        }
    }
    
    hyp.requiredMatched = requiredPresent;
    hyp.requiredTotal = formula.requiredCount;
    
    // All required intervals must be present for complex chords
    if (formula.requiredCount > 3 && requiredPresent < formula.requiredCount)
    {
        return 0.0f;
    }
    
    // For triads, need at least 2 of 3 (allows omissions)
    if (formula.requiredCount <= 3 && requiredPresent < 2)
    {
        return 0.0f;
    }
    
    // Base score from required intervals
    float baseScore = (totalRequiredWeight > 0.0f) 
        ? (requiredScore / totalRequiredWeight) 
        : 0.0f;
    
    // Bonus for optional intervals
    int optionalPresent = 0;
    float optionalScore = 0.0f;
    
    for (int i = 0; i < formula.optionalCount; ++i)
    {
        int interval = formula.optionalIntervals[i];
        if (interval < 0) break;
        
        if (relativePCs[interval])
        {
            optionalScore += INTERVAL_IMPORTANCE[interval] * 0.3f;  // Extensions worth less
            ++optionalPresent;
        }
    }
    
    hyp.optionalMatched = optionalPresent;
    
    // Add optional bonus (capped)
    baseScore += std::min(optionalScore / totalRequiredWeight, 0.2f);
    
    // Penalty for extra notes not in formula
    int extraCount = 0;
    for (int pc = 0; pc < 12; ++pc)
    {
        if (!relativePCs[pc]) continue;
        
        bool inFormula = false;
        
        // Check required
        for (int i = 0; i < formula.requiredCount && formula.requiredIntervals[i] >= 0; ++i)
        {
            if (formula.requiredIntervals[i] == pc)
            {
                inFormula = true;
                break;
            }
        }
        
        // Check optional
        if (!inFormula)
        {
            for (int i = 0; i < formula.optionalCount && formula.optionalIntervals[i] >= 0; ++i)
            {
                if (formula.optionalIntervals[i] == pc)
                {
                    inFormula = true;
                    break;
                }
            }
        }
        
        if (!inFormula)
            ++extraCount;
    }
    
    hyp.extraNotes = extraCount;
    
    // Extra note penalty (light - allows extensions not explicitly listed)
    float penalty = static_cast<float>(extraCount) * 0.08f;
    baseScore = std::max(baseScore - penalty, 0.0f);
    
    // Complexity penalty (prefer simpler chords when ambiguous)
    float complexityPenalty = (formula.complexity - 1) * 0.02f;
    baseScore -= complexityPenalty;
    
    return std::max(baseScore, 0.0f);
}

} // namespace ChordDetection
