# Technical Design Document

**MIDI Chord Detector - VST3 Instrument Plugin**

This document provides a high-level technical overview for developers who want to understand, build, test, or extend the chord detection plugin.

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

MIDI Chord Detector is a **VST3 Instrument plugin** that analyzes incoming MIDI notes and detects the chord being played using a pattern-based interval matching algorithm. It displays the chord name in real-time as you play.

**Key features:**

- Real-time chord detection from MIDI input
- 60+ chord patterns (triads, 7ths, extensions, altered dominants)
- Interval-based pattern matching with optimized scoring
- Slash chord notation for inversions (configurable)
- Voicing classification (close, open, drop-2, drop-3, rootless)
- Low latency, suitable for live performance

### Why VST3 Instrument (Not MIDI FX)?

This plugin is intentionally a **VST3 Instrument**, not a MIDI Effect. This is a design constraint driven by DAW compatibility:

- **Cubase AI / Elements** (commonly bundled with audio interfaces) does not support MIDI FX plugins
- VST3 Instruments receive MIDI input reliably across all major DAWs
- The plugin does not synthesize audio; it only analyzes MIDI and displays results
- MIDI pass-through is implemented to forward notes to downstream instruments

### Supported DAWs

**Tested:**
- Cubase (all versions)

**Should work (untested):**
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
│   MIDI Track ──────► VST3 Instrument ──────► UI Display         │
│                      (Chord Detector)       (shows chord name)  │
│                            │                                    │
│                            ▼                                    │
│                      MIDI Pass-Through                          │
│                      (to downstream)                            │
└─────────────────────────────────────────────────────────────────┘
```

### Internal Data Flow

```text
┌─────────────────────────────────────────────────────────────────┐
│                    PluginProcessor                              │
│                    (Audio Thread)                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   MIDI Input ──► Note Tracking ──► ChordDetector                │
│   (Note On/Off,     (active notes,      (pattern matching,     │
│    Sustain CC)      pitch classes)       scoring)              │
│                                           │                     │
│                                           ▼                     │
│                                    ChordCandidate               │
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
│   ChordDisplayComponent                                         │
│   ┌───────────────────────────────────────────┐                │
│   │                                           │                │
│   │           Cmaj7/E                         │                │
│   │                                           │                │
│   │   Confidence: 0.85  |  Root Position     │                │
│   └───────────────────────────────────────────┘                │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### Thread Model

| Thread | Components | Communication |
|--------|-----------|---------------|
| Audio Thread | PluginProcessor, ChordDetector | Lock-free, no allocations |
| UI Thread | PluginEditor, ChordDisplayComponent | Reads atomic chord pointer |

Cross-thread communication uses atomic pointers for the current chord. No mutexes or locks are used.

---

## 3. Project Structure

```text
midi-chord-detector-plugin/
├── CMakeLists.txt              # Build configuration
├── build.ps1                   # Windows build script
├── build.bat                   # Windows build script (cmd)
├── README.md                   # Project overview
├── LICENSE                     # MIT license
├── Docs/
│   ├── Algorithm-Explanation.md         # How the algorithm works
│   ├── Algorithm-Implementation.md      # Implementation details
│   └── Technical-Design.md              # This file
├── Resources/
│   └── JUCE/                   # JUCE framework (Git submodule)
├── Source/
│   ├── PluginProcessor.cpp/.h  # JUCE audio processor
│   ├── PluginEditor.cpp/.h     # JUCE UI editor
│   ├── chord_detection/
│   │   ├── api/
│   │   │   └── JuceChordDetector.h     # JUCE wrapper
│   │   └── detector/
│   │       ├── ChordDetector.h/.cpp    # Main orchestrator
│   │       ├── ChordPattern.h          # Pattern structure
│   │       ├── ChordPatterns.h/.cpp    # 60+ patterns
│   │       ├── ChordScoring.h/.cpp     # Scoring engine
│   │       ├── ChordCandidate.h        # Result structure
│   │       ├── VoicingAnalyzer.h/.cpp  # Voicing classification
│   │       ├── ChordFormatter.h/.cpp   # Display formatting
│   │       ├── NoteUtils.h/.cpp        # Utilities
│   │       └── ChordTypes.h            # Enums & constants
│   ├── ui/
│   │   └── ChordDisplayComponent.h/.cpp # Chord display UI
│   ├── standalone/
│   │   ├── Main.cpp                    # Standalone app entry
│   │   └── MainComponent.h/.cpp        # Standalone UI
│   └── tests/
│       ├── ChordDetectionTests.cpp     # Unit tests
│       ├── CMakeLists.txt              # Test build config
│       └── chord_tests.json            # Test data
└── build/                      # CMake build output (gitignored)
```

