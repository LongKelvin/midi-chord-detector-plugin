// ChordDetector.cpp
// Implementation of Optimized MIDI Chord Detector

#include "ChordDetector.h"
#include <sstream>
#include <iomanip>
#include <regex>

namespace ChordDetection {

// ============================================================================
// STATIC DATA INITIALIZATION
// ============================================================================

const std::vector<std::string> OptimizedChordDetector::NOTE_NAMES = {
    "C", "C♯", "D", "D♯", "E", "F", "F♯", "G", "G♯", "A", "A♯", "B"
};

const std::vector<std::string> OptimizedChordDetector::NOTE_NAMES_FLAT = {
    "C", "D♭", "D", "E♭", "E", "F", "G♭", "G", "A♭", "A", "B♭", "B"
};

const std::map<std::string, std::string> OptimizedChordDetector::ENHARMONIC_MAP = {
    {"C#", "C♯"}, {"Db", "D♭"}, {"D#", "D♯"}, {"Eb", "E♭"},
    {"E#", "F"},  {"Fb", "E"},  {"F#", "F♯"}, {"Gb", "G♭"},
    {"G#", "G♯"}, {"Ab", "A♭"}, {"A#", "A♯"}, {"Bb", "B♭"},
    {"B#", "C"},  {"Cb", "B"}
};

const std::map<int, std::string> OptimizedChordDetector::DEGREE_MAP = {
    {0, "R"}, {1, "♭9"}, {2, "9"}, {3, "♭3/♯9"}, {4, "3"}, {5, "11"}, 
    {6, "♭5/♯11"}, {7, "5"}, {8, "♯5/♭13"}, {9, "6/13"}, {10, "♭7"}, 
    {11, "7"}, {12, "R"}, {13, "♭9"}, {14, "9"}, {15, "♯9"}, {16, "m10"}, 
    {17, "11"}, {18, "♯11"}, {19, "m12"}, {20, "♭13"}, {21, "13"}, 
    {22, "m14"}, {23, "m15"}
};

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

OptimizedChordDetector::OptimizedChordDetector(bool enableContext, 
                                               SlashChordMode slashMode)
    : enableContext_(enableContext)
    , slashChordMode_(slashMode)
{
    initializeChordPatterns();
    buildIntervalIndex();
}

OptimizedChordDetector::~OptimizedChordDetector() {
    chordHistory_.clear();
}

// ============================================================================
// CHORD PATTERN INITIALIZATION
// ============================================================================

void OptimizedChordDetector::initializeChordPatterns() {
    // TRIADS
    chordPatterns_["major"] = ChordPattern(
        {0, 4, 7}, 100, {0, 4, 7}, {}, {4, 7}, "{root}", "major"
    );
    
    chordPatterns_["minor"] = ChordPattern(
        {0, 3, 7}, 100, {0, 3, 7}, {}, {3, 7}, "{root}m", "minor"
    );
    
    chordPatterns_["diminished"] = ChordPattern(
        {0, 3, 6}, 100, {0, 3, 6}, {}, {3, 6}, "{root}dim", "diminished"
    );
    
    chordPatterns_["augmented"] = ChordPattern(
        {0, 4, 8}, 100, {0, 4, 8}, {}, {4, 8}, "{root}aug", "augmented"
    );
    
    chordPatterns_["sus2"] = ChordPattern(
        {0, 2, 7}, 95, {0, 2, 7}, {}, {2, 7}, "{root}sus2", "suspended"
    );
    
    chordPatterns_["sus4"] = ChordPattern(
        {0, 5, 7}, 95, {0, 5, 7}, {}, {5, 7}, "{root}sus4", "suspended"
    );
    
    chordPatterns_["power5"] = ChordPattern(
        {0, 7}, 80, {0, 7}, {}, {7}, "{root}5", "power"
    );
    
    // SEVENTH CHORDS
    chordPatterns_["major7"] = ChordPattern(
        {0, 4, 7, 11}, 115, {0, 4, 11}, {7}, {4, 11}, "{root}maj7", "major"
    );
    
    chordPatterns_["minor7"] = ChordPattern(
        {0, 3, 7, 10}, 115, {0, 3, 10}, {7}, {3, 10}, "{root}m7", "minor"
    );
    
    chordPatterns_["dominant7"] = ChordPattern(
        {0, 4, 7, 10}, 115, {0, 4, 10}, {7}, {4, 10}, "{root}7", "dominant"
    );
    
    chordPatterns_["diminished7"] = ChordPattern(
        {0, 3, 6, 9}, 115, {0, 3, 6, 9}, {}, {3, 6, 9}, "{root}dim7", "diminished"
    );
    
    chordPatterns_["half-diminished7"] = ChordPattern(
        {0, 3, 6, 10}, 115, {0, 3, 6, 10}, {}, {3, 6, 10}, "{root}m7♭5", "half-diminished"
    );
    
    chordPatterns_["augmented7"] = ChordPattern(
        {0, 4, 8, 10}, 110, {0, 4, 8, 10}, {}, {4, 8, 10}, "{root}aug7", "augmented"
    );
    
    chordPatterns_["augmented-major7"] = ChordPattern(
        {0, 4, 8, 11}, 110, {0, 4, 8, 11}, {}, {4, 8, 11}, "{root}+maj7", "augmented"
    );
    
    chordPatterns_["minor-major7"] = ChordPattern(
        {0, 3, 7, 11}, 110, {0, 3, 11}, {7}, {3, 11}, "{root}m(maj7)", "minor"
    );
    
    chordPatterns_["7sus4"] = ChordPattern(
        {0, 5, 7, 10}, 108, {0, 5, 10}, {7}, {5, 10}, "{root}7sus4", "suspended"
    );
    
    // SIXTH CHORDS
    chordPatterns_["major6"] = ChordPattern(
        {0, 4, 7, 9}, 105, {0, 4, 9}, {7}, {4, 9}, "{root}6", "major"
    );
    
    chordPatterns_["minor6"] = ChordPattern(
        {0, 3, 7, 9}, 105, {0, 3, 9}, {7}, {3, 9}, "{root}m6", "minor"
    );
    
    chordPatterns_["6/9"] = ChordPattern(
        {0, 4, 7, 9, 14}, 110, {0, 4, 9, 14}, {7}, {4, 9, 14}, "{root}6/9", "major"
    );
    
    chordPatterns_["minor6/9"] = ChordPattern(
        {0, 3, 7, 9, 14}, 110, {0, 3, 9, 14}, {7}, {3, 9, 14}, "{root}m6/9", "minor"
    );
    
    // NINTH CHORDS
    chordPatterns_["major9"] = ChordPattern(
        {0, 4, 7, 11, 14}, 125, {0, 4, 11, 14}, {7}, {4, 11, 14}, "{root}maj9", "major"
    );
    
    chordPatterns_["minor9"] = ChordPattern(
        {0, 3, 7, 10, 14}, 125, {0, 3, 10, 14}, {7}, {3, 10, 14}, "{root}m9", "minor"
    );
    
    chordPatterns_["dominant9"] = ChordPattern(
        {0, 4, 7, 10, 14}, 125, {0, 4, 10, 14}, {7}, {4, 10, 14}, "{root}9", "dominant"
    );
    
    chordPatterns_["dominant7♭9"] = ChordPattern(
        {0, 4, 7, 10, 13}, 120, {0, 4, 10, 13}, {7}, {4, 10, 13}, "{root}7♭9", "dominant"
    );
    
    chordPatterns_["dominant7♯9"] = ChordPattern(
        {0, 4, 7, 10, 15}, 120, {0, 4, 10, 15}, {7}, {4, 10, 15}, "{root}7♯9", "dominant"
    );
    
    chordPatterns_["minor-major9"] = ChordPattern(
        {0, 3, 7, 11, 14}, 120, {0, 3, 11, 14}, {7}, {3, 11, 14}, "{root}m(maj9)", "minor"
    );
    
    // ELEVENTH CHORDS
    chordPatterns_["major11"] = ChordPattern(
        {0, 4, 7, 11, 14, 17}, 130, {0, 4, 11, 14, 17}, {7}, {4, 11, 14, 17}, "{root}maj11", "major"
    );
    
    chordPatterns_["minor11"] = ChordPattern(
        {0, 3, 7, 10, 14, 17}, 130, {0, 3, 10, 14, 17}, {7}, {3, 10, 14, 17}, "{root}m11", "minor"
    );
    
    chordPatterns_["dominant11"] = ChordPattern(
        {0, 4, 7, 10, 14, 17}, 130, {0, 4, 10, 14, 17}, {7}, {4, 10, 14, 17}, "{root}11", "dominant"
    );
    
    chordPatterns_["dominant7♯11"] = ChordPattern(
        {0, 4, 7, 10, 18}, 125, {0, 4, 10, 18}, {7}, {4, 10, 18}, "{root}7♯11", "dominant"
    );
    
    chordPatterns_["major7♯11"] = ChordPattern(
        {0, 4, 7, 11, 18}, 125, {0, 4, 11, 18}, {7}, {4, 11, 18}, "{root}maj7♯11", "major"
    );
    
    chordPatterns_["major9♯11"] = ChordPattern(
        {0, 4, 7, 11, 14, 18}, 130, {0, 4, 11, 14, 18}, {7}, {4, 11, 14, 18}, "{root}maj9♯11", "major"
    );
    
    chordPatterns_["minor11♭5"] = ChordPattern(
        {0, 3, 6, 10, 14, 17}, 125, {0, 3, 6, 10, 14, 17}, {}, {3, 6, 10, 14, 17}, "{root}m11♭5", "half-diminished"
    );
    
    // THIRTEENTH CHORDS
    chordPatterns_["major13"] = ChordPattern(
        {0, 4, 7, 11, 14, 21}, 135, {0, 4, 11, 21}, {7, 14}, {4, 11, 21}, "{root}maj13", "major"
    );
    
    chordPatterns_["minor13"] = ChordPattern(
        {0, 3, 7, 10, 14, 21}, 135, {0, 3, 10, 21}, {7, 14}, {3, 10, 21}, "{root}m13", "minor"
    );
    
    chordPatterns_["dominant13"] = ChordPattern(
        {0, 4, 7, 10, 14, 21}, 135, {0, 4, 10, 21}, {7, 14}, {4, 10, 21}, "{root}13", "dominant"
    );
    
    chordPatterns_["dominant13♯11"] = ChordPattern(
        {0, 4, 7, 10, 18, 21}, 135, {0, 4, 10, 18, 21}, {7}, {4, 10, 18, 21}, "{root}13♯11", "dominant"
    );
    
    chordPatterns_["dominant7♭13"] = ChordPattern(
        {0, 4, 7, 10, 20}, 125, {0, 4, 10, 20}, {7}, {4, 10, 20}, "{root}7♭13", "dominant"
    );
    
    chordPatterns_["dominant13♭9"] = ChordPattern(
        {0, 4, 7, 10, 13, 21}, 130, {0, 4, 10, 13, 21}, {7}, {4, 10, 13, 21}, "{root}13♭9", "dominant"
    );
    
    chordPatterns_["dominant13♯9"] = ChordPattern(
        {0, 4, 7, 10, 15, 21}, 130, {0, 4, 10, 15, 21}, {7}, {4, 10, 15, 21}, "{root}13♯9", "dominant"
    );
    
    // ALTERED DOMINANTS
    chordPatterns_["dominant7♭5"] = ChordPattern(
        {0, 4, 6, 10}, 118, {0, 4, 6, 10}, {}, {4, 6, 10}, "{root}7♭5", "dominant"
    );
    
    chordPatterns_["dominant7♯5"] = ChordPattern(
        {0, 4, 8, 10}, 118, {0, 4, 8, 10}, {}, {4, 8, 10}, "{root}7♯5", "dominant"
    );
    
    chordPatterns_["dominant7♭5♭9"] = ChordPattern(
        {0, 4, 6, 10, 13}, 122, {0, 4, 6, 10, 13}, {}, {4, 6, 10, 13}, "{root}7♭5♭9", "dominant"
    );
    
    chordPatterns_["dominant7♯5♭9"] = ChordPattern(
        {0, 4, 8, 10, 13}, 122, {0, 4, 8, 10, 13}, {}, {4, 8, 10, 13}, "{root}7♯5♭9", "dominant"
    );
    
    chordPatterns_["dominant7♭5♯9"] = ChordPattern(
        {0, 4, 6, 10, 15}, 122, {0, 4, 6, 10, 15}, {}, {4, 6, 10, 15}, "{root}7♭5♯9", "dominant"
    );
    
    chordPatterns_["dominant7♯5♯9"] = ChordPattern(
        {0, 4, 8, 10, 15}, 122, {0, 4, 8, 10, 15}, {}, {4, 8, 10, 15}, "{root}7♯5♯9", "dominant"
    );
    
    chordPatterns_["altered"] = ChordPattern(
        {0, 4, 6, 10, 13}, 120, {0, 4, 10}, {6, 8, 13, 15}, {4, 10}, "{root}7alt", "dominant"
    );
    
    chordPatterns_["dominant7♯5♯9♭13"] = ChordPattern(
        {0, 4, 8, 10, 15, 20}, 128, {0, 4, 8, 10, 15, 20}, {}, {4, 8, 10, 15, 20}, "{root}7♯5♯9♭13", "dominant"
    );
    
    chordPatterns_["dominant9♯11"] = ChordPattern(
        {0, 4, 7, 10, 14, 18}, 130, {0, 4, 10, 14, 18}, {7}, {4, 10, 14, 18}, "{root}9♯11", "dominant"
    );
    
    chordPatterns_["dominant9♭13"] = ChordPattern(
        {0, 4, 7, 10, 14, 20}, 130, {0, 4, 10, 14, 20}, {7}, {4, 10, 14, 20}, "{root}9♭13", "dominant"
    );
    
    chordPatterns_["dominant7♯9♯11"] = ChordPattern(
        {0, 4, 7, 10, 15, 18}, 128, {0, 4, 10, 15, 18}, {7}, {4, 10, 15, 18}, "{root}7♯9♯11", "dominant"
    );
    
    chordPatterns_["dominant7♭9♯11"] = ChordPattern(
        {0, 4, 7, 10, 13, 18}, 128, {0, 4, 10, 13, 18}, {7}, {4, 10, 13, 18}, "{root}7♭9♯11", "dominant"
    );
    
    chordPatterns_["dominant7♭9♭13"] = ChordPattern(
        {0, 4, 7, 10, 13, 20}, 128, {0, 4, 10, 13, 20}, {7}, {4, 10, 13, 20}, "{root}7♭9♭13", "dominant"
    );
    
    chordPatterns_["dominant7♯9♭13"] = ChordPattern(
        {0, 4, 7, 10, 15, 20}, 128, {0, 4, 10, 15, 20}, {7}, {4, 10, 15, 20}, "{root}7♯9♭13", "dominant"
    );
    
    // ADD CHORDS
    chordPatterns_["add9"] = ChordPattern(
        {0, 4, 7, 14}, 105, {0, 4, 7, 14}, {}, {4, 7, 14}, "{root}add9", "major"
    );
    
    chordPatterns_["minor-add9"] = ChordPattern(
        {0, 3, 7, 14}, 105, {0, 3, 7, 14}, {}, {3, 7, 14}, "{root}m(add9)", "minor"
    );
    
    chordPatterns_["add11"] = ChordPattern(
        {0, 4, 7, 17}, 100, {0, 4, 7, 17}, {}, {4, 7, 17}, "{root}add11", "major"
    );
    
    chordPatterns_["add♯11"] = ChordPattern(
        {0, 4, 7, 18}, 100, {0, 4, 7, 18}, {}, {4, 7, 18}, "{root}add♯11", "major"
    );
    
    // SPECIAL CHORDS
    chordPatterns_["major7♯5"] = ChordPattern(
        {0, 4, 8, 11}, 115, {0, 4, 8, 11}, {}, {4, 8, 11}, "{root}maj7♯5", "augmented"
    );
    
    chordPatterns_["minor7♭5"] = ChordPattern(
        {0, 3, 6, 10}, 115, {0, 3, 6, 10}, {}, {3, 6, 10}, "{root}m7♭5", "half-diminished"
    );
    
    chordPatterns_["quartal"] = ChordPattern(
        {0, 5, 10}, 90, {0, 5, 10}, {}, {5, 10}, "{root}quartal", "quartal"
    );
    
    chordPatterns_["quartal-7"] = ChordPattern(
        {0, 5, 10, 15}, 95, {0, 5, 10, 15}, {}, {5, 10, 15}, "{root}quartal7", "quartal"
    );
}

void OptimizedChordDetector::buildIntervalIndex() {
    intervalIndex_.clear();
    
    for (const auto& [chordType, pattern] : chordPatterns_) {
        std::vector<int> signature = pattern.intervals;
        std::sort(signature.begin(), signature.end());
        
        // Remove duplicates
        auto last = std::unique(signature.begin(), signature.end());
        signature.erase(last, signature.end());
        
        intervalIndex_[signature].push_back(chordType);
    }
}

// ============================================================================
// HELPER METHODS
// ============================================================================

int OptimizedChordDetector::midiToPitchClass(int midi) const {
    return midi % 12;
}

std::string OptimizedChordDetector::getNoteName(int pitchClass, bool preferSharp) const {
    pitchClass = pitchClass % 12;
    if (pitchClass < 0) pitchClass += 12;
    
    if (preferSharp) {
        return NOTE_NAMES[pitchClass];
    } else {
        return NOTE_NAMES_FLAT[pitchClass];
    }
}

std::string OptimizedChordDetector::replaceRoot(const std::string& pattern, 
                                                const std::string& rootName) const {
    std::string result = pattern;
    size_t pos = result.find("{root}");
    if (pos != std::string::npos) {
        result.replace(pos, 6, rootName);
    }
    return result;
}

int OptimizedChordDetector::parseNoteInput(const std::string& noteStr) {
    std::string processed = noteStr;
    
    // Convert to uppercase
    std::transform(processed.begin(), processed.end(), processed.begin(), ::toupper);
    
    // Replace enharmonics
    for (const auto& [old, newStr] : ENHARMONIC_MAP) {
        std::string oldUpper = old;
        std::transform(oldUpper.begin(), oldUpper.end(), oldUpper.begin(), ::toupper);
        
        size_t pos = 0;
        while ((pos = processed.find(oldUpper, pos)) != std::string::npos) {
            processed.replace(pos, oldUpper.length(), newStr);
            pos += newStr.length();
        }
    }
    
    // Parse note name and octave using regex
    std::regex noteRegex("([A-G][♯♭#b]?)(\\d?)");
    std::smatch match;
    
    if (!std::regex_match(processed, match, noteRegex)) {
        throw std::runtime_error("Invalid note format: " + noteStr);
    }
    
    std::string noteName = match[1].str();
    int octave = match[2].str().empty() ? 4 : std::stoi(match[2].str());
    
    // Replace # and b with proper symbols
    size_t pos = noteName.find('#');
    if (pos != std::string::npos) noteName.replace(pos, 1, "♯");
    pos = noteName.find('b');
    if (pos != std::string::npos) noteName.replace(pos, 1, "♭");
    
    // Find pitch class
    int pitchClass = -1;
    for (size_t i = 0; i < NOTE_NAMES.size(); ++i) {
        if (NOTE_NAMES[i] == noteName) {
            pitchClass = static_cast<int>(i);
            break;
        }
    }
    
    if (pitchClass == -1) {
        for (size_t i = 0; i < NOTE_NAMES_FLAT.size(); ++i) {
            if (NOTE_NAMES_FLAT[i] == noteName) {
                pitchClass = static_cast<int>(i);
                break;
            }
        }
    }
    
    if (pitchClass == -1) {
        throw std::runtime_error("Unknown note: " + noteName);
    }
    
    // Calculate MIDI number (C4 = 60)
    int midi = pitchClass + (octave + 1) * 12;
    return midi;
}

VoicingType OptimizedChordDetector::classifyVoicing(const std::vector<int>& midiNotes) const {
    if (midiNotes.size() < 2) {
        return VoicingType::UNKNOWN;
    }
    
    std::vector<int> sorted = midiNotes;
    std::sort(sorted.begin(), sorted.end());
    
    int span = sorted.back() - sorted.front();
    
    // Close voicing: within one octave
    if (span <= 12) {
        return VoicingType::CLOSE;
    }
    
    // Open voicing
    if (span > 12) {
        if (sorted.size() >= 4) {
            std::vector<int> intervals;
            for (size_t i = 0; i < sorted.size() - 1; ++i) {
                intervals.push_back(sorted[i + 1] - sorted[i]);
            }
            
            if (intervals.size() >= 2 && intervals[0] > 7) {
                return VoicingType::DROP_2;
            }
            
            if (intervals.size() >= 3 && intervals[1] > 7) {
                return VoicingType::DROP_3;
            }
        }
        
        return VoicingType::OPEN;
    }
    
    return VoicingType::UNKNOWN;
}

// ============================================================================
// SCORING ALGORITHM
// ============================================================================

float OptimizedChordDetector::computeScore(
    const std::vector<int>& intervals,
    const ChordPattern& pattern,
    int bassPitchClass,
    int potentialRoot,
    VoicingType voicingType) const
{
    float score = static_cast<float>(pattern.baseScore);
    
    // Check required intervals
    bool allRequiredPresent = true;
    for (int req : pattern.required) {
        if (std::find(intervals.begin(), intervals.end(), req) == intervals.end()) {
            allRequiredPresent = false;
            break;
        }
    }
    
    if (!allRequiredPresent) {
        return 0.0f;
    }
    
    // Convert to sets for easier operations
    std::set<int> intervalSet(intervals.begin(), intervals.end());
    std::set<int> patternSet(pattern.intervals.begin(), pattern.intervals.end());
    
    // MASSIVE bonus for exact match
    if (intervalSet == patternSet) {
        score += 150.0f;  // Increased from 120
    }
    
    // Required intervals bonus
    int requiredCount = 0;
    for (int interval : intervals) {
        if (std::find(pattern.required.begin(), pattern.required.end(), interval) 
            != pattern.required.end()) {
            requiredCount++;
        }
    }
    score += requiredCount * 30.0f;  // Increased from 25
    
    // Optional intervals bonus
    int optionalCount = 0;
    for (int interval : intervals) {
        if (std::find(pattern.optional.begin(), pattern.optional.end(), interval) 
            != pattern.optional.end()) {
            optionalCount++;
        }
    }
    score += optionalCount * 10.0f;
    
    // REDUCED root position bonus (was causing wrong root in inversions)
    if (bassPitchClass == potentialRoot) {
        score += 15.0f;  // Reduced from 40
    }
    
    // INCREASED important interval bonus (3rd and 7th define quality)
    int importantPresent = 0;
    for (int interval : intervals) {
        if (std::find(pattern.importantIntervals.begin(), 
                     pattern.importantIntervals.end(), interval) 
            != pattern.importantIntervals.end()) {
            importantPresent++;
        }
    }
    score += importantPresent * 30.0f;  // Increased from 20
    
    // Match ratio bonus - INCREASED importance
    int matchedCount = 0;
    for (int interval : intervals) {
        if (patternSet.find(interval) != patternSet.end()) {
            matchedCount++;
        }
    }
    
    if (!pattern.intervals.empty()) {
        float matchRatio = static_cast<float>(matchedCount) / 
                          static_cast<float>(pattern.intervals.size());
        score += matchRatio * 80.0f;  // Increased from 60
    }
    
    // Penalty for extra intervals
    int extraCount = 0;
    for (int interval : intervals) {
        if (patternSet.find(interval) == patternSet.end() &&
            std::find(pattern.optional.begin(), pattern.optional.end(), interval) 
            == pattern.optional.end()) {
            extraCount++;
        }
    }
    score -= extraCount * 8.0f;
    
    // Voicing considerations
    if (voicingType == VoicingType::ROOTLESS) {
        score += 10.0f;
    } else if (voicingType == VoicingType::CLOSE) {
        score += 5.0f;
    }
    
    // Must have root (unless rootless)
    if (std::find(intervals.begin(), intervals.end(), 0) == intervals.end() &&
        voicingType != VoicingType::ROOTLESS) {
        score -= 40.0f;
    }
    
    // Prefer chords with 3rd or sus
    bool hasThird = false;
    for (int interval : {2, 3, 4, 5}) {
        if (std::find(intervals.begin(), intervals.end(), interval) != intervals.end()) {
            hasThird = true;
            break;
        }
    }
    if (!hasThird) {
        score -= 25.0f;
    }
    
    return score;
}

float OptimizedChordDetector::computeConfidence(
    float bestScore,
    float secondBestScore,
    int noteCount,
    bool exactMatch) const
{
    // Score margin
    float scoreMargin = bestScore - secondBestScore;
    float marginConfidence = std::min(scoreMargin / 100.0f, 1.0f);
    
    // Absolute score
    float absoluteConfidence = std::min(bestScore / 250.0f, 1.0f);
    
    // Note count
    float noteConfidence = std::min(static_cast<float>(noteCount) / 6.0f, 1.0f);
    
    // Exact match
    float exactConfidence = exactMatch ? 1.0f : 0.5f;
    
    // Weighted combination
    float confidence = 0.35f * marginConfidence +
                      0.25f * absoluteConfidence +
                      0.15f * noteConfidence +
                      0.25f * exactConfidence;
    
    return confidence;
}

std::shared_ptr<ChordCandidate> OptimizedChordDetector::resolveAmbiguity(
    const std::vector<std::shared_ptr<ChordCandidate>>& candidates,
    int bassPitchClass) const
{
    (void)bassPitchClass; // Suppress unused parameter warning
    
    if (candidates.empty()) {
        return nullptr;
    }
    
    if (candidates.size() < 2) {
        return candidates[0];
    }
    
    auto top = candidates[0];
    auto second = candidates[1];
    
    // Special case: minor6 vs relative major
    // Prefer minor6 when it has minor 3rd
    if (top->chordType == "minor6" && second->chordType == "minor") {
        return top;
    }
    if (second->chordType == "minor6" && top->chordType == "minor") {
        return second;
    }
    
    // Default: return highest scored
    return top;
}

// ============================================================================
// MAIN DETECTION METHOD
// ============================================================================

std::shared_ptr<ChordCandidate> OptimizedChordDetector::detectChord(
    const std::vector<int>& midiNotes)
{
    if (midiNotes.size() < 2) {
        return nullptr;
    }
    
    // Remove duplicates and sort
    std::vector<int> sortedNotes = midiNotes;
    std::sort(sortedNotes.begin(), sortedNotes.end());
    auto last = std::unique(sortedNotes.begin(), sortedNotes.end());
    sortedNotes.erase(last, sortedNotes.end());
    
    // Get pitch classes
    std::vector<int> pitchClasses;
    for (int midi : sortedNotes) {
        pitchClasses.push_back(midiToPitchClass(midi));
    }
    
    // Get unique pitch classes
    std::vector<int> uniquePitchClasses = pitchClasses;
    std::sort(uniquePitchClasses.begin(), uniquePitchClasses.end());
    last = std::unique(uniquePitchClasses.begin(), uniquePitchClasses.end());
    uniquePitchClasses.erase(last, uniquePitchClasses.end());
    
    if (uniquePitchClasses.size() < 2) {
        return nullptr;
    }
    
    int bassPitchClass = pitchClasses[0];
    VoicingType voicingType = classifyVoicing(sortedNotes);
    
    // Try each pitch class as potential root
    std::vector<std::shared_ptr<ChordCandidate>> candidates;
    
    for (int potentialRoot : uniquePitchClasses) {
        // Calculate intervals from this root
        std::vector<int> intervals;
        for (int pc : uniquePitchClasses) {
            int interval = (pc - potentialRoot + 12) % 12;
            intervals.push_back(interval);
            
            // Extended intervals for extended chords
            if (sortedNotes.size() > 3) {
                int extendedInterval = interval + 12;
                if (extendedInterval <= 24) {
                    intervals.push_back(extendedInterval);
                }
            }
        }
        
        // Remove duplicates and sort
        std::sort(intervals.begin(), intervals.end());
        auto lastInterval = std::unique(intervals.begin(), intervals.end());
        intervals.erase(lastInterval, intervals.end());
        
        // Quick lookup for exact matches
        std::vector<int> signature = intervals;
        std::vector<std::string> candidateTypes;
        
        auto it = intervalIndex_.find(signature);
        if (it != intervalIndex_.end()) {
            candidateTypes = it->second;
        }
        
        // If no exact match, try all patterns
        if (candidateTypes.empty()) {
            for (const auto& [type, _] : chordPatterns_) {
                candidateTypes.push_back(type);
            }
        }
        
        // Score each candidate
        for (const std::string& chordType : candidateTypes) {
            const ChordPattern& pattern = chordPatterns_.at(chordType);
            
            float score = computeScore(intervals, pattern, bassPitchClass, 
                                      potentialRoot, voicingType);
            
            if (score > 80.0f) {  // Minimum threshold
                auto candidate = std::make_shared<ChordCandidate>();
                
                candidate->root = potentialRoot;
                candidate->rootName = getNoteName(potentialRoot);
                candidate->chordType = chordType;
                candidate->pattern = pattern.intervals;
                candidate->intervals = intervals;
                candidate->score = score;
                candidate->voicingType = voicingType;
                candidate->noteNumbers = sortedNotes;
                candidate->pitchClasses = uniquePitchClasses;
                
                // Determine position and slash notation
                std::string slashNotation;
                if (bassPitchClass == potentialRoot) {
                    candidate->position = "Root Position";
                } else {
                    int bassInterval = (bassPitchClass - potentialRoot + 12) % 12;
                    std::string bassNoteName = getNoteName(bassPitchClass);
                    
                    // Check if standard inversion
                    bool isStandardInversion = 
                        std::find(pattern.intervals.begin(), 
                                 pattern.intervals.end(), bassInterval) 
                        != pattern.intervals.end();
                    
                    std::string inversionName;
                    if (bassInterval == 3 || bassInterval == 4) {
                        inversionName = "1st Inversion";
                    } else if (bassInterval == 6 || bassInterval == 7 || bassInterval == 8) {
                        inversionName = "2nd Inversion";
                    } else if (bassInterval == 9 || bassInterval == 10 || bassInterval == 11) {
                        inversionName = "3rd Inversion";
                    } else {
                        inversionName = "Slash Chord";
                    }
                    
                    // Handle slash chord mode
                    if (slashChordMode_ == SlashChordMode::ALWAYS) {
                        candidate->position = "Slash/" + bassNoteName;
                        slashNotation = "/" + bassNoteName;
                    } else if (slashChordMode_ == SlashChordMode::NEVER) {
                        candidate->position = inversionName;
                    } else {  // AUTO
                        if (isStandardInversion) {
                            candidate->position = inversionName + " (/" + bassNoteName + ")";
                            slashNotation = "/" + bassNoteName;
                        } else {
                            candidate->position = "Slash/" + bassNoteName;
                            slashNotation = "/" + bassNoteName;
                        }
                    }
                }
                
                // Build chord name
                candidate->chordName = replaceRoot(pattern.display, candidate->rootName);
                if (!slashNotation.empty()) {
                    candidate->chordName += slashNotation;
                }
                
                // Build degree names
                for (int interval : intervals) {
                    auto degreeIt = DEGREE_MAP.find(interval);
                    if (degreeIt != DEGREE_MAP.end()) {
                        candidate->degrees.push_back(degreeIt->second);
                    } else {
                        candidate->degrees.push_back(std::to_string(interval));
                    }
                }
                
                // Build note names
                for (int pc : pitchClasses) {
                    candidate->noteNames.push_back(getNoteName(pc));
                }
                
                candidates.push_back(candidate);
            }
        }
    }
    
    if (candidates.empty()) {
        return nullptr;
    }
    
    // Sort by score
    std::sort(candidates.begin(), candidates.end(),
              [](const std::shared_ptr<ChordCandidate>& a, 
                 const std::shared_ptr<ChordCandidate>& b) {
                  return a->score > b->score;
              });
    
    auto best = candidates[0];
    float secondBestScore = candidates.size() > 1 ? candidates[1]->score : 0.0f;
    
    // Check exact match
    std::set<int> intervalSet(best->intervals.begin(), best->intervals.end());
    std::set<int> patternSet(best->pattern.begin(), best->pattern.end());
    bool exactMatch = (intervalSet == patternSet);
    
    best->confidence = computeConfidence(best->score, secondBestScore,
                                        static_cast<int>(sortedNotes.size()), 
                                        exactMatch);
    
    // Resolve ambiguity
    if (candidates.size() > 1) {
        std::vector<std::shared_ptr<ChordCandidate>> topCandidates;
        for (size_t i = 0; i < std::min(size_t(3), candidates.size()); ++i) {
            topCandidates.push_back(candidates[i]);
        }
        best = resolveAmbiguity(topCandidates, bassPitchClass);
    }
    
    // Update context if enabled
    if (enableContext_) {
        chordHistory_.push_back(best);
        if (chordHistory_.size() > 10) {
            chordHistory_.erase(chordHistory_.begin());
        }
    }
    
    return best;
}

// ============================================================================
// OUTPUT FORMATTING
// ============================================================================

std::string OptimizedChordDetector::formatOutput(
    const std::shared_ptr<ChordCandidate>& result) const
{
    if (!result) {
        return "No chord detected";
    }
    
    std::stringstream ss;
    
    // Confidence emoji
    std::string confIndicator = result->confidence > 0.8f ? "[HIGH]" : 
                               (result->confidence > 0.6f ? "[MED]" : "[LOW]");
    
    ss << "\n" << std::string(70, '=') << "\n";
    ss << "OPTIMIZED CHORD DETECTION RESULT\n";
    ss << std::string(70, '=') << "\n";
    ss << "\nCHORD: " << result->chordName << "\n";
    ss << confIndicator << " Confidence: " << std::fixed << std::setprecision(1) 
       << (result->confidence * 100.0f) << "% (Score: " 
       << std::fixed << std::setprecision(0) << result->score << ")\n";
    ss << "Position: " << result->position << "\n";
    ss << "Root: " << result->rootName << "\n";
    
    // Get quality
    auto patternIt = chordPatterns_.find(result->chordType);
    if (patternIt != chordPatterns_.end()) {
        ss << "Quality: " << patternIt->second.quality << "\n";
    }
    
    // Voicing type
    std::string voicingStr;
    switch (result->voicingType) {
        case VoicingType::CLOSE: voicingStr = "close"; break;
        case VoicingType::OPEN: voicingStr = "open"; break;
        case VoicingType::ROOTLESS: voicingStr = "rootless"; break;
        case VoicingType::DROP_2: voicingStr = "drop_2"; break;
        case VoicingType::DROP_3: voicingStr = "drop_3"; break;
        default: voicingStr = "unknown"; break;
    }
    ss << "Voicing: " << voicingStr << "\n";
    
    // MIDI notes
    ss << "\nMIDI Notes: ";
    for (size_t i = 0; i < result->noteNumbers.size(); ++i) {
        if (i > 0) ss << " ";
        ss << result->noteNumbers[i];
    }
    ss << "\n";
    
    // Note names
    ss << "Note Names: ";
    for (size_t i = 0; i < result->noteNames.size(); ++i) {
        if (i > 0) ss << " ";
        ss << result->noteNames[i];
    }
    ss << "\n";
    
    // Intervals
    ss << "Intervals: ";
    for (size_t i = 0; i < result->intervals.size(); ++i) {
        if (i > 0) ss << " ";
        ss << result->intervals[i];
    }
    ss << "\n";
    
    // Degrees
    ss << "Degrees: ";
    for (size_t i = 0; i < result->degrees.size(); ++i) {
        if (i > 0) ss << " ";
        ss << result->degrees[i];
    }
    ss << "\n";
    
    ss << std::string(70, '=') << "\n";
    
    return ss.str();
}

} // namespace ChordDetection
