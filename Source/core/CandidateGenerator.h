#pragma once

#include "ChordFormula.h"
#include "ChordTemplates.h"
#include "ChromaVector.h"
#include "MidiNoteState.h"
#include <array>
#include <bitset>
#include <cstdint>

namespace ChordDetection
{

/**
 * ChordHypothesis - A candidate chord interpretation
 * 
 * As per spec: "This layer does not decide – it enumerates possibilities."
 * Each hypothesis represents one possible chord interpretation from the played notes.
 * 
 * Updated for H-WCTM: Now uses cosine similarity with weighted templates.
 */
struct ChordHypothesis
{
    int rootPitchClass;             // 0-11 (C=0, C#=1, etc.)
    int bassPitchClass;             // Actual bass note (for slash chords)
    int formulaIndex;               // Index into CHORD_TEMPLATES array
    
    // Template info (H-WCTM)
    const char* templateName;       // "maj7", "m7", etc.
    const char* templateFullName;   // "Major 7th", etc.
    float cosineSimilarity;         // Raw cosine similarity score
    bool isShellVoicing;            // Is this from a shell template?
    int complexity;                 // Template complexity level
    
    // Matching information
    int requiredMatched;            // Count of required intervals matched
    int requiredTotal;              // Total required intervals in formula
    int optionalMatched;            // Count of optional intervals matched
    int extraNotes;                 // Notes not in formula
    bool hasForbiddenInterval;      // If true, this hypothesis is invalid
    
    // Raw score (before temporal/contextual factors)
    float baseScore;
    
    // Timing
    double timestamp;
    
    ChordHypothesis()
        : rootPitchClass(-1)
        , bassPitchClass(-1)
        , formulaIndex(-1)
        , templateName("")
        , templateFullName("")
        , cosineSimilarity(0.0f)
        , isShellVoicing(false)
        , complexity(1)
        , requiredMatched(0)
        , requiredTotal(0)
        , optionalMatched(0)
        , extraNotes(0)
        , hasForbiddenInterval(false)
        , baseScore(0.0f)
        , timestamp(0.0)
    {}
    
    bool isValid() const
    {
        return rootPitchClass >= 0 && formulaIndex >= 0 && !hasForbiddenInterval && baseScore > 0.0f;
    }
    
    /**
     * Get chord name (e.g., "Cmaj7", "Dm7/A")
     */
    void getChordName(char* buffer, int bufferSize) const;
    
    /**
     * Compare two hypotheses for equality (same chord)
     */
    bool isSameChord(const ChordHypothesis& other) const
    {
        return rootPitchClass == other.rootPitchClass
            && formulaIndex == other.formulaIndex
            && bassPitchClass == other.bassPitchClass;
    }
};

// Maximum candidates per frame
constexpr int MAX_CANDIDATES_PER_FRAME = 48;  // 12 roots × ~4 quality types

/**
 * CandidateGenerator - Generates chord hypotheses from pitch classes
 * 
 * As per spec:
 * - Input: Set of active pitch classes
 * - Output: List of ChordHypothesis
 * 
 * Real-time safe: No allocations, uses fixed-size arrays.
 */
class CandidateGenerator
{
public:
    CandidateGenerator();
    ~CandidateGenerator() = default;
    
    /**
     * Generate all chord hypotheses from current pitch class set
     * 
     * @param pitchClasses Active pitch classes (0-11)
     * @param bassPitchClass The bass note's pitch class (for slash chord detection)
     * @param currentTimeMs Current timestamp
     * @param outCandidates Output array (must have space for MAX_CANDIDATES_PER_FRAME)
     * @return Number of valid candidates generated
     */
    int generateCandidates(
        const std::bitset<12>& pitchClasses,
        int bassPitchClass,
        double currentTimeMs,
        ChordHypothesis* outCandidates
    ) const;
    
    /**
     * Generate candidates directly from MidiNoteState
     * Convenience method that extracts pitch classes and bass
     */
    int generateCandidatesFromNoteState(
        const MidiNoteState& noteState,
        double currentTimeMs,
        ChordHypothesis* outCandidates
    ) const;
    
    /**
     * Get minimum pitch class count for chord detection
     */
    int getMinimumNoteCount() const { return minimumNoteCount_; }
    void setMinimumNoteCount(int count) { minimumNoteCount_ = count; }
    
    /**
     * Get minimum base score threshold for candidate inclusion
     */
    float getMinimumBaseScore() const { return minimumBaseScore_; }
    void setMinimumBaseScore(float score) { minimumBaseScore_ = score; }
    
private:
    int minimumNoteCount_;
    float minimumBaseScore_;
    
    /**
     * Build weighted ChromaVector from MidiNoteState
     * Applies bass boost, register weighting, velocity scaling, and sustain decay
     */
    ChromaVector buildChromaVector(
        const MidiNoteState& noteState,
        double currentTimeMs
    ) const;
    
    /**
     * Core H-WCTM matching using ChromaVector and templates
     * Tests all root transpositions against all templates using cosine similarity
     */
    int generateCandidatesFromChroma(
        const ChromaVector& chroma,
        int bassPitchClass,
        double currentTimeMs,
        ChordHypothesis* outCandidates
    ) const;
    
    /**
     * Score a single chord formula against played pitch classes (legacy)
     * 
     * @param relativePCs Pitch classes relative to tested root
     * @param formula Chord formula to test
     * @param hyp Output hypothesis (partially filled)
     * @return Base score (0.0 = no match, higher = better)
     */
    float scoreFormula(
        const std::bitset<12>& relativePCs,
        const ChordFormula& formula,
        ChordHypothesis& hyp
    ) const;
};

} // namespace ChordDetection