### Why This Structure?

| Directory | Purpose |
|-----------|---------|
| `Source/chord_detection/` | Core detection logic, JUCE-independent |
| `Source/ui/` | UI components (JUCE-dependent) |
| `Source/tests/` | Standalone test executable |
| `Resources/JUCE/` | JUCE as Git submodule |
| `Docs/` | Algorithm and design documentation |

---

## 4. Core Components

### 4.1 PluginProcessor

**File:** `PluginProcessor.h/.cpp`

Main JUCE audio processor for the VST3 plugin.

**Responsibilities:**

- Receive MIDI events from host
- Track note on/off and sustain pedal (CC64)
- Trigger chord detection on note changes
- Pass MIDI through unchanged (for downstream instruments)
- Communicate chord results to UI thread atomically

**Key Methods:**

```cpp
void processBlock(AudioBuffer<float>&, MidiBuffer& midiMessages);
void processMidiMessage(const MidiMessage& message);
std::shared_ptr<ChordCandidate> getCurrentChord() const;
```

**Real-Time Safety:**
- No allocations in processBlock
- Atomic pointer for UI communication
- Pre-allocated chord buffers

### 4.2 ChordDetector

**File:** `Source/chord_detection/detector/ChordDetector.h/.cpp`

Main orchestrator for chord detection algorithm.

**Responsibilities:**

- Entry point for detection
- Coordinate pattern matching, scoring, and selection
- Manage pattern database and interval index
- Track chord history (optional)

**Key Methods:**

```cpp
ChordDetector(bool enableContext, SlashChordMode slashMode);
std::shared_ptr<ChordCandidate> detectChord(const std::vector<int>& midiNotes);
void setSlashChordMode(SlashChordMode mode);
```

**Not Thread-Safe:** Use separate instances per thread.

### 4.3 ChordPatterns

**File:** `Source/chord_detection/detector/ChordPatterns.h/.cpp`

60+ chord pattern definitions and interval index builder.

**Responsibilities:**

- Define all chord patterns (intervals, scoring, display)
- Build interval index for O(1) exact match lookup
- Provide pattern data to detector

**Key Functions:**

```cpp
void initializePatterns(std::map<std::string, ChordPattern>& patterns);
void buildIntervalIndex(
    const std::map<std::string, ChordPattern>& patterns,
    std::map<std::vector<int>, std::vector<std::string>>& index);
```

### 4.4 ChordScoring

**File:** `Source/chord_detection/detector/ChordScoring.h/.cpp`

Scoring engine for chord candidates.

**Responsibilities:**

- Score candidates using weighted factors
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

### 4.5 VoicingAnalyzer

**File:** `Source/chord_detection/detector/VoicingAnalyzer.h/.cpp`

Classifies chord voicing types.

**Responsibilities:**

- Analyze note spacing
- Classify voicing (close, open, drop-2, drop-3, rootless)
- Calculate note span

**Key Functions:**

```cpp
VoicingType classifyVoicing(const std::vector<int>& midiNotes);
int calculateSpan(const std::vector<int>& midiNotes);
bool isCloseVoicing(const std::vector<int>& midiNotes);
```

### 4.6 NoteUtils

**File:** `Source/chord_detection/detector/NoteUtils.h/.cpp`

Utility functions for MIDI/pitch operations.

**Responsibilities:**

- Convert MIDI to pitch class
- Calculate intervals
- Generate note names and degree labels
- Format display strings

**Key Functions:**

```cpp
int midiToPitchClass(int midiNote);
int intervalBetween(int root, int pitchClass);
std::string getNoteName(int pitchClass);
std::string getDegreeName(int interval);
```

