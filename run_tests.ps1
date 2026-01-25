# Build script for chord detection tests
# Run from MidiChordDetector directory

param(
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "   Chord Detection Test Builder" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$testDir = Join-Path $scriptDir "Source\tests"
$buildDir = Join-Path $testDir "build"

# Clean if requested
if ($Clean -and (Test-Path $buildDir)) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $buildDir
}

# Create build directory
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

Push-Location $buildDir

try {
    Write-Host "==> Configuring CMake..." -ForegroundColor Green
    cmake .. -DCMAKE_BUILD_TYPE=Release
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed"
    }
    
    Write-Host ""
    Write-Host "==> Building tests..." -ForegroundColor Green
    cmake --build . --config Release
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed"
    }
    
    Write-Host ""
    Write-Host "==> Running tests..." -ForegroundColor Green
    Write-Host ""
    
    $testExe = ".\Release\ChordDetectionTests.exe"
    if (-not (Test-Path $testExe)) {
        $testExe = ".\ChordDetectionTests.exe"
    }
    
    if (Test-Path $testExe) {
        & $testExe
        $exitCode = $LASTEXITCODE
    } else {
        Write-Host "ERROR: Test executable not found!" -ForegroundColor Red
        $exitCode = 1
    }
    
    Write-Host ""
    if ($exitCode -eq 0) {
        Write-Host "ALL TESTS PASSED!" -ForegroundColor Green
    } else {
        Write-Host "SOME TESTS FAILED" -ForegroundColor Red
    }
    
    exit $exitCode
}
finally {
    Pop-Location
}
