# MIDI Chord Detector

A real-time MIDI chord detection VST3 plugin with **pattern-based harmonic recognition** and **optimized scoring**. Designed for jazz, live performance, practice, and composition.

![Version](https://img.shields.io/badge/version-3.0.0-blue)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)
![VST3](https://img.shields.io/badge/VST3-Instrument-green)

## Overview

MIDI Chord Detector v3.0 uses a **pattern-based detection algorithm** with optimized scoring for accurate chord identification. The algorithm matches input notes against 100+ chord patterns with intelligent inversion detection and slash chord support.

The plugin analyzes MIDI input using pattern matching with weighted scoring, confidence calculation, and voicing classification to produce accurate, musically-sensible chord names.

**Note:** This is a **VST3 Instrument**, not a MIDI FX plugin. The MIDI FX version is available in other branches.

## Features

### Core Capabilities
- **100+ chord patterns** - Comprehensive coverage from triads to extended jazz voicings
- **Pattern-based detection** - Fast matching algorithm with optimized scoring
- **Accurate inversion detection** - 150pt bonus for root position, intelligent slash chord handling
- **Confidence scoring** - Multi-factor weighted confidence calculation
- **Voicing classification** - Identifies close, open, drop-2, drop-3, and rootless voicings
- **Real-time safe** - No heap allocations in audio thread

### Technical Features
- **Event-driven architecture** - Responds instantly to MIDI events (Note On/Off, Sustain Pedal)
- **Sustain pedal support** - Respects CC64 for natural playing
- **Slash chord modes** - Auto, Always, or Never show bass note indicators
- **Clean UI** - Minimal, distraction-free chord display
- **MIDI pass-through** - Forwards all MIDI to downstream instruments
- **Zero latency** - No audio processing, pure MIDI analysis

### Supported Chord Types (100+ Patterns)

**Basic Triads:**
- Major, Minor, Diminished, Augmented
- Suspended (sus2, sus4)

**7th Chords:**
- Major 7th, Dominant 7th, Minor 7th, Minor/Major 7th
- Diminished 7th, Half-diminished (m7♭5)
- Augmented 7th, Dominant 7sus4

**Extended Chords:**
- 9th (Major 9, Dominant 9, Minor 9)
- 11th (Major 11, Dominant 11, Minor 11)
- 13th (Major 13, Dominant 13, Minor 13)

**Altered Dominants:**
- 7♭5, 7♯5, 7♭9, 7♯9
- 7♯5♭9, 7♯5♯9, 7♯11
- 13♭9, 13♯9, 7♭9♯11

**Add Chords:**
- add9, m(add9), add11, add13
- 6, m6, 6/9, m6/9

**Special:**
- Power chords (5)
- Quartal voicings

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    OptimizedChordDetector                       │
├─────────────────────────────────────────────────────────────────┤
│  Input: std::vector<int> MIDI notes                             │
│                                                                 │
│  ┌─────────────────┐                                            │
│  │ Pitch Class     │  Convert MIDI → pitch classes (0-11)       │
│  │ Extraction      │  Identify bass note and interval set       │
│  └────────┬────────┘                                            │
│           │                                                     │
│           ▼                                                     │
│  ┌─────────────────┐                                            │
│  │ Voicing         │  Classify: close, open, drop-2/3, rootless │
│  │ Classification  │                                            │
│  └────────┬────────┘                                            │
│           │                                                     │
│           ▼                                                     │
│  ┌─────────────────┐                                            │
│  │ Pattern         │  Test all 12 roots × 100+ patterns         │
│  │ Matching        │  Score each candidate chord                │
│  └────────┬────────┘                                            │
│           │                                                     │
│           ▼                                                     │
│  ┌─────────────────┐                                            │
│  │ Score           │  +150pt root position, +30pt required,     │
│  │ Calculation     │  +20pt optional, -15pt missing required    │
│  └────────┬────────┘                                            │
│           │                                                     │
│           ▼                                                     │
│  ┌─────────────────┐                                            │
│  │ Best Candidate  │  Select highest score                      │
│  │ Selection       │  Calculate confidence                      │
│  └────────┬────────┘                                            │
│           │                                                     │
│           ▼                                                     │
│  Output: ChordCandidate { name, root, type, confidence, ... }   │
└─────────────────────────────────────────────────────────────────┘
```

### Scoring Weights

| Factor | Points | Description |
|--------|--------|-------------|
| Root Position Bonus | +150 | Bass note matches chord root |
| Required Interval | +30 | Each required interval present |
| Optional Interval | +20 | Each optional interval present |
| Missing Required | -15 | Penalty for missing required intervals |
| Voicing Type | ×1.0-1.2 | Bonus for valid voicing patterns |

### Confidence Calculation

```
margin = (bestScore - secondBest) / bestScore
absolute = score / maxPossible
noteCount = notes.size() / 6.0

confidence = 0.4 × margin + 0.4 × absolute + 0.2 × noteCount + exactBonus
```

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
- **Minimum notes for chord:** 2
- **Memory window:** 200ms (temporal reasoning)
- **Decay half-life:** 100ms (exponential weighting)
- **Minimum confidence:** 0.45 (detection threshold)

### Voice Separation
- **Bass:** below E3 (MIDI note 52)
- **Melody:** above C5 (MIDI note 72)
- **Chord tones:** E3 to C5

These settings are optimized for typical keyboard playing and cannot currently be changed (may be configurable in future versions).

## How It Works

The plugin uses a **pattern-based detection algorithm** with optimized scoring:

### 1. Note State Tracking
- Tracks all active notes
- Respects sustain pedal (CC64)
- Converts MIDI notes to pitch classes (0-11)

### 2. Voicing Classification
Analyzes note spacing to identify:
- **Close voicing** - Notes within an octave
- **Open voicing** - Notes spread across > 2 octaves
- **Drop-2/Drop-3** - Jazz piano voicings with dropped notes
- **Rootless** - Common jazz voicings without root

### 3. Pattern Matching
- Tests each of 12 possible roots (C through B)
- Evaluates against 100+ chord pattern definitions
- Calculates match scores using weighted intervals

### 4. Scoring
Point-based scoring system:
- **+150 points** - Root position (bass = chord root)
- **+30 points** - Each required interval present
- **+20 points** - Each optional interval present
- **-15 points** - Each missing required interval
- **×1.0-1.2** - Voicing type multiplier

### 5. Confidence Calculation
Multi-factor weighted confidence:
- **40% margin** - Score gap between best and second-best
- **40% absolute** - Raw score as % of maximum
- **20% note count** - More notes = higher confidence
- **+0.1 bonus** - Exact interval match

## Known Limitations

- **Jazz detection** - Handles most jazz harmony; highly complex polychords may misidentify
- **Only tested with Cubase** - May have issues with other DAWs
- **Windows only** - No macOS build currently available
- **No user settings** - Configuration is hard-coded
- **Basic UI** - Minimal display, no customization options

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

Contributions are welcome! Areas that need improvement:

- **Additional chord patterns** - Add more specialized voicings
- **Scoring weight tuning** - Optimize weights based on real-world testing
- **macOS support** - Cross-platform build
- **User-configurable settings** - Expose engine parameters in UI
- **Testing with other DAWs** - Verify compatibility
- **UI improvements** - Visual feedback for confidence, chord history

## Author

**Long Kelvin**  
GitHub: [https://github.com/LongKelvin](https://github.com/LongKelvin)

## Acknowledgments

- Built with [JUCE Framework](https://juce.com/)
- Pattern-based detection algorithm with optimized scoring
- Chord patterns based on music theory interval analysis
- Inspired by the need for jazz-capable chord feedback in Cubase AI/Elements

---

**Tested with:** Cubase AI 13  
**Last updated:** January 2026
