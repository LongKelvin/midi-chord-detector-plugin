# Technical Design Document

**MidiChordDetector - VST3 Instrument Plugin**

This document provides a high-level technical overview of the project for developers who want to understand, build, test, or extend the chord detection plugin.

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [High-Level Architecture](#2-high-level-architecture)
3. [Project Structure](#3-project-structure)
4. [Core Components](#4-core-components)
5. [Algorithm Integration](#5-algorithm-integration)
6. [Testing Strategy](#6-testing-strategy)
7. [Build and Requirements](#7-build-and-requirements)
8. [Contribution Guidelines](#8-contribution-guidelines)

---

## 1. Project Overview

### What This Plugin Does

MidiChordDetector is a **VST3 Instrument plugin** that analyzes incoming MIDI notes and detects the chord being played. It displays the chord name in real-time as you play.

**Key features:**

- Real-time chord detection from MIDI input
- Support for triads, 7th chords, extensions (9, 11, 13), and altered dominants
- Slash chord notation for inversions
- Temporal stability (chords do not flicker)
- Low latency, suitable for live performance

### Why VST3 Instrument (Not MIDI FX)?

This plugin is intentionally a **VST3 Instrument**, not a MIDI Effect. This is a design constraint driven by DAW compatibility:

- **Cubase AI / Elements** (commonly bundled with audio interfaces) does not support MIDI FX plugins
- VST3 Instruments receive MIDI input reliably across all major DAWs
- The plugin does not synthesize audio; it only analyzes MIDI and displays results

### Supported DAWs

Tested with:
- Cubase (all versions)
- Ableton Live
- FL Studio
- Reaper
- Studio One

### Core Constraints

| Constraint | Reason |
|------------|--------|
| No audio synthesis | This is a display-only chord analyzer |
| Real-time safe | No heap allocations in audio thread |
| Deterministic | Same input always produces same output |
| MIDI-only | No audio analysis; pitch detection is not in scope |

---

## 2. High-Level Architecture

### System Overview

```text
┌─────────────────────────────────────────────────────────────────┐
│                         DAW Host                                │
│                                                                 │
│   MIDI Track ──────► Plugin Instance ──────► UI Display         │
│                            │                                    │
│                      (no audio out)                             │
└─────────────────────────────────────────────────────────────────┘
```

### Internal Data Flow

```text
┌─────────────────────────────────────────────────────────────────┐
│                    PluginProcessor                              │
│                    (Audio Thread)                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   MIDI Input ──► MidiNoteState ──► ChordDetectorEngine          │
│                                           │                     │
│                                           ▼                     │
│                                    ResolvedChord                │
│                                           │                     │
│                              (atomic swap to UI thread)         │
│                                           │                     │
└───────────────────────────────────────────│─────────────────────┘
                                            │
                                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                    PluginEditor                                 │
│                    (UI Thread)                                  │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   Display: "Cmaj7"  [confidence: 0.85]                          │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### Thread Model

| Thread | Components | Communication |
|--------|-----------|---------------|
| Audio Thread | PluginProcessor, ChordDetectorEngine | Lock-free, no allocations |
| UI Thread | PluginEditor | Reads atomic chord pointer |

Cross-thread communication uses a double-buffered atomic pointer swap. No mutexes or locks are used.

---

## 3. Project Structure

```text
MidiChordDetector/
├── CMakeLists.txt              # Build configuration
├── build.ps1                   # Windows build script
├── Docs/
│   ├── H-WCTM-Algorithm-Implementation.md
│   └── Technical-Design.md     # This file
├── Resources/
│   ├── Fonts/                  # UI fonts
│   └── JUCE/                   # JUCE framework (submodule)
├── Source/
│   ├── PluginProcessor.cpp     # JUCE audio processor
│   ├── PluginProcessor.h
│   ├── PluginEditor.cpp        # JUCE UI editor
│   ├── PluginEditor.h
│   ├── core/                   # Chord detection engine
│   │   ├── ChordDetectorEngine.h/.cpp
│   │   ├── CandidateGenerator.h/.cpp
│   │   ├── ChordResolver.h/.cpp
│   │   ├── ConfidenceScorer.h/.cpp
│   │   ├── HarmonicMemory.h/.cpp
│   │   ├── MidiNoteState.h/.cpp
│   │   ├── ChordTemplates.h
│   │   ├── ChromaVector.h
│   │   └── ...
│   ├── tests/                  # Standalone test suite
│   │   ├── ChordDetectionTests.cpp
│   │   └── CMakeLists.txt
│   └── ui/                     # UI components
└── build/                      # CMake build output
```

### Why This Structure?

| Directory | Purpose |
|-----------|---------|
| `Source/core/` | Isolates detection logic from JUCE. Can be unit tested standalone. |
| `Source/tests/` | Standalone executable. No JUCE audio dependencies. |
| `Resources/JUCE/` | JUCE as a Git submodule. Keeps dependency explicit. |

---

## 4. Core Components

### 4.1 ChordDetectorEngine

**File:** [ChordDetectorEngine.h](../Source/core/ChordDetectorEngine.h)

The main orchestrator. Owns all detection subsystems and coordinates the pipeline.

**Responsibilities:**

- Receive MIDI events (noteOn, noteOff, sustain pedal)
- Trigger detection on request
- Return ResolvedChord result

**Must not:**

- Perform any heap allocations
- Block or wait
- Access UI directly

**Key methods:**

```cpp
void noteOn(int noteNumber, float velocity, double currentTimeMs);
void noteOff(int noteNumber, double currentTimeMs);
ResolvedChord detectChord(double currentTimeMs);
```

### 4.2 MidiNoteState

**File:** [MidiNoteState.h](../Source/core/MidiNoteState.h)

Tracks which notes are currently active, with velocity and timing.

**Responsibilities:**

- Maintain bitset of active notes (0-127)
- Handle sustain pedal logic
- Provide pitch class set for detection
- Identify bass (lowest) note

### 4.3 CandidateGenerator

**File:** [CandidateGenerator.h](../Source/core/CandidateGenerator.h)

Implements the H-WCTM matching algorithm. Generates chord hypotheses.

**Responsibilities:**

- Build weighted ChromaVector from note state
- Test all 12 roots against all templates
- Calculate cosine similarity scores
- Output array of ChordHypothesis

**Must not:**

- Make final chord decisions (that is ChordResolver's job)
- Apply temporal logic (that is HarmonicMemory's job)

### 4.4 HarmonicMemory

**File:** [HarmonicMemory.h](../Source/core/HarmonicMemory.h)

Provides temporal stability by tracking chord hypotheses over time.

**Responsibilities:**

- Store recent harmonic frames (sliding window)
- Apply exponential time decay to older frames
- Reinforce candidates that persist across frames
- Output ReinforcedCandidate array

### 4.5 ConfidenceScorer

**File:** [ConfidenceScorer.h](../Source/core/ConfidenceScorer.h)

Calculates final confidence scores using multiple weighted factors.

**Scoring factors:**

- Interval coverage (0.25)
- Missing core tones penalty (0.20)
- Bass alignment (0.15)
- Temporal stability (0.30)
- Complexity penalty (0.05)
- Velocity weight (0.05)

### 4.6 ChordResolver

**File:** [ChordResolver.h](../Source/core/ChordResolver.h)

Makes the final chord selection from scored candidates.

**Responsibilities:**

- Apply stability preference (keep previous chord if close)
- Apply simplicity preference (prefer simpler chords when scores are close)
- Enforce minimum confidence threshold
- Build final ResolvedChord output

---

## 5. Algorithm Integration

### Where Templates Live

Chord templates are defined in [ChordTemplates.h](../Source/core/ChordTemplates.h).

Each template specifies:
- Name and full name
- 12-bin float weights (index 0 = root)
- Complexity level
- Priority (for tie-breaking)
- Shell voicing flag

### Where Scoring Happens

1. **CandidateGenerator:** Cosine similarity + extension bonus + priority bonus = baseScore
2. **ConfidenceScorer:** Multi-factor weighted confidence calculation
3. **ChordResolver:** Final selection with stability/simplicity preferences

### Where to Add Improvements

| Task | Location |
|------|----------|
| Add new chord type | `ChordTemplates.h` - add to CHORD_TEMPLATES array |
| Adjust scoring weights | `ConfidenceScorer.h` - ScoringWeights struct |
| Change stability behavior | `ChordResolver.h` - ResolutionConfig |
| Modify bass boost | `ChromaVector.h` - ChromaWeights |
| Change decay rate | `HarmonicMemory.h` - setDecayHalfLifeMs() |

---

## 6. Testing Strategy

### Test Harness

**File:** [ChordDetectionTests.cpp](../Source/tests/ChordDetectionTests.cpp)

A standalone C++ executable that tests the chord detection core without JUCE audio dependencies.

### Building Tests

```powershell
cd Source/tests
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

### Running Tests

```powershell
.\build\Release\ChordDetectionTests.exe
```

### Test Output

```text
========================================
H-WCTM CHORD DETECTION TEST SUITE
========================================

--- Basic Triads ---
[PASS] C Major -> C (conf: 0.513)
[PASS] C Minor -> Cm (conf: 0.536)
...

========================================
TEST SUMMARY
========================================
Total:  41
Passed: 41
Failed: 0
Rate:   100%
========================================
```

### What Tests Cover

| Category | Examples |
|----------|----------|
| Basic triads | Major, minor, diminished, augmented, sus2, sus4 |
| 7th chords | maj7, m7, 7, mMaj7, m7b5, dim7 |
| Extended chords | maj9, 9, m9, m11, 13, 6 |
| Altered dominants | 7b9, 7#9, 7b5, 7#5, 7sus4 |
| Inversions | C/E, C/G, Cm/Eb, Dm7/F |
| Register variations | Wide voicings, low register, high register |

### Why Deterministic Tests Matter

The chord detector must be:
- **Deterministic:** Same input always produces same output
- **Testable:** Every change can be validated automatically
- **Regression-safe:** Breaking changes are caught immediately

Non-deterministic behavior (timing-dependent, random) would make the test suite useless.

---

## 7. Build and Requirements

### Compiler Requirements

| Platform | Compiler |
|----------|----------|
| Windows | Visual Studio 2019+ (MSVC 14.2+) |
| macOS | Xcode 12+ (Clang) |
| Linux | GCC 9+ or Clang 10+ |

### JUCE Version

JUCE 7.x (included as submodule in `Resources/JUCE/`)

### Building the Plugin

```powershell
# Windows (PowerShell)
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

```bash
# macOS / Linux
cmake -B build
cmake --build build --config Release
```

### Output Location

Built VST3 plugin: `build/MidiChordDetector_artefacts/Release/VST3/`

### Optional: Build Script

```powershell
.\build.ps1  # Windows convenience script
```

---

## 8. Contribution Guidelines

### Adding New Chord Templates

1. Open [ChordTemplates.h](../Source/core/ChordTemplates.h)
2. Add entry to `CHORD_TEMPLATES` array
3. Follow weight guidelines:
   - Root, 3rd, 7th: 1.0
   - 5th: 0.2
   - Extensions: 0.3-0.8
4. Set appropriate complexity (1-4) and priority
5. Add test case to `ChordDetectionTests.cpp`
6. Run tests to validate

### Extending Detection Logic

**Safe to modify:**

- Template weights and priorities
- Scoring weights in ConfidenceScorer
- Resolution thresholds in ChordResolver

**Requires careful testing:**

- ChromaVector weighting (bass boost, register weights)
- CandidateGenerator scoring formula
- HarmonicMemory decay parameters

**Do not modify without understanding side effects:**

- MidiNoteState bitset logic
- ChordDetectorEngine pipeline order
- Cross-thread communication in PluginProcessor

### Validating Changes

1. Run the full test suite
2. Verify no regressions in existing test cases
3. Add new test cases for new functionality
4. Test in DAW with real MIDI input

### Code Style

- C++17 standard
- No exceptions in real-time code
- No heap allocations in audio thread
- Use fixed-size arrays and pre-allocated buffers
- Document non-obvious design decisions

---

## Appendix: Quick Reference

### Key Files for Common Tasks

| Task | File |
|------|------|
| Add chord template | `Source/core/ChordTemplates.h` |
| Adjust scoring | `Source/core/ConfidenceScorer.h` |
| Change resolution rules | `Source/core/ChordResolver.h` |
| Modify bass boost | `Source/core/ChromaVector.h` |
| Add test case | `Source/tests/ChordDetectionTests.cpp` |
| Modify UI | `Source/PluginEditor.cpp` |

### Configuration Constants

| Constant | Location | Default |
|----------|----------|---------|
| Bass boost | ChromaVector.h | 3.0 |
| Memory window | HarmonicMemory.h | 200ms |
| Decay half-life | HarmonicMemory.h | 100ms |
| Min confidence | ChordResolver.h | 0.45 |
| Simplicity margin | ChordResolver.h | 0.01 |

### Test Command

```powershell
cd Source/tests
cmake --build build --config Release; .\build\Release\ChordDetectionTests.exe
```
