# Algorithm Implementation Guide

## Pattern-Based Interval Matching for MIDI Chord Detection

This document describes the implementation details of the chord detection algorithm for developers who need to understand, modify, or extend the codebase.

---

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
2. [Module Descriptions](#2-module-descriptions)
3. [Detection Pipeline](#3-detection-pipeline)
4. [Pattern Database](#4-pattern-database)
5. [Scoring System](#5-scoring-system)
6. [Confidence Calculation](#6-confidence-calculation)
7. [Testing Strategy](#7-testing-strategy)
8. [Performance Considerations](#8-performance-considerations)
9. [Extending the System](#9-extending-the-system)

---

## 1. Architecture Overview

### Module Structure

```
chord_detection/
├── api/
│   └── JuceChordDetector.h          # JUCE wrapper API
├── detector/
│   ├── ChordDetector.h/.cpp         # Main orchestrator
│   ├── ChordPattern.h               # Pattern structure definition
│   ├── ChordPatterns.h/.cpp         # 60+ pattern database
│   ├── ChordScoring.h/.cpp          # Scoring algorithms
│   ├── ChordCandidate.h             # Detection result structure
│   ├── VoicingAnalyzer.h/.cpp       # Voicing classification
│   ├── ChordFormatter.h/.cpp        # Display name formatting
│   ├── NoteUtils.h/.cpp             # Pitch/interval utilities
│   └── ChordTypes.h                 # Enums and constants
```

### Key Design Principles

1. **Separation of Concerns** - Each module has a single responsibility
2. **No JUCE Dependencies** - Core detection logic is framework-agnostic
3. **Real-Time Safe** - No allocations in detection path
4. **Testable** - Pure functions, deterministic behavior
5. **Extensible** - Easy to add patterns or modify scoring

---

## 2. Module Descriptions

### ChordDetector (Main Orchestrator)

**File:** `ChordDetector.h/.cpp`

**Responsibilities:**
- Entry point for chord detection
- Coordinates all sub-modules
- Manages pattern database and interval index
- Tracks chord history (optional)

**Key Methods:**
```cpp
ChordDetector(bool enableContext, SlashChordMode slashMode);
std::shared_ptr<ChordCandidate> detectChord(const std::vector<int>& midiNotes);
```

**Not Thread-Safe:** Use separate instances per thread or synchronize externally.

### ChordPatterns (Pattern Database)

**File:** `ChordPatterns.h/.cpp`

**Responsibilities:**
- Define all 60+ chord patterns
- Build interval index for fast lookup
- Provide pattern data to detector

**Pattern Definition:**
```cpp
struct ChordPattern {
    std::vector<int> intervals;         // [0, 4, 7, 11]
    int baseScore;                      // 100-135
    std::vector<int> required;          // [0, 4, 11]
    std::vector<int> optional;          // [7]
    std::vector<int> importantIntervals;// [4, 11]
    std::string display;                // "{root}maj7"
    std::string quality;                // "major"
};
```

**Interval Index:**
- Maps interval signatures to chord types
- Enables O(1) exact match lookup
- Fallback to full scan if no exact match

### ChordScoring (Scoring Engine)

**File:** `ChordScoring.h/.cpp`

**Responsibilities:**
- Score candidate chords using weighted factors
- Calculate confidence from score distribution
- Implement scoring heuristics

**Key Functions:**
```cpp
float computeScore(
    const std::vector<int>& intervals,
    const ChordPattern& pattern,
    int bassPitchClass,
    int potentialRoot,
    VoicingType voicingType);

float computeConfidence(
    float bestScore,
    float secondBestScore,
    int noteCount,
    bool exactMatch);
```

### VoicingAnalyzer (Voicing Classification)

**File:** `VoicingAnalyzer.h/.cpp`

**Responsibilities:**
- Classify chord voicing types
- Calculate note span
- Detect drop voicings

**Key Functions:**
```cpp
VoicingType classifyVoicing(const std::vector<int>& midiNotes);
int calculateSpan(const std::vector<int>& midiNotes);
bool isCloseVoicing(const std::vector<int>& midiNotes);
```

### NoteUtils (Utility Functions)

**File:** `NoteUtils.h/.cpp`

**Responsibilities:**
- Convert MIDI notes to pitch classes
- Calculate intervals between pitch classes
- Generate note names and degree labels
- Format display strings

**Key Functions:**
```cpp
int midiToPitchClass(int midiNote);
int intervalBetween(int root, int pitchClass);
std::string getNoteName(int pitchClass);
std::string getDegreeName(int interval);
```

---

## 3. Detection Pipeline

### Entry Point

```cpp
// In ChordDetector::detectChord()
auto result = detector.detectChord({60, 64, 67, 71});
// Returns std::shared_ptr<ChordCandidate>
```

### Pipeline Steps

#### Step 1: Input Validation & Preprocessing

```cpp
// Minimum note count check
if (midiNotes.size() < kDefaultMinimumNotes) {
    return nullptr;
}

// Remove duplicates, sort
std::vector<int> sortedNotes = midiNotes;
std::sort(sortedNotes.begin(), sortedNotes.end());
auto last = std::unique(sortedNotes.begin(), sortedNotes.end());
sortedNotes.erase(last, sortedNotes.end());
```

#### Step 2: Pitch Class Extraction

```cpp
// Convert MIDI → pitch class
std::vector<int> pitchClasses;
for (int midi : sortedNotes) {
    pitchClasses.push_back(NoteUtils::midiToPitchClass(midi));
}

// Get unique pitch classes
std::vector<int> uniquePitchClasses = pitchClasses;
// ... deduplicate ...

// Identify bass
int bassPitchClass = pitchClasses[0];
```

#### Step 3: Voicing Classification

```cpp
VoicingType voicingType = VoicingAnalyzer::classifyVoicing(sortedNotes);
```

#### Step 4: Root Testing Loop

```cpp
for (int potentialRoot : uniquePitchClasses) {
    // Calculate intervals from this root
    std::vector<int> intervals;
    for (int pc : uniquePitchClasses) {
        int interval = NoteUtils::intervalBetween(potentialRoot, pc);
        intervals.push_back(interval);
        
        // Extended intervals for 9ths, 11ths, 13ths
        if (sortedNotes.size() > 3) {
            intervals.push_back(interval + 12);
        }
    }
    
    // ... continue to pattern matching ...
}
```

#### Step 5: Pattern Lookup

```cpp
// Try exact match first (fast path)
auto it = intervalIndex_.find(intervals);
if (it != intervalIndex_.end()) {
    candidateTypes = it->second;  // Found exact match
} else {
    // Fallback: test all patterns
    for (const auto& [type, _] : chordPatterns_) {
        candidateTypes.push_back(type);
    }
}
```

#### Step 6: Candidate Scoring

```cpp
for (const std::string& chordType : candidateTypes) {
    const ChordPattern& pattern = chordPatterns_.at(chordType);
    
    float score = ChordScoring::computeScore(
        intervals, pattern, bassPitchClass, potentialRoot, voicingType);
    
    if (score > kMinimumScoreThreshold) {
        // Create candidate, populate fields
        candidates.push_back(candidate);
    }
}
```

#### Step 7: Best Candidate Selection

```cpp
// Sort by score
std::sort(candidates.begin(), candidates.end(),
    [](const auto& a, const auto& b) { return a->score > b->score; });

auto best = candidates[0];
float secondBestScore = candidates.size() > 1 ? candidates[1]->score : 0.0f;

// Calculate confidence
best->confidence = ChordScoring::computeConfidence(
    best->score, secondBestScore, noteCount, exactMatch);

// Resolve ambiguities (e.g., m6 vs m)
if (candidates.size() > 1) {
    best = resolveAmbiguity(topCandidates, bassPitchClass);
}
```

---

## 4. Pattern Database

### Pattern Structure

Each pattern defines:

1. **Intervals** - Semitone distances from root (e.g., [0, 4, 7, 11] = maj7)
2. **Base Score** - Starting score (80-135, higher = preferred when ambiguous)
3. **Required** - Intervals that must be present
4. **Optional** - Intervals that don't disqualify if missing
5. **Important** - Quality-defining intervals (typically 3rd and 7th)
6. **Display** - Format string with {root} placeholder
7. **Quality** - Category label (major, minor, dominant, etc.)

### Example Pattern: Dominant 7th

```cpp
patterns["dominant7"] = ChordPattern(
    {0, 4, 7, 10},  // Root, M3, P5, m7
    115,            // Base score (higher than basic triads)
    {0, 4, 10},     // Must have root, 3rd, 7th
    {7},            // 5th is optional
    {4, 10},        // 3rd and 7th define dominant quality
    "{root}7",      // Display as "C7", "D7", etc.
    "dominant"      // Quality category
);
```

### Pattern Guidelines

| Chord Complexity | Base Score | Example |
|------------------|-----------|---------|
| Triads | 80-100 | major, minor, dim |
| 7th chords | 100-115 | maj7, m7, 7 |
| 9th chords | 120-125 | maj9, m9, 9 |
| 11th chords | 125-130 | maj11, m11, 11 |
| 13th chords | 130-135 | maj13, m13, 13 |
| Altered | 115-128 | 7♯11, 7alt, 7♭9 |

**Why higher base scores for complex chords?**

When ambiguous, prefer the more specific interpretation:
- C-E-G-B-D could be Cmaj9 or C(add9) with extra notes
- Higher base score for maj9 ensures it's chosen

### Interval Index

The interval index enables fast exact-match lookup:

```cpp
// Built during initialization
intervalIndex[{0, 4, 7}] = ["major"];
intervalIndex[{0, 3, 7}] = ["minor"];
intervalIndex[{0, 4, 7, 11}] = ["major7"];
intervalIndex[{0, 3, 7, 10}] = ["minor7"];
```

**Collision handling:** Multiple chord types can share intervals (e.g., C6 and Am7 both map to [0, 4, 7, 9]). All matching types are scored.

---

## 5. Scoring System

### Scoring Formula

```cpp
float score = pattern.baseScore;

// Exact match: +150
if (intervalSet == patternSet)
    score += 150.0f;

// Required intervals: +30 each (or disqualify)
for (int req : pattern.required)
    if (intervals contains req)
        score += 30.0f;
    else
        return 0.0f;  // Must have all required

// Optional intervals: +10 each
for (int opt : pattern.optional)
    if (intervals contains opt)
        score += 10.0f;

// Important intervals: +30 each
for (int imp : pattern.importantIntervals)
    if (intervals contains imp)
        score += 30.0f;

// Root position: +15
if (bassPitchClass == potentialRoot)
    score += 15.0f;

// Match ratio: +80 max
matchRatio = matchedCount / pattern.intervals.size();
score += matchRatio * 80.0f;

// Extra intervals: -8 each
for (int interval : intervals)
    if (not in pattern or optional)
        score -= 8.0f;

// Voicing bonuses
if (voicing == Rootless) score += 10.0f;
if (voicing == Close) score += 5.0f;
```

### Scoring Rationale

| Factor | Weight | Rationale |
|--------|--------|-----------|
| Exact match | +150 | Strongly prefer perfect interval matches |
| Required | +30 | Core chord tones must be present |
| Important | +30 | 3rd and 7th define chord quality |
| Optional | +10 | Extensions add color but aren't essential |
| Root position | +15 | Modest bonus to avoid over-favoring root |
| Match ratio | +80 | Reward high percentage of pattern matched |
| Extra intervals | -8 | Small penalty allows melody notes |

**Key insight:** Root position gets only +15 (not +150) to avoid forcing root position when the actual chord is an inversion or related chord.

---

## 6. Confidence Calculation

### Multi-Factor Confidence

```cpp
float marginConf = std::min((bestScore - secondBest) / 100.0f, 1.0f);
float absoluteConf = std::min(bestScore / 250.0f, 1.0f);
float noteConf = std::min(noteCount / 6.0f, 1.0f);
float exactConf = exactMatch ? 1.0f : 0.5f;

confidence = 0.4f * marginConf +
             0.4f * absoluteConf +
             0.1f * noteConf +
             0.1f * exactConf;
```

### Factor Breakdown

1. **Margin Confidence (40%)**
   - Measures ambiguity
   - Large gap → high confidence
   - Small gap → low confidence (multiple plausible chords)

2. **Absolute Confidence (40%)**
   - Measures match quality
   - High score → strong match
   - Low score → weak match

3. **Note Count Confidence (10%)**
   - More notes → more information
   - Fewer notes → higher uncertainty

4. **Exact Match Bonus (10%)**
   - Perfect interval match → bonus
   - Imperfect match → reduced confidence

### Confidence Ranges

| Confidence | Interpretation |
|------------|---------------|
| 0.8 - 1.0 | Very high confidence, unambiguous |
| 0.6 - 0.8 | High confidence, likely correct |
| 0.4 - 0.6 | Moderate confidence, some ambiguity |
| 0.2 - 0.4 | Low confidence, multiple interpretations |
| 0.0 - 0.2 | Very low confidence, uncertain |

---

## 7. Testing Strategy

### Unit Test Structure

**File:** `Source/tests/ChordDetectionTests.cpp`

Standalone executable (no JUCE dependencies) testing 40+ chord patterns.

### Test Categories

1. **Basic Triads** - major, minor, dim, aug, sus2, sus4
2. **Seventh Chords** - maj7, m7, 7, mMaj7, dim7, m7♭5
3. **Extended Chords** - 9, 11, 13 variants
4. **Altered Dominants** - 7♭9, 7♯9, 7♯11, 7alt
5. **Inversions** - First, second, third inversions
6. **Edge Cases** - Sparse voicings, wide ranges, ambiguous sets

### Test Execution

```powershell
cd Source/tests
cmake -B build
cmake --build build --config Release
.\build\Release\ChordDetectionTests.exe
```

### Adding New Tests

```cpp
TEST_CASE("New Chord Pattern") {
    ChordDetector detector;
    
    // Test root position
    auto result = detector.detectChord({60, 64, 68});  // C-E-G♯ = Caug
    REQUIRE(result != nullptr);
    REQUIRE(result->chordName == "Caug");
    
    // Test inversion
    result = detector.detectChord({64, 68, 72});  // E-G♯-C = Caug/E
    REQUIRE(result != nullptr);
    REQUIRE(result->root == 0);  // Root is still C
}
```

---

## 8. Performance Considerations

### Real-Time Safety

**No Allocations in Detection Path:**
- Pattern database is pre-initialized
- Interval index is pre-built
- All buffers are pre-allocated or use stack allocation
- No `new`, `std::make_shared` in hot path (candidates use pre-allocated pool in actual plugin)

### Optimization Strategies

1. **Interval Index** - O(1) exact match lookup avoids testing all patterns
2. **Early Disqualification** - Missing required intervals → score = 0, skip
3. **Top-N Candidates** - Only track top candidates, discard low scorers
4. **Minimal Copies** - Use references and moves where possible

### Typical Performance

- **60+ patterns** tested per root
- **12 roots** tested per detection
- **~720 pattern tests** per chord detection (worst case)
- **Execution time:** <1ms on modern CPU (Release build)

---

## 9. Extending the System

### Adding a New Chord Pattern

1. **Define the pattern in ChordPatterns.cpp:**

```cpp
patterns["newChord"] = ChordPattern(
    {0, 3, 6, 10},    // intervals
    115,              // base score
    {0, 3, 6, 10},    // required
    {},               // optional
    {3, 6, 10},       // important
    "{root}dim7",     // display
    "diminished"      // quality
);
```

2. **Add test case in ChordDetectionTests.cpp:**

```cpp
TEST_CASE("Diminished 7th") {
    ChordDetector detector;
    auto result = detector.detectChord({60, 63, 66, 69});
    REQUIRE(result->chordName == "Cdim7");
}
```

3. **Run tests to validate**

### Tuning Scoring Weights

Edit `ChordScoring.cpp`:

```cpp
// Increase exact match bonus
if (intervalSet == patternSet) {
    score += 200.0f;  // Was 150
}

// Reduce extra interval penalty
score -= static_cast<float>(extraCount) * 5.0f;  // Was 8
```

**Always test changes extensively!**

### Adding New Voicing Types

1. Define enum in `ChordTypes.h`
2. Implement classification in `VoicingAnalyzer.cpp`
3. Add scoring bonus in `ChordScoring.cpp`

---

## 10. Known Issues & Future Work

### Current Limitations

1. **No harmonic context** - Cannot disambiguate C6 vs Am7 without key
2. **No progression analysis** - Each chord detected independently
3. **No voice leading** - Doesn't track which notes changed
4. **Limited ambiguity resolution** - Simple heuristics only

### Future Enhancements

1. **Key detection** - Infer key from chord history
2. **Progression probability** - Use Markov models or harmonic function
3. **Voice leading analysis** - Track note continuity
4. **Machine learning** - Train on labeled chord progressions
5. **User feedback loop** - Allow manual corrections to improve heuristics

---

## References

1. **JUCE Framework** - Audio plugin development
2. **Music Theory** - Berklee, jazz harmony textbooks
3. **Pattern Matching** - Set-based algorithms
4. **Real-Time Audio** - Lock-free programming, audio thread constraints

---

**Document Version:** 3.0.0  
**Last Updated:** January 2026  
**Author:** Long Kelvin
