#!/usr/bin/env pwsh
<#
.SYNOPSIS
    One-click build script for JUCE MIDI Chord Detector Plugin
#>

param(
    [switch]$Clean,
    [switch]$Release,
    [switch]$Debug,
    [switch]$DownloadJuce
)

# Configuration
$ErrorActionPreference = "Stop"
$PROJECT_DIR = $PSScriptRoot
$RESOURCES_DIR = Join-Path $PROJECT_DIR "Resources"
$JUCE_DEFAULT_PATH = Join-Path $RESOURCES_DIR "JUCE"
$JUCE_GIT_URL = "https://github.com/juce-framework/JUCE.git"
$BUILD_DIR = Join-Path $PROJECT_DIR "build"

# Determine build configuration
$BuildConfig = if ($Debug) { "Debug" } else { "Release" }

# Colors for output
function Write-ColorOutput {
    param(
        [Parameter(Mandatory=$true)]
        [string]$Message,
        [System.ConsoleColor]$Color = "White"
    )
    Write-Host $Message -ForegroundColor $Color
}

function Write-Step {
    param([string]$Message)
    Write-ColorOutput "`n==> $Message" "Cyan"
}

function Write-Success {
    param([string]$Message)
    Write-ColorOutput "[OK] $Message" "Green"
}

function Write-Error-Custom {
    param([string]$Message)
    Write-ColorOutput "[ERROR] $Message" "Red"
}

function Write-Warning-Custom {
    param([string]$Message)
    Write-ColorOutput "[WARNING] $Message" "Yellow"
}

# Banner
Clear-Host
Write-ColorOutput "===============================================================" "Cyan"
Write-ColorOutput "       JUCE MIDI Chord Detector - Build Script                " "Cyan"
Write-ColorOutput "       One-Click Build for Windows                            " "Cyan"
Write-ColorOutput "===============================================================" "Cyan"

# Check prerequisites
Write-Step "Checking prerequisites..."

if ($PSVersionTable.PSVersion.Major -lt 5) {
    Write-Error-Custom "PowerShell 5.0 or higher required."
    exit 1
}
Write-Success "PowerShell $($PSVersionTable.PSVersion)"

try {
    $cmakeVersion = cmake --version 2>&1 | Select-Object -First 1
    Write-Success "CMake found: $cmakeVersion"
} catch {
    Write-Error-Custom "CMake not found. Please install CMake from https://cmake.org/download/"
    exit 1
}

# Check Visual Studio
try {
    $vsWhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vsWhere) {
        $vsPath = & $vsWhere -latest -property installationPath
        Write-Success "Visual Studio found: $vsPath"
    } else {
        $msbuild = Get-Command msbuild -ErrorAction SilentlyContinue
        if ($msbuild) {
            Write-Success "MSBuild found: $($msbuild.Source)"
        } else {
            Write-Warning-Custom "Visual Studio or MSBuild not found in standard paths."
        }
    }
} catch {
    Write-Warning-Custom "Could not verify Visual Studio installation"
}

# Check/Download JUCE
Write-Step "Checking JUCE Framework..."
if (Test-Path $JUCE_DEFAULT_PATH) {
    $JUCE_PATH = $JUCE_DEFAULT_PATH
    Write-Success "JUCE found at: $JUCE_PATH"
} elseif ($DownloadJuce -or -not (Test-Path $JUCE_DEFAULT_PATH)) {
    Write-Step "JUCE not found. Downloading..."
    try {
        git --version > $null
    } catch {
        Write-Error-Custom "Git not found. Install Git or manually place JUCE in $JUCE_DEFAULT_PATH"
        exit 1
    }
    
    if (-not (Test-Path $RESOURCES_DIR)) {
        New-Item -ItemType Directory -Path $RESOURCES_DIR -Force | Out-Null
    }
    
    Write-ColorOutput "Cloning JUCE (master branch)..." "Yellow"
    git clone --depth 1 --branch master $JUCE_GIT_URL $JUCE_DEFAULT_PATH
    $JUCE_PATH = $JUCE_DEFAULT_PATH
}

