# Pattern-Based Interval Matching Algorithm

## Overview

This document describes the **Pattern-Based Interval Matching** algorithm used in the MIDI Chord Detector VST3 plugin for real-time chord detection from MIDI input.

### What This Document Covers

- How the algorithm works step-by-step
- Why it uses interval matching instead of template matching
- Scoring system and confidence calculation
- Known limitations and design trade-offs
- How to extend the pattern database

**Target Audience:** Contributors, developers, and researchers interested in MIDI chord detection.

---

## 1. Background & Motivation

### The Challenge

Real-time MIDI chord detection for piano and keyboard is challenging because:

- **Inversions are common** - The bass note often isn't the root
- **Incomplete voicings** - Jazz players omit the 5th or even the root
- **Extensions are ubiquitous** - 9ths, 11ths, 13ths, altered tones
- **Sustain pedal complications** - Notes sustain beyond their intended release
- **Ambiguous pitch class sets** - C6 and Am7 are identical (C-E-G-A)

### Design Goals

1. **Accurate jazz harmony detection** - Handle extensions, alterations, rootless voicings
2. **Real-time performance** - No allocations, deterministic, low latency
3. **Honest about ambiguity** - Use confidence scores instead of forcing wrong answers
4. **Extensible** - Easy to add new chord patterns
5. **Testable** - Deterministic behavior for automated testing

---

## 2. Algorithm Overview

### Core Concept

Instead of asking "what chord exactly matches these notes?", the algorithm asks:

> **"For each possible root, which chord pattern best explains these intervals?"**

The algorithm:
1. Converts MIDI notes to pitch classes (C=0, C♯=1, ..., B=11)
2. Tests all 12 possible roots
3. Calculates intervals from each potential root
4. Scores each (root, chord type) combination
5. Returns the highest-scoring match with confidence

### Why Intervals, Not Templates?

This algorithm uses **interval sets** rather than weighted templates because:

- Intervals are invariant to transposition (C major and D major have identical interval structures)
- Pattern matching is discrete and fast (set membership tests)
- Scoring is transparent and tunable (explicit bonuses/penalties)
- No floating-point chroma vectors or cosine similarity needed

---

## 3. Algorithm Pipeline

```
MIDI Input
    ↓
[1] Pitch Class Extraction
    ↓  (unique pitch classes, bass identification)
    ↓
[2] Voicing Classification  
    ↓  (close/open/drop-2/drop-3/rootless)
    ↓
[3] Root Testing Loop (12 iterations)
    ↓  For each root: calculate intervals
    ↓
[4] Pattern Lookup
    ↓  (interval index for fast exact matches)
    ↓
[5] Candidate Scoring
    ↓  (weighted factors: exact match, required, optional, root position)
    ↓
[6] Best Candidate Selection
    ↓  (sort by score, resolve ambiguities)
    ↓
[7] Confidence Calculation
    ↓  (margin, absolute score, note count, exact match)
    ↓
Output: ChordCandidate
```

---

## 4. Detailed Algorithm Steps

### Step 1: Pitch Class Extraction

**Input:** MIDI note numbers (e.g., [60, 64, 67, 71] = C4, E4, G4, B4)

**Process:**
- Convert to pitch classes: `pitchClass = midiNote % 12`
- Remove duplicates
- Sort ascending
- Identify bass pitch class (lowest note)

**Output:** Unique pitch classes [0, 4, 7, 11], bass = 0

### Step 2: Voicing Classification

Analyze note distribution to classify voicing type:

| Voicing | Criteria |
|---------|----------|
| Close | All notes within 12 semitones |
| Open | Notes span > 12 semitones |
| Rootless | Root interval (0) not present after root testing |
| Drop-2 | Second voice from top dropped an octave |
| Drop-3 | Third voice from top dropped an octave |

**Purpose:** Voicing type influences scoring (e.g., rootless voicing gets +10 bonus)

### Step 3: Root Testing Loop

For each of 12 possible roots (C through B):

**Calculate intervals:**
```cpp
for (int potentialRoot : pitchClasses) {
    std::vector<int> intervals;
    for (int pc : pitchClasses) {
        int interval = (pc - potentialRoot + 12) % 12;
        intervals.push_back(interval);
        
        // Extended intervals for 9ths, 11ths, 13ths
        if (noteCount > 3) {
            intervals.push_back(interval + 12);
        }
    }
}
```

**Example:**
- Pitch classes: [0, 4, 7, 11]
- Root = 0 (C): intervals = [0, 4, 7, 11]
- Root = 4 (E): intervals = [0, 3, 8, 11]

### Step 4: Pattern Lookup

**Interval Index (Fast Path):**

The algorithm maintains a reverse index mapping interval signatures to chord types:
```cpp
intervalIndex[{0, 4, 7}] = ["major"]
intervalIndex[{0, 3, 7}] = ["minor"]
intervalIndex[{0, 4, 7, 11}] = ["major7"]
```

If detected intervals exactly match an index entry, only those chord types are tested (O(1) lookup).

**Full Pattern Scan (Fallback):**

If no exact match, all 60+ patterns are tested. This handles incomplete voicings and extensions.

### Step 5: Candidate Scoring

For each (root, chordType) pair, compute a score using weighted factors:

