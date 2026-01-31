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
    , historyWriteIndex_(0)
    , historySize_(0)
{
    ChordPatterns::initializePatterns(chordPatterns_);
    ChordPatterns::buildIntervalIndex(chordPatterns_, intervalIndex_);
    
    // Pre-allocate history buffer
    chordHistory_.resize(kMaxChordHistorySize);
    
    // Pre-allocate working buffers to avoid allocations in detectChord()
    // Reserve for typical chord sizes (up to 10 notes)
    workingSortedNotes_.reserve(16);
    workingPitchClasses_.reserve(16);
    workingUniquePitchClasses_.reserve(12);  // Max unique pitch classes
    workingIntervals_.reserve(24);           // Max intervals including extensions
    workingCandidates_.reserve(50);          // Typical max candidates per detection
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
    
    // Reuse pre-allocated buffers instead of creating new vectors
    workingSortedNotes_.clear();
    workingSortedNotes_.assign(midiNotes.begin(), midiNotes.end());
    std::sort(workingSortedNotes_.begin(), workingSortedNotes_.end());
    auto last = std::unique(workingSortedNotes_.begin(), workingSortedNotes_.end());
    workingSortedNotes_.erase(last, workingSortedNotes_.end());
    
    // Get pitch classes using pre-allocated buffer
    workingPitchClasses_.clear();
    for (int midi : workingSortedNotes_) {
        workingPitchClasses_.push_back(NoteUtils::midiToPitchClass(midi));
    }
    
    // Get unique pitch classes using pre-allocated buffer
    workingUniquePitchClasses_.clear();
    workingUniquePitchClasses_.assign(workingPitchClasses_.begin(), workingPitchClasses_.end());
    std::sort(workingUniquePitchClasses_.begin(), workingUniquePitchClasses_.end());
    last = std::unique(workingUniquePitchClasses_.begin(), workingUniquePitchClasses_.end());
    workingUniquePitchClasses_.erase(last, workingUniquePitchClasses_.end());
    
    if (workingUniquePitchClasses_.size() < kDefaultMinimumNotes) {
        return nullptr;
    }
    
    int bassPitchClass = workingPitchClasses_[0];
    VoicingType voicingType = VoicingAnalyzer::classifyVoicing(workingSortedNotes_);
    
    // Reuse pre-allocated candidates buffer
    workingCandidates_.clear();
    
    for (int potentialRoot : workingUniquePitchClasses_) {
        // Reuse pre-allocated intervals buffer
        workingIntervals_.clear();
        for (int pc : workingUniquePitchClasses_) {
            int interval = NoteUtils::intervalBetween(potentialRoot, pc);
            workingIntervals_.push_back(interval);
            
            // Extended intervals for extended chords
            if (workingSortedNotes_.size() > 3) {
                int extendedInterval = interval + kPitchClassCount;
                if (extendedInterval <= kMaxIntervals) {
                    workingIntervals_.push_back(extendedInterval);
                }
            }
        }
        
        // Remove duplicates and sort
        std::sort(workingIntervals_.begin(), workingIntervals_.end());
        auto lastInterval = std::unique(workingIntervals_.begin(), workingIntervals_.end());
        workingIntervals_.erase(lastInterval, workingIntervals_.end());
        
        // Quick lookup for exact matches
        std::vector<int> signature = workingIntervals_;  // Still need a copy for map lookup
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
                workingIntervals_, pattern, bassPitchClass, potentialRoot, voicingType, chordType);
            
            if (score > kMinimumScoreThreshold) {
                auto candidate = std::make_shared<ChordCandidate>();
                
                candidate->root = potentialRoot;
                candidate->rootName = NoteUtils::getNoteName(potentialRoot);
                candidate->chordType = chordType;
                candidate->pattern = pattern.intervals;
                candidate->intervals = workingIntervals_;
                candidate->score = score;
                candidate->voicingType = voicingType;
                candidate->noteNumbers = workingSortedNotes_;
                candidate->pitchClasses = workingUniquePitchClasses_;
                
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
                candidate->degrees.reserve(workingIntervals_.size());
                for (int interval : workingIntervals_) {
                    candidate->degrees.push_back(NoteUtils::getDegreeName(interval));
                }
                
                // Build note names
                candidate->noteNames.reserve(workingPitchClasses_.size());
                for (int pc : workingPitchClasses_) {
                    candidate->noteNames.push_back(NoteUtils::getNoteName(pc));
                }
                
                workingCandidates_.push_back(candidate);
            }
        }
    }
    
    if (workingCandidates_.empty()) {
        return nullptr;
    }
    
    // Sort by score
    std::sort(workingCandidates_.begin(), workingCandidates_.end(),
              [](const std::shared_ptr<ChordCandidate>& a, 
                 const std::shared_ptr<ChordCandidate>& b) {
                  return a->score > b->score;
              });
    
    auto best = workingCandidates_[0];
    float secondBestScore = workingCandidates_.size() > 1 ? workingCandidates_[1]->score : 0.0f;
    std::string secondBestType = workingCandidates_.size() > 1 ? workingCandidates_[1]->chordType : "";
    
    // Check exact match
    std::set<int> intervalSet(best->intervals.begin(), best->intervals.end());
    std::set<int> patternSet(best->pattern.begin(), best->pattern.end());
    bool exactMatch = (intervalSet == patternSet);
    
    // Use enhanced confidence that applies ambiguity penalty
    best->confidence = ChordScoring::computeEnhancedConfidence(
        best->score, secondBestScore,
        static_cast<int>(workingSortedNotes_.size()), 
        exactMatch,
        best->chordType, secondBestType);
    
    // Compute ambiguity and populate alternatives
    if (workingCandidates_.size() > 1) {
        best->ambiguityScore = ChordScoring::computeAmbiguityScore(
            best->score, secondBestScore, best->chordType, secondBestType);
        best->isAmbiguous = (best->ambiguityScore > 0.3f);  // 30% threshold for ambiguity
        
        // Add top alternatives (different chord names only)
        std::set<std::string> addedChordNames;
        addedChordNames.insert(best->chordName);
        
        for (size_t i = 1; i < workingCandidates_.size() && best->alternatives.size() < 4; ++i) {
            const auto& alt = workingCandidates_[i];
            
            // Skip if same chord name already added
            if (addedChordNames.find(alt->chordName) != addedChordNames.end()) {
                continue;
            }
            
            // Use improved shouldIncludeAlternative filter
            float scoreRatio = alt->score / best->score;
            bool sameRoot = (alt->root == best->root);
            
            if (!ChordScoring::shouldIncludeAlternative(
                    best->chordType, alt->chordType, scoreRatio, sameRoot)) {
                // Skip if filter says not to include
                // But still check if score is acceptable for continuing loop
                if (scoreRatio < 0.5f) {
                    break;  // No point checking lower-scoring alternatives
                }
                continue;
            }
            
            // Determine relationship to main chord
            std::string relationship;
            if (alt->root != best->root) {
                // Different root - could be relative minor/major or enharmonic
                int rootDiff = (alt->root - best->root + 12) % 12;
                if (rootDiff == 9 && best->chordType.find("major") != std::string::npos 
                    && alt->chordType.find("minor") != std::string::npos) {
                    relationship = "Relative minor";
                } else if (rootDiff == 3 && best->chordType.find("minor") != std::string::npos 
                           && alt->chordType.find("major") != std::string::npos) {
                    relationship = "Relative major";
                } else if ((best->chordType == "major6" && alt->chordType == "minor7") ||
                           (best->chordType == "minor7" && alt->chordType == "major6")) {
                    relationship = "Enharmonic (6th/m7)";
                } else if ((best->chordType == "minor6" && alt->chordType == "half-diminished7") ||
                           (best->chordType == "half-diminished7" && alt->chordType == "minor6") ||
                           (best->chordType == "minor6" && alt->chordType == "minor7b5") ||
                           (best->chordType == "minor7b5" && alt->chordType == "minor6")) {
                    relationship = "Enharmonic (m6/m7♭5)";
                } else {
                    relationship = "Alternative root";
                }
            } else {
                // Same root - different voicing or extension
                if (alt->chordType.find("shell") != std::string::npos) {
                    relationship = "Shell voicing";
                } else if (alt->chordType.find("rootless") != std::string::npos) {
                    relationship = "Rootless voicing";
                } else if (alt->chordType.find("reduced") != std::string::npos) {
                    relationship = "Reduced voicing";
                } else if (best->position != alt->position) {
                    relationship = "Inversion";
                } else {
                    relationship = "Similar quality";
                }
            }
            
            // Compute confidence for this alternative
            float altConfidence = ChordScoring::computeConfidence(
                alt->score, 0.0f,  // No second-best for alternatives
                static_cast<int>(workingSortedNotes_.size()), 
                false);  // Not comparing exact match for alternatives
            
            best->alternatives.emplace_back(
                alt->chordName,
                alt->rootName,
                alt->chordType,
                relationship,
                alt->score,
                altConfidence
            );
            
            addedChordNames.insert(alt->chordName);
        }
    }
    
    // Resolve ambiguity (may change 'best' selection)
    if (workingCandidates_.size() > 1) {
        std::vector<std::shared_ptr<ChordCandidate>> topCandidates;
        topCandidates.reserve(3);
        for (size_t i = 0; i < std::min(size_t(3), workingCandidates_.size()); ++i) {
            topCandidates.push_back(workingCandidates_[i]);
        }
        best = resolveAmbiguity(topCandidates, bassPitchClass);
    }
    
    // Update context using circular buffer (O(1) instead of O(n))
    if (enableContext_) {
        chordHistory_[historyWriteIndex_] = best;
        historyWriteIndex_ = (historyWriteIndex_ + 1) % kMaxChordHistorySize;
        if (historySize_ < kMaxChordHistorySize) {
            ++historySize_;
        }
    }
    
    return best;
}

} // namespace ChordDetection
