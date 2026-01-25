# Resources Directory

This directory contains external dependencies and resources for the MIDI Chord Detector plugin.

## JUCE Framework

The build script automatically downloads the JUCE framework to this location:
- `MidiChordDetector/Resources/JUCE/`

You don't need to manually download JUCE - the build script handles it automatically.

## Manual Installation

If you prefer to install JUCE manually:

1. Download JUCE from: https://juce.com/get-juce/
2. Extract/clone to this directory: `MidiChordDetector/Resources/JUCE/`
3. Ensure `MidiChordDetector/Resources/JUCE/CMakeLists.txt` exists
4. Run the build script

## Fonts

Custom fonts used by the plugin UI should be stored in `MidiChordDetector/Resources/Fonts/`
(Currently using default system fonts)

## Git Ignore

JUCE is excluded from version control via `.gitignore` to keep the repository size small.
The build script will automatically download it when needed.
