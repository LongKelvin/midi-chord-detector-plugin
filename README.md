# MIDI Chord Detector

A real-time MIDI chord detection VST3 plugin that displays the currently playing chord on-screen. Designed for live performance, practice, and composition.

![Version](https://img.shields.io/badge/version-1.0.0-blue)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)
![VST3](https://img.shields.io/badge/VST3-Instrument-green)

## Overview

MIDI Chord Detector analyzes MIDI input in real-time and displays the detected chord name. It's implemented as a **VST3 Instrument** to ensure compatibility with Cubase AI/Elements and other DAWs that don't support MIDI FX plugins.

**Note:** This is a **VST3 Instrument**, not a MIDI FX plugin. The MIDI FX version is available in other branches.

## Features

- **Real-time chord detection** - Displays chords as you play
- **Event-driven architecture** - Responds instantly to MIDI events (Note On/Off, Sustain Pedal)
- **Sustain pedal support** - Respects CC64 for natural playing
- **Rolled chord detection** - Captures arpeggiated chords played within 150ms
- **Voice separation** - Intelligently separates bass, chord tones, and melody
- **Clean UI** - Minimal, distraction-free chord display
- **MIDI pass-through** - Forwards all MIDI to downstream instruments
- **Zero latency** - No audio processing, pure MIDI analysis

### Supported Chord Types

The detector recognizes common chord types including:
- Major, Minor, Diminished, Augmented
- Dominant 7th, Major 7th, Minor 7th
- Extended chords (9th, 11th, 13th)
- Suspended chords (sus2, sus4)
- Various alterations and jazz voicings

**Note:** The chord detection algorithm works well with basic chords but may not be perfect for complex jazz voicings or unusual chord structures. It's designed for common chord progressions in pop, rock, and basic jazz contexts.

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

1. Download the latest release package from the [Releases](https://github.com/LongKelvin/midi-chord-detector-plugin/releases) page
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
   git clone https://github.com/LongKelvin/midi-chord-detector-plugin.git
   cd midi-chord-detector-plugin
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

- **Minimum notes for chord:** 2
- **Time window for rolled chords:** 150ms
- **Voice separation:**
  - Bass: below E3 (MIDI note 52)
  - Melody: above C5 (MIDI note 72)
  - Chord tones: E3 to C5

These settings are optimized for typical keyboard playing and cannot currently be changed (may be configurable in future versions).

## How It Works

The plugin uses a sophisticated chord detection algorithm:

1. **Event-Driven Detection** - Chord analysis triggers only on MIDI events (Note On/Off, Sustain Pedal), not on timers
2. **Note State Tracking** - Maintains accurate state of held notes and sustain pedal
3. **Voice Separation** - Separates bass, chord tones, and melody notes
4. **Pitch Class Analysis** - Converts notes to pitch classes (0-11) for root-independent analysis
5. **Pattern Matching** - Tests all 12 possible roots against all chord type definitions
6. **Weighted Scoring** - Scores candidates based on interval importance and completeness
7. **Best Match Selection** - Selects the highest-scoring chord that meets minimum threshold

The algorithm prioritizes:
- Common chord types over exotic ones
- Root position over inversions (but detects and displays both)
- Complete chord intervals over partial matches

## Known Limitations

- **Chord detection is not perfect** - Works well for common chords (major, minor, 7ths, etc.) but may misidentify complex jazz chords or polychords
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

- chord detection algorithm accuracy
- macOS support
- User-configurable settings
- Additional chord types
- Testing with other DAWs
- UI improvements

## Author

**Long Kelvin**  
GitHub: [https://github.com/LongKelvin](https://github.com/LongKelvin)

## Acknowledgments

- Built with [JUCE Framework](https://juce.com/)
- Chord detection algorithm based on weighted pitch class analysis
- Inspired by the need for real-time chord feedback in Cubase AI/Elements

---

**Tested with:** Cubase AI 13  
**Last updated:** January 2026
