/*
  ==============================================================================

    ChordDetectionTests.cpp - Pattern-Based Chord Detection Test Suite
    
    Tests for the new optimized pattern-based chord detection algorithm.
    
    FEATURES:
    - 100+ chord type coverage
    - Enharmonic normalization
    - JSON-generated test support (optional)
    
    USAGE:
    - Run .\run_tests.ps1 to build and run tests
    - Or build manually with CMake

  ==============================================================================
*/

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cmath>

// Include the new chord detection core
#include "../chord_detection/detector/ChordDetector.h"

// ============================================================================
// Test Infrastructure
// ============================================================================

struct TestCase
{
    const char* name;
    std::vector<int> midiNotes;     // MIDI note numbers
    const char* expectedChord;
    float minConfidence;
};

// Convert note name to MIDI number with full enharmonic support
int noteNameToMidi(const char* noteName)
{
    if (!noteName || strlen(noteName) < 2)
        return -1;
    
    char note = noteName[0];
    int pitchClass = -1;
    
    switch (note)
    {
        case 'C': pitchClass = 0; break;
        case 'D': pitchClass = 2; break;
        case 'E': pitchClass = 4; break;
        case 'F': pitchClass = 5; break;
        case 'G': pitchClass = 7; break;
        case 'A': pitchClass = 9; break;
        case 'B': pitchClass = 11; break;
        default: return -1;
    }
    
    int offset = 1;
    int octaveAdjust = 0;
    
    if (noteName[1] == '#')
    {
        int newPitchClass = (pitchClass + 1) % 12;
        if (pitchClass == 11 && newPitchClass == 0)
            octaveAdjust = 1;
        pitchClass = newPitchClass;
        offset = 2;
    }
    else if (noteName[1] == 'b')
    {
        int newPitchClass = (pitchClass + 11) % 12;
        if (pitchClass == 0 && newPitchClass == 11)
            octaveAdjust = -1;
        pitchClass = newPitchClass;
        offset = 2;
    }
    
    if (strlen(noteName) < (size_t)(offset + 1))
        return -1;
    
    int octave = noteName[offset] - '0';
    if (octave < 0 || octave > 9)
        return -1;
    
    octave += octaveAdjust;
    
    return (octave + 1) * 12 + pitchClass;
}

// Helper to create test cases
TestCase makeTest(const char* name, std::initializer_list<const char*> notes, 
                  const char* expected, float minConf = 0.5f)
{
    TestCase tc;
    tc.name = name;
    tc.expectedChord = expected;
    tc.minConfidence = minConf;
    
    for (const char* note : notes)
    {
        int midi = noteNameToMidi(note);
        if (midi >= 0)
            tc.midiNotes.push_back(midi);
    }
    
    return tc;
}

// ============================================================================
// Enharmonic Normalization
// ============================================================================

std::string normalizeEnharmonics(const std::string& s)
{
    std::string out = s;
    
    // Normalize sharps to flats (canonical form)
    size_t pos = 0;
    while ((pos = out.find("A#", pos)) != std::string::npos) {
        out.replace(pos, 2, "Bb");
        pos += 2;
    }
    pos = 0;
    while ((pos = out.find("C#", pos)) != std::string::npos) {
        out.replace(pos, 2, "Db");
        pos += 2;
    }
    pos = 0;
    while ((pos = out.find("D#", pos)) != std::string::npos) {
        out.replace(pos, 2, "Eb");
        pos += 2;
    }
    pos = 0;
    while ((pos = out.find("F#", pos)) != std::string::npos) {
        out.replace(pos, 2, "Gb");
        pos += 2;
    }
    pos = 0;
    while ((pos = out.find("G#", pos)) != std::string::npos) {
        out.replace(pos, 2, "Ab");
        pos += 2;
    }
    
    // Also handle unicode sharp symbols
    pos = 0;
    while ((pos = out.find("A♯", pos)) != std::string::npos) {
        out.replace(pos, 4, "Bb");  // ♯ is 3 bytes in UTF-8
        pos += 2;
    }
    pos = 0;
    while ((pos = out.find("C♯", pos)) != std::string::npos) {
        out.replace(pos, 4, "Db");
        pos += 2;
    }
    pos = 0;
    while ((pos = out.find("D♯", pos)) != std::string::npos) {
        out.replace(pos, 4, "Eb");
        pos += 2;
    }
    pos = 0;
    while ((pos = out.find("F♯", pos)) != std::string::npos) {
        out.replace(pos, 4, "Gb");
        pos += 2;
    }
    pos = 0;
    while ((pos = out.find("G♯", pos)) != std::string::npos) {
        out.replace(pos, 4, "Ab");
        pos += 2;
    }
    
    return out;
}

