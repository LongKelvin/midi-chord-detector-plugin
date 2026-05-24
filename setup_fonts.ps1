#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Downloads the Bravura SMuFL music font required by NotationComponent.

.DESCRIPTION
    Bravura is the reference SMuFL font by Steinberg (SIL Open Font License).
    It must be present at fonts/Bravura.otf before the first CMake configure.

    This script mirrors the JUCE auto-download pattern used in build.ps1.
    It is called automatically by build.ps1 if the font is missing.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File setup_fonts.ps1
#>

$ErrorActionPreference = "Stop"

# Resolve script directory reliably regardless of how the script is invoked.
# $PSScriptRoot is empty when called from some child PowerShell processes.
$_scriptDir = if ($PSScriptRoot) {
    $PSScriptRoot
} elseif ($MyInvocation.MyCommand.Path) {
    Split-Path -Parent $MyInvocation.MyCommand.Path
} else {
    $PWD.Path
}

$PROJECT_DIR = $_scriptDir
$FONT_DIR    = Join-Path $PROJECT_DIR "fonts"
$FONT_PATH   = Join-Path $FONT_DIR "Bravura.otf"

# ── Helpers ────────────────────────────────────────────────────────────────────
function Write-Step    { param([string]$m) Write-Host "`n==> $m" -ForegroundColor Cyan }
function Write-OK      { param([string]$m) Write-Host "[OK] $m"  -ForegroundColor Green }
function Write-Warn    { param([string]$m) Write-Host "[WARN] $m" -ForegroundColor Yellow }
function Write-Fail    { param([string]$m) Write-Host "[ERROR] $m" -ForegroundColor Red }

Write-Host "=============================================================" -ForegroundColor Cyan
Write-Host "  Bravura SMuFL Font Setup                                   " -ForegroundColor Cyan
Write-Host "=============================================================" -ForegroundColor Cyan

# ── Already present ─────────────────────────────────────────────────────────
if (Test-Path $FONT_PATH) {
    Write-OK "Bravura.otf already at: $FONT_PATH"
    exit 0
}

Write-Step "Bravura.otf not found — downloading..."

# Create fonts/ directory
if (-not (Test-Path $FONT_DIR)) {
    New-Item -ItemType Directory -Path $FONT_DIR -Force | Out-Null
}

# ── Attempt 1: direct raw download from GitHub master ─────────────────────
$DIRECT_URL = "https://github.com/steinbergmedia/bravura/raw/master/redist/Bravura.otf"
Write-Host "  Trying: $DIRECT_URL" -ForegroundColor DarkGray
try {
    Invoke-WebRequest -Uri $DIRECT_URL -OutFile $FONT_PATH -UseBasicParsing
    Write-OK "Bravura.otf downloaded to: $FONT_PATH"
    exit 0
} catch {
    Write-Warn "Direct download failed ($_). Trying releases archive..."
}

# ── Attempt 2: latest GitHub release archive ──────────────────────────────
try {
    Write-Step "Fetching latest release info from GitHub API..."
    $apiUrl  = "https://api.github.com/repos/steinbergmedia/bravura/releases/latest"
    $release = Invoke-RestMethod -Uri $apiUrl -UseBasicParsing
    $tag     = $release.tag_name
    Write-Host "  Latest release: $tag" -ForegroundColor DarkGray

    $archiveUrl = "https://github.com/steinbergmedia/bravura/archive/refs/tags/$tag.zip"
    $tmpZip     = Join-Path $env:TEMP "bravura_dl.zip"
    $tmpDir     = Join-Path $env:TEMP "bravura_dl"

    Write-Host "  Downloading archive: $archiveUrl" -ForegroundColor DarkGray
    Invoke-WebRequest -Uri $archiveUrl -OutFile $tmpZip -UseBasicParsing

    Write-Host "  Extracting..." -ForegroundColor DarkGray
    if (Test-Path $tmpDir) { Remove-Item $tmpDir -Recurse -Force }
    Expand-Archive -Path $tmpZip -DestinationPath $tmpDir -Force

    # Find the OTF inside the extracted tree
    $extracted = Get-ChildItem -Path $tmpDir -Filter "Bravura.otf" -Recurse |
                 Select-Object -First 1

    if (-not $extracted) {
        throw "Bravura.otf not found inside the downloaded archive."
    }

    # Ensure destination directory exists (belt-and-suspenders)
    $null = New-Item -ItemType Directory -Path $FONT_DIR -Force -ErrorAction SilentlyContinue
    Copy-Item $extracted.FullName -Destination $FONT_PATH -Force
    Write-OK "Bravura.otf extracted from release $tag and placed at: $FONT_PATH"

    # Cleanup temp files
    Remove-Item $tmpZip -Force -ErrorAction SilentlyContinue
    Remove-Item $tmpDir -Recurse -Force -ErrorAction SilentlyContinue

    exit 0

} catch {
    Write-Fail "Automated download failed: $_"
    Write-Host ""
    Write-Host "  Manual steps:" -ForegroundColor Yellow
    Write-Host "    1. Go to: https://github.com/steinbergmedia/bravura/releases" -ForegroundColor Yellow
    Write-Host "    2. Download the latest release zip." -ForegroundColor Yellow
    Write-Host "    3. Copy 'redist/Bravura.otf' from the zip." -ForegroundColor Yellow
    Write-Host "    4. Place it at: $FONT_PATH" -ForegroundColor Yellow
    exit 1
}
