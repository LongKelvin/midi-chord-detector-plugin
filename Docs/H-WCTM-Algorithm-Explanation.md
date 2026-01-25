# H-WCTM Algorithm Implementation

## Harmonic-Weighted Chord Template Matching

This document describes the **H-WCTM (Harmonic-Weighted Chord Template Matching)** algorithm used in this project for **MIDI-based piano chord detection**, designed specifically for **VST3 Instrument plugins** (JUCE, C++).

### What This Document Covers

- What H-WCTM is
- Why it exists
- How it works step by step
- How it differs from classic chord detection
- How developers can extend or improve it

**Target Audience:** Contributors, DSP developers, and researchers.

---

## 1. Background & Motivation

### Problem Statement

Classic chord detection algorithms work poorly in real-world piano and jazz contexts because:

- Chords are often **inverted**
- Root notes may be **missing**
- Extensions (9, 11, 13) are common
- Voicings omit the 5th or even the 3rd
- Melody notes coexist with harmony
- The sustain pedal blurs note-off timing

In MIDI-based plugins, these issues are amplified because:

- There is no audio context
- Pitch classes alone can be ambiguous
- Timing is event-driven, not frame-based

---

## 2. What Is H-WCTM?

**H-WCTM** stands for:

> **Harmonic-Weighted Chord Template Matching**

It is an enhanced form of classic **Chord Template Matching (CTM)** that introduces:

- Harmonic importance weighting
- Partial template matching
- Penalties for out-of-template tones
- Temporal stability (harmonic memory)
- Confidence-based decision-making

Instead of asking:

> "Which chord exactly matches these notes?"

H-WCTM asks:

> “Which chord hypothesis best explains these notes harmonically?”

---

## 3. Why Classic CTM Fails

Classic CTM uses binary logic:

- Note present → match
- Note missing → fail

This approach breaks down in real music.

### Example Failures

| Input Notes | Classic CTM Result | Musical Reality |
|-------------|--------------------|-----------------|
| C–E–G–B–D   | No match           | Cmaj9           |
| E–G–B       | Em                 | C/E             |
| C–F–G       | Csus4 only         | Often C7sus     |
| C–E–Bb–D#   | Unknown            | C7#9            |

Classic CTM has **no concept of importance or context**.

---

## 4. Core Concepts of H-WCTM

### 4.1 Harmonic Weighting

Not all chord tones are equal.

| Tone Type    | Importance                   |
|--------------|------------------------------|
| Root         | Critical                     |
| 3rd          | Defines quality              |
| 7th          | Defines function             |
| 5th          | Optional                     |
| Extensions   | Optional but informative     |
| Alterations  | Strong color                 |

Each chord template assigns **weights** to its tones.

### 4.2 Partial Matching

A chord does **not** need all notes present.

**Example:**

- C7 = C–E–G–Bb
- Input = E–Bb–D
- Still strongly implies C7

H-WCTM allows missing tones but penalizes excessive absence.

### 4.3 Penalty for Non-Chord Tones

Extra notes reduce confidence but do not invalidate detection.

- Melody notes
- Passing tones
- Grace notes

These are expected in real playing.

### 4.4 Temporal Harmonic Memory

Chord detection is **not stateless**.

Once a chord is detected:

- It persists across frames
- Small note changes do not immediately change the chord
- Sustained chords remain active

This avoids flickering results.

---

## 5. High-Level Algorithm Flow

```text
Incoming MIDI Events
         ↓
Active Note Set (with sustain)
         ↓
Pitch Class Reduction
         ↓
Chord Template Scoring (Harmonic Weighted)
         ↓
Temporal Memory Smoothing
         ↓
Final Chord + Confidence
```

---

## 6. Detailed Algorithm Steps

### Step 1: MIDI Note Tracking

- Track `NOTE_ON` / `NOTE_OFF` events
- Respect sustain pedal state
- Maintain active note set

```text
ActiveNotes = {note, velocity, timestamp}
```

### Step 2: Pitch Class Extraction

Convert MIDI notes to pitch classes:

```text
pitchClass = midiNote % 12
```

Duplicates are collapsed into a set.

### Step 3: Chord Templates

Each chord template contains:

```text
ChordTemplate {
  name
  root
  pitchClasses
  weightMap (PC → weight)
}
```

**Example: C7**

| Pitch | Weight |
|-------|--------|
| C     | 1.0    |
| E     | 0.9    |
| Bb    | 0.9    |
| G     | 0.4    |

### Step 4: Scoring Function

For each template:

```text
score = sum(weights of matched tones)
        - penalty(missing core tones)
        - penalty(extra tones)
```

**Penalty rules:**

- Root and 3rd missing → heavy penalty
- 5th missing → minor penalty

### Step 5: Confidence Calculation

```text
confidence = score / maxPossibleScore
```

Confidence is clamped between 0 and 1.

Low confidence → chord not emitted.

### Step 6: Temporal Memory

**If:**

- New chord score is only slightly higher
- Previous chord confidence is high

**Then:** Keep previous chord

This stabilizes detection.

---

## 7. Handling Known Limitations

### Inversions

C/E and Em share pitch classes.

H-WCTM cannot disambiguate without context.

**Resolution strategies (future work):**

- Bass note tracking
- Key estimation
- Harmonic progression modeling

### Extended Chords

If no template exists:

- Algorithm falls back to the closest 7th chord
- Confidence reflects ambiguity

This is expected behavior, not a bug.

---

## 8. Why This Works for MIDI VST Plugins

- Event-driven (not audio-frame based)
- Deterministic and testable
- Low CPU usage
- No DSP or audio synthesis required
- Suitable for JUCE VST3 Instrument architecture

---

## 9. Extending H-WCTM

Developers can improve detection by:

- Adding new chord templates
- Adjusting harmonic weights
- Introducing bass-note heuristics
- Adding key-aware scoring
- Implementing chord progression probability

---

## 10. Summary

H-WCTM is a practical, extensible, and testable chord detection strategy that:

- Works with real piano playing
- Handles jazz harmony better than classic CTM
- Accepts ambiguity instead of forcing wrong answers
- Fits naturally into a VST instrument architecture

It prioritizes musical correctness over mathematical purity.

---

## 11. References

1. **Adam Stark – Chord Detector & Chromagram**  
   <https://github.com/adamstark/Chord-Detector-and-Chromagram>

2. **O. Gillet et al. – Chord Detection with Weighted Templates**  
   IEEE, 2011

3. **ISMIR 2008 – Symbolic Chord Recognition**  
   <https://archives.ismir.net/ismir2008/paper/000200.pdf>

4. **Piano Chord Recognition (University of Rochester)**  
   <https://hajim.rochester.edu/ece/sites/zduan/teaching/ece477/projects/2015/Shan_Anis_ReportFinal.pdf>