# MIDI Chord Detector

A real-time MIDI chord detection VST3 plugin with **pattern-based interval matching** for jazz, live performance, and composition.

![Version](https://img.shields.io/badge/version-3.0.0-blue)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)
![VST3](https://img.shields.io/badge/VST3-Instrument-green)

## Overview

MIDI Chord Detector analyzes incoming MIDI notes and identifies chords in real-time using a pattern-based interval matching algorithm with 60+ chord patterns.

**Key Features:**
- 60+ chord patterns (triads, 7ths, 9ths, 11ths, 13ths, altered dominants)
- Real-time detection with confidence scoring
- Slash chord notation for inversions
- Sustain pedal support
- Zero latency MIDI pass-through

**Note:** This is a **VST3 Instrument** (not MIDI FX) for compatibility with Cubase AI/Elements and other DAWs.

## Features

- **60+ Chord Patterns** - Triads, 7ths, 9ths, 11ths, 13ths, altered dominants, add chords
- **Accurate Detection** - Pattern-based interval matching with weighted scoring
- **Inversion Support** - Intelligent slash chord notation (Auto/Always/Never modes)
- **Voicing Classification** - Close, open, drop-2, drop-3, rootless
- **Real-Time Safe** - No allocations in audio thread, zero latency
- **Sustain Pedal** - Respects CC64 for natural playing
- **MIDI Pass-Through** - Forwards MIDI to downstream instruments

### Supported Chord Types

**Basic:** Major, Minor, Diminished, Augmented, Sus2, Sus4, Power5  
**7th Chords:** maj7, 7, m7, m(maj7), dim7, m7♭5, aug7, +maj7, 7sus4  
**6th Chords:** 6, m6, 6/9, m6/9  
**Extended:** maj9, 9, m9, maj11, 11, m11, maj13, 13, m13  
**Altered Dominants:** 7♭5, 7♯5, 7♭9, 7♯9, 7♯11, 7alt, 9♯11, 13♭9, etc.  
**Add Chords:** add9, m(add9), add11, add♯11  
**Special:** Quartal voicings

## How It Works

1. **MIDI Input** → Convert to pitch classes (0-11)
2. **Voicing Analysis** → Classify chord voicing type
3. **Root Testing** → Test all 12 possible roots
4. **Pattern Matching** → Score against 60+ chord patterns
5. **Confidence Calculation** → Multi-factor confidence scoring
6. **Output** → Display chord name with optional slash notation

**Algorithm Details:** See [Docs/Algorithm-Explanation.md](Docs/Algorithm-Explanation.md)

## Quick Start

1. **Load Plugin** - Add "MIDI Chord Detector" as a VST3 Instrument
2. **Route MIDI** - Connect MIDI output to a sound-generating instrument
3. **Play** - Chord names display in real-time as you play

**Tested with:** Cubase AI 13  
**Should work with:** FL Studio, Reaper, Studio One, Ableton Live, Logic Pro (via bridging)

## Installation

**From Release:**
1. Download from [Releases](../../releases)
2. Extract `MIDI Chord Detector.vst3` to `C:\Program Files\Common Files\VST3\`
3. Restart DAW and rescan plugins

**From Source:**
```powershell
.\build.ps1
# Plugin will be in: build\MidiChordDetector_artefacts\Release\VST3\
```

**Requirements:** Windows 10/11, Visual Studio 2022+, CMake 3.15+

## Known Limitations

- **Ambiguous chords** - C6/Am7 share identical notes; algorithm uses scoring heuristics
- **Sus chord ambiguity** - Csus2 vs Gsus4 without context
- **Sparse voicings** - May misidentify incomplete voicings without harmonic context
- **Platform** - Windows only, tested only with Cubase
- **No user settings** - Engine parameters are hard-coded

## Troubleshooting

**No MIDI detected:** Use as VST Instrument, not audio insert  
**No chord shown:** Play 2+ notes simultaneously  
**Plugin missing:** Check VST3 folder path, rescan plugins

## Documentation

- [Algorithm Explanation](Docs/Algorithm-Explanation.md) - How the detection works
- [Algorithm Implementation](Docs/Algorithm-Implementation.md) - Technical details for developers
- [Technical Design](Docs/Technical-Design.md) - Architecture and contribution guide

## Contributing

Contributions welcome! Areas for improvement:
- Additional chord patterns and voicings
- Scoring weight tuning
- macOS support
- User-configurable settings in UI

See [Technical Design](Docs/Technical-Design.md) for contribution guidelines.

## License

MIT License - see [LICENSE](LICENSE)

## Credits

Built with [JUCE Framework](https://juce.com/)  
Author: Long Kelvin - [GitHub](https://github.com/LongKelvin)

---

**Version:** 3.0.0 | **Platform:** Windows | **Last Updated:** January 2026
