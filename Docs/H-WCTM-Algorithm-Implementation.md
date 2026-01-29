# H-WCTM Algorithm Implementation

**Harmonic-Weighted Chord Template Matching**

This document describes the implementation of the H-WCTM chord detection algorithm. It is intended for developers who need to understand, modify, or extend the core chord detector.

---

## Table of Contents

1. [Overview](#1-overview)
2. [Algorithm Pipeline](#2-algorithm-pipeline)
3. [MIDI Note Tracking](#3-midi-note-tracking)
4. [Chroma Vector Construction](#4-chroma-vector-construction)
5. [Chord Template Matching](#5-chord-template-matching)
6. [Candidate Generation](#6-candidate-generation)
7. [Harmonic Memory](#7-harmonic-memory)
8. [Confidence Scoring](#8-confidence-scoring)
9. [Chord Resolution](#9-chord-resolution)
10. [Design Decisions](#10-design-decisions)
11. [Known Limitations](#11-known-limitations)
12. [References](#12-references)

---

## 1. Overview

H-WCTM (Harmonic-Weighted Chord Template Matching) is a chord detection algorithm designed for real-time MIDI-based piano chord recognition in VST3 plugins.

**Core principle:**

> Chords are hypotheses evolving over time, not instant detections.

The algorithm uses **cosine similarity** between weighted chroma vectors and predefined chord templates, combined with temporal memory for stability.

**Key characteristics:**

- Float-weighted templates (not binary)
- Bass boost for root detection
- Temporal reinforcement across frames
- Multi-factor confidence scoring
- Real-time safe (no allocations in audio thread)

---

## 2. Algorithm Pipeline

```text
MIDI Input (note on/off, sustain pedal)
         │
         ▼
┌─────────────────────────────┐
│   MidiNoteState             │  Track active notes with velocity and timestamp
└─────────────────────────────┘
         │
         ▼
┌─────────────────────────────┐
│   ChromaVector              │  Build weighted 12-bin pitch class profile
└─────────────────────────────┘
         │
         ▼
┌─────────────────────────────┐
│   CandidateGenerator        │  Test all 12 roots × all templates
│   (H-WCTM matching)         │  Generate ChordHypothesis candidates
└─────────────────────────────┘
         │
         ▼
┌─────────────────────────────┐
│   HarmonicMemory            │  Apply exponential time decay
│                             │  Reinforce persistent candidates
└─────────────────────────────┘
         │
         ▼
┌─────────────────────────────┐
│   ConfidenceScorer          │  Calculate multi-factor confidence
│                             │  (bass alignment, stability, complexity)
└─────────────────────────────┘
         │
         ▼
┌─────────────────────────────┐
│   ChordResolver             │  Select final chord
│                             │  Apply stability and simplicity preferences
└─────────────────────────────┘
         │
         ▼
     ResolvedChord (output)
```

**Code entry point:** `ChordDetectorEngine::detectChord(currentTimeMs)`

---

## 3. MIDI Note Tracking

**Component:** `MidiNoteState` ([MidiNoteState.h](../Source/core/MidiNoteState.h))

Tracks all active MIDI notes with extended information for temporal reasoning.

### Data Stored Per Note

```cpp
struct ActiveNote {
    int midiNote;        // 0-127
    int pitchClass;      // midiNote % 12
    float velocity;      // 0.0-1.0
    double noteOnTime;   // Timestamp in milliseconds
};
```

### Operations

| Method | Purpose |
|--------|---------|
| `noteOn(note, velocity, time)` | Add note to active set |
| `noteOff(note)` | Remove note (or mark for sustain release) |
| `setSustainPedal(pressed)` | Track sustain pedal state |
| `getPitchClassSet()` | Return 12-bit set of active pitch classes |
| `getLowestNote()` | Return bass note for slash chord detection |

### Sustain Pedal Handling

When the sustain pedal is pressed:
1. Released keys remain in the active set
2. They are marked as "pending release"
3. On pedal release, all pending notes are removed

This matches real piano behavior where the sustain pedal extends note duration.

---

## 4. Chroma Vector Construction

**Component:** `ChromaVector` ([ChromaVector.h](../Source/core/ChromaVector.h))

A 12-dimensional float vector representing pitch class energy. Unlike binary pitch class sets, chroma vectors encode **relative importance** of each note.

### Weight Calculation Per Note

```text
weight = velocity
       × registerWeight(midiNote)
       × bassBoost(if lowest note)
       × sustainDecay(noteAge)
```

### Register Weighting

| MIDI Range | Region | Weight |
|------------|--------|--------|
| < 60 | Harmony (bass) | 1.0 |
| 60-72 | Chord voicing | 1.0 |
| 72-84 | Melody | 0.5 |
| > 84 | High ornaments | 0.2 |

**Purpose:** Reduces influence of melody notes that happen to coincide with chord tones.

### Bass Boost

The lowest note receives a **3.0x multiplier**.

```cpp
if (isLowestNote)
    weight *= bassBoost;  // default = 3.0
```

**Purpose:** The bass note typically defines the chord root or inversion. Strong bass boost ensures the algorithm detects C/E as a C chord with E in the bass, not Em.

### Sustain Decay

Notes held longer receive reduced weight:

```text
decayMultiplier = 0.85^(noteAge / 100ms)
```

**Purpose:** Recently played notes are more relevant than sustained notes from earlier.

---

## 5. Chord Template Matching

**Component:** `ChordTemplates.h` ([ChordTemplates.h](../Source/core/ChordTemplates.h))

Templates define the expected pitch class profile for each chord type, using float weights.

### Template Structure

```cpp
struct ChordTemplate {
    const char* name;           // "maj7", "m7", "7", etc.
    const char* fullName;       // "Major 7th", etc.
    std::array<float, 12> bins; // Weighted template (index 0 = root)
    int complexity;             // 1 = simple, higher = more complex
    float priority;             // Tie-breaking multiplier
    bool isShell;               // Shell voicing template
};
```

### Interval Index Mapping

```text
Index:  0    1    2    3    4    5    6    7    8    9    10   11
Name:   R    b9   9    m3   M3   11   #11  P5   #5   13   b7   M7
                       #9
```

### Weight Guidelines

| Interval | Weight | Reason |
|----------|--------|--------|
| Root | 1.0 | Defines chord |
| 3rd (m3 or M3) | 1.0 | Defines quality |
| 7th (b7 or M7) | 1.0 | Defines function |
| 5th | 0.2 | Often omitted in jazz |
| Extensions | 0.3-0.8 | Color tones |

**Critical insight:** The 5th is weighted at **0.2**, not 1.0. Jazz voicings routinely omit the 5th, and weighting it heavily would cause missed detections.

### Example Template: Dominant 7

```cpp
{"7", "Dominant 7th",
 {1.0f,  // Root
  0.0f,  // b9
  0.0f,  // 9
  0.0f,  // m3
  1.0f,  // M3 (defines major quality)
  0.0f,  // 11
  0.0f,  // #11
  0.2f,  // P5 (low weight - optional)
  0.0f,  // #5
  0.0f,  // 13
  1.0f,  // b7 (defines dominant function)
  0.0f}, // M7
 2, 1.2f, false}
```

### Shell Voicing Templates

Shell voicings (3rd + 7th only, no root) are included as first-class templates:

```cpp
// Dominant 7 Shell: E-Bb (the tritone)
{"7", "Dominant 7th",
 {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
 2, 1.3f, true}  // Higher priority - tritone is definitive
```

---

## 6. Candidate Generation

**Component:** `CandidateGenerator` ([CandidateGenerator.h](../Source/core/CandidateGenerator.h), [CandidateGenerator.cpp](../Source/core/CandidateGenerator.cpp))

Generates chord hypotheses by testing all 12 possible roots against all templates.

### Core Algorithm (Pseudocode)

```text
function generateCandidates(noteState, currentTime):
    chroma = buildChromaVector(noteState, currentTime)
    bassPitchClass = noteState.getLowestNote() % 12
    candidates = []

    for root in 0..11:
        shiftedChroma = chroma.shifted(root)  // Rotate so root is at index 0
        
        for template in CHORD_TEMPLATES:
            similarity = cosineSimilarity(shiftedChroma, template.bins)
            
            if similarity >= threshold:
                hyp = ChordHypothesis()
                hyp.rootPitchClass = root
                hyp.bassPitchClass = bassPitchClass
                hyp.cosineSimilarity = similarity
                hyp.baseScore = calculateBaseScore(similarity, template, shiftedChroma)
                candidates.append(hyp)
    
    return candidates
```

### Cosine Similarity

```cpp
float cosineSimilarity(const ChromaVector& a, const float* templateBins) {
    float dot = 0, magA = 0, magB = 0;
    for (int i = 0; i < 12; ++i) {
        dot += a[i] * templateBins[i];
        magA += a[i] * a[i];
        magB += templateBins[i] * templateBins[i];
    }
    return dot / (sqrt(magA) * sqrt(magB));
}
```

### Base Score Calculation

The base score combines cosine similarity with bonuses for matched extensions and template priority:

```text
baseScore = cosineSimilarity
          + extensionBonus       // +0.06 per matched extension bin
          + priorityBonus        // (priority - 1.0) * 0.15
```

**Extension bins:** Indices 1, 2, 3, 5, 6, 8, 9, 10, 11 (all non-root intervals)

### Output: ChordHypothesis

```cpp
struct ChordHypothesis {
    int rootPitchClass;       // 0-11
    int bassPitchClass;       // For slash chord detection
    int formulaIndex;         // Index into CHORD_TEMPLATES
    const char* templateName; // "maj7", "m7", etc.
    float cosineSimilarity;   // Raw cosine similarity
    float baseScore;          // Adjusted score with bonuses
    double timestamp;
};
```

---

## 7. Harmonic Memory

**Component:** `HarmonicMemory` ([HarmonicMemory.h](../Source/core/HarmonicMemory.h), [HarmonicMemory.cpp](../Source/core/HarmonicMemory.cpp))

Provides temporal stability by reinforcing chord hypotheses that persist across multiple frames.

### Design Principle

> A wrong chord briefly is worse than a delayed correct chord.

### Frame Storage

Maintains a sliding window of recent "harmonic frames" (default: 200ms, max 16 frames):

```cpp
struct HarmonicFrame {
    double timestamp;
    std::array<ChordHypothesis, MAX_CANDIDATES_PER_FRAME> candidates;
    int candidateCount;
};
```

### Exponential Time Decay

Older frames receive diminishing weight:

```text
weight = exp(-deltaTime / halfLife)
```

Default half-life: **100ms**

### Reinforcement Algorithm (Pseudocode)

```text
function getReinforcedCandidates():
    currentTime = newestFrame.timestamp
    reinforcedMap = {}
    
    for frame in frames:
        decayWeight = exp(-(currentTime - frame.timestamp) / halfLifeMs)
        
        for candidate in frame.candidates:
            key = (root, formulaIndex, bass)
            if key in reinforcedMap:
                reinforcedMap[key].score += candidate.baseScore * decayWeight
                reinforcedMap[key].frameCount += 1
            else:
                reinforcedMap[key] = ReinforcedCandidate(candidate, decayWeight)
    
    return sorted(reinforcedMap.values(), by=score)
```

### Output: ReinforcedCandidate

```cpp
struct ReinforcedCandidate {
    ChordHypothesis hypothesis;
    float reinforcementScore;   // Accumulated temporal score
    int frameCount;             // Frames this chord appeared in
    double firstSeenTime;
    double lastSeenTime;
};
```

---

## 8. Confidence Scoring

**Component:** `ConfidenceScorer` ([ConfidenceScorer.h](../Source/core/ConfidenceScorer.h), [ConfidenceScorer.cpp](../Source/core/ConfidenceScorer.cpp))

Calculates final confidence as a weighted sum of multiple factors.

### Scoring Factors

| Factor | Weight | Description |
|--------|--------|-------------|
| Interval Coverage | 0.25 | Required intervals present |
| Missing Core Penalty | 0.20 | Penalty for missing 3rd/7th |
| Bass Alignment | 0.15 | Root matches bass note |
| Temporal Stability | 0.30 | Persists across frames |
| Complexity Penalty | 0.05 | Prefer simpler chords |
| Velocity Weight | 0.05 | Louder notes matter more |

### Confidence Calculation (Pseudocode)

```text
function scoreCandidate(reinforced, context):
    hyp = reinforced.hypothesis
    
    intervalScore = calculateIntervalCoverage(hyp)
    missingScore = calculateMissingCorePenalty(hyp)
    bassScore = calculateBassAlignment(hyp, context.bassPitchClass)
    stabilityScore = calculateTemporalStability(reinforced)
    complexityScore = calculateComplexityPenalty(hyp)
    velocityScore = calculateVelocityWeight(hyp, context.avgVelocity)
    
    // Integrate baseScore from CandidateGenerator
    baseBonus = (hyp.baseScore - 0.9) * 0.3
    
    confidence = weights.intervalCoverage * intervalScore
              + weights.missingCoreTones * missingScore
              + weights.bassAlignment * bassScore
              + weights.temporalStability * stabilityScore
              + weights.complexityPenalty * complexityScore
              + weights.velocityWeight * velocityScore
              + baseBonus
    
    return ScoredCandidate(hyp, confidence)
```

### Bass Alignment Scoring

```cpp
float calculateBassAlignment(const ChordHypothesis& hyp, int bassPitchClass) {
    if (hyp.rootPitchClass == bassPitchClass)
        return 1.0f;  // Root position
    
    // Check if bass is a chord tone (3rd or 5th = inversion)
    int bassInterval = (bassPitchClass - hyp.rootPitchClass + 12) % 12;
    if (bassInterval == 3 || bassInterval == 4 || bassInterval == 7)
        return 0.95f;  // Valid inversion
    
    return 0.7f;  // Non-chord tone in bass
}
```

---

## 9. Chord Resolution

**Component:** `ChordResolver` ([ChordResolver.h](../Source/core/ChordResolver.h), [ChordResolver.cpp](../Source/core/ChordResolver.cpp))

Selects the final chord from scored candidates, applying stability and simplicity preferences.

### Resolution Rules

1. **Temporal Stability:** If the previous chord is still in top candidates and within margin, keep it
2. **Simplicity Preference:** If two chords score within `simplicityMargin`, prefer the simpler one
3. **Minimum Confidence:** Below threshold, output "No Chord"

### Configuration

```cpp
struct ResolutionConfig {
    float minimumConfidence = 0.45f;
    float stabilityMargin = 0.1f;     // Hysteresis for chord changes
    float simplicityMargin = 0.01f;   // Prefer simpler when very close
    bool allowSlashChords = true;
};
```

### Resolution Algorithm (Pseudocode)

```text
function resolve(scoredCandidates, context, currentTime):
    if scoredCandidates.empty() or topScore < minimumConfidence:
        return NoChord
    
    best = scoredCandidates[0]
    
    // Check if previous chord is still viable
    if previousChord.isValid():
        for candidate in scoredCandidates:
            if candidate matches previousChord:
                if best.confidence - candidate.confidence < stabilityMargin:
                    return previousChord  // Keep stable
    
    // Apply simplicity preference
    for candidate in scoredCandidates[1:]:
        if best.confidence - candidate.confidence < simplicityMargin:
            if candidate.complexity < best.complexity:
                best = candidate
    
    return ResolvedChord(best)
```

### Output: ResolvedChord

```cpp
struct ResolvedChord {
    ChordHypothesis chord;
    float confidence;
    int bassPitchClass;
    char chordName[32];       // "Cmaj7", "Dm7/A", etc.
    const char* qualityName;  // "Major 7th", etc.
    bool isSlashChord;
    int inversionType;        // 0=root, 1=first, 2=second
};
```

---

## 10. Design Decisions

### Why Weighted Matching (Not Binary)?

Binary template matching fails on real music:

| Input | Binary Result | Weighted Result |
|-------|--------------|-----------------|
| C-E-B (no G) | No match for Cmaj7 | Cmaj7 (5th is optional) |
| E-Bb (tritone shell) | Unknown | C7 (shell voicing) |
| C-E-G-A | C6 or Am7? | Context-dependent |

Float weights allow **partial matches** with proportional confidence.

### Why Tolerate Ambiguity?

Some chord sets are genuinely ambiguous:
- C6 and Am7 share identical pitch classes (C-E-G-A)
- C/E and Em share identical pitch classes (E-G-C)

Rather than forcing a wrong answer, the algorithm:
1. Reports the most likely interpretation
2. Provides confidence score reflecting ambiguity
3. Uses temporal context to prefer stable interpretations

### Why Bass Boost = 3.0?

The bass note is the strongest indicator of chord function:
- C-E-G with C in bass = C major
- C-E-G with E in bass = C/E (first inversion)
- C-E-G with G in bass = C/G (second inversion)

Without bass boost, the algorithm cannot distinguish these cases.

### Why Low 5th Weight?

Jazz pianists routinely omit the 5th:
- Cmaj7 often voiced as C-E-B
- Dm7 often voiced as D-F-C
- The 5th adds no harmonic information (already implied by root)

High 5th weight would cause missed detections on common voicings.

---

## 11. Known Limitations

### Inversions vs. Related Chords

C/E (C major first inversion) and Em share identical pitch classes when bass is dominant:

```text
Input: E3-G4-C5
Bass-boosted chroma: E=3.0, G=1.0, C=1.0

This matches Em better than C/E because:
- Em template: Root=E(1.0), m3=G(1.0), P5=B(0.2)
- C/E interpretation requires recognizing E as the 3rd of C
```

**Mitigation:** Bass alignment scoring improves this, but without harmonic context (key, progression), perfect disambiguation is impossible.

### Extended Chord Equivalences

Certain chords are pitch-class identical:
- C6 = Am7 (C-E-G-A)
- Cm6 = Am7b5 (C-Eb-G-A)

The algorithm accepts both as valid interpretations.

### Missing Root Detection

Shell voicings (no root) rely on template matching:

```text
Input: E-Bb (C7 shell)
Algorithm tests all 12 roots and finds C7 shell template matches best.
```

This works when the template library includes shell voicings.

### Polychords and Clusters

Dense voicings with many notes may match multiple templates similarly. The algorithm selects the highest-scoring interpretation but confidence will be lower.

---

## 12. References

1. Adam Stark - Chord Detector & Chromagram  
   https://github.com/adamstark/Chord-Detector-and-Chromagram

2. O. Gillet et al. - Chord Detection with Weighted Templates  
   IEEE, 2011

3. ISMIR 2008 - Symbolic Chord Recognition  
   https://archives.ismir.net/ismir2008/paper/000200.pdf

4. Piano Chord Recognition (University of Rochester)  
   https://hajim.rochester.edu/ece/sites/zduan/teaching/ece477/projects/2015/Shan_Anis_ReportFinal.pdf