### 4.7 ChordDisplayComponent

**File:** `Source/ui/ChordDisplayComponent.h/.cpp`

UI component for displaying detected chords.

**Responsibilities:**

- Render chord name
- Show confidence and additional info
- Update on timer (polls from audio thread)

---

## 5. Algorithm Integration

### Detection Entry Point

```cpp
// In PluginProcessor::processMidiMessage()
if (message.isNoteOn()) {
    chordDetector_.noteOn(noteNumber);
} else if (message.isNoteOff()) {
    chordDetector_.noteOff(noteNumber);
} else if (message.isController() && message.getControllerNumber() == 64) {
    chordDetector_.setSustainPedal(message.getControllerValue() >= 64);
}

// Detect chord after state change
auto detectedChord = chordDetector_.detectChord(activeNotesList);
```

### Pattern Database Location

Chord patterns are defined in `Source/chord_detection/detector/ChordPatterns.cpp`.

Each pattern specifies:
- **Intervals** - Semitone distances from root
- **Base Score** - Starting score (80-135)
- **Required** - Intervals that must be present
- **Optional** - Intervals that don't disqualify
- **Important** - Quality-defining intervals (3rd, 7th)
- **Display** - Format string with `{root}` placeholder
- **Quality** - Category label

### Scoring Breakdown

Scoring happens in `Source/chord_detection/detector/ChordScoring.cpp`:

1. **ChordScoring::computeScore()** - Calculate weighted score for a candidate
2. **ChordScoring::computeConfidence()** - Calculate confidence from score distribution

### Where to Make Changes

| Task | Location |
|------|----------|
| Add new chord type | `ChordPatterns.cpp` - add to `initializePatterns()` |
| Adjust scoring weights | `ChordScoring.cpp` - modify bonuses/penalties |
| Change minimum threshold | `ChordTypes.h` - modify `kMinimumScoreThreshold` |
| Add new voicing type | `ChordTypes.h` + `VoicingAnalyzer.cpp` |
| Change slash chord behavior | `ChordDetector.cpp` - `setSlashChordMode()` |

---

## 6. Testing Strategy

### Test Harness

**File:** `Source/tests/ChordDetectionTests.cpp`

A standalone C++ executable that tests the chord detection core without JUCE dependencies.

### Building Tests

```powershell
cd Source/tests
cmake -B build
cmake --build build --config Release
```

### Running Tests

```powershell
.\build\Release\ChordDetectionTests.exe
```

Or use the convenience script:

```powershell
.\run_tests.ps1
```

### Test Output

```text
========================================
CHORD DETECTION TEST SUITE
========================================

--- Basic Triads ---
[PASS] C Major (C-E-G) -> C
[PASS] C Minor (C-Eb-G) -> Cm
[PASS] C Diminished (C-Eb-Gb) -> Cdim
[PASS] C Augmented (C-E-G#) -> Caug

--- Seventh Chords ---
[PASS] C Major 7 (C-E-G-B) -> Cmaj7
[PASS] C Dominant 7 (C-E-G-Bb) -> C7
[PASS] C Minor 7 (C-Eb-G-Bb) -> Cm7

========================================
TEST SUMMARY
========================================
Total:  42
Passed: 42
Failed: 0
Rate:   100%
========================================
```

### What Tests Cover

| Category | Examples |
|----------|----------|
| Basic triads | major, minor, dim, aug, sus2, sus4 |
| 7th chords | maj7, m7, 7, mMaj7, dim7, m7♭5, aug7 |
| Extended chords | maj9, 9, m9, maj11, 11, m11, maj13, 13, m13 |
| Altered dominants | 7♭9, 7♯9, 7♭5, 7♯5, 7♯11, 7alt |
| Sixth chords | 6, m6, 6/9, m6/9 |
| Add chords | add9, m(add9), add11, add♯11 |
| Inversions | C/E, C/G, Cm/Eb, Dm7/F |
| Voicings | Close, open, rootless, wide spread |

### Why Deterministic Tests Matter

