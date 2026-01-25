#!/usr/bin/env pwsh
<#
.SYNOPSIS
    One-click build script for JUCE MIDI Chord Detector Plugin

.DESCRIPTION
    This script automatically:
    - Checks for JUCE installation
    - Downloads JUCE if needed
    - Configures CMake
    - Builds the VST3 plugin
    - Handles common errors

.PARAMETER Clean
    Clean build directory before building

.PARAMETER Release
    Build in Release mode (default)

.PARAMETER Debug
    Build in Debug mode

.PARAMETER DownloadJuce
    Force download JUCE even if it exists

.EXAMPLE
    .\build.ps1
    .\build.ps1 -Clean
    .\build.ps1 -Debug
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
        [string]$Message,
        [string]$Color = "White"
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

# Check PowerShell version
if ($PSVersionTable.PSVersion.Major -lt 5) {
    Write-Error-Custom "PowerShell 5.0 or higher required. Current version: $($PSVersionTable.PSVersion)"
    exit 1
}
Write-Success "PowerShell $($PSVersionTable.PSVersion)"

# Check CMake
try {
    $cmakeVersion = cmake --version 2>&1 | Select-Object -First 1
    Write-Success "CMake found: $cmakeVersion"
} catch {
    Write-Error-Custom "CMake not found. Please install CMake from https://cmake.org/download/"
    Write-ColorOutput "After installation, restart PowerShell and run this script again." "Yellow"
    exit 1
}

# Check Visual Studio / MSBuild
try {
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vsWhere) {
        $vsPath = & $vsWhere -latest -property installationPath
        Write-Success "Visual Studio found: $vsPath"
    } else {
        Write-Warning-Custom "Visual Studio not found via vswhere, checking MSBuild..."
        $msbuild = Get-Command msbuild -ErrorAction SilentlyContinue
        if ($msbuild) {
            Write-Success "MSBuild found: $($msbuild.Source)"
        } else {
            Write-Error-Custom "Visual Studio or MSBuild not found"
            Write-ColorOutput "Please install Visual Studio 2019 or later with C++ tools" "Yellow"
            exit 1
        }
    }
} catch {
    Write-Warning-Custom "Could not verify Visual Studio installation"
}

# Check/Download JUCE
Write-Step "Checking JUCE Framework..."

# Check if JUCE exists in Resources folder
if (Test-Path $JUCE_DEFAULT_PATH) {
    $JUCE_PATH = $JUCE_DEFAULT_PATH
    Write-Success "JUCE found at: $JUCE_PATH"
} elseif ($DownloadJuce -or -not (Test-Path $JUCE_DEFAULT_PATH)) {
    Write-Step "JUCE not found. Downloading from GitHub..."
    
    # Check git
    try {
        $gitVersion = git --version 2>&1
        Write-Success "Git found: $gitVersion"
    } catch {
        Write-Error-Custom "Git not found. Please install Git from https://git-scm.com/"
        Write-ColorOutput "Alternatively, download JUCE manually to: $JUCE_DEFAULT_PATH" "Yellow"
        Write-ColorOutput "Download from: https://juce.com/get-juce/" "Yellow"
        exit 1
    }
    
    # Create Resources directory if needed
    if (-not (Test-Path $RESOURCES_DIR)) {
        New-Item -ItemType Directory -Path $RESOURCES_DIR -Force | Out-Null
        Write-Success "Created Resources directory"
    }
    
    # Clone or update JUCE
    if (Test-Path $JUCE_DEFAULT_PATH) {
        Write-ColorOutput "Updating existing JUCE repository..." "Yellow"
        Push-Location $JUCE_DEFAULT_PATH
        git pull
        Pop-Location
    } else {
        Write-ColorOutput "Cloning JUCE to Resources folder (this may take a few minutes)..." "Yellow"
        git clone --depth 1 --branch master $JUCE_GIT_URL $JUCE_DEFAULT_PATH
    }
    
    $JUCE_PATH = $JUCE_DEFAULT_PATH
    Write-Success "JUCE ready at: $JUCE_PATH"
} else {
    $JUCE_PATH = $JUCE_DEFAULT_PATH
}

