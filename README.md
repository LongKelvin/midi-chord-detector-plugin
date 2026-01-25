# MIDI Chord Detector

A real-time MIDI chord detection VST3 plugin with **advanced harmonic recognition** and **temporal reasoning**. Designed for jazz, live performance, practice, and composition.

![Version](https://img.shields.io/badge/version-2.0.0-blue)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)
![VST3](https://img.shields.io/badge/VST3-Instrument-green)

## Overview

MIDI Chord Detector v2.0 uses a sophisticated **layered architecture** that treats chords as hypotheses evolving over time, not instant detections. The key principle: *"A wrong chord briefly is worse than a delayed correct chord."*

The plugin analyzes MIDI input using temporal reasoning with a sliding memory window, multi-factor confidence scoring, and intelligent resolution rules to produce stable, musically-sensible chord names.

**Note:** This is a **VST3 Instrument**, not a MIDI FX plugin. The MIDI FX version is available in other branches.

## Features

### Core Capabilities
- **Advanced harmonic recognition** - Jazz-capable detection with 24 chord types
- **Temporal reasoning** - 200ms sliding memory window with exponential decay
- **Multi-factor confidence scoring** - Weighted analysis of intervals, bass, stability, and complexity
- **Hysteresis protection** - Prevents rapid flickering between similar chords
- **Real-time safe** - No heap allocations in audio thread, fixed-size buffers

### Technical Features
- **Event-driven architecture** - Responds instantly to MIDI events (Note On/Off, Sustain Pedal)
- **Sustain pedal support** - Respects CC64 for natural playing
- **Rolled chord detection** - Captures arpeggiated chords via temporal memory
- **Voice separation** - Intelligently separates bass, chord tones, and melody
- **Clean UI** - Minimal, distraction-free chord display
- **MIDI pass-through** - Forwards all MIDI to downstream instruments
- **Zero latency** - No audio processing, pure MIDI analysis

### Supported Chord Types (24 Total)

**Tier 1 - MVP Jazz Chords (~80% of real jazz harmony):**
- Major, Minor, Dominant 7th, Minor 7th, Major 7th
- Minor 7♭5 (Half-diminished), Diminished 7th

**Tier 2 - Extended Support:**
- Diminished, Augmented, Augmented 7th
- Minor/Major 7th, Dominant 7sus4
- 6th chords (Major 6, Minor 6)
- 9th chords (Dominant 9, Major 9, Minor 9)
- Suspended (sus2, sus4, 7sus4)
- Added tone chords (add9)
- Alterations (7♯5, 7♭9)

## Architecture

```
┌─────────────────┐
│  MidiNoteState  │  Active notes with timestamps
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│CandidateGenerator│  Enumerate all chord hypotheses
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ HarmonicMemory  │  Temporal reasoning with decay
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ConfidenceScorer │  Multi-factor scoring
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  ChordResolver  │  Final selection with rules
└─────────────────┘
```

### Confidence Scoring Weights

| Factor | Weight | Description |
|--------|--------|-------------|
| Temporal Stability | 0.30 | How consistently the chord appears across memory frames |
| Interval Coverage | 0.25 | How well notes match required chord intervals |
| Missing Core Tones | 0.20 | Penalty for missing root, 3rd, or 7th |
| Bass Alignment | 0.15 | Bonus when bass note matches chord root |
| Complexity Penalty | 0.05 | Prefer simpler chords (Occam's razor) |
| Velocity Weight | 0.05 | Louder notes contribute more |

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

The plugin uses a **layered detection pipeline** with temporal reasoning:

### 1. Note State Tracking
- Tracks all active notes with timestamps
- Respects sustain pedal (CC64)
- Builds pitch class set for analysis

### 2. Candidate Generation
- Tests each note as potential root
- Evaluates against 24 chord formula definitions
- Calculates formula match scores using interval importance weights

### 3. Harmonic Memory
- Maintains sliding 200ms window of recent frames
- Applies exponential decay (older = less weight)
- Reinforces consistently appearing candidates
- Enables temporal stability calculation

### 4. Confidence Scoring
Multi-factor weighted scoring:
- **Temporal stability (30%)** - Consistency across memory frames
- **Interval coverage (25%)** - How well notes match chord intervals
- **Missing core tones (20%)** - Penalty for absent root/3rd/7th
- **Bass alignment (15%)** - Bonus when bass = root
- **Complexity penalty (5%)** - Prefer simpler chords
- **Velocity weighting (5%)** - Louder notes matter more

### 5. Resolution
- Applies hysteresis (prefer current chord if close)
- Requires confidence delta > 0.05 to switch
- Outputs stable, musically-sensible results

## Known Limitations

- **Jazz detection is Tier 1** - Handles ~80% of real jazz harmony; complex voicings/polychords may misidentify
- **Only tested with Cubase** - May have issues with other DAWs
- **Windows only** - No macOS build currently available
- **No user settings** - Configuration is hard-coded
- **Basic UI** - Minimal display, no customization options
- **Latency** - ~10-50ms detection delay for temporal stability (intentional design choice)

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

- **Tier 2+ chord formulas** - Add 11th, 13th, altered extensions
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
- Layered detection architecture with temporal reasoning
- Chord formula system based on music theory interval analysis
- Inspired by the need for jazz-capable chord feedback in Cubase AI/Elements

---

**Tested with:** Cubase AI 13  
**Last updated:** January 2026
