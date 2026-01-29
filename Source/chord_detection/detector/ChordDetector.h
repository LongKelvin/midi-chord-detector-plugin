// ChordDetector.h
// Optimized MIDI Chord Detector for JUCE
// C++ implementation maintaining Python algorithm logic

#pragma once

#include <vector>
#include <string>
#include <map>
#include <set>
#include <tuple>
#include <memory>
#include <algorithm>
#include <cmath>

namespace ChordDetection {

// ============================================================================
// ENUMS
// ============================================================================

enum class VoicingType {
    CLOSE,
    OPEN,
    ROOTLESS,
    DROP_2,
    DROP_3,
    UNKNOWN
};

enum class SlashChordMode {
    AUTO,    // Smart slash/inversion
    ALWAYS,  // Always show /bass
    NEVER    // Traditional inversions
};

// ============================================================================
// STRUCTS
// ============================================================================

struct ChordPattern {
    std::vector<int> intervals;
    int baseScore;
    std::vector<int> required;
    std::vector<int> optional;
    std::vector<int> importantIntervals;
    std::string display;
    std::string quality;
    
    ChordPattern() : baseScore(100) {}
    
    ChordPattern(const std::vector<int>& intervals_,
                 int baseScore_,
                 const std::vector<int>& required_,
                 const std::vector<int>& optional_,
                 const std::vector<int>& important_,
                 const std::string& display_,
                 const std::string& quality_)
        : intervals(intervals_)
        , baseScore(baseScore_)
        , required(required_)
        , optional(optional_)
        , importantIntervals(important_)
        , display(display_)
        , quality(quality_)
    {}
};

struct ChordCandidate {
    std::string chordName;
    int root;
    std::string rootName;
    std::string chordType;
    std::vector<int> pattern;
    std::vector<int> intervals;
    float score;
    float confidence;
    std::string position;
    VoicingType voicingType;
    std::vector<std::string> degrees;
    std::vector<std::string> noteNames;
    std::vector<int> noteNumbers;
    std::vector<int> pitchClasses;
    
    ChordCandidate()
        : root(0)
        , score(0.0f)
        , confidence(0.0f)
        , voicingType(VoicingType::UNKNOWN)
    {}
};

// ============================================================================
// CHORD DETECTOR CLASS
// ============================================================================

class OptimizedChordDetector {
public:
    OptimizedChordDetector(bool enableContext = false, 
                          SlashChordMode slashMode = SlashChordMode::AUTO);
    ~OptimizedChordDetector();
    
    // Main detection method
    std::shared_ptr<ChordCandidate> detectChord(const std::vector<int>& midiNotes);
    
    // Helper methods
    int parseNoteInput(const std::string& noteStr);
    std::string getNoteName(int pitchClass, bool preferSharp = true) const;
    std::string formatOutput(const std::shared_ptr<ChordCandidate>& result) const;
    
    // Configuration
    void setSlashChordMode(SlashChordMode mode) { slashChordMode_ = mode; }
    void setEnableContext(bool enable) { enableContext_ = enable; }
    
    // Access chord patterns (for quality lookup)
    const std::map<std::string, ChordPattern>& getChordPatterns() const { return chordPatterns_; }
    
private:
    // Internal methods
    void initializeChordPatterns();
    void buildIntervalIndex();
    
    int midiToPitchClass(int midi) const;
    VoicingType classifyVoicing(const std::vector<int>& midiNotes) const;
    
    float computeScore(const std::vector<int>& intervals,
                      const ChordPattern& pattern,
                      int bassPitchClass,
                      int potentialRoot,
                      VoicingType voicingType) const;
    
    float computeConfidence(float bestScore,
                           float secondBestScore,
                           int noteCount,
                           bool exactMatch) const;
    
    std::shared_ptr<ChordCandidate> resolveAmbiguity(
        const std::vector<std::shared_ptr<ChordCandidate>>& candidates,
        int bassPitchClass) const;
    
    std::string replaceRoot(const std::string& pattern, 
                           const std::string& rootName) const;
    
    // Member variables
    bool enableContext_;
    SlashChordMode slashChordMode_;
    
    std::map<std::string, ChordPattern> chordPatterns_;
    std::map<std::vector<int>, std::vector<std::string>> intervalIndex_;
    std::vector<std::shared_ptr<ChordCandidate>> chordHistory_;
    
    // Note names
    static const std::vector<std::string> NOTE_NAMES;
    static const std::vector<std::string> NOTE_NAMES_FLAT;
    static const std::map<std::string, std::string> ENHARMONIC_MAP;
    static const std::map<int, std::string> DEGREE_MAP;
};

} // namespace ChordDetection