```cpp
float score = pattern.baseScore;  // Starting value (80-135)

// MASSIVE bonus for exact match
if (intervals == pattern.intervals)
    score += 150.0f;

// Required intervals (must all be present)
for (int req : pattern.required)
    if (intervals contains req)
        score += 30.0f;
    else
        return 0.0f;  // Disqualified

// Optional intervals
for (int opt : pattern.optional)
    if (intervals contains opt)
        score += 10.0f;

// Important intervals (3rd, 7th)
for (int imp : pattern.importantIntervals)
    if (intervals contains imp)
        score += 30.0f;

// Root position bonus
if (bassPitchClass == potentialRoot)
    score += 15.0f;

// Match ratio bonus
matchRatio = matched / pattern.intervals.size();
score += matchRatio * 80.0f;

// Penalty for extra intervals
for (int interval : intervals)
    if (interval not in pattern or optional)
        score -= 8.0f;

// Voicing bonuses
if (voicing == Rootless) score += 10.0f;
if (voicing == Close) score += 5.0f;
```

**Scoring Philosophy:**

- Exact matches get huge bonuses (+150) to strongly prefer perfect fits
- Required intervals are mandatory (score = 0 if missing)
- Extra intervals get small penalties (-8) to allow melody notes
- Root position gets modest bonus (+15) to avoid over-favoring root over correct chord

### Step 6: Best Candidate Selection

After scoring all candidates:

1. **Sort by score** (descending)
2. **Resolve ambiguities** - Special case: prefer m6 over m when both match
3. **Format chord name** - Apply slash notation based on bass note and slash mode
4. **Populate metadata** - Degree names, note names, position description

**Ambiguity Resolution:**

Some chord pairs frequently co-occur and need special handling:

| Ambiguity | Resolution |
|-----------|-----------|
| m6 vs m | Prefer m6 (6th is distinctive) |
| C6 vs Am7 | Rely on scoring (C6 has higher base score) |
| Csus2 vs Gsus4 | Root position scoring decides |

### Step 7: Confidence Calculation

Confidence reflects how certain the algorithm is:

```cpp
float marginConf = min((bestScore - secondBest) / 100.0, 1.0);
float absoluteConf = min(bestScore / 250.0, 1.0);
float noteConf = min(noteCount / 6.0, 1.0);
float exactConf = exactMatch ? 1.0 : 0.5;

confidence = 0.4 * marginConf +
             0.4 * absoluteConf +
             0.1 * noteConf +
             0.1 * exactConf;
```

**Factors:**

- **40% margin** - Gap between best and second-best (ambiguity measure)
- **40% absolute** - Raw score as percentage of high threshold
- **10% note count** - More notes = more information
- **10% exact match** - Bonus for perfect interval match

---

## 5. Known Limitations

### Ambiguous Pitch Class Sets

Some note combinations have multiple valid interpretations:

| Notes | Possible Chords | Algorithm Behavior |
|-------|----------------|-------------------|
| C-E-G-A | C6, Am7 | Prefers C6 (higher base score) |
| C-Eb-G-A | Cm6, Am7♭5 | Prefers Cm6 (m6 pattern priority) |
| E-G-C | Em, C/E | Depends on bass position and scoring |

The algorithm uses heuristics (base scores, root position bonus) but cannot always determine musical intent without harmonic context (key, progression).

### Incomplete Voicings

Jazz voicings often omit the 5th or root:

- **Shell voicings** (3rd + 7th only): Algorithm can detect these if root position scoring favors the correct root
- **Sparse voicings** (e.g., just root + 7th): May misidentify without the 3rd

### Sus Chord Ambiguity

Suspended chords can be ambiguous:

- **C-D-G**: Could be Csus2 or Gsus4
- Algorithm uses root position scoring and base scores to decide

### Design Trade-offs

The algorithm **intentionally accepts ambiguity** rather than forcing incorrect answers. Low confidence scores signal uncertainty. This is honest about the limitations of pure MIDI analysis without harmonic context.

---

## 6. Extending the Algorithm

### Adding New Chord Patterns

1. Open `Source/chord_detection/detector/ChordPatterns.cpp`
2. Add entry to `initializePatterns()`:

```cpp
patterns["newChordType"] = ChordPattern(
    {0, 4, 7, 10, 14},  // intervals: root, M3, P5, m7, M9
    120,                 // base score
    {0, 4, 10},         // required: root, 3rd, 7th
    {7, 14},            // optional: 5th, 9th
    {4, 10, 14},        // important: 3rd, 7th, 9th
    "{root}9",          // display template
    "dominant"          // quality category
);
```

3. Run tests to validate

### Tuning Scoring Weights

Scoring weights are in `Source/chord_detection/detector/ChordScoring.cpp`:

- Adjust bonuses/penalties to change detection behavior
- Test extensively with real-world MIDI input
- Document rationale for changes

### Improving Ambiguity Resolution

Add special cases in `ChordDetector::resolveAmbiguity()`:

```cpp
// Example: prefer sus4 over sus2 when bass is root
if (top->chordType == "sus2" && second->chordType == "sus4") {
    if (bassPitchClass == top->root) {
        return top;  // Root position sus2 is more common
    }
}
```

---

## 7. References

1. **MIDI Specification** - MIDI Manufacturers Association
2. **Music Theory** - Interval structures, chord voicings, jazz harmony
3. **Pattern Matching** - Set-based matching for discrete data
4. **Real-Time Systems** - Lock-free programming, audio thread constraints

---

**Document Version:** 3.0.0  
**Last Updated:** January 2026  
**Author:** Long Kelvin
