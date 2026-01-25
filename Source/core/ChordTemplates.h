/*
  ==============================================================================

    ChordTemplates.h
    
    Float-weighted chord templates for H-WCTM algorithm
    
    Based on research: Use FLOAT weights, not binary!
    - 5th weight = 0.2 (often omitted in jazz)
    - Shell voicings (3rd + 7th) included as first-class templates
    - Designed for cosine similarity matching

  ==============================================================================
*/

#pragma once

#include <array>
#include <cmath>

namespace ChordDetection
{

// ============================================================================
// Template Structure
// ============================================================================

struct ChordTemplate
{
    const char* name;           // Display name: "maj7", "m7", "7", etc.
    const char* fullName;       // Full name: "Major 7th", etc.
    std::array<float, 12> bins; // Weighted template (index 0 = root)
    int complexity;             // 1 = simple, higher = more complex
    float priority;             // For tie-breaking (higher = prefer)
    bool isShell;               // Is this a shell voicing template?
};

// ============================================================================
// Template Definitions
// ============================================================================

/**
 * CHORD_TEMPLATES - Weighted templates for cosine similarity matching
 * 
 * Interval indices:
 *   0 = Root
 *   1 = b2/b9
 *   2 = 2/9
 *   3 = m3/#9
 *   4 = M3
 *   5 = P4/11
 *   6 = b5/#11
 *   7 = P5
 *   8 = #5/b13
 *   9 = 6/13
 *   10 = b7
 *   11 = M7
 * 
 * Weight guidelines (from research):
 * - Root: 1.0
 * - 3rd (defines quality): 1.0
 * - 7th (defines chord type): 1.0
 * - 5th: 0.2 (CRITICAL: often omitted in jazz!)
 * - Extensions: 0.3-0.5
 */
constexpr ChordTemplate CHORD_TEMPLATES[] = {
    
    // ========== MAJOR FAMILY ==========
    
    // Major Triad: C-E-G (displayed as just "C", not "Cmaj")
    {"", "Major",
     {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f},
     1, 1.0f, false},
    
    // Major 7: C-E-G-B
    {"maj7", "Major 7th",
     {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 1.0f},
     2, 1.1f, false},
    
    // Major 7 Shell: E-B (3rd + 7th, no root) - very common jazz voicing
    {"maj7", "Major 7th",
     {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
     2, 1.15f, true},
    
    // Major 9: C-E-G-B-D
    {"maj9", "Major 9th",
     {1.0f, 0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 1.0f},
     3, 1.2f, false},
    
    // Major 6: C-E-G-A
    {"6", "Major 6th",
     {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.7f, 0.0f, 0.0f},
     2, 0.95f, false},
    
    // Major 6/9: C-E-G-A-D
    {"6/9", "Major 6/9",
     {1.0f, 0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.7f, 0.0f, 0.0f},
     3, 1.0f, false},
    
    // Add9: C-E-G-D (add9, no 7th)
    {"add9", "Add 9",
     {1.0f, 0.0f, 0.6f, 0.0f, 1.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f},
     2, 0.9f, false},
    
    // ========== MINOR FAMILY ==========
    
    // Minor Triad: C-Eb-G
    {"m", "Minor",
     {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f},
     1, 1.0f, false},
    
    // Minor 7: C-Eb-G-Bb
    {"m7", "Minor 7th",
     {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 1.0f, 0.0f},
     2, 1.1f, false},
    
    // Minor 7 Shell: Eb-Bb (m3 + b7, no root) - common jazz voicing
    {"m7", "Minor 7th",
     {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
     2, 1.15f, true},
    
    // Minor 9: C-Eb-G-Bb-D
    {"m9", "Minor 9th",
     {1.0f, 0.0f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 1.0f, 0.0f},
     3, 1.2f, false},
    
    // Minor 11: C-Eb-G-Bb-D-F
    {"m11", "Minor 11th",
     {1.0f, 0.0f, 0.4f, 1.0f, 0.0f, 0.5f, 0.0f, 0.2f, 0.0f, 0.0f, 1.0f, 0.0f},
     4, 1.25f, false},
    
    // Minor 6: C-Eb-G-A
    {"m6", "Minor 6th",
     {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.7f, 0.0f, 0.0f},
     2, 0.95f, false},
    
    // Minor/Major 7: C-Eb-G-B (minor with major 7th)
    {"mMaj7", "Minor/Major 7th",
     {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 1.0f},
     3, 0.9f, false},
    
    // ========== DOMINANT FAMILY ==========
    
    // Dominant 7: C-E-G-Bb
    {"7", "Dominant 7th",
     {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 1.0f, 0.0f},
     2, 1.1f, false},
    
    // Dominant 7 Shell: E-Bb (M3 + b7) - the tritone! Most important jazz shell
    {"7", "Dominant 7th",
     {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
     2, 1.2f, true},  // Higher priority - tritone is definitive
    
    // Dominant 9: C-E-G-Bb-D
    {"9", "Dominant 9th",
     {1.0f, 0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 1.0f, 0.0f},
     3, 1.15f, false},
    
    // Dominant 13: C-E-G-Bb-D-(F)-A
    {"13", "Dominant 13th",
     {1.0f, 0.0f, 0.4f, 0.0f, 1.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.6f, 1.0f, 0.0f},
     4, 1.2f, false},
    
    // 7sus4: C-F-G-Bb
    {"7sus4", "Dominant 7th Sus4",
     {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.2f, 0.0f, 0.0f, 1.0f, 0.0f},
     2, 1.0f, false},
    
    // 7#9 (Hendrix chord): C-E-G-Bb-D# (has both M3 and #9/m3)
    {"7#9", "Dominant 7th #9",
     {1.0f, 0.0f, 0.0f, 0.6f, 1.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 1.0f, 0.0f},
     3, 1.1f, false},
    
    // 7b9: C-E-G-Bb-Db
    {"7b9", "Dominant 7th b9",
     {1.0f, 0.6f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 1.0f, 0.0f},
     3, 1.1f, false},
    
    // 7#5 (Aug7): C-E-G#-Bb
    {"7#5", "Dominant 7th #5",
     {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.8f, 0.0f, 1.0f, 0.0f},
     3, 1.0f, false},
    
    // 7b5: C-E-Gb-Bb
    {"7b5", "Dominant 7th b5",
     {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.8f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
     3, 1.0f, false},
    
    // 7alt (multiple alterations): C-E-Gb-Bb-Db or C-E-G#-Bb-D#
    {"7alt", "Altered Dominant",
     {1.0f, 0.5f, 0.0f, 0.5f, 1.0f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 1.0f, 0.0f},
     4, 1.15f, false},
    
    // ========== DIMINISHED FAMILY ==========
    
    // Diminished Triad: C-Eb-Gb
    {"dim", "Diminished",
     {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
     2, 0.9f, false},
    
    // Diminished 7: C-Eb-Gb-Bbb (A)
    {"dim7", "Diminished 7th",
     {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f},
     3, 1.0f, false},
    
    // Half-Diminished (m7b5): C-Eb-Gb-Bb
    {"m7b5", "Half Diminished",
     {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
     3, 1.05f, false},
    
    // Half-Dim Shell: Eb-Bb with b5 context
    {"m7b5", "Half Diminished",
     {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.8f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
     3, 1.1f, true},
    
    // ========== AUGMENTED FAMILY ==========
    
    // Augmented Triad: C-E-G#
    {"aug", "Augmented",
     {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f},
     2, 0.85f, false},
    
    // Augmented Major 7: C-E-G#-B
    {"augMaj7", "Augmented Major 7th",
     {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f},
     3, 0.9f, false},
    
    // ========== SUSPENDED FAMILY ==========
    
    // Sus2: C-D-G
    {"sus2", "Suspended 2nd",
     {1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.8f, 0.0f, 0.0f, 0.0f, 0.0f},
     1, 0.85f, false},
    
    // Sus4: C-F-G
    {"sus4", "Suspended 4th",
     {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.8f, 0.0f, 0.0f, 0.0f, 0.0f},
     1, 0.85f, false},
    
    // ========== SPECIAL/QUARTAL ==========
    
    // Power Chord: C-G (ambiguous - no 3rd)
    {"5", "Power Chord",
     {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
     1, 0.7f, false},
    
    // Quartal voicing: C-F-Bb (stacked 4ths) - common in modal jazz
    {"sus", "Quartal",
     {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.8f, 0.0f},
     2, 0.8f, false},
};

constexpr int CHORD_TEMPLATE_COUNT = sizeof(CHORD_TEMPLATES) / sizeof(ChordTemplate);

// ============================================================================
// Template Lookup Helpers
// ============================================================================

/**
 * Find the best matching template using cosine similarity
 * 
 * @param shiftedChroma Input chroma vector, already shifted so potential root is at index 0
 * @param outScore Output: the similarity score
 * @return Index of best matching template, or -1 if none above threshold
 */
inline int findBestTemplate(const float* shiftedChroma, float& outScore, float threshold = 0.6f)
{
    int bestIdx = -1;
    float bestScore = threshold;
    
    for (int i = 0; i < CHORD_TEMPLATE_COUNT; ++i)
    {
        const auto& tmpl = CHORD_TEMPLATES[i];
        
        // Calculate cosine similarity
        float dot = 0.0f;
        float magA = 0.0f;
        float magB = 0.0f;
        
        for (int j = 0; j < 12; ++j)
        {
            dot += shiftedChroma[j] * tmpl.bins[j];
            magA += shiftedChroma[j] * shiftedChroma[j];
            magB += tmpl.bins[j] * tmpl.bins[j];
        }
        
        if (magA < 0.0001f || magB < 0.0001f)
            continue;
        
        float similarity = dot / (std::sqrt(magA) * std::sqrt(magB));
        
        // Apply template priority
        float score = similarity * tmpl.priority;
        
        if (score > bestScore)
        {
            bestScore = score;
            bestIdx = i;
        }
    }
    
    outScore = bestScore;
    return bestIdx;
}

} // namespace ChordDetection
