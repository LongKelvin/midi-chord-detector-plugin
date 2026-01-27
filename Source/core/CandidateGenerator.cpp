#include "CandidateGenerator.h"
#include "ChromaVector.h"
#include "ChordTemplates.h"
#include <cstring>
#include <cstdio>
#include <cmath>
#include <algorithm>

namespace ChordDetection
{

void ChordHypothesis::getChordName(char* buffer, int bufferSize) const
{
    if (!isValid() || bufferSize < 16)
    {
        snprintf(buffer, bufferSize, "N.C.");
        return;
    }
    
    const char* rootName = PITCH_CLASS_NAMES[rootPitchClass];
    
    // Use template name if available (H-WCTM), fallback to formula only if templateName is null
    // Note: empty templateName ("") means "Major" so output is just root like "C"
    const char* symbol = templateName;
    if (symbol == nullptr)
    {
        // No template name - try legacy formula
        if (formulaIndex >= 0 && formulaIndex < CHORD_FORMULA_COUNT)
        {
            symbol = CHORD_FORMULAS[formulaIndex].symbol;
        }
        else
        {
            symbol = "";
        }
    }
    
    if (bassPitchClass == rootPitchClass || bassPitchClass < 0)
    {
        // Root position
        snprintf(buffer, bufferSize, "%s%s", rootName, symbol);
    }
    else
    {
        // Slash chord
        const char* bassName = PITCH_CLASS_NAMES[bassPitchClass];
        snprintf(buffer, bufferSize, "%s%s/%s", rootName, symbol, bassName);
    }
}

// ============================================================================
// CandidateGenerator Implementation - H-WCTM Algorithm
// ============================================================================

CandidateGenerator::CandidateGenerator()
    : minimumNoteCount_(2)
    , minimumBaseScore_(0.4f)  // Lower threshold for cosine similarity
{
}

/**
 * Build a ChromaVector from the current MidiNoteState
 * Applies bass boost, register weighting, and velocity scaling
 */
ChromaVector CandidateGenerator::buildChromaVector(
    const MidiNoteState& noteState,
    double currentTimeMs
) const
{
    ChromaVector chroma;
    
    // Get lowest note for bass boost determination
    int lowestNote = noteState.getLowestNote();
    
    // Add all active notes with appropriate weighting
    for (int midiNote = 0; midiNote < 128; ++midiNote)
    {
        float velocity = noteState.getNoteVelocity(midiNote);
        if (velocity <= 0.0f)
            continue;
        
        bool isBass = (lowestNote >= 0 && midiNote == lowestNote);
        bool isSustained = noteState.isNoteSustained(midiNote);
        
        // Get note onset time for decay calculation
        double onsetTime = noteState.getNoteOnsetTime(midiNote);
        double age = isSustained ? (currentTimeMs - onsetTime) : 0.0;
        
        chroma.addNote(midiNote, velocity, isBass, DEFAULT_CHROMA_WEIGHTS, age);
    }
    
    return chroma;
}

/**
 * Generate candidates using H-WCTM: Hybrid Weighted Chroma-Template Matching
 */
int CandidateGenerator::generateCandidatesFromNoteState(
    const MidiNoteState& noteState,
    double currentTimeMs,
    ChordHypothesis* outCandidates
) const
{
    int noteCount = noteState.getActiveNoteCount();
    if (noteCount < minimumNoteCount_)
        return 0;
    
    // Build weighted chroma vector
    ChromaVector chroma = buildChromaVector(noteState, currentTimeMs);
    
    // Get bass pitch class
    int lowestNote = noteState.getLowestNote();
    int bassPitchClass = (lowestNote >= 0) ? (lowestNote % 12) : -1;
    
    return generateCandidatesFromChroma(chroma, bassPitchClass, currentTimeMs, outCandidates);
}

/**
 * Core H-WCTM matching: test all root transpositions against all templates
 */
int CandidateGenerator::generateCandidatesFromChroma(
    const ChromaVector& chroma,
    int bassPitchClass,
    double currentTimeMs,
    ChordHypothesis* outCandidates
) const
{
    int candidateCount = 0;
    
    // Temporary array for sorting
    struct ScoredCandidate {
        ChordHypothesis hyp;
        float sortScore;
    };
    
    static ScoredCandidate tempCandidates[12 * CHORD_TEMPLATE_COUNT];
    int tempCount = 0;
    
    // Test all 12 possible roots
    for (int rootPC = 0; rootPC < 12; ++rootPC)
    {
        // Transpose chroma so potential root pitch class is at bin 0
        // This allows direct comparison with templates (which have root at index 0)
        ChromaVector transposed = chroma.shifted(rootPC);
        const auto& bins = transposed.getBins();
        
        // Check if root is significantly present
        bool rootPresent = bins[0] > 0.1f;
        float rootWeight = bins[0];
        
        // Test against all chord templates
        for (int tIdx = 0; tIdx < CHORD_TEMPLATE_COUNT; ++tIdx)
        {
            const ChordTemplate& tpl = CHORD_TEMPLATES[tIdx];
            
            // Calculate cosine similarity
            float similarity = transposed.cosineSimilarityWithTemplate(tpl.bins.data());
            
            // Apply root presence modifier
            float adjustedScore = similarity;
            
            if (tpl.isShell)
            {
                // Shell voicings don't require root
                // But if root IS present, slight bonus
                if (rootPresent)
                    adjustedScore *= 1.05f;
            }
            else
            {
                // Full voicings prefer root
                if (rootPresent)
                    adjustedScore *= 1.1f;
                else
                    adjustedScore *= 0.55f;  // STRONGER penalty without root
            }
            
            // ================================================================
            // CHARACTERISTIC INTERVAL DETECTION
            // These intervals DEFINE the chord type and should be rewarded
            // CRITICAL: Only trust these when root IS present!
            // Without root, interval positions are ambiguous.
            // ================================================================
            
            // Check for intervals (only meaningful when root is present)
            bool hasB5 = bins[6] > 0.1f;
            bool hasP5 = bins[7] > 0.1f;
            bool hasSharp5 = bins[8] > 0.1f;
            bool hasMinor7 = bins[10] > 0.1f;
            bool hasMajor7 = bins[11] > 0.1f;
            bool has9th = bins[2] > 0.1f;
            bool hasM3 = bins[4] > 0.1f;
            bool hasm3 = bins[3] > 0.1f;
            
            // Template name check helpers
            bool isDimTemplate = (tpl.name != nullptr && 
                (strcmp(tpl.name, "dim") == 0 || strcmp(tpl.name, "dim7") == 0 || strcmp(tpl.name, "m7b5") == 0));
            bool isAugTemplate = (tpl.name != nullptr && 
                (strcmp(tpl.name, "aug") == 0 || strcmp(tpl.name, "augMaj7") == 0));
            bool is7thTemplate = (tpl.name != nullptr && 
                (strcmp(tpl.name, "7") == 0 || strcmp(tpl.name, "maj7") == 0 || strcmp(tpl.name, "m7") == 0 ||
                 strcmp(tpl.name, "dim7") == 0 || strcmp(tpl.name, "m7b5") == 0));
            bool is9thTemplate = (tpl.name != nullptr && 
                (strcmp(tpl.name, "9") == 0 || strcmp(tpl.name, "maj9") == 0 || strcmp(tpl.name, "m9") == 0));
            bool isMinorTemplate = (tpl.name != nullptr && 
                (strcmp(tpl.name, "m") == 0 || strcmp(tpl.name, "m7") == 0 || strcmp(tpl.name, "m9") == 0));
            bool isMajorTemplate = (tpl.name != nullptr && strcmp(tpl.name, "") == 0);
            bool isTriadTemplate = (tpl.name != nullptr && 
                (strcmp(tpl.name, "") == 0 || strcmp(tpl.name, "m") == 0 || 
                 strcmp(tpl.name, "dim") == 0 || strcmp(tpl.name, "aug") == 0 ||
                 strcmp(tpl.name, "sus2") == 0 || strcmp(tpl.name, "sus4") == 0));
            
            // ================================================================
            // CRITICAL: Characteristic interval detection ONLY when root is present
            // Without root, what looks like "b5" might just be a different chord
            // ================================================================
            if (rootPresent)
            {
                // Diminished detection: if b5 present without P5, boost dim templates
                if (hasB5 && !hasP5)
                {
                    if (isDimTemplate)
                        adjustedScore *= 1.25f;  // Big boost for dim when b5 is the characteristic
                    else if (isMinorTemplate || isMajorTemplate)
                        adjustedScore *= 0.7f;   // Penalize major/minor when b5 is present
                }
                
                // If P5 is present, penalize dim/aug (they shouldn't have P5)
                if (hasP5)
                {
                    if (isDimTemplate || isAugTemplate)
                        adjustedScore *= 0.6f;   // Strong penalty
                }
                
                // Augmented detection: if #5 present without P5, boost aug templates  
                if (hasSharp5 && !hasP5)
                {
                    if (isAugTemplate)
                        adjustedScore *= 1.25f;  // Big boost for aug when #5 is the characteristic
                    else if (isMajorTemplate)
                        adjustedScore *= 0.7f;   // Penalize major when #5 is present
                }
                
                // 7th chord detection: if 7th interval present, boost 7th templates
                if (hasMajor7 || hasMinor7)
                {
                    if (is7thTemplate || is9thTemplate)
                        adjustedScore *= 1.2f;   // Boost 7th/9th templates when 7th is present
                }
                else
                {
                    // CRITICAL: Penalize 7th/9th templates when NO 7th is present
                    // This prevents triads from being detected as 7th chords
                    if (is7thTemplate)
                        adjustedScore *= 0.65f;  // Strong penalty for 7th template without 7th
                    if (is9thTemplate)
                        adjustedScore *= 0.55f;  // Even stronger penalty for 9th template without 7th
                }
                
                // 9th chord detection: if 9th interval present AND 7th, boost 9th templates
                if (has9th && (hasMajor7 || hasMinor7))
                {
                    if (is9thTemplate)
                        adjustedScore *= 1.15f;  // Extra boost for 9th templates
                    else if (is7thTemplate)
                        adjustedScore *= 0.95f;  // Slight penalty for plain 7th when 9th present
                }
            }
            else
            {
                // Root NOT present - be very skeptical of complex chord interpretations
                // When root is missing, prefer simpler triads over extensions
                if (!isTriadTemplate)
                    adjustedScore *= 0.75f;  // Additional penalty for complex templates without root
            }
            
            // Bass note alignment bonus
            if (bassPitchClass >= 0)
            {
                int bassRelative = (bassPitchClass - rootPC + 12) % 12;
                
                // ================================================================
                // SPECIAL HANDLING FOR SYMMETRIC CHORDS
                // Augmented triads are symmetric (Caug=Eaug=G#aug) with chord tones
                // at intervals 0, 4, 8 (root, M3, #5). All inversions are equivalent.
                // Convention: name by the BASS note when it's a chord tone.
                // ================================================================
                if (isAugTemplate)
                {
                    // Augmented chord tones: root(0), M3(4), #5(8)
                    // If bass is one of these, prefer root=bass
                    if (bassRelative == 0)
                    {
                        // Bass is root - STRONG bonus for symmetric chord naming
                        adjustedScore *= 1.25f;
                    }
                    else if (bassRelative == 4 || bassRelative == 8)
                    {
                        // Bass is M3 or #5 - STRONG penalty (prefer bass as root)
                        adjustedScore *= 0.55f;
                    }
                    else
                    {
                        // Bass not a chord tone - unusual, penalize
                        adjustedScore *= 0.75f;
                    }
                }
                // Diminished 7th chords are also symmetric (every 3 semitones)
                // Chord tones: root(0), m3(3), b5(6), dim7(9)
                else if (tpl.name != nullptr && strcmp(tpl.name, "dim7") == 0)
                {
                    if (bassRelative == 0)
                    {
                        adjustedScore *= 1.25f;
                    }
                    else if (bassRelative == 3 || bassRelative == 6 || bassRelative == 9)
                    {
                        adjustedScore *= 0.55f;
                    }
                    else
                    {
                        adjustedScore *= 0.75f;
                    }
                }
                else
                {
                    // Non-symmetric chords: normal bass handling
                    if (bassRelative == 0)
                    {
                        // Bass is root - small bonus
                        adjustedScore *= 1.02f;
                    }
                    else if (bassRelative == 7)
                    {
                        // Bass is 5th - neutral (2nd inversion)
                        adjustedScore *= 1.0f;
                    }
                    else if (bassRelative == 4 || bassRelative == 3)
                    {
                        // Bass is 3rd (major or minor) - neutral (1st inversion)
                        adjustedScore *= 1.0f;
                    }
                    else if (bassRelative == 10)
                    {
                        // Bass is b7 - acceptable for dominant family
                        adjustedScore *= 0.98f;
                    }
                    else
                    {
                        // Bass is unusual - could be slash chord or mismatch
                        adjustedScore *= 0.90f;
                    }
                }
            }
            
            // Apply template complexity penalty (prefer simpler interpretations)
            // Reduced penalty when template's extension tones are present
            float complexityPenalty = (tpl.complexity - 1) * 0.01f;
            adjustedScore -= complexityPenalty;
            
            // Extension bonus: reward templates whose extension bins match the input
            // Extensions/tensions: b9(1), 9(2), #9(3), 11(5), #11/b5(6), #5/b13(8), 13(9), b7(10), M7(11)
            float extensionBonus = 0.0f;
            constexpr int extensionBins[] = {1, 2, 3, 5, 6, 8, 9, 10, 11};  // All extensions and 7ths
            for (int eb : extensionBins)
            {
                // If template expects this extension AND input has it
                if (tpl.bins[eb] > 0.1f && bins[eb] > 0.1f)
                {
                    extensionBonus += 0.06f;  // Bonus per matched extension
                }
            }
            adjustedScore += extensionBonus;
            
            // Apply template priority as additive bonus (scaled to be meaningful)
            // Power chord (0.7) should get significant penalty: (0.7-1.0)*0.15 = -0.045
            // 7th chords (1.1) should get small bonus: (1.1-1.0)*0.15 = +0.015
            float priorityBonus = (tpl.priority - 1.0f) * 0.15f;
            adjustedScore += priorityBonus;
            
            // Shell voicing slight penalty vs full voicing (all else equal)
            if (tpl.isShell)
                adjustedScore *= 0.95f;
            
            // Only add if above threshold
            if (adjustedScore >= minimumBaseScore_)
            {
                ChordHypothesis hyp;
                hyp.rootPitchClass = rootPC;
                hyp.bassPitchClass = bassPitchClass;
                hyp.formulaIndex = tIdx;  // Use template index as formula index
                hyp.templateName = tpl.name;
                hyp.templateFullName = tpl.fullName;
                hyp.cosineSimilarity = similarity;
                hyp.isShellVoicing = tpl.isShell;
                hyp.complexity = tpl.complexity;
                hyp.baseScore = adjustedScore;
                hyp.timestamp = currentTimeMs;
                
                // Legacy compatibility fields
                hyp.requiredMatched = 0;
                hyp.requiredTotal = 0;
                hyp.optionalMatched = 0;
                hyp.extraNotes = 0;
                hyp.hasForbiddenInterval = false;
                
                if (tempCount < 12 * CHORD_TEMPLATE_COUNT)
                {
                    tempCandidates[tempCount].hyp = hyp;
                    tempCandidates[tempCount].sortScore = adjustedScore;
                    ++tempCount;
                }
            }
        }
    }
    
    // Sort by score descending
    std::sort(tempCandidates, tempCandidates + tempCount,
        [](const ScoredCandidate& a, const ScoredCandidate& b) {
            return a.sortScore > b.sortScore;
        });
    
    // Copy top candidates, removing near-duplicates
    for (int i = 0; i < tempCount && candidateCount < MAX_CANDIDATES_PER_FRAME; ++i)
    {
        bool isDuplicate = false;
        
        // Check if we already have a very similar candidate
        for (int j = 0; j < candidateCount; ++j)
        {
            if (tempCandidates[i].hyp.rootPitchClass == outCandidates[j].rootPitchClass)
            {
                // Same root - check if it's essentially the same chord type
                float scoreDiff = std::abs(tempCandidates[i].sortScore - outCandidates[j].baseScore);
                if (scoreDiff < 0.05f)
                {
                    // Very similar score, probably redundant
                    isDuplicate = true;
                    break;
                }
            }
        }
        
        if (!isDuplicate)
        {
            outCandidates[candidateCount++] = tempCandidates[i].hyp;
        }
    }
    
    return candidateCount;
}

/**
 * Legacy interface using bitset - converts to ChromaVector
 */
int CandidateGenerator::generateCandidates(
    const std::bitset<12>& pitchClasses,
    int bassPitchClass,
    double currentTimeMs,
    ChordHypothesis* outCandidates
) const
{
    // Convert bitset to simple chroma vector (equal weights)
    ChromaVector chroma;
    for (int pc = 0; pc < 12; ++pc)
    {
        if (pitchClasses[pc])
        {
            // Use middle register, moderate velocity
            int midiNote = 60 + pc;  // C4-B4
            bool isBass = (pc == bassPitchClass);
            chroma.addNote(midiNote, 0.7f, isBass, DEFAULT_CHROMA_WEIGHTS, 0.0);
        }
    }
    
    return generateCandidatesFromChroma(chroma, bassPitchClass, currentTimeMs, outCandidates);
}

/**
 * Legacy formula scoring - kept for compatibility but not primary path
 */
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
            optionalScore += INTERVAL_IMPORTANCE[interval] * 0.3f;
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
    
    // Extra note penalty
    float penalty = static_cast<float>(extraCount) * 0.08f;
    baseScore = std::max(baseScore - penalty, 0.0f);
    
    // Complexity penalty
    float complexityPenalty = (formula.complexity - 1) * 0.02f;
    baseScore -= complexityPenalty;
    
    return std::max(baseScore, 0.0f);
}

} // namespace ChordDetection
