# Building on macOS

The project ships with Windows-oriented build scripts (`build.ps1`, `setup_fonts.ps1`), but
the `CMakeLists.txt` itself is cross-platform: all Windows-specific logic is guarded by
`if(WIN32)`. These steps build the VST3 plugin and the standalone test app on macOS
(verified on Apple Silicon / macOS 15, CMake 4.4, AppleClang 21).

## Prerequisites

| Tool | Install | Notes |
|------|---------|-------|
| Xcode Command Line Tools | `xcode-select --install` | Provides `clang`, `xcodebuild` |
| CMake ≥ 3.15 | `brew install cmake` | Build system generator |
| Git | preinstalled / `brew install git` | Used to fetch JUCE |
| JUCE 8.0.x | see below | Framework, not vendored in the repo |
| `Bravura.otf` | already in `fonts/` | SMuFL font for the notation staff |

## 1. Fetch JUCE

The build looks for JUCE in `Resources/JUCE` (among other locations). Clone it there:

```bash
git clone --depth 1 --branch 8.0.12 https://github.com/juce-framework/JUCE.git Resources/JUCE
```

`CMakeLists.txt` auto-detects this path — no `-DJUCE_DIR` needed. To use a JUCE installed
elsewhere, pass `-DJUCE_DIR=/path/to/JUCE` or set the `JUCE_DIR` environment variable.

## 2. Configure & build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

## 3. Output

| Artifact | Path |
|----------|------|
| VST3 plugin | `build/MidiChordDetector_artefacts/Release/VST3/MIDI Chord Detector.vst3` |
| Standalone app | `build/MidiChordDetectorStandalone_artefacts/Release/MIDI Chord Detector Standalone.app` |

Because `COPY_PLUGIN_AFTER_BUILD TRUE` is set, JUCE also installs the VST3 to
`~/Library/Audio/Plug-Ins/VST3/` and ad-hoc code-signs it automatically. The
`code has no resources but signature indicates...` / `replacing existing signature`
lines during the build are JUCE's normal ad-hoc signing step, not errors.

Both binaries are built for the host architecture (arm64 on Apple Silicon). To produce a
universal binary, add `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"` at configure time.

## Build errors encountered & fixes

### `error: unknown type name 'size_t'` in `ChordTypes.h`

```
Source/chord_detection/detector/ChordTypes.h:23:18: error: unknown type name 'size_t'
```

**Cause.** `ChordTypes.h` uses `size_t` (line 23, `kMaxChordHistorySize`) but only included
`<cstdint>`. On MSVC (the original Windows toolchain) `size_t` happens to be visible
transitively, so the code compiled. Clang/GCC are stricter and require the type's own header.

**Fix.** Add `#include <cstddef>` to `ChordTypes.h`:

```cpp
#include <cstdint>
#include <cstddef>  // for std::size_t (implicitly available on MSVC, required on Clang/GCC)
```

This is a portability fix and is correct on all platforms, not a macOS-only workaround.

### Warnings (non-fatal)

The build emits warnings that do **not** stop compilation:
- `-Wimplicit-int-float-conversion` in `NotationComponent.h` / `PluginEditor.cpp` (int→float in
  `drawLine` coordinates).
- `-Wswitch-enum` in `ChordTypes.h` (`VoicingType::Unknown` handled by `default:`).
- `-Wunused-private-field` for `passMidiThrough_` in `PluginProcessor.h`.
- `JUCE_DISPLAY_SPLASH_SCREEN is ignored` — this JUCE version has no splash screen, so the
  compile-definition is a harmless no-op.

These are safe to leave, but are candidates for cleanup if you want a warning-free build.