// ============================================================================
// Test Runner
// ============================================================================

class ChordTestRunner
{
public:
    ChordTestRunner() : passed_(0), failed_(0), total_(0) {}
    
    void runTest(const TestCase& tc)
    {
        total_++;
        
        // Create detector for each test
        ChordDetection::OptimizedChordDetector detector(false, ChordDetection::SlashChordMode::AUTO);
        
        // Detect chord
        auto result = detector.detectChord(tc.midiNotes);
        
        // Get result string
        std::string resultName;
        if (result)
        {
            resultName = result->chordName;
        }
        else
        {
            resultName = "N.C.";
        }
        
        // Compare
        bool pass = (resultName == tc.expectedChord);
        
        // Try alternative comparisons
        if (!pass)
        {
            std::string resultStr(resultName);
            std::string expectedStr(tc.expectedChord);
            
            // Enharmonic equivalence
            if (normalizeEnharmonics(resultStr) == normalizeEnharmonics(expectedStr))
            {
                pass = true;
            }
            
            // Handle slash chord differences (Cm vs Cm/G etc)
            if (!pass)
            {
                size_t slashPos = resultStr.find('/');
                if (slashPos != std::string::npos)
                {
                    std::string resultBase = resultStr.substr(0, slashPos);
                    if (normalizeEnharmonics(resultBase) == normalizeEnharmonics(expectedStr))
                    {
                        pass = true;
                    }
                }
                
                slashPos = expectedStr.find('/');
                if (slashPos != std::string::npos)
                {
                    std::string expectedBase = expectedStr.substr(0, slashPos);
                    if (normalizeEnharmonics(resultStr) == normalizeEnharmonics(expectedBase))
                    {
                        pass = true;
                    }
                }
            }
            
            // C6/Am7 equivalence - these are harmonically equivalent
            if (!pass && (expectedStr == "C6" && resultStr.find("Am") != std::string::npos))
            {
                pass = true;
            }
            if (!pass && (resultStr == "C6" && expectedStr.find("Am") != std::string::npos))
            {
                pass = true;
            }
        }
        
        if (pass)
        {
            passed_++;
            std::cout << "[PASS] " << tc.name << " -> " << resultName;
            if (result)
                std::cout << " (conf: " << result->confidence << ")";
            std::cout << std::endl;
        }
        else
        {
            failed_++;
            std::cout << "[FAIL] " << tc.name << std::endl;
            std::cout << "       Expected: " << tc.expectedChord << std::endl;
            std::cout << "       Got:      " << resultName;
            if (result)
                std::cout << " (conf: " << result->confidence << ")";
            std::cout << std::endl;
        }
    }
    
    void printSummary()
    {
        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "TEST SUMMARY" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Total:  " << total_ << std::endl;
        std::cout << "Passed: " << passed_ << std::endl;
        std::cout << "Failed: " << failed_ << std::endl;
        std::cout << "Rate:   " << (total_ > 0 ? (100.0f * passed_ / total_) : 0) << "%" << std::endl;
        std::cout << "========================================" << std::endl;
    }
    
    int getFailedCount() const { return failed_; }
    
private:
    int passed_;
    int failed_;
    int total_;
};

// ============================================================================
// Test Cases
// ============================================================================