The chord detector must be:
- **Deterministic** - Same input always produces same output
- **Testable** - Every change can be validated automatically
- **Regression-safe** - Breaking changes are caught immediately

Non-deterministic behavior (timing-dependent, random) would make testing impossible.

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

### Adding New Chord Patterns

1. Open `Source/chord_detection/detector/ChordPatterns.cpp`
2. Add entry to `initializePatterns()`:

```cpp
patterns["myNewChord"] = ChordPattern(
    {0, 3, 6, 10},      // intervals
    115,                 // base score
    {0, 3, 6, 10},      // required
    {},                  // optional
    {3, 6, 10},         // important
    "{root}dim7",       // display
    "diminished"        // quality
);
```

3. Add test case to `Source/tests/ChordDetectionTests.cpp`:

```cpp
TEST_CASE("New Chord Type") {
    ChordDetector detector;
    auto result = detector.detectChord({60, 63, 66, 69});  // Notes
    REQUIRE(result != nullptr);
    REQUIRE(result->chordName == "Cdim7");
    REQUIRE(result->confidence > 0.5);
}
```

4. Run tests to validate:

```powershell
.\run_tests.ps1
```

### Tuning Scoring Weights

**File to edit:** `Source/chord_detection/detector/ChordScoring.cpp`

**What can be tuned:**
- Exact match bonus (+150)
- Required interval bonus (+30)
- Optional interval bonus (+10)
- Important interval bonus (+30)
- Root position bonus (+15)
- Match ratio multiplier (×80)
- Extra interval penalty (-8)

**Always test changes extensively with real MIDI input!**

### Extending Detection Logic

**Safe to modify:**
- Pattern definitions and base scores
- Scoring bonuses/penalties
- Confidence calculation weights

**Requires careful testing:**
- Root testing loop logic
- Interval calculation
- Voicing classification

**Do not modify without deep understanding:**
- MIDI event handling in PluginProcessor
- Thread communication (atomic pointers)
- JUCE audio thread constraints

### Validating Changes

1. **Run unit tests** - Ensure no regressions
2. **Test in DAW** - Verify real-world behavior
3. **Add new tests** - Cover new functionality
4. **Document changes** - Update relevant docs

### Code Style

- **C++17** standard
- **No exceptions** in real-time code
- **No allocations** in audio thread
- **Use fixed-size containers** where possible
- **Document non-obvious decisions**
- **Follow existing naming conventions**

---

## Appendix: Quick Reference

### Key Files for Common Tasks

| Task | File |
|------|------|
| Add chord pattern | `Source/chord_detection/detector/ChordPatterns.cpp` |
| Adjust scoring | `Source/chord_detection/detector/ChordScoring.cpp` |
| Modify voicing classification | `Source/chord_detection/detector/VoicingAnalyzer.cpp` |
| Change UI display | `Source/ui/ChordDisplayComponent.cpp` |
| Add test case | `Source/tests/ChordDetectionTests.cpp` |
| Modify MIDI handling | `Source/PluginProcessor.cpp` |

### Configuration Constants

| Constant | Location | Default | Description |
|----------|----------|---------|-------------|
| kPitchClassCount | ChordTypes.h | 12 | Pitch classes (C-B) |
| kDefaultMinimumNotes | ChordTypes.h | 2 | Min notes for detection |
| kMinimumScoreThreshold | ChordTypes.h | 80.0 | Min score to accept |
| kMaxChordHistorySize | ChordTypes.h | 10 | Chord history buffer |

### Build Commands

```powershell
# Full plugin build
cmake -B build
cmake --build build --config Release

# Test build and run
cd Source/tests
cmake -B build
cmake --build build --config Release
.\build\Release\ChordDetectionTests.exe

# Or use convenience script
.\run_tests.ps1
```

### Plugin Installation

```powershell
# Copy VST3 to system directory (requires admin)
xcopy "build\MidiChordDetector_artefacts\Release\VST3\MIDI Chord Detector.vst3" `
      "C:\Program Files\Common Files\VST3\MIDI Chord Detector.vst3" /E /I /Y
```

---

**Document Version:** 3.0.0  
**Last Updated:** January 2026  
**Author:** Long Kelvin
