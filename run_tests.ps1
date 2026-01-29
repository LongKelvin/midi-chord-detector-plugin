# =============================================================================
# Build script for chord detection tests - IMPROVED VERSION
# =============================================================================
# 
# Features:
# - Automatically generates C++ tests from chord_tests.json
# - Detects Python availability
# - Provides fallback to manual tests if Python unavailable
# - Clean build support
# - Better error reporting
#
# Usage:
#   .\build.ps1           # Normal build with auto-generation
#   .\build.ps1 -Clean    # Clean build
#   .\build.ps1 -Verbose  # Show detailed output
#
# =============================================================================

param(
    [switch]$Clean,
    [switch]$Verbose
)

$ErrorActionPreference = "Stop"

# =============================================================================
# Configuration
# =============================================================================

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$testDir = Join-Path $scriptDir "Source" | Join-Path -ChildPath "tests"
$buildDir = Join-Path $testDir "build"
$jsonFile = Join-Path $testDir "chord_tests.json"
$generatorScript = Join-Path $testDir "json_to_cpp_tests.py"

# =============================================================================
# Helper Functions
# =============================================================================

function Write-Header {
    param([string]$Text)
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "   $Text" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host ""
}

function Write-Step {
    param([string]$Text)
    Write-Host "==> $Text" -ForegroundColor Green
}

function Write-Info {
    param([string]$Text)
    Write-Host "    $Text" -ForegroundColor Gray
}

function Write-Warning {
    param([string]$Text)
    Write-Host "WARNING: $Text" -ForegroundColor Yellow
}

function Write-Error-Custom {
    param([string]$Text)
    Write-Host "ERROR: $Text" -ForegroundColor Red
}

function Get-PythonCommand {
    # Returns the best Python command to use, or $null if not found
    # Priority: py -3 (Windows Launcher) > python3 > C:\Python312\python.exe
    
    # Try py -3 (Python Launcher - most reliable on Windows)
    try {
        $result = py -3 --version 2>&1
        if ($result -match "Python 3") {
            return @("py", "-3")
        }
    }
    catch { }
    
    # Try python3
    try {
        $result = python3 --version 2>&1
        if ($result -match "Python 3") {
            return @("python3")
        }
    }
    catch { }
    
    # Try direct path to avoid Windows Store stub
    $directPython = "C:\Python312\python.exe"
    if (Test-Path $directPython) {
        try {
            $result = & $directPython --version 2>&1
            if ($result -match "Python 3") {
                return @($directPython)
            }
        }
        catch { }
    }
    
    return $null
}

# =============================================================================
# Main Script
# =============================================================================

Write-Header "Chord Detection Test Builder"

# Verify test directory exists
if (-not (Test-Path $testDir)) {
    Write-Error-Custom "Test directory not found: $testDir"
    Write-Info "Expected location: Source\tests"
    exit 1
}

Write-Info "Test directory: $testDir"

# Check Python availability
$pythonCmd = Get-PythonCommand
if ($pythonCmd) {
    $pythonVersion = & $pythonCmd[0] $pythonCmd[1..($pythonCmd.Length-1)] --version 2>&1
    Write-Info "Python detected: $pythonVersion"
    Write-Info "Using command: $($pythonCmd -join ' ')"
    Write-Info "JSON test generation ENABLED"
    $pythonAvailable = $true
} else {
    Write-Warning "Python 3 not found - JSON test generation DISABLED"
    Write-Info "Only manual tests will be available (20 tests vs 188 total)"
    Write-Info "To enable: Install Python 3 from python.org"
    $pythonAvailable = $false
}

# Check for required files
if (-not (Test-Path $jsonFile)) {
    Write-Warning "chord_tests.json not found - JSON tests unavailable"
    $pythonAvailable = $false
}

if (-not (Test-Path $generatorScript)) {
    Write-Warning "json_to_cpp_tests.py not found - JSON tests unavailable"
    $pythonAvailable = $false
}

# Clean if requested
if ($Clean -and (Test-Path $buildDir)) {
    Write-Step "Cleaning build directory..."
    Remove-Item -Recurse -Force $buildDir
    Write-Info "Build directory cleaned"
}

