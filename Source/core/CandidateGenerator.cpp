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
    
    // Pre-compute which pitch classes are present for quick lookup
    const auto& rawBins = chroma.getBins();
    bool pitchPresent[12];
    for (int i = 0; i < 12; ++i) {
        pitchPresent[i] = rawBins[i] > 0.1f;
    }
    
    // Test all 12 possible roots
    for (int rootPC = 0; rootPC < 12; ++rootPC)
    {
        // Transpose chroma so potential root pitch class is at bin 0
        ChromaVector transposed = chroma.shifted(rootPC);
        const auto& bins = transposed.getBins();
        
        // Check if root is significantly present
        bool rootPresent = bins[0] > 0.1f;
        float rootWeight = bins[0];
        
        // Pre-compute intervals for this root
        bool hasB2 = bins[1] > 0.1f;  // b9
        bool has2 = bins[2] > 0.1f;   // 9
        bool hasm3 = bins[3] > 0.1f;  // m3 / #9
        bool hasM3 = bins[4] > 0.1f;  // M3
        bool has4 = bins[5] > 0.1f;   // 11
        bool hasB5 = bins[6] > 0.1f;  // b5 / #11
        bool hasP5 = bins[7] > 0.1f;  // P5
        bool hasSharp5 = bins[8] > 0.1f; // #5 / b13
        bool has6 = bins[9] > 0.1f;   // 6 / 13
        bool hasMinor7 = bins[10] > 0.1f; // b7
        bool hasMajor7 = bins[11] > 0.1f; // M7
        
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
                if (rootPresent)
                    adjustedScore *= 1.03f;
            }
            else
            {
                // Full voicings prefer root
                if (rootPresent)
                    adjustedScore *= 1.08f;
                else
                    adjustedScore *= 0.6f;  // Penalty without root
            }
            
            // ================================================================
            // Template type detection (simplified using string prefix matching)
            // ================================================================
            const char* tname = tpl.name;
            bool isDimTemplate = (tname && (strcmp(tname, "dim") == 0 || strcmp(tname, "dim7") == 0 || 
                                           strcmp(tname, "m7b5") == 0 || strcmp(tname, "m7b5(9)") == 0 ||
                                           strcmp(tname, "m11b5") == 0));
            bool isAugTemplate = (tname && (strcmp(tname, "aug") == 0 || strcmp(tname, "augmaj7") == 0 ||
                                           strcmp(tname, "aug7") == 0 || strcmp(tname, "7#5") == 0));
            bool isTriadTemplate = (tname && (strcmp(tname, "") == 0 || strcmp(tname, "m") == 0 || 
                                             strcmp(tname, "dim") == 0 || strcmp(tname, "aug") == 0 ||
                                             strcmp(tname, "sus2") == 0 || strcmp(tname, "sus4") == 0 ||
                                             strcmp(tname, "5") == 0));
            bool isMinorTemplate = (tname && tname[0] == 'm' && (tname[1] == '\0' || tname[1] == '7' || 
                                   tname[1] == '9' || tname[1] == '1' || tname[1] == '6' || tname[1] == 'a'));
            bool isMajorTemplate = (tname && strcmp(tname, "") == 0);
            
            // Detect 7th chord templates
            bool is7thTemplate = (tname && (strstr(tname, "7") != nullptr || strstr(tname, "9") != nullptr ||
                                           strstr(tname, "11") != nullptr || strstr(tname, "13") != nullptr ||
                                           strcmp(tname, "6") == 0 || strcmp(tname, "6/9") == 0));
            bool isDom7Family = (tname && ((strcmp(tname, "7") == 0 || tname[0] == '7' || tname[0] == '9' ||
                                          strncmp(tname, "13", 2) == 0 || strncmp(tname, "11", 2) == 0) &&
                                          tname[0] != 'm'));
            bool isMaj7Family = (tname && strstr(tname, "maj") != nullptr);
            bool isMin7Family = (tname && tname[0] == 'm' && tname[1] != 'a');
            
            // ================================================================
            // CHARACTERISTIC INTERVAL DETECTION
            // Only meaningful when root IS present
            // ================================================================
            if (rootPresent)
            {
                // === DIMINISHED vs MINOR discrimination ===
                // b5 is THE defining characteristic of diminished chords
                if (hasB5 && !hasP5)
                {
                    if (isDimTemplate)
                        adjustedScore *= 1.2f;  // Boost dim templates
                    else if (isMinorTemplate && !isDimTemplate)
                        adjustedScore *= 0.75f;  // Penalize regular minor
                    else if (isMajorTemplate)
                        adjustedScore *= 0.7f;
                }
                
                // If P5 is present, diminished should be penalized
                if (hasP5 && !hasB5)
                {
                    if (isDimTemplate && strcmp(tname, "dim") == 0)
                        adjustedScore *= 0.6f;  // Strong penalty for dim triad when P5 present
                }
                
                // === AUGMENTED vs MAJOR discrimination ===
                if (hasSharp5 && !hasP5)
                {
                    if (isAugTemplate)
                        adjustedScore *= 1.2f;
                    else if (isMajorTemplate)
                        adjustedScore *= 0.7f;
                }
                
                if (hasP5 && !hasSharp5)
                {
                    if (isAugTemplate)
                        adjustedScore *= 0.6f;
                }
                
                // === 7th CHORD DETECTION ===
                // The presence of 7th interval is crucial
                bool has7th = (hasMajor7 || hasMinor7);
                
                if (has7th)
                {
                    if (is7thTemplate)
                        adjustedScore *= 1.15f;  // Boost 7th templates when 7th present
                }
                else
                {
                    // No 7th present - penalize all 7th+ chord templates
                    if (is7thTemplate && !isTriadTemplate)
                        adjustedScore *= 0.7f;
                }
                
                // === MAJOR vs MINOR 7th type ===
                if (hasMajor7 && !hasMinor7)
                {
                    if (isMaj7Family)
                        adjustedScore *= 1.1f;
                    else if (isDom7Family)
                        adjustedScore *= 0.8f;
                }
                else if (hasMinor7 && !hasMajor7)
                {
                    if (isDom7Family || isMin7Family)
                        adjustedScore *= 1.1f;
                    else if (isMaj7Family)
                        adjustedScore *= 0.8f;
                }
                
                // === DOMINANT TRITONE DETECTION ===
                // The tritone (M3 + b7) is THE defining sound of dominant 7ths
                if (hasM3 && hasMinor7)
                {
                    if (isDom7Family)
                        adjustedScore *= 1.12f;  // Strong boost for dom7 family
                }
                
                // === ADD9 DETECTION (no 7th, but has 9th) ===
                // add9 = root, M3, P5, M9 (no 7th)
                // madd9 = root, m3, P5, M9 (no 7th)
                if (has2 && !has7th)
                {
                    bool isAdd9Template = (tname && strcmp(tname, "add9") == 0);
                    bool ismadd9Template = (tname && strcmp(tname, "madd9") == 0);
                    
                    if (hasM3 && hasP5 && isAdd9Template)
                        adjustedScore *= 1.4f;  // VERY strong boost for add9
                    else if (hasm3 && hasP5 && ismadd9Template)
                        adjustedScore *= 1.4f;  // VERY strong boost for madd9
                    else if (is7thTemplate && !isAdd9Template && !ismadd9Template)
                        adjustedScore *= 0.75f;  // Penalize 7th templates when no 7th
                }
                
                // === 6 CHORD DETECTION (M6 present, no b7/M7) ===
                // 6 = root, M3, P5, M6 (no 7th)
                // m6 = root, m3, P5, M6 (no 7th)
                if (has6 && !has7th)
                {
                    bool is6Template = (tname && strcmp(tname, "6") == 0);
                    bool ism6Template = (tname && strcmp(tname, "m6") == 0);
                    bool is69Template = (tname && strcmp(tname, "6/9") == 0);
                    bool ism69Template = (tname && strcmp(tname, "m6/9") == 0);
                    
                    if (hasM3 && hasP5 && (is6Template || (has2 && is69Template)))
                        adjustedScore *= 1.4f;  // VERY strong boost for 6 chord
                    else if (hasm3 && hasP5 && (ism6Template || (has2 && ism69Template)))
                        adjustedScore *= 1.4f;  // VERY strong boost for m6 chord
                    else if (is7thTemplate && !is6Template && !ism6Template && !is69Template && !ism69Template)
                        adjustedScore *= 0.8f;  // Penalize 7th templates
                }
                
                // === m11 PENALTY when 9th is missing ===
                // m11 requires: root, m3, P5, b7, 9, 11
                // If we have m3, b7, 11, but NO 9th, penalize m11
                bool ism11Template = (tname && strcmp(tname, "m11") == 0);
                if (ism11Template && hasm3 && hasMinor7 && has4 && !has2)
                {
                    adjustedScore *= 0.75f;  // Missing 9th - this isn't a proper m11
                }
                
                // === DIM7 DETECTION (has bb7 = same as M6, but with b5) ===
                // dim7 = root, m3, b5, bb7(M6)
                if (has6 && hasB5 && hasm3 && !hasP5)  // bb7 is enharmonic with M6
                {
                    bool isDim7Template = (tname && strcmp(tname, "dim7") == 0);
                    bool isDimTriadTemplate = (tname && strcmp(tname, "dim") == 0);
                    
                    if (isDim7Template)
                        adjustedScore *= 1.3f;  // Strong boost for dim7
                    else if (isDimTriadTemplate)
                        adjustedScore *= 0.8f;  // Penalize dim triad when bb7 present
                }
                
                // === 9th DETECTION ===
                if (has2 && has7th)  // 9th = 2nd octave up, requires 7th
                {
                    bool is9thTemplate = (tname && (tname[0] == '9' || strstr(tname, "9") != nullptr));
                    if (is9thTemplate)
                        adjustedScore *= 1.1f;
                }
                
                // === 11th DETECTION ===
                if (has4 && has7th)
                {
                    bool is11thTemplate = (tname && strstr(tname, "11") != nullptr);
                    if (is11thTemplate)
                        adjustedScore *= 1.1f;
                }
                
                // === #11 DETECTION ===
                if (hasB5 && hasP5)  // #11 often coexists with P5
                {
                    bool isSharp11Template = (tname && strstr(tname, "#11") != nullptr);
                    if (isSharp11Template)
                        adjustedScore *= 1.15f;
                }
                
                // === 13th DETECTION ===
                if (has6 && has7th)  // 13th = 6th octave up
                {
                    bool is13thTemplate = (tname && strstr(tname, "13") != nullptr);
                    bool is9thOnlyTemplate = (tname && strstr(tname, "9") != nullptr && 
                                              strstr(tname, "13") == nullptr);
                    
                    if (is13thTemplate)
                        adjustedScore *= 1.14f;   // Boost 13th templates
                    else if (is9thOnlyTemplate)
                        adjustedScore *= 0.94f;   // Penalty for 9th when 13 present
                }
                
                // === b9 DETECTION ===
                if (hasB2 && hasMinor7 && hasM3)  // b9 on dominant
                {
                    bool isB9Template = (tname && strstr(tname, "b9") != nullptr);
                    if (isB9Template)
                        adjustedScore *= 1.15f;
                }
                
                // === #9 DETECTION ===
                if (hasm3 && hasM3 && hasMinor7)  // #9 = m3, with M3 = Hendrix chord
                {
                    bool isSharp9Template = (tname && strstr(tname, "#9") != nullptr);
                    if (isSharp9Template)
                        adjustedScore *= 1.15f;
                }
                
                // === b5 on dominant ===
                if (hasB5 && hasM3 && hasMinor7 && !hasP5)
                {
                    bool isB5DomTemplate = (tname && (strstr(tname, "b5") != nullptr || strstr(tname, "7alt") != nullptr));
                    if (isB5DomTemplate && !isDimTemplate)
                        adjustedScore *= 1.15f;
                }
                
                // === #5 on dominant ===
                if (hasSharp5 && hasM3 && hasMinor7)
                {
                    bool isSharp5DomTemplate = (tname && (strstr(tname, "#5") != nullptr || strcmp(tname, "aug7") == 0));
                    if (isSharp5DomTemplate)
                        adjustedScore *= 1.15f;
                    
                    // Specific 9#5 detection: if 9th is present, boost 9#5 over 7#5
                    if (has2)
                    {
                        bool is9Sharp5 = (tname && strcmp(tname, "9#5") == 0);
                        bool is7Sharp5 = (tname && strcmp(tname, "7#5") == 0);
                        if (is9Sharp5)
                            adjustedScore *= 1.25f;  // Boost 9#5 when 9 present
                        else if (is7Sharp5)
                            adjustedScore *= 0.85f;  // Penalize 7#5 when 9 present - use 9#5
                    }
                }
                
                // === b13 detection ===
                if (hasSharp5 && hasMinor7 && hasM3 && hasP5)  // b13 = #5, but with P5
                {
                    bool isB13Template = (tname && strstr(tname, "b13") != nullptr);
                    if (isB13Template)
                        adjustedScore *= 1.2f;
                }
            }
            else
            {
                // Root NOT present - be skeptical of complex interpretations
                if (!isTriadTemplate && !tpl.isShell)
                    adjustedScore *= 0.8f;
            }
            
            // ================================================================
            // BASS NOTE ALIGNMENT
            // ================================================================
            if (bassPitchClass >= 0)
            {
                int bassRelative = (bassPitchClass - rootPC + 12) % 12;
                
                // Handle symmetric chords (aug, dim7)
                if (isAugTemplate && !hasMinor7 && !hasMajor7)
                {
                    // Augmented chord tones: 0, 4, 8
                    if (bassRelative == 0)
                        adjustedScore *= 1.2f;
                    else if (bassRelative == 4 || bassRelative == 8)
                        adjustedScore *= 0.6f;
                    else
                        adjustedScore *= 0.7f;
                }
                else if (tname && strcmp(tname, "dim7") == 0)
                {
                    // Dim7 chord tones: 0, 3, 6, 9
                    if (bassRelative == 0)
                        adjustedScore *= 1.2f;
                    else if (bassRelative == 3 || bassRelative == 6 || bassRelative == 9)
                        adjustedScore *= 0.6f;
                    else
                        adjustedScore *= 0.7f;
                }
                else
                {
                    // Non-symmetric chords: bass alignment
                    // ROOT POSITION is strongly preferred for disambiguation
                    if (bassRelative == 0)
                    {
                        // Bass is root - STRONGLY PREFER
                        adjustedScore *= 1.15f;
                    }
                    else if (bassRelative == 3 || bassRelative == 4)
                    {
                        // Bass is 3rd (1st inversion)
                        adjustedScore *= 0.97f;
                    }
                    else if (bassRelative == 7)
                    {
                        // Bass is 5th (2nd inversion)
                        adjustedScore *= 0.95f;
                    }
                    else if (bassRelative == 10 || bassRelative == 11)
                    {
                        // Bass is 7th (3rd inversion)
                        adjustedScore *= 0.92f;
                    }
                    else if (bassRelative == 2 || bassRelative == 9)
                    {
                        // Bass is 9th or 6th
                        if (is7thTemplate)
                            adjustedScore *= 0.90f;
                        else
                            adjustedScore *= 0.82f;
                    }
                    else
                    {
                        // Non-chord-tone bass - slash chord territory
                        adjustedScore *= 0.78f;
                    }
                }
            }
            
            // Apply complexity penalty (slight)
            float complexityPenalty = (tpl.complexity - 1) * 0.008f;
            adjustedScore -= complexityPenalty;
            
            // Extension bonus for matched extensions
            float extensionBonus = 0.0f;
            constexpr int extensionBins[] = {1, 2, 3, 5, 6, 8, 9, 10, 11};
            for (int eb : extensionBins)
            {
                if (tpl.bins[eb] > 0.1f && bins[eb] > 0.1f)
                {
                    extensionBonus += 0.05f;
                }
            }
            adjustedScore += extensionBonus;
            
            // Apply template priority as additive bonus
            float priorityBonus = (tpl.priority - 1.0f) * 0.12f;
            adjustedScore += priorityBonus;
            
            // Shell voicing slight penalty
            if (tpl.isShell)
                adjustedScore *= 0.96f;
            
            // Only add if above threshold
            if (adjustedScore >= minimumBaseScore_)
            {
                ChordHypothesis hyp;
                hyp.rootPitchClass = rootPC;
                hyp.bassPitchClass = bassPitchClass;
                hyp.formulaIndex = tIdx;
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
        
        for (int j = 0; j < candidateCount; ++j)
        {
            if (tempCandidates[i].hyp.rootPitchClass == outCandidates[j].rootPitchClass)
            {
                float scoreDiff = std::abs(tempCandidates[i].sortScore - outCandidates[j].baseScore);
                if (scoreDiff < 0.05f)
                {
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
