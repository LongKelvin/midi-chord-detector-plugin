#!/usr/bin/env bash
#
# build.sh — One-click, self-bootstrapping build for the MIDI Chord Detector
#            on macOS / Linux.
#
# Counterpart of build.ps1. Auto-detects the OS, fetches JUCE and the Bravura
# font on first run (no separate setup step), configures with CMake, builds the
# VST3 plugin and the standalone app, and (via COPY_PLUGIN_AFTER_BUILD) installs
# the VST3 to the user plugin folder.
#
# Usage:
#   ./build.sh                 # Release build (bootstraps deps if missing)
#   ./build.sh --debug         # Debug build
#   ./build.sh --clean         # wipe build/ first
#   ./build.sh --universal     # macOS universal binary (arm64 + x86_64)
#
# Environment:
#   JUCE_TAG=8.0.4 ./build.sh  # pin a different JUCE tag (default: 8.0.12)
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"
BUILD_DIR="$PROJECT_DIR/build"
RESOURCES_DIR="$PROJECT_DIR/Resources"
JUCE_DIR="$RESOURCES_DIR/JUCE"
JUCE_TAG="${JUCE_TAG:-8.0.12}"
JUCE_GIT_URL="https://github.com/juce-framework/JUCE.git"

FONT_DIR="$PROJECT_DIR/fonts"
FONT_PATH="$FONT_DIR/Bravura.otf"
FONT_DIRECT_URL="https://github.com/steinbergmedia/bravura/raw/master/redist/Bravura.otf"

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

# ── OS detection ──────────────────────────────────────────────────────────────
case "$(uname -s)" in
    Darwin) OS_NAME="macOS" ;;
    Linux)  OS_NAME="Linux" ;;
    *)      OS_NAME="$(uname -s)" ;;
esac

# ── Colored output helpers ────────────────────────────────────────────────────
step()  { printf '\n\033[36m==> %s\033[0m\n' "$1"; }
ok()    { printf '\033[32m[OK] %s\033[0m\n'   "$1"; }
warn()  { printf '\033[33m[WARN] %s\033[0m\n' "$1"; }
fail()  { printf '\033[31m[ERROR] %s\033[0m\n' "$1"; }

echo "============================================================="
echo "  JUCE MIDI Chord Detector — Build Script ($BUILD_CONFIG)"
echo "  $OS_NAME"
echo "============================================================="

# ── Prerequisites ─────────────────────────────────────────────────────────────
step "Checking prerequisites..."
missing=0
for tool in cmake git curl; do
    if command -v "$tool" >/dev/null 2>&1; then
        ok "$tool found: $(command -v "$tool")"
    else
        fail "$tool not found."
        missing=1
    fi
done

if [[ "$OS_NAME" == "macOS" ]]; then
    if xcode-select -p >/dev/null 2>&1; then
        ok "Xcode command line tools: $(xcode-select -p)"
    else
        fail "Xcode command line tools missing. Run: xcode-select --install"
        missing=1
    fi
fi

if [[ "$missing" -ne 0 ]]; then
    echo ""
    fail "Install the missing tools and re-run. On macOS: brew install cmake git"
    exit 1
fi
ok "CMake: $(cmake --version | head -1)"

# ── JUCE (auto-fetch if missing) ──────────────────────────────────────────────
step "Checking JUCE framework..."
if [[ -f "$JUCE_DIR/CMakeLists.txt" ]]; then
    ok "JUCE already present at: $JUCE_DIR"
else
    warn "JUCE not found at $JUCE_DIR — cloning $JUCE_TAG (shallow)."
    mkdir -p "$RESOURCES_DIR"
    git clone --depth 1 --branch "$JUCE_TAG" "$JUCE_GIT_URL" "$JUCE_DIR"
    ok "JUCE $JUCE_TAG cloned."
fi

# ── Bravura font (auto-fetch if missing) ──────────────────────────────────────
step "Checking Bravura SMuFL font..."
if [[ -f "$FONT_PATH" ]]; then
    ok "Bravura.otf already present at: $FONT_PATH"
else
    warn "Bravura.otf missing — downloading..."
    mkdir -p "$FONT_DIR"

    # Attempt 1: direct raw download from GitHub master.
    echo "  Trying: $FONT_DIRECT_URL"
    if curl -fSL "$FONT_DIRECT_URL" -o "$FONT_PATH" 2>/dev/null && [[ -s "$FONT_PATH" ]]; then
        ok "Bravura.otf downloaded to: $FONT_PATH"
    else
        warn "Direct download failed. Trying releases archive..."
        rm -f "$FONT_PATH"

        # Attempt 2: latest GitHub release archive.
        TAG="$(curl -fsSL https://api.github.com/repos/steinbergmedia/bravura/releases/latest \
                | sed -n 's/.*"tag_name" *: *"\([^"]*\)".*/\1/p' | head -1 || true)"
        if [[ -n "$TAG" ]]; then
            echo "  Latest release: $TAG"
            ARCHIVE_URL="https://github.com/steinbergmedia/bravura/archive/refs/tags/$TAG.zip"
            TMP_DIR="$(mktemp -d)"
            TMP_ZIP="$TMP_DIR/bravura.zip"
            echo "  Downloading archive: $ARCHIVE_URL"
            if curl -fSL "$ARCHIVE_URL" -o "$TMP_ZIP" && unzip -q "$TMP_ZIP" -d "$TMP_DIR"; then
                EXTRACTED="$(find "$TMP_DIR" -name 'Bravura.otf' -print -quit || true)"
                if [[ -n "$EXTRACTED" ]]; then
                    cp "$EXTRACTED" "$FONT_PATH"
                    ok "Bravura.otf extracted from release $TAG and placed at: $FONT_PATH"
                fi
            fi
            rm -rf "$TMP_DIR"
        fi

        # Manual fallback.
        if [[ ! -f "$FONT_PATH" ]]; then
            fail "Automated download failed."
            echo ""
            echo "  Manual steps:"
            echo "    1. Go to: https://github.com/steinbergmedia/bravura/releases"
            echo "    2. Download the latest release zip."
            echo "    3. Copy 'redist/Bravura.otf' from the zip."
            echo "    4. Place it at: $FONT_PATH"
            exit 1
        fi
    fi
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
if [[ "$UNIVERSAL" -eq 1 && "$OS_NAME" == "macOS" ]]; then
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

if [[ "$OS_NAME" == "macOS" ]]; then
    INSTALLED="$HOME/Library/Audio/Plug-Ins/VST3/MIDI Chord Detector.vst3"
    if [[ -d "$INSTALLED" ]]; then
        ok "Installed to: $INSTALLED"
    else
        warn "VST3 not found in user plugin folder; copy it manually if needed."
    fi
fi

echo ""
ok "Build process finished!"
