# MIDI Chord Detector

A real-time MIDI chord detection VST3 plugin with **pattern-based interval matching** and **optimized scoring**. Designed for jazz, live performance, practice, and composition.

![Version](https://img.shields.io/badge/version-2.1.0-blue)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)
![VST3](https://img.shields.io/badge/VST3-Instrument-green)

## Overview

MIDI Chord Detector uses a **pattern-based interval matching algorithm** with optimized scoring for accurate chord identification. The algorithm converts MIDI notes to pitch classes, calculates intervals from potential roots, and matches against 60+ chord patterns with intelligent inversion detection and slash chord support.

The plugin analyzes MIDI input by:
1. Converting notes to pitch classes and intervals
2. Testing all 12 possible roots against pattern database
3. Scoring candidates using weighted factors (exact match bonus, required intervals, root position)
4. Selecting the highest-scoring chord with confidence calculation

**Note:** This is a **VST3 Instrument**, not a MIDI FX plugin. The MIDI FX version is available in other branches.

## Features

### Core Capabilities
- **60+ chord patterns** - Comprehensive coverage from triads to extended jazz voicings
- **Interval-based detection** - Fast pattern matching with optimized scoring
- **Accurate inversion detection** - 150pt exact match bonus, 15pt root position bonus, intelligent slash chord handling
- **Confidence scoring** - Multi-factor weighted confidence calculation (score margin, absolute score, note count, exact match)
- **Voicing classification** - Identifies close, open, drop-2, drop-3, and rootless voicings
- **Real-time safe** - No heap allocations in audio thread

### Technical Features
- **Event-driven architecture** - Responds instantly to MIDI events (Note On/Off, Sustain Pedal)
- **Sustain pedal support** - Respects CC64 for natural playing
- **Slash chord modes** - Auto, Always, or Never show bass note indicators
- **Clean UI** - Minimal, distraction-free chord display
- **MIDI pass-through** - Forwards all MIDI to downstream instruments
- **Zero latency** - No audio processing, pure MIDI analysis

### Supported Chord Types (60+ Patterns)

**Basic Triads (7 patterns):**
- Major, Minor, Diminished, Augmented
- Suspended (sus2, sus4), Power 5

**Seventh Chords (9 patterns):**
- Major 7th, Dominant 7th, Minor 7th, Minor/Major 7th
- Diminished 7th, Half-diminished (m7♭5)
- Augmented 7th, Augmented Major 7th, Dominant 7sus4

**Sixth Chords (4 patterns):**
- 6, m6, 6/9, m6/9

**Ninth Chords (6 patterns):**
- Major 9, Dominant 9, Minor 9
- Dominant 7♭9, Dominant 7♯9, Minor/Major 9

**Eleventh Chords (7 patterns):**
- Major 11, Dominant 11, Minor 11
- Dominant 7♯11, Major 7♯11, Major 9♯11, Minor 11♭5

**Thirteenth Chords (7 patterns):**
- Major 13, Dominant 13, Minor 13
- Dominant 13♯11, Dominant 7♭13, Dominant 13♭9, Dominant 13♯9

**Altered Dominants (12 patterns):**
- 7♭5, 7♯5, 7♭5♭9, 7♯5♭9, 7♭5♯9, 7♯5♯9
- 7alt (altered scale), 7♯5♯9♭13
- 9♯11, 9♭13, 7♯9♯11, 7♭9♯11, 7♭9♭13, 7♯9♭13

**Add Chords (4 patterns):**
- add9, m(add9), add11, add♯11

**Special (4 patterns):**
- Major 7♯5, m7♭5 (half-diminished)
- Quartal, Quartal 7

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                       ChordDetector                             │
├─────────────────────────────────────────────────────────────────┤
│  Input: std::vector<int> MIDI notes                             │
│                                                                 │
│  ┌─────────────────┐                                            │
│  │ Pitch Class     │  Convert MIDI → pitch classes (0-11)       │
│  │ Extraction      │  Identify bass note, remove duplicates     │
│  └────────┬────────┘                                            │
│           │                                                     │
│           ▼                                                     │
│  ┌─────────────────┐                                            │
│  │ Voicing         │  Classify: close, open, drop-2/3, rootless │
│  │ Analysis        │  (VoicingAnalyzer module)                  │
│  └────────┬────────┘                                            │
│           │                                                     │
│           ▼                                                     │
│  ┌─────────────────┐                                            │
│  │ Root Testing    │  For each of 12 possible roots:           │
│  │ Loop            │  - Calculate intervals from root           │
│  │                 │  - Quick lookup in interval index          │
│  └────────┬────────┘                                            │
│           │                                                     │
│           ▼                                                     │
│  ┌─────────────────┐                                            │
│  │ Pattern         │  For each candidate chord type:            │
│  │ Scoring         │  - Match intervals against pattern         │
│  │ (ChordScoring)  │  - +150 exact match, +30 req, +10 opt     │
│  │                 │  - +15 root position, -8 extra intervals   │
│  └────────┬────────┘                                            │
│           │                                                     │
│           ▼                                                     │
│  ┌─────────────────┐                                            │
│  │ Best Candidate  │  Sort by score, select highest             │
│  │ Selection       │  Apply ambiguity resolution (m6 vs m)      │
│  │                 │  Calculate confidence (margin-based)       │
│  └────────┬────────┘                                            │
│           │                                                     │
│           ▼                                                     │
│  ┌─────────────────┐                                            │
│  │ Chord           │  Format display name with root             │
│  │ Formatting      │  Add slash notation if needed              │
│  │                 │  Build degree and note name lists          │
│  └────────┬────────┘                                            │
│           │                                                     │
│           ▼                                                     │
│  Output: ChordCandidate { name, root, confidence, ... }         │
└─────────────────────────────────────────────────────────────────┘
```

### Core Components

| Component | Responsibility |
|-----------|---------------|
| **ChordDetector** | Main orchestrator, entry point for detection |
| **ChordPatterns** | 60+ pattern definitions, interval index builder |
| **ChordScoring** | Score computation and confidence calculation |
| **VoicingAnalyzer** | Classifies chord voicing types |
| **ChordFormatter** | Builds display strings with root substitution |
| **NoteUtils** | Pitch class conversions, note names, intervals |
│           ▼                                                     │
│  Output: ChordCandidate { name, root, type, confidence, ... }   │
└─────────────────────────────────────────────────────────────────┘
```

### Scoring Weights

| Factor | Points | Description |
|--------|--------|-------------|
| Exact Match Bonus | +150 | Intervals exactly match pattern |
| Required Interval | +30 each | Each required interval present |
| Optional Interval | +10 each | Each optional interval present |
| Important Interval | +30 each | 3rd or 7th present (quality-defining) |
| Root Position | +15 | Bass note matches chord root |
| Extra Intervals | -8 each | Penalty for notes outside pattern/optional |
| Match Ratio Bonus | +80 max | Percentage of pattern intervals matched |
| Voicing Bonus | +5-10 | Rootless (+10) or close (+5) voicing |

### Confidence Calculation

Confidence is computed from multiple weighted factors:

```
marginComponent = min((bestScore - secondBest) / 100, 1.0) × 0.4
absoluteComponent = min(bestScore / 250, 1.0) × 0.4
noteCountComponent = min(noteCount / 6, 1.0) × 0.1
exactMatchBonus = exactMatch ? 0.1 : 0.0

confidence = marginComponent + absoluteComponent + noteCountComponent + exactMatchBonus
```

Factors:
- **40% score margin** - Gap between best and second-best candidate
- **40% absolute score** - Raw score as percentage of threshold
- **10% note count** - More notes increase confidence
- **10% exact match** - Bonus when intervals perfectly match pattern

## Usage in DAW

### Cubase (Tested & Verified)

1. **Create an Instrument Track**
   - Add a new Instrument Track in your project
   
2. **Load MIDI Chord Detector**
   - In the VST Instruments rack, load "MIDI Chord Detector"
   - The plugin UI will show "N.C." (No Chord) initially

3. **Route MIDI Output**
   - The MIDI Chord Detector forwards MIDI, so you need to route it to an actual sound-generating instrument:
   - **Option A:** Insert track → Add your synth/piano VST after the MIDI Chord Detector
   - **Option B:** Use MIDI routing to send output to another instrument track

4. **Play and Monitor**
   - Play your MIDI keyboard or load a MIDI clip
   - The chord name will display in real-time as you play
   - The chord clears immediately when you release all notes

### Other DAWs

**This plugin has only been tested with Cubase.** It should work in any DAW that supports VST3 Instruments, but functionality is not guaranteed. Please test and report issues.

DAWs that should theoretically work:
- FL Studio
- Reaper
- Studio One
- Ableton Live (with VST3 support)
- Logic Pro (via bridging)

## Installation

### From Release Package

1. Download the latest release package from the [Releases](../../releases) page
2. **Close your DAW completely**
3. Extract `MIDI Chord Detector.vst3` folder to your VST3 directory:
   ```
   C:\Program Files\Common Files\VST3\
   ```
4. Restart your DAW
5. Rescan VST plugins if necessary
6. The plugin will appear under **VST Instruments** (not MIDI FX)

### From Source Code

#### Prerequisites

- Windows 10/11
- Visual Studio 2022 or later (with C++ desktop development)
- CMake 3.15 or later
- Git

#### Build Steps

1. **Clone the repository**
   ```powershell
   git clone https://github.com/YOUR_USERNAME/midi-fx-chord-detector.git
   cd midi-fx-chord-detector/MidiChordDetector
   ```

2. **Run the build script**
   ```powershell
   .\build.ps1
   ```
   
   Or manually:
   ```powershell
   mkdir build
   cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   cmake --build . --config Release
   ```

3. **Install the plugin**
   
   The built plugin will be in:
   ```
   build\MidiChordDetector_artefacts\Release\VST3\MIDI Chord Detector.vst3
   ```
   
   Copy this folder to your VST3 directory (requires admin permissions):
   ```powershell
   # Run PowerShell as Administrator
   xcopy "build\MidiChordDetector_artefacts\Release\VST3\MIDI Chord Detector.vst3" "C:\Program Files\Common Files\VST3\MIDI Chord Detector.vst3" /E /I /Y
   ```

## Configuration

The plugin uses the following default settings:

### Engine Parameters
- **Minimum notes for chord:** 2 (configurable via API)
- **Minimum score threshold:** 80.0 (filters low-quality matches)
- **Slash chord mode:** Auto/Always/Never (configurable)
- **Context tracking:** Optional chord history (up to 10 chords)

These settings can be adjusted programmatically but are not exposed in the UI yet.


### Platform and Testing

- **Only tested with Cubase** - May have compatibility issues with other DAWs
- **Windows only** - No macOS build currently available
- **No user settings** - Engine parameters are hard-coded

## Troubleshooting

### Plugin doesn't receive MIDI

- Ensure you're using it as a **VST Instrument**, not in an audio insert slot
- Check that the track is armed/enabled for MIDI input
- Verify MIDI is being sent to the track (check DAW's MIDI monitoring)

### No chord detected (shows "N.C.")

- Play at least 2 notes simultaneously
- Check that notes are actually being held (not just quick taps)
- Try playing simpler, more recognizable chords first

### Plugin doesn't appear in DAW

- Verify the .vst3 folder is in the correct location
- Rescan VST plugins in your DAW
- Check that your DAW supports VST3 format
- Run the DAW as administrator if permission issues occur

## Technical Details

- **Format:** VST3
- **Type:** Instrument (with MIDI I/O)
- **Audio I/O:** Stereo output (silent)
- **MIDI I/O:** Input and output (pass-through)
- **Latency:** Zero (no audio processing)
- **Real-time safe:** Yes
- **Category:** Instrument/Tools

## Branch Information

This branch contains the **VST3 Instrument version** of the MIDI Chord Detector.

For the **MIDI FX version** (requires DAW support for MIDI FX plugins), check other branches.

## License

MIT License - see [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! 

## Author

**Long Kelvin**  
GitHub: [https://github.com/LongKelvin](https://github.com/LongKelvin)

## Acknowledgments

- Built with [JUCE Framework](https://juce.com/)
- Pattern-based interval matching with optimized scoring
- Chord patterns based on music theory and jazz voicing analysis
- Designed for practical use in Cubase AI/Elements (DAWs with limited MIDI FX support)

---

**Version:** 3.0.0 (Pattern-Based Interval Matching)  
**Tested with:** Cubase AI 13  
**Last updated:** January 2026
