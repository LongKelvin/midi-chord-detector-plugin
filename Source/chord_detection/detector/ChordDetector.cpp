// ChordDetector.cpp
// Main chord detector implementation
//
// This file contains the core detection logic that orchestrates
// the specialized modules.

#include "ChordDetector.h"
#include <algorithm>
#include <set>

namespace ChordDetection {

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

ChordDetector::ChordDetector(bool enableContext, SlashChordMode slashMode)
    : enableContext_(enableContext)
    , slashChordMode_(slashMode)
{
    ChordPatterns::initializePatterns(chordPatterns_);
    ChordPatterns::buildIntervalIndex(chordPatterns_, intervalIndex_);
}

ChordDetector::~ChordDetector() {
    chordHistory_.clear();
}

// ============================================================================
// AMBIGUITY RESOLUTION
// ============================================================================

std::shared_ptr<ChordCandidate> ChordDetector::resolveAmbiguity(
    const std::vector<std::shared_ptr<ChordCandidate>>& candidates,
    int bassPitchClass) const
{
    (void)bassPitchClass;  // Reserved for future use
    
    if (candidates.empty()) {
        return nullptr;
    }
    
    if (candidates.size() < 2) {
        return candidates[0];
    }
    
    auto top = candidates[0];
    auto second = candidates[1];
    
    // Special case: minor6 vs relative major
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

std::shared_ptr<ChordCandidate> ChordDetector::detectChord(
    const std::vector<int>& midiNotes)
{
    if (midiNotes.size() < kDefaultMinimumNotes) {
        return nullptr;
    }
    
    // Remove duplicates and sort
    std::vector<int> sortedNotes = midiNotes;
    std::sort(sortedNotes.begin(), sortedNotes.end());
    auto last = std::unique(sortedNotes.begin(), sortedNotes.end());
    sortedNotes.erase(last, sortedNotes.end());
    
    // Get pitch classes
    std::vector<int> pitchClasses;
    pitchClasses.reserve(sortedNotes.size());
    for (int midi : sortedNotes) {
        pitchClasses.push_back(NoteUtils::midiToPitchClass(midi));
    }
    
    // Get unique pitch classes
    std::vector<int> uniquePitchClasses = pitchClasses;
    std::sort(uniquePitchClasses.begin(), uniquePitchClasses.end());
    last = std::unique(uniquePitchClasses.begin(), uniquePitchClasses.end());
    uniquePitchClasses.erase(last, uniquePitchClasses.end());
    
    if (uniquePitchClasses.size() < kDefaultMinimumNotes) {
        return nullptr;
    }
    
    int bassPitchClass = pitchClasses[0];
    VoicingType voicingType = VoicingAnalyzer::classifyVoicing(sortedNotes);
    
    // Try each pitch class as potential root
    std::vector<std::shared_ptr<ChordCandidate>> candidates;
    
    for (int potentialRoot : uniquePitchClasses) {
        // Calculate intervals from this root
        std::vector<int> intervals;
        for (int pc : uniquePitchClasses) {
            int interval = NoteUtils::intervalBetween(potentialRoot, pc);
            intervals.push_back(interval);
            
            // Extended intervals for extended chords
            if (sortedNotes.size() > 3) {
                int extendedInterval = interval + kPitchClassCount;
                if (extendedInterval <= kMaxIntervals) {
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
            candidateTypes.reserve(chordPatterns_.size());
            for (const auto& [type, _] : chordPatterns_) {
                candidateTypes.push_back(type);
            }
        }
        
        // Score each candidate
        for (const std::string& chordType : candidateTypes) {
            const ChordPattern& pattern = chordPatterns_.at(chordType);
            
            float score = ChordScoring::computeScore(
                intervals, pattern, bassPitchClass, potentialRoot, voicingType);
            
            if (score > kMinimumScoreThreshold) {
                auto candidate = std::make_shared<ChordCandidate>();
                
                candidate->root = potentialRoot;
                candidate->rootName = NoteUtils::getNoteName(potentialRoot);
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
                    int bassInterval = NoteUtils::intervalBetween(potentialRoot, bassPitchClass);
                    std::string bassNoteName = NoteUtils::getNoteName(bassPitchClass);
                    
                    // Check if standard inversion
                    bool isStandardInversion = pattern.hasInterval(bassInterval);
                    
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
                    if (slashChordMode_ == SlashChordMode::Always) {
                        candidate->position = "Slash/" + bassNoteName;
                        slashNotation = "/" + bassNoteName;
                    } else if (slashChordMode_ == SlashChordMode::Never) {
                        candidate->position = inversionName;
                    } else {  // Auto
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
                candidate->chordName = NoteUtils::replaceRootTemplate(
                    pattern.display, candidate->rootName);
                if (!slashNotation.empty()) {
                    candidate->chordName += slashNotation;
                }
                
                // Build degree names
                candidate->degrees.reserve(intervals.size());
                for (int interval : intervals) {
                    candidate->degrees.push_back(NoteUtils::getDegreeName(interval));
                }
                
                // Build note names
                candidate->noteNames.reserve(pitchClasses.size());
                for (int pc : pitchClasses) {
                    candidate->noteNames.push_back(NoteUtils::getNoteName(pc));
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
    
    best->confidence = ChordScoring::computeConfidence(
        best->score, secondBestScore,
        static_cast<int>(sortedNotes.size()), 
        exactMatch);
    
    // Resolve ambiguity
    if (candidates.size() > 1) {
        std::vector<std::shared_ptr<ChordCandidate>> topCandidates;
        topCandidates.reserve(3);
        for (size_t i = 0; i < std::min(size_t(3), candidates.size()); ++i) {
            topCandidates.push_back(candidates[i]);
        }
        best = resolveAmbiguity(topCandidates, bassPitchClass);
    }
    
    // Update context if enabled
    if (enableContext_) {
        chordHistory_.push_back(best);
        if (chordHistory_.size() > kMaxChordHistorySize) {
            chordHistory_.erase(chordHistory_.begin());
        }
    }
    
    return best;
}

} // namespace ChordDetection
