#!/usr/bin/env bash
#
# build.sh — One-click build for the MIDI Chord Detector on macOS / Linux.
#
# Counterpart of build.ps1. Configures with CMake, builds the VST3 plugin and
# the standalone app, and (via COPY_PLUGIN_AFTER_BUILD) installs the VST3 to the
# user plugin folder.
#
# Usage:
#   ./build.sh                 # Release build
#   ./build.sh --debug         # Debug build
#   ./build.sh --clean         # wipe build/ first
#   ./build.sh --universal     # macOS universal binary (arm64 + x86_64)
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"
BUILD_DIR="$PROJECT_DIR/build"
JUCE_DIR="$PROJECT_DIR/Resources/JUCE"

BUILD_CONFIG="Release"
DO_CLEAN=0
UNIVERSAL=0

for arg in "$@"; do
    case "$arg" in
        --debug)     BUILD_CONFIG="Debug" ;;
        --release)   BUILD_CONFIG="Release" ;;
        --clean)     DO_CLEAN=1 ;;
        --universal) UNIVERSAL=1 ;;
        *) echo "Unknown option: $arg"; exit 1 ;;
    esac
done

step()  { printf '\n\033[36m==> %s\033[0m\n' "$1"; }
ok()    { printf '\033[32m[OK] %s\033[0m\n'   "$1"; }
warn()  { printf '\033[33m[WARN] %s\033[0m\n' "$1"; }
fail()  { printf '\033[31m[ERROR] %s\033[0m\n' "$1"; }

echo "============================================================="
echo "  JUCE MIDI Chord Detector — Build Script ($BUILD_CONFIG)"
echo "  macOS / Linux"
echo "============================================================="

# ── Prerequisites ─────────────────────────────────────────────────────────────
step "Checking prerequisites..."
if ! command -v cmake >/dev/null 2>&1; then
    fail "CMake not found. Install it (macOS: brew install cmake) then re-run."
    exit 1
fi
ok "CMake: $(cmake --version | head -1)"

# ── JUCE (auto-fetch via setup.sh if missing) ─────────────────────────────────
if [[ ! -f "$JUCE_DIR/CMakeLists.txt" ]]; then
    warn "JUCE not found at $JUCE_DIR — running setup.sh to fetch it."
    "$SCRIPT_DIR/setup.sh"
fi
ok "JUCE found at: $JUCE_DIR"

# ── Bravura font ──────────────────────────────────────────────────────────────
if [[ ! -f "$PROJECT_DIR/fonts/Bravura.otf" ]]; then
    warn "Bravura.otf missing — running setup_fonts.sh."
    "$SCRIPT_DIR/setup_fonts.sh"
fi

# ── Clean ─────────────────────────────────────────────────────────────────────
if [[ "$DO_CLEAN" -eq 1 && -d "$BUILD_DIR" ]]; then
    step "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
    ok "Cleaned."
fi

# ── Configure ─────────────────────────────────────────────────────────────────
step "Configuring CMake..."
CMAKE_ARGS=(-B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_CONFIG")
if [[ "$UNIVERSAL" -eq 1 && "$(uname)" == "Darwin" ]]; then
    CMAKE_ARGS+=(-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64")
    echo "  Building a universal binary (arm64 + x86_64)"
fi
cmake "${CMAKE_ARGS[@]}"
ok "Configuration complete."

# ── Build ─────────────────────────────────────────────────────────────────────
step "Building ($BUILD_CONFIG)..."
cmake --build "$BUILD_DIR" --config "$BUILD_CONFIG" --parallel
ok "Build successful."

# ── Verify output ─────────────────────────────────────────────────────────────
step "Verifying build output..."
VST3_PATH="$BUILD_DIR/MidiChordDetector_artefacts/$BUILD_CONFIG/VST3/MIDI Chord Detector.vst3"
if [[ -d "$VST3_PATH" ]]; then
    ok "VST3 built at: $VST3_PATH"
else
    warn "Could not find built .vst3 at expected location."
fi

if [[ "$(uname)" == "Darwin" ]]; then
    INSTALLED="$HOME/Library/Audio/Plug-Ins/VST3/MIDI Chord Detector.vst3"
    if [[ -d "$INSTALLED" ]]; then
        ok "Installed to: $INSTALLED"
    else
        warn "VST3 not found in user plugin folder; copy it manually if needed."
    fi
fi

echo ""
ok "Build process finished!"
