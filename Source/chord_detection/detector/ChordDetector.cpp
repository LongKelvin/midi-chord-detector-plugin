// ChordDetector.cpp
// Main chord detector implementation
//
// This file contains the core detection logic that orchestrates
// the specialized modules.

#include "ChordDetector.h"
#include <algorithm>
#include <array>
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
    if (candidates.empty()) {
        return nullptr;
    }

    if (candidates.size() < 2) {
        return candidates[0];
    }

    auto& top    = candidates[0];
    auto& second = candidates[1];

    // Large score gap — no real ambiguity, trust the score.
    if (top->score - second->score > 40.0f) {
        return top;
    }

    const auto& typeA = top->chordType;
    const auto& typeB = second->chordType;

    // -------------------------------------------------------------------------
    // Case 1: C6 vs Am7 — same four notes, different root interpretation.
    //   C major6 : {C,E,G,A} — root=C, intervals={0,4,7,9}
    //   A minor7 : {A,C,E,G} — root=A, intervals={0,3,7,10}
    // Disambiguation: whichever root matches the bass wins.
    // -------------------------------------------------------------------------
    const bool isMajor6vsMinor7 = (typeA == "major6" && typeB == "minor7") ||
                                   (typeA == "minor7" && typeB == "major6");
    if (isMajor6vsMinor7) {
        if (top->root    == bassPitchClass) return top;
        if (second->root == bassPitchClass) return second;
        // No bass clue: prefer major6 (more common in root-position context).
        return (typeA == "major6") ? top : second;
    }

    // -------------------------------------------------------------------------
    // Case 2: Diminished7 enharmonics — all four inversions of a dim7 chord
    // share the same four pitch classes (e.g., Bdim7 = Ddim7 = Fdim7 = Abdim7).
    // Disambiguation: whichever root is the bass note wins.
    // -------------------------------------------------------------------------
    if (typeA == "diminished7" && typeB == "diminished7") {
        if (top->root    == bassPitchClass) return top;
        if (second->root == bassPitchClass) return second;
        return top; // fall back to highest score
    }

    // -------------------------------------------------------------------------
    // Case 3: Minor6 vs minor — e.g., Am6 {A,C,E,F#} vs Am {A,C,E}.
    // If the minor6 root is the bass, prefer it (strong evidence).
    // Otherwise trust the score.
    // -------------------------------------------------------------------------
    const bool isMinor6vsMinor = (typeA == "minor6" && typeB == "minor") ||
                                  (typeA == "minor"  && typeB == "minor6");
    if (isMinor6vsMinor) {
        auto& minor6cand = (typeA == "minor6") ? top : second;
        if (minor6cand->root == bassPitchClass) return minor6cand;
        return top; // trust score
    }

    // Default: highest score wins.
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
            
            // Extended intervals for extended chords — but skip interval==0
            // (root): adding 0+12=12 would double-count the root and inflate scores.
            if (sortedNotes.size() > 3 && interval != 0) {
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

    // =========================================================================
    // ROOTLESS CANDIDATE SEARCH 
    // Try each pitch class NOT present in the played notes as a virtual root.
    // Jazz players routinely omit the root; scoring against absent roots with
    // VoicingType::Rootless gives +10 bonus and waives the –40 no-root penalty.
    // =========================================================================
    {
        // Build a fast membership table (stack-allocated, no heap).
        std::array<bool, kPitchClassCount> present{};
        for (int pc : uniquePitchClasses) {
            present[static_cast<size_t>(pc)] = true;
        }

        for (int virtualRoot = 0; virtualRoot < kPitchClassCount; ++virtualRoot) {
            if (present[static_cast<size_t>(virtualRoot)]) continue; // skip present roots

            // Intervals from the absent virtual root — note 0 is NOT added because
            // the root itself is not played.
            std::vector<int> intervals;
            intervals.reserve(uniquePitchClasses.size() * 2);
            for (int pc : uniquePitchClasses) {
                int interval = NoteUtils::intervalBetween(virtualRoot, pc);
                intervals.push_back(interval);
                // Extended intervals (skip interval==0 guard is implicit since
                // root is absent and intervalBetween never produces 0 here).
                if (sortedNotes.size() > 3) {
                    int ext = interval + kPitchClassCount;
                    if (ext <= kMaxIntervals) {
                        intervals.push_back(ext);
                    }
                }
            }

            std::sort(intervals.begin(), intervals.end());
            auto lastRootless = std::unique(intervals.begin(), intervals.end());
            intervals.erase(lastRootless, intervals.end());

            // Fast lookup first; fall back to full scan.
            std::vector<std::string> candidateTypes;
            auto it = intervalIndex_.find(intervals);
            if (it != intervalIndex_.end()) {
                candidateTypes = it->second;
            } else {
                candidateTypes.reserve(chordPatterns_.size());
                for (const auto& [type, _] : chordPatterns_) {
                    candidateTypes.push_back(type);
                }
            }

            for (const std::string& chordType : candidateTypes) {
                const ChordPattern& pattern = chordPatterns_.at(chordType);

                float score = ChordScoring::computeScore(
                    intervals, pattern, bassPitchClass, virtualRoot,
                    VoicingType::Rootless);

                if (score > kMinimumScoreThreshold) {
                    auto candidate = std::make_shared<ChordCandidate>();
                    candidate->root      = virtualRoot;
                    candidate->rootName  = NoteUtils::getNoteName(virtualRoot);
                    candidate->chordType = chordType;
                    candidate->pattern   = pattern.intervals;
                    candidate->intervals = intervals;
                    candidate->score     = score;
                    candidate->voicingType = VoicingType::Rootless;
                    candidate->noteNumbers  = sortedNotes;
                    candidate->pitchClasses = uniquePitchClasses;

                    // Position: rootless with slash notation showing actual bass.
                    std::string bassName = NoteUtils::getNoteName(bassPitchClass);
                    candidate->position  = "Rootless/" + bassName;
                    candidate->chordName = NoteUtils::replaceRootTemplate(
                        pattern.display, candidate->rootName)
                        + "/" + bassName;

                    candidate->degrees.reserve(intervals.size());
                    for (int interval : intervals) {
                        candidate->degrees.push_back(NoteUtils::getDegreeName(interval));
                    }
                    candidate->noteNames.reserve(pitchClasses.size());
                    for (int pc : pitchClasses) {
                        candidate->noteNames.push_back(NoteUtils::getNoteName(pc));
                    }

                    candidates.push_back(candidate);
                }
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