# Verify JUCE installation
$juceCMake = Join-Path $JUCE_PATH "CMakeLists.txt"
if (-not (Test-Path $juceCMake)) {
    Write-Error-Custom "JUCE CMakeLists.txt not found at: $juceCMake"
    Write-ColorOutput "Please ensure JUCE is properly installed" "Yellow"
    exit 1
}
Write-Success "JUCE installation verified"

# Clean build directory if requested
if ($Clean -and (Test-Path $BUILD_DIR)) {
    Write-Step "Cleaning build directory..."
    Remove-Item -Recurse -Force $BUILD_DIR
    Write-Success "Build directory cleaned"
}

# Create build directory
Write-Step "Creating build directory..."
if (-not (Test-Path $BUILD_DIR)) {
    New-Item -ItemType Directory -Path $BUILD_DIR | Out-Null
}
Write-Success "Build directory ready"

# Configure CMake
Write-Step "Configuring CMake..."
Write-ColorOutput "JUCE Path: $JUCE_PATH" "Gray"
Write-ColorOutput "Build Config: $BuildConfig" "Gray"

Push-Location $BUILD_DIR

try {
    $cmakeArgs = @(
        "..",
        "-DJUCE_DIR=`"$JUCE_PATH`"",
        "-DCMAKE_BUILD_TYPE=$BuildConfig"
    )
    
    Write-ColorOutput "Running: cmake $($cmakeArgs -join ' ')" "Gray"
    & cmake @cmakeArgs
    
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed"
    }
    
    Write-Success "CMake configuration complete"
} catch {
    Write-Error-Custom "CMake configuration failed: $_"
    Pop-Location
    exit 1
}

# Build
Write-Step "Building plugin ($BuildConfig mode)..."
Write-ColorOutput "This may take a few minutes..." "Gray"

try {
    & cmake --build . --config $BuildConfig --parallel
    
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed"
    }
    
    Write-Success "Build complete!"
} catch {
    Write-Error-Custom "Build failed: $_"
    Pop-Location
    exit 1
}

Pop-Location

# Check output
Write-Step "Verifying build output..."

$vst3RelativePath = "MidiChordDetector_artefacts\$BuildConfig\VST3\MIDI Chord Detector.vst3"
$vst3Path = Join-Path $BUILD_DIR $vst3RelativePath
$systemVst3Path = Join-Path $env:CommonProgramFiles "VST3\MIDI Chord Detector.vst3"

if (Test-Path $vst3Path) {
    Write-Success "VST3 plugin built successfully!"
    Write-ColorOutput "Location: $vst3Path" "Gray"
    
    # Check if copied to system
    if (Test-Path $systemVst3Path) {
        Write-Success "Plugin installed to: $systemVst3Path"
    } else {
        Write-Warning-Custom "Plugin not automatically installed to system VST3 folder"
        Write-ColorOutput "Run as administrator to auto-install, or copy manually" "Yellow"
    }
} else {
    Write-Warning-Custom "VST3 plugin not found at expected location: $vst3Path"
}

# Summary
Write-ColorOutput "" "White"
Write-ColorOutput "===============================================================" "Green"
Write-ColorOutput "                    BUILD COMPLETE                             " "Green"
Write-ColorOutput "===============================================================" "Green"
Write-ColorOutput "" "White"
Write-ColorOutput "Build Configuration: $BuildConfig" "White"
Write-ColorOutput "JUCE Path: $JUCE_PATH" "White"
Write-ColorOutput "Build Output: $BUILD_DIR" "White"
Write-ColorOutput "" "White"
Write-ColorOutput "Next Steps:" "Yellow"
Write-ColorOutput "1. Launch Cubase" "White"
Write-ColorOutput "2. Open Studio -> VST Plug-in Manager" "White"
Write-ColorOutput "3. Click 'Update' to scan for new plugins" "White"
Write-ColorOutput "4. Find 'MIDI Chord Detector' in MIDI FX" "White"
Write-ColorOutput "5. Add to MIDI track and play chords!" "White"
Write-ColorOutput "" "White"
Write-ColorOutput "For troubleshooting, see BUILD-GUIDE.md" "Gray"
Write-ColorOutput "" "White"

# Open build directory
$openBuild = Read-Host "Open build directory? (Y/n)"
if ($openBuild -ne "n" -and $openBuild -ne "N") {
    Invoke-Item $BUILD_DIR
}

Write-ColorOutput "" "White"
Write-ColorOutput "Happy chord detecting!" "Cyan"
