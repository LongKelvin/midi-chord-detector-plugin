@echo off
REM One-click build script for MIDI Chord Detector
REM Simple batch file that calls the PowerShell script

echo.
echo ========================================
echo   MIDI Chord Detector - Build Script
echo ========================================
echo.

REM Check if PowerShell is available
powershell -Command "Write-Host 'PowerShell detected'" >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo Using Windows PowerShell...
    powershell -ExecutionPolicy Bypass -File "%~dp0build.ps1"
) else (
    pwsh -Command "Write-Host 'PowerShell detected'" >nul 2>nul
    if %ERRORLEVEL% EQU 0 (
        echo Using PowerShell Core...
        pwsh -ExecutionPolicy Bypass -File "%~dp0build.ps1"
    ) else (
        echo ERROR: PowerShell not found!
        echo Please install PowerShell to use this script.
        pause
        exit /b 1
    )
)

pause