# Create build directory
if (-not (Test-Path $buildDir)) {
    Write-Step "Creating build directory..."
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

# Generate tests from JSON if Python available
if ($pythonAvailable) {
    Write-Step "Generating C++ tests from JSON..."
    
    $generatedHeader = Join-Path $buildDir "generated_tests_from_json.h"
    
    try {
        # Use the detected Python command
        $pythonExe = $pythonCmd[0]
        $pythonArgs = $pythonCmd[1..($pythonCmd.Length-1)]
        
        if ($pythonArgs) {
            $output = & $pythonExe $pythonArgs $generatorScript $jsonFile 2>&1
        } else {
            $output = & $pythonExe $generatorScript $jsonFile 2>&1
        }
        
        if ($LASTEXITCODE -ne 0) {
            throw "Python script failed with exit code $LASTEXITCODE. Output: $output"
        }
        
        $output | Out-File -FilePath $generatedHeader -Encoding utf8
        
        if (Test-Path $generatedHeader) {
            $lineCount = (Get-Content $generatedHeader).Count
            if ($lineCount -gt 10) {
                Write-Info "Generated header: $lineCount lines"
                Write-Info "Location: $generatedHeader"
            } else {
                throw "Generated header appears empty or incomplete ($lineCount lines)"
            }
        }
        else {
            throw "Generated header not created"
        }
    }
    catch {
        Write-Warning "Failed to generate tests from JSON: $_"
        Write-Info "Continuing with manual tests only"
        $pythonAvailable = $false
    }
}

Push-Location $buildDir

try {
    # Configure CMake
    Write-Step "Configuring CMake..."
    
    if ($Verbose) {
        cmake .. -DCMAKE_BUILD_TYPE=Release
    } else {
        cmake .. -DCMAKE_BUILD_TYPE=Release 2>&1 | Out-Null
    }
    
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed"
    }
    
    Write-Info "Configuration complete"
    
    # Build
    Write-Step "Building tests..."
    
    if ($Verbose) {
        cmake --build . --config Release
    } else {
        cmake --build . --config Release 2>&1 | Out-Null
    }
    
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed"
    }
    
    Write-Info "Build complete"
    
    # Find test executable
    Write-Step "Running tests..."
    Write-Host ""
    
    $testExe = $null
    $possiblePaths = @(
        ".\Release\ChordDetectionTests.exe",
        ".\ChordDetectionTests.exe",
        "Release\ChordDetectionTests.exe",
        "ChordDetectionTests.exe"
    )
    
    foreach ($path in $possiblePaths) {
        if (Test-Path $path) {
            $testExe = $path
            break
        }
    }
    
    if ($testExe) {
        Write-Info "Executable: $testExe"
        Write-Host ""
        
        # Run tests
        & $testExe
        $exitCode = $LASTEXITCODE
        
        Write-Host ""
        if ($exitCode -eq 0) {
            Write-Host "========================================" -ForegroundColor Green
            Write-Host "   ALL TESTS PASSED!" -ForegroundColor Green
            Write-Host "========================================" -ForegroundColor Green
        } else {
            Write-Host "========================================" -ForegroundColor Red
            Write-Host "   SOME TESTS FAILED" -ForegroundColor Red
            Write-Host "========================================" -ForegroundColor Red
        }
    } else {
        Write-Error-Custom "Test executable not found in any expected location!"
        Write-Info "Searched paths:"
        foreach ($path in $possiblePaths) {
            Write-Info "  - $path"
        }
        $exitCode = 1
    }
    
    # Show summary
    Write-Host ""
    Write-Host "Build Summary:" -ForegroundColor Cyan
    Write-Host "  Python:          $(if ($pythonAvailable) { 'Available' } else { 'Not Available' })"
    Write-Host "  JSON Tests:      $(if ($pythonAvailable) { 'Enabled (188 total tests)' } else { 'Disabled (20 tests only)' })"
    Write-Host "  Test Result:     $(if ($exitCode -eq 0) { 'PASS' } else { 'FAIL' })"
    
    exit $exitCode
}
catch {
    Write-Error-Custom $_
    Write-Host ""
    Write-Host "Build failed. Check errors above." -ForegroundColor Red
    exit 1
}
finally {
    Pop-Location
}
