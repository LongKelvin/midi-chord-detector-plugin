#!/usr/bin/env bash
#
# setup_fonts.sh — Download the Bravura SMuFL music font required by NotationComponent.
#
# Bravura is the reference SMuFL font by Steinberg (SIL Open Font License).
# It must be present at fonts/Bravura.otf before the first CMake configure.
#
# This is the macOS/Linux counterpart of setup_fonts.ps1 and mirrors the same
# download strategy. It is called automatically by build.sh if the font is missing.
#
# Usage:
#   ./setup_fonts.sh
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"
FONT_DIR="$PROJECT_DIR/fonts"
FONT_PATH="$FONT_DIR/Bravura.otf"

DIRECT_URL="https://github.com/steinbergmedia/bravura/raw/master/redist/Bravura.otf"

# ── Colored output helpers ──────────────────────────────────────────────────
step()  { printf '\n\033[36m==> %s\033[0m\n' "$1"; }
ok()    { printf '\033[32m[OK] %s\033[0m\n'   "$1"; }
warn()  { printf '\033[33m[WARN] %s\033[0m\n' "$1"; }
fail()  { printf '\033[31m[ERROR] %s\033[0m\n' "$1"; }

echo "============================================================="
echo "  Bravura SMuFL Font Setup"
echo "============================================================="

# ── Already present ─────────────────────────────────────────────────────────
if [[ -f "$FONT_PATH" ]]; then
    ok "Bravura.otf already at: $FONT_PATH"
    exit 0
fi

step "Bravura.otf not found — downloading..."
mkdir -p "$FONT_DIR"

# ── Attempt 1: direct raw download from GitHub master ─────────────────────────
echo "  Trying: $DIRECT_URL"
if curl -fSL "$DIRECT_URL" -o "$FONT_PATH" 2>/dev/null && [[ -s "$FONT_PATH" ]]; then
    ok "Bravura.otf downloaded to: $FONT_PATH"
    exit 0
fi
warn "Direct download failed. Trying releases archive..."
rm -f "$FONT_PATH"

# ── Attempt 2: latest GitHub release archive ──────────────────────────────────
step "Fetching latest release info from GitHub API..."
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
            rm -rf "$TMP_DIR"
            exit 0
        fi
    fi
    rm -rf "$TMP_DIR"
fi

# ── Manual fallback ───────────────────────────────────────────────────────────
fail "Automated download failed."
echo ""
echo "  Manual steps:"
echo "    1. Go to: https://github.com/steinbergmedia/bravura/releases"
echo "    2. Download the latest release zip."
echo "    3. Copy 'redist/Bravura.otf' from the zip."
echo "    4. Place it at: $FONT_PATH"
exit 1