# Verify JUCE
if (-not (Test-Path (Join-Path $JUCE_PATH "CMakeLists.txt"))) {
    Write-Error-Custom "JUCE CMakeLists.txt not found at $JUCE_PATH"
    exit 1
}

# Check/Download Bravura SMuFL font
Write-Step "Checking Bravura SMuFL font..."
$FONT_PATH = Join-Path $PROJECT_DIR "fonts\Bravura.otf"
if (Test-Path $FONT_PATH) {
    Write-Success "Bravura.otf found"
} else {
    Write-ColorOutput "Bravura.otf not found. Running setup_fonts.ps1..." "Yellow"
    $setupScript = Join-Path $PROJECT_DIR "setup_fonts.ps1"
    & powershell -ExecutionPolicy Bypass -File $setupScript
    if ($LASTEXITCODE -ne 0) {
        Write-Error-Custom "Font setup failed. Cannot continue without Bravura.otf."
        exit 1
    }
    Write-Success "Bravura.otf ready"
}

# Clean
if ($Clean -and (Test-Path $BUILD_DIR)) {
    Write-Step "Cleaning build directory..."
    Remove-Item -Recurse -Force $BUILD_DIR
    Write-Success "Cleaned"
}

# Create Build Dir
if (-not (Test-Path $BUILD_DIR)) {
    New-Item -ItemType Directory -Path $BUILD_DIR | Out-Null
}

# VST3 Path Logic - Fixed for PS 5.1 expansion and Path Separators
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]"Administrator")
if ($isAdmin) {
    $vst3CopyDir = Join-Path ${env:CommonProgramFiles} "VST3"
    Write-ColorOutput "[INFO] Admin Mode: Installing to System VST3 folder" "Cyan"
} else {
    $vst3CopyDir = Join-Path ${env:AppData} "VST3"
    Write-ColorOutput "[INFO] User Mode: Installing to AppData VST3 folder" "Yellow"
}

# Ensure destination exists
if (-not (Test-Path $vst3CopyDir)) {
    New-Item -ItemType Directory -Path $vst3CopyDir -Force | Out-Null
}

# Configure
Write-Step "Configuring CMake..."
Push-Location $BUILD_DIR
try {
    # Use backslashes for Windows paths to avoid CMake parsing issues
    $normJucePath = $JUCE_PATH.Replace("/", "\")
    $normVstPath = $vst3CopyDir.Replace("/", "\")

    & cmake .. `
        "-DJUCE_DIR=$normJucePath" `
        "-DCMAKE_BUILD_TYPE=$BuildConfig" `
        "-DJUCE_VST3_COPY_DIR_OVERRIDE=$normVstPath"
    
    if ($LASTEXITCODE -ne 0) { throw "CMake config failed" }
    Write-Success "Configuration complete"
} catch {
    Write-Error-Custom "Config failed: $_"
    Pop-Location
    exit 1
}

# Build
Write-Step "Building plugin ($BuildConfig)..."
try {
    & cmake --build . --config $BuildConfig --parallel
    if ($LASTEXITCODE -ne 0) { throw "Build execution failed" }
    Write-Success "Build successful"
} catch {
    Write-Error-Custom "Build failed: $_"
    Pop-Location
    exit 1
}
Pop-Location

# Verify Result
Write-Step "Verifying build output..."
# Note: JUCE adds "_artefacts" to the project name folder usually
$vst3RelativePath = "MidiChordDetector_artefacts\$BuildConfig\VST3\MIDI Chord Detector.vst3"
$vst3Path = Join-Path $BUILD_DIR $vst3RelativePath

if (Test-Path $vst3Path) {
    Write-Success "VST3 found at: $vst3Path"
    $targetFile = Join-Path $vst3CopyDir "MIDI Chord Detector.vst3"
    if (Test-Path $targetFile) {
        Write-Success "Plugin successfully copied to: $targetFile"
    } else {
        Write-Warning-Custom "Plugin not found in VST3 folder. You may need to copy it manually."
    }
} else {
    Write-Warning-Custom "Could not find built .vst3 file at expected location."
}

Write-ColorOutput "`nBuild Process Finished!" "Green"
$openBuild = Read-Host "Open build directory? (Y/n)"
if ($openBuild -ne "n") { Invoke-Item $BUILD_DIR }