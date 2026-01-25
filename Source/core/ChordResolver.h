#pragma once

#include "ConfidenceScorer.h"
#include <array>

namespace ChordDetection
{

/**
 * ResolvedChord - Final chord output after resolution
 */
struct ResolvedChord
{
    ChordHypothesis chord;
    float confidence;
    int bassPitchClass;
    
    // Display info
    char chordName[32];
    const char* qualityName;
    
    // Inversion info
    bool isSlashChord;
    int inversionType;
    
    // Debug/info
    int candidatesConsidered;
    double timestamp;
    
    ResolvedChord()
        : confidence(0.0f)
        , bassPitchClass(-1)
        , qualityName("No Chord")
        , isSlashChord(false)
        , inversionType(0)
        , candidatesConsidered(0)
        , timestamp(0.0)
    {
        chordName[0] = '\0';
    }
    
    bool isValid() const { return chord.isValid() && confidence > 0.0f; }
    
    void buildDisplayName()
    {
        if (!chord.isValid())
        {
            snprintf(chordName, sizeof(chordName), "N.C.");
            qualityName = "No Chord";
            return;
        }
        
        chord.getChordName(chordName, sizeof(chordName));
        qualityName = CHORD_FORMULAS[chord.formulaIndex].fullName;
    }
};

/**
 * ResolutionConfig - Rules for chord resolution
 */
struct ResolutionConfig
{
    // Minimum confidence to report a chord
    float minimumConfidence;
    
    // Score margin required to prefer temporally stable chord
    float stabilityMargin;
    
    // Score margin required to prefer simpler quality
    float simplicityMargin;
    
    // Allow slash chords (bass ≠ root)
    bool allowSlashChords;
    
    // Minimum frames for temporal preference
    int minimumStableFrames;
    
    ResolutionConfig()
        : minimumConfidence(0.45f)
        , stabilityMargin(0.1f)
        , simplicityMargin(0.05f)
        , allowSlashChords(true)
        , minimumStableFrames(2)
    {}
};

/**
 * ChordResolver - Final chord selection
 * 
 * As per spec:
 * 1. Prefer temporally stable chord over newest
 * 2. Prefer simpler quality if scores are close
 * 3. Allow slash chords if bass ≠ root
 * 
 * Key principle: "A wrong chord briefly is worse than a delayed correct chord."
 * 
 * Real-time safe: No allocations.
 */
class ChordResolver
{
public:
    ChordResolver();
    ~ChordResolver() = default;
    
    /**
     * Resolve the final chord from scored candidates
     * 
     * @param scoredCandidates Array of scored candidates (sorted by confidence)
     * @param candidateCount Number of candidates
     * @param context Scoring context (for bass detection)
     * @param currentTimeMs Current timestamp
     * @return Resolved chord result
     */
    ResolvedChord resolve(
        const ScoredCandidate* scoredCandidates,
        int candidateCount,
        const ScoringContext& context,
        double currentTimeMs
    );
    
    /**
     * Get the previously resolved chord
     */
    const ResolvedChord& getPreviousChord() const { return previousChord_; }
    
    /**
     * Configuration
     */
    ResolutionConfig& getConfig() { return config_; }
    const ResolutionConfig& getConfig() const { return config_; }
    void setConfig(const ResolutionConfig& config) { config_ = config; }
    
    /**
     * Clear previous chord state
     */
    void reset();
    
private:
    ResolutionConfig config_;
    ResolvedChord previousChord_;
    double lastChordChangeTime_;
    
    /**
     * Compare two candidates for resolution rules
     * Returns true if candidate A should be preferred over B
     */
    bool shouldPrefer(
        const ScoredCandidate& a,
        const ScoredCandidate& b
    ) const;
    
    /**
     * Check if a candidate matches the previous chord
     */
    bool matchesPreviousChord(const ScoredCandidate& candidate) const;
    
    /**
     * Calculate inversion type from bass interval
     */
    int calculateInversionType(const ChordHypothesis& hyp) const;
};

} // namespace ChordDetection
