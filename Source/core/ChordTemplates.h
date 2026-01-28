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
 * - 5th: 0.2-0.6 (often omitted in jazz)
 * - Extensions: 0.3-0.8
 */
constexpr ChordTemplate CHORD_TEMPLATES[] = {
    
    // ========== MAJOR FAMILY ==========
    
    // Major Triad: C-E-G
    {"", "Major",
     {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.8f, 0.0f, 0.0f, 0.0f, 0.0f},
     1, 1.1f, false},
    
    // Major 7: C-E-G-B
    {"maj7", "Major 7th",
     {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f},
     2, 1.15f, false},
    
    // Major 7 Shell
    {"maj7", "Major 7th",
     {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
     2, 0.9f, true},
    
    // Major 9: C-E-G-B-D
    {"maj9", "Major 9th",
     {1.0f, 0.0f, 0.9f, 0.0f, 1.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 1.0f},
     2, 1.2f, false},
    
    // Major 11: C-E-G-B-D-F
    {"maj11", "Major 11th",
     {1.0f, 0.0f, 0.6f, 0.0f, 1.0f, 0.8f, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 1.0f},
     3, 1.45f, false},
    
    // Major 7#11: C-E-G-B-F#
    {"maj7#11", "Major 7th #11",
     {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.9f, 0.3f, 0.0f, 0.0f, 0.0f, 1.0f},
     3, 1.4f, false},
    
    // Major 13: C-E-G-B-D-A
    {"maj13", "Major 13th",
     {1.0f, 0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.9f, 0.0f, 1.0f},
     3, 1.45f, false},
    
    // Major 6: C-E-G-A - VERY HIGH PRIORITY over m7 inversions
    {"6", "Major 6th",
     {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f},
     2, 1.55f, false},
    
    // Major 6/9: C-E-G-A-D
    {"6/9", "Major 6/9",
     {1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f},
     3, 1.6f, false},
    
    // Add9: C-E-G-D - VERY HIGH PRIORITY over 9sus4 inversions
    {"add9", "Add 9",
     {1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f},
     2, 1.55f, false},
    
    // Add4/Add11: C-E-G-F
    {"add11", "Add 11",
     {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.8f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.0f},
     2, 1.05f, false},
    
    // ========== MINOR FAMILY ==========
    
    // Minor Triad: C-Eb-G
    {"m", "Minor",
     {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.8f, 0.0f, 0.0f, 0.0f, 0.0f},
     1, 1.1f, false},
    
    // Minor 7: C-Eb-G-Bb
    {"m7", "Minor 7th",
     {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f},
     2, 1.15f, false},
    
    // Minor 7 Shell
    {"m7", "Minor 7th",
     {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
     2, 0.9f, true},
    
    // Minor 9: C-Eb-G-Bb-D
    {"m9", "Minor 9th",
     {1.0f, 0.0f, 0.9f, 1.0f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 1.0f, 0.0f},
     2, 1.2f, false},
    
    // Minor 11: C-Eb-G-Bb-D-F
    {"m11", "Minor 11th",
     {1.0f, 0.0f, 0.6f, 1.0f, 0.0f, 0.9f, 0.0f, 0.2f, 0.0f, 0.0f, 1.0f, 0.0f},
     2, 1.45f, false},
    
    // Minor 13: C-Eb-G-Bb-D-F-A (often omits 11th)
    {"m13", "Minor 13th",
     {1.0f, 0.0f, 0.6f, 1.0f, 0.0f, 0.5f, 0.0f, 0.3f, 0.0f, 1.0f, 1.0f, 0.0f},
     3, 1.55f, false},
    
    // Minor 6: C-Eb-G-A - VERY HIGH PRIORITY over m7b5 inversions
    {"m6", "Minor 6th",
     {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f},
     2, 1.55f, false},
    
    // Minor 6/9: C-Eb-G-A-D
    {"m6/9", "Minor 6/9",
     {1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f},
     3, 1.6f, false},
    
    // Minor/Major 7: C-Eb-G-B
    {"m(maj7)", "Minor/Major 7th",
     {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 1.0f},
     2, 1.35f, false},
    
    // Minor/Major 9: C-Eb-G-B-D
    {"m(maj9)", "Minor/Major 9th",
     {1.0f, 0.0f, 0.8f, 1.0f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.0f, 1.0f},
     3, 1.4f, false},
    
    // Minor add9: C-Eb-G-D - VERY HIGH PRIORITY
    {"madd9", "Minor Add 9",
     {1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f},
     2, 1.55f, false},
    
    // ========== DOMINANT FAMILY ==========
    
    // Dominant 7: C-E-G-Bb
    {"7", "Dominant 7th",
     {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f},
     2, 1.15f, false},
    
    // Dominant 7 Shell
    {"7", "Dominant 7th",
     {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
     2, 0.9f, true},
    
    // Dominant 9: C-E-G-Bb-D
    {"9", "Dominant 9th",
     {1.0f, 0.0f, 0.9f, 0.0f, 1.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 1.0f, 0.0f},
     2, 1.2f, false},
    
    // Dominant 11: C-E-G-Bb-D-F
    {"11", "Dominant 11th",
     {1.0f, 0.0f, 0.6f, 0.0f, 1.0f, 0.9f, 0.0f, 0.2f, 0.0f, 0.0f, 1.0f, 0.0f},
     2, 1.45f, false},
    
    // Dominant 13: C-E-Bb-D-A
    {"13", "Dominant 13th",
     {1.0f, 0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.9f, 1.0f, 0.0f},
     2, 1.45f, false},
    
    // 7sus4: C-F-G-Bb
    {"7sus4", "Dominant 7th Sus4",
     {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.4f, 0.0f, 0.0f, 1.0f, 0.0f},
     2, 1.25f, false},
    
    // 9sus4: C-F-G-Bb-D - Lower priority vs add9
    {"9sus4", "Dominant 9th Sus4",
     {1.0f, 0.0f, 0.8f, 0.0f, 0.0f, 1.0f, 0.0f, 0.3f, 0.0f, 0.0f, 1.0f, 0.0f},
     2, 1.2f, false},
    
    // 13sus4: C-F-G-Bb-D-A
    {"13sus4", "Dominant 13th Sus4",
     {1.0f, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.2f, 0.0f, 0.9f, 1.0f, 0.0f},
     3, 1.35f, false},
    
    // ========== ALTERED DOMINANT FAMILY ==========
    
    // 7b9: C-E-G-Bb-Db
    {"7b9", "Dominant 7th b9",
     {1.0f, 0.9f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 1.0f, 0.0f},
     3, 1.35f, false},
    
    // 7#9: C-E-G-Bb-D#
    {"7#9", "Dominant 7th #9",
     {1.0f, 0.0f, 0.0f, 0.9f, 1.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 1.0f, 0.0f},
     3, 1.35f, false},
    
    // 7b5: C-E-Gb-Bb
    {"7b5", "Dominant 7th b5",
     {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, -0.2f, 0.0f, 0.0f, 1.0f, 0.0f},
     3, 1.3f, false},
    
    // 7#5: C-E-G#-Bb
    {"7#5", "Dominant 7th #5",
     {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -0.2f, 1.0f, 0.0f, 1.0f, 0.0f},
     3, 1.3f, false},
    
    // 7#11: C-E-G-Bb-F#
    {"7#11", "Dominant 7th #11",
     {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.9f, 0.3f, 0.0f, 0.0f, 1.0f, 0.0f},
     3, 1.4f, false},
    
    // 7b13: C-E-G-Bb-Ab
    {"7b13", "Dominant 7th b13",
     {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.3f, 0.9f, 0.0f, 1.0f, 0.0f},
     3, 1.4f, false},
    
    // 9b5: C-E-Gb-Bb-D
    {"9b5", "Dominant 9th b5",
     {1.0f, 0.0f, 0.8f, 0.0f, 1.0f, 0.0f, 1.0f, -0.2f, 0.0f, 0.0f, 1.0f, 0.0f},
     3, 1.4f, false},
    
    // 9#5: C-E-G#-Bb-D - higher priority over 7#5 when 9 present
    {"9#5", "Dominant 9th #5",
     {1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, -0.1f, 1.0f, 0.0f, 1.0f, 0.0f},
     3, 1.55f, false},
    
    // 9#11: C-E-G-Bb-D-F#
    {"9#11", "Dominant 9th #11",
     {1.0f, 0.0f, 0.8f, 0.0f, 1.0f, 0.0f, 0.9f, 0.2f, 0.0f, 0.0f, 1.0f, 0.0f},
     3, 1.45f, false},
    
    // 7b9b5: C-E-Gb-Bb-Db
    {"7b9b5", "Dominant 7th b9 b5",
     {1.0f, 0.9f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, -0.2f, 0.0f, 0.0f, 1.0f, 0.0f},
     4, 1.5f, false},
    
    // 7#9b5: C-E-Gb-Bb-D#
    {"7#9b5", "Dominant 7th #9 b5",
     {1.0f, 0.0f, 0.0f, 0.9f, 1.0f, 0.0f, 1.0f, -0.2f, 0.0f, 0.0f, 1.0f, 0.0f},
     4, 1.5f, false},
    
    // 7b9#5: C-E-G#-Bb-Db
    {"7b9#5", "Dominant 7th b9 #5",
     {1.0f, 0.9f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -0.2f, 1.0f, 0.0f, 1.0f, 0.0f},
     4, 1.5f, false},
    
    // 7#9#5: C-E-G#-Bb-D#
    {"7#9#5", "Dominant 7th #9 #5",
     {1.0f, 0.0f, 0.0f, 0.9f, 1.0f, 0.0f, 0.0f, -0.2f, 1.0f, 0.0f, 1.0f, 0.0f},
     4, 1.5f, false},
    
    // 7alt: C-E-Gb/G#-Bb-Db/D#
    {"7alt", "Altered Dominant",
     {1.0f, 0.5f, 0.0f, 0.5f, 1.0f, 0.0f, 0.5f, -0.2f, 0.5f, 0.0f, 1.0f, 0.0f},
     4, 1.3f, false},
    
    // 13b9: C-E-G-Bb-Db-A
    {"13b9", "Dominant 13th b9",
     {1.0f, 0.9f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.9f, 1.0f, 0.0f},
     3, 1.5f, false},
    
    // 13#9: C-E-G-Bb-D#-A
    {"13#9", "Dominant 13th #9",
     {1.0f, 0.0f, 0.0f, 0.9f, 1.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.9f, 1.0f, 0.0f},
     3, 1.5f, false},
    
    // 13#11: C-E-G-Bb-D-F#-A
    {"13#11", "Dominant 13th #11",
     {1.0f, 0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.8f, 0.2f, 0.0f, 0.9f, 1.0f, 0.0f},
     3, 1.5f, false},
    
    // 13b9b13: C-E-G-Bb-Db-Ab (yes, both b13 and 13!)
    {"13b9b13", "Dominant 13th b9 b13",
     {1.0f, 0.9f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.2f, 0.8f, 0.0f, 1.0f, 0.0f},
     4, 1.55f, false},
    
    // ========== DIMINISHED FAMILY ==========
    
    // Diminished Triad: C-Eb-Gb
    {"dim", "Diminished",
     {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.2f, -0.3f, 0.0f, 0.0f, 0.0f, 0.0f},
     1, 1.15f, false},
    
    // Diminished 7: C-Eb-Gb-Bbb - MUCH HIGHER PRIORITY
    {"dim7", "Diminished 7th",
     {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.2f, -0.3f, 0.0f, 1.0f, 0.0f, 0.0f},
     2, 1.55f, false},
    
    // Half-Diminished (m7b5): C-Eb-Gb-Bb
    {"m7b5", "Half Diminished",
     {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.2f, -0.3f, 0.0f, 0.0f, 1.0f, 0.0f},
     2, 1.4f, false},
    
    // Half-Dim Shell
    {"m7b5", "Half Diminished",
     {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
     2, 1.25f, true},
    
    // Half-Dim 9: C-Eb-Gb-Bb-D
    {"m7b5(9)", "Half Diminished 9",
     {1.0f, 0.0f, 0.9f, 1.0f, 0.0f, 0.0f, 1.2f, -0.3f, 0.0f, 0.0f, 1.0f, 0.0f},
     3, 1.5f, false},
    
    // Half-Dim 11 (m11b5): C-Eb-Gb-Bb-D-F
    {"m11b5", "Minor 11th b5",
     {1.0f, 0.0f, 0.7f, 1.0f, 0.0f, 0.9f, 1.2f, -0.3f, 0.0f, 0.0f, 1.0f, 0.0f},
     3, 1.55f, false},
    
    // ========== AUGMENTED FAMILY ==========
    
    // Augmented Triad: C-E-G#
    {"aug", "Augmented",
     {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -0.3f, 1.2f, 0.0f, 0.0f, 0.0f},
     1, 1.25f, false},
    
    // Augmented Major 7: C-E-G#-B
    {"augmaj7", "Augmented Major 7th",
     {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -0.3f, 1.2f, 0.0f, 0.0f, 1.0f},
     2, 1.4f, false},
    
    // Augmented 7 (7#5): C-E-G#-Bb (duplicate but for alt naming)
    {"aug7", "Augmented 7th",
     {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -0.3f, 1.2f, 0.0f, 1.0f, 0.0f},
     2, 1.28f, false},
    
    // ========== SUSPENDED FAMILY ==========
    
    // Sus2: C-D-G - HIGH PRIORITY over inversions
    {"sus2", "Suspended 2nd",
     {1.0f, 0.0f, 1.2f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
     1, 1.4f, false},
    
    // Sus4: C-F-G
    {"sus4", "Suspended 4th",
     {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.9f, 0.0f, 0.0f, 0.0f, 0.0f},
     1, 1.0f, false},
    
    // Sus2/4: C-D-F-G (has both 2nd and 4th)
    {"sus2/4", "Suspended 2nd/4th",
     {1.0f, 0.0f, 0.9f, 0.0f, 0.0f, 0.9f, 0.0f, 0.8f, 0.0f, 0.0f, 0.0f, 0.0f},
     2, 1.1f, false},
    
    // ========== POWER AND SPECIAL ==========
    
    // Power Chord: C-G
    {"5", "Power Chord",
     {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
     1, 0.7f, false},
};

constexpr int CHORD_TEMPLATE_COUNT = sizeof(CHORD_TEMPLATES) / sizeof(ChordTemplate);

// ============================================================================
// Template Lookup Helpers
// ============================================================================

/**
 * Check if a template name matches any of the given names
 */
inline bool templateNameMatches(const char* name, const char* match)
{
    if (name == nullptr || match == nullptr) return false;
    // Simple string compare
    const char* a = name;
    const char* b = match;
    while (*a && *b) {
        if (*a != *b) return false;
        ++a; ++b;
    }
    return *a == *b;
}

/**
 * Find the best matching template using cosine similarity
 * 
 * @param shiftedChroma Input chroma vector, already shifted so potential root is at index 0
 * @param outScore Output: the similarity score
 * @return Index of best matching template, or -1 if none above threshold
 */
inline int findBestTemplate(const float* shiftedChroma, float& outScore, float threshold = 0.5f)
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
