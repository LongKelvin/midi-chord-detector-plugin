#!/usr/bin/env bash
#
# setup.sh — One-time environment setup for building on macOS / Linux.
#
# Checks prerequisites, fetches JUCE into Resources/JUCE, and downloads the
# Bravura font. Safe to re-run: existing dependencies are left untouched.
#
# Usage:
#   ./setup.sh                 # clone JUCE 8.0.12 into Resources/JUCE
#   JUCE_TAG=8.0.4 ./setup.sh  # pin a different JUCE tag
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"
RESOURCES_DIR="$PROJECT_DIR/Resources"
JUCE_DIR="$RESOURCES_DIR/JUCE"
JUCE_TAG="${JUCE_TAG:-8.0.12}"
JUCE_GIT_URL="https://github.com/juce-framework/JUCE.git"

step()  { printf '\n\033[36m==> %s\033[0m\n' "$1"; }
ok()    { printf '\033[32m[OK] %s\033[0m\n'   "$1"; }
warn()  { printf '\033[33m[WARN] %s\033[0m\n' "$1"; }
fail()  { printf '\033[31m[ERROR] %s\033[0m\n' "$1"; }

echo "============================================================="
echo "  MIDI Chord Detector — Environment Setup (macOS / Linux)"
echo "============================================================="

# ── Prerequisite checks ───────────────────────────────────────────────────────
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

if [[ "$(uname)" == "Darwin" ]]; then
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

# ── JUCE ──────────────────────────────────────────────────────────────────────
step "Checking JUCE framework..."
if [[ -f "$JUCE_DIR/CMakeLists.txt" ]]; then
    ok "JUCE already present at: $JUCE_DIR"
else
    step "Cloning JUCE $JUCE_TAG (shallow) into $JUCE_DIR ..."
    mkdir -p "$RESOURCES_DIR"
    git clone --depth 1 --branch "$JUCE_TAG" "$JUCE_GIT_URL" "$JUCE_DIR"
    ok "JUCE $JUCE_TAG cloned."
fi

# ── Bravura font ────────────────────────────────────────────────────────────────
step "Checking Bravura SMuFL font..."
if [[ -f "$PROJECT_DIR/fonts/Bravura.otf" ]]; then
    ok "Bravura.otf already present."
else
    "$SCRIPT_DIR/setup_fonts.sh"
fi

echo ""
ok "Setup complete. Next: ./build.sh"