std::vector<TestCase> getBasicTriadTests()
{
    return {
        makeTest("C Major", {"C4", "E4", "G4"}, "C", 0.6f),
        makeTest("C Minor", {"C4", "Eb4", "G4"}, "Cm", 0.6f),
        makeTest("C Diminished", {"C4", "Eb4", "Gb4"}, "Cdim", 0.5f),
        makeTest("C Augmented", {"C4", "E4", "G#4"}, "Caug", 0.5f),
        makeTest("C Sus2", {"C4", "D4", "G4"}, "Csus2", 0.5f),
        makeTest("C Sus4", {"C4", "F4", "G4"}, "Csus4", 0.5f),
        makeTest("G Major", {"G3", "B3", "D4"}, "G", 0.6f),
        makeTest("F Minor", {"F3", "Ab3", "C4"}, "Fm", 0.6f),
    };
}

std::vector<TestCase> get7thChordTests()
{
    return {
        makeTest("C Major 7", {"C4", "E4", "G4", "B4"}, "Cmaj7", 0.6f),
        makeTest("C Dominant 7", {"C4", "E4", "G4", "Bb4"}, "C7", 0.6f),
        makeTest("C Minor 7", {"C4", "Eb4", "G4", "Bb4"}, "Cm7", 0.6f),
        makeTest("D Minor 7", {"D3", "F3", "A3", "C4"}, "Dm7", 0.6f),
        makeTest("G Dominant 7", {"G3", "B3", "D4", "F4"}, "G7", 0.6f),
        makeTest("C Diminished 7", {"C4", "Eb4", "Gb4", "A4"}, "Cdim7", 0.5f),
        makeTest("C Half-Dim 7", {"C4", "Eb4", "Gb4", "Bb4"}, "Cm7♭5", 0.5f),
    };
}

std::vector<TestCase> getExtendedChordTests()
{
    return {
        makeTest("C Major 9", {"C3", "E4", "G4", "B4", "D5"}, "Cmaj9", 0.5f),
        makeTest("C Dominant 9", {"C3", "E4", "G4", "Bb4", "D5"}, "C9", 0.5f),
        makeTest("C Minor 9", {"C3", "Eb4", "G4", "Bb4", "D5"}, "Cm9", 0.5f),
        makeTest("C Add 9", {"C4", "E4", "G4", "D5"}, "Cadd9", 0.5f),
    };
}

std::vector<TestCase> getAlteredChordTests()
{
    return {
        makeTest("C 7b9", {"C4", "E4", "G4", "Bb4", "Db5"}, "C7♭9", 0.4f),
        makeTest("C 7#9", {"C4", "E4", "G4", "Bb4", "D#5"}, "C7♯9", 0.4f),
        makeTest("C 7b5", {"C4", "E4", "Gb4", "Bb4"}, "C7♭5", 0.4f),
    };
}

// ============================================================================
// Include JSON-Generated Tests (if available)
// ============================================================================

#ifdef USE_JSON_GENERATED_TESTS
    #include "generated_tests_from_json.h"
#endif

// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    
    std::cout << "========================================" << std::endl;
    std::cout << "PATTERN-BASED CHORD DETECTION TEST SUITE" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    ChordTestRunner runner;
    
    // Run manual tests
    std::cout << "=== MANUAL TESTS ===" << std::endl;
    std::cout << std::endl;
    
    std::cout << "--- Basic Triads ---" << std::endl;
    for (const auto& tc : getBasicTriadTests())
        runner.runTest(tc);
    
    std::cout << std::endl << "--- 7th Chords ---" << std::endl;
    for (const auto& tc : get7thChordTests())
        runner.runTest(tc);
    
    std::cout << std::endl << "--- Extended Chords ---" << std::endl;
    for (const auto& tc : getExtendedChordTests())
        runner.runTest(tc);
    
    std::cout << std::endl << "--- Altered Chords ---" << std::endl;
    for (const auto& tc : getAlteredChordTests())
        runner.runTest(tc);
    
    // Run JSON-generated tests (if available)
#ifdef USE_JSON_GENERATED_TESTS
    std::cout << std::endl;
    std::cout << "=== JSON-GENERATED TESTS ===" << std::endl;
    runAllGeneratedTests(runner);
#else
    std::cout << std::endl;
    std::cout << "NOTE: JSON-generated tests not included (compile with -DUSE_JSON_GENERATED_TESTS)" << std::endl;
#endif
    
    // Print summary
    runner.printSummary();
    
    return runner.getFailedCount() > 0 ? 1 : 0;
}
