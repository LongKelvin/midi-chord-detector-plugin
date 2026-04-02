// ChordScoring.cpp
// Chord detection scoring implementation

#include "ChordScoring.h"
#include <algorithm>
#include <set>
#include <cmath>

namespace ChordDetection {
namespace ChordScoring {

float computeScore(
    const std::vector<int>& intervals,
    const ChordPattern& pattern,
    int bassPitchClass,
    int potentialRoot,
    VoicingType voicingType)
{
    float score = static_cast<float>(pattern.baseScore);
    
    // Check required intervals - must all be present
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
    
    // Convert to sets for easier comparison
    std::set<int> intervalSet(intervals.begin(), intervals.end());
    std::set<int> patternSet(pattern.intervals.begin(), pattern.intervals.end());
    
    // MASSIVE bonus for exact match
    if (intervalSet == patternSet) {
        score += 150.0f;
    }
    
    // Required intervals bonus
    int requiredCount = 0;
    for (int interval : intervals) {
        if (std::find(pattern.required.begin(), pattern.required.end(), interval) 
            != pattern.required.end()) {
            requiredCount++;
        }
    }
    score += static_cast<float>(requiredCount) * 30.0f;
    
    // Optional intervals bonus
    int optionalCount = 0;
    for (int interval : intervals) {
        if (std::find(pattern.optional.begin(), pattern.optional.end(), interval) 
            != pattern.optional.end()) {
            optionalCount++;
        }
    }
    score += static_cast<float>(optionalCount) * 10.0f;
    
    // Root position bonus — strengthened to be meaningful relative to score range.
    if (bassPitchClass == potentialRoot) {
        score += 25.0f;
    }
    
    // Important interval bonus (3rd and 7th define chord quality)
    int importantPresent = 0;
    for (int interval : intervals) {
        if (std::find(pattern.importantIntervals.begin(), 
                     pattern.importantIntervals.end(), interval) 
            != pattern.importantIntervals.end()) {
            importantPresent++;
        }
    }
    score += static_cast<float>(importantPresent) * 30.0f;
    
    // Match ratio bonus
    int matchedCount = 0;
    for (int interval : intervals) {
        if (patternSet.find(interval) != patternSet.end()) {
            matchedCount++;
        }
    }
    
    if (!pattern.intervals.empty()) {
        float matchRatio = static_cast<float>(matchedCount) / 
                          static_cast<float>(pattern.intervals.size());
        score += matchRatio * 80.0f;
    }
    
    // Penalty for extra intervals not in pattern or optional
    int extraCount = 0;
    for (int interval : intervals) {
        if (patternSet.find(interval) == patternSet.end() &&
            std::find(pattern.optional.begin(), pattern.optional.end(), interval) 
            == pattern.optional.end()) {
            extraCount++;
        }
    }
    // Jazz extensions (9, 11, 13) should not be treated as scoring errors.
    // Halved penalty vs. original to be less aggressive for complex chords.
    score -= static_cast<float>(extraCount) * 4.0f;
    
    // Voicing considerations
    if (voicingType == VoicingType::Rootless) {
        score += 10.0f;
    } else if (voicingType == VoicingType::Close) {
        score += 5.0f;
    }
    
    // Must have root (unless rootless voicing)
    if (std::find(intervals.begin(), intervals.end(), 0) == intervals.end() &&
        voicingType != VoicingType::Rootless) {
        score -= 40.0f;
    }
    
    // Prefer chords with 3rd or sus interval
    bool hasThirdOrSus = false;
    for (int interval : {2, 3, 4, 5}) {  // sus2=2, m3=3, M3=4, sus4=5
        if (std::find(intervals.begin(), intervals.end(), interval) != intervals.end()) {
            hasThirdOrSus = true;
            break;
        }
    }
    if (!hasThirdOrSus) {
        score -= 25.0f;
    }
    
    return score;
}

float computeConfidence(
    float bestScore,
    float secondBestScore,
    int noteCount,
    bool exactMatch) noexcept
{
    // Score margin component
    float scoreMargin = bestScore - secondBestScore;
    float marginConfidence = std::min(scoreMargin / 100.0f, 1.0f);
    
    // Absolute score component
    float absoluteConfidence = std::min(bestScore / 250.0f, 1.0f);
    
    // Note count component (more notes = more confident)
    float noteConfidence = std::min(static_cast<float>(noteCount) / 6.0f, 1.0f);
    
    // Exact match component
    float exactConfidence = exactMatch ? 1.0f : 0.5f;
    
    // Weighted combination
    float confidence = 0.35f * marginConfidence +
                      0.25f * absoluteConfidence +
                      0.15f * noteConfidence +
                      0.25f * exactConfidence;
    
    return confidence;
}

} // namespace ChordScoring
} // namespace ChordDetection
