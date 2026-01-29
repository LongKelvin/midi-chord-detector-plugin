// VoicingAnalyzer.cpp
// Voicing classification implementation

#include "VoicingAnalyzer.h"
#include <algorithm>

namespace ChordDetection {
namespace VoicingAnalyzer {

VoicingType classifyVoicing(const std::vector<int>& midiNotes) {
    if (midiNotes.size() < 2) {
        return VoicingType::Unknown;
    }
    
    // Sort notes from low to high
    std::vector<int> sorted = midiNotes;
    std::sort(sorted.begin(), sorted.end());
    
    int span = sorted.back() - sorted.front();
    
    // Close voicing: within one octave
    if (span <= 12) {
        return VoicingType::Close;
    }
    
    // Open voicing analysis
    if (span > 12) {
        // For 4+ note chords, check for drop voicings
        if (sorted.size() >= 4) {
            std::vector<int> intervals = getAdjacentIntervals(sorted);
            
            // Drop 2: large interval at bottom (2nd voice dropped)
            if (intervals.size() >= 2 && intervals[0] > 7) {
                return VoicingType::Drop2;
            }
            
            // Drop 3: large interval at second position (3rd voice dropped)
            if (intervals.size() >= 3 && intervals[1] > 7) {
                return VoicingType::Drop3;
            }
        }
        
        return VoicingType::Open;
    }
    
    return VoicingType::Unknown;
}

int calculateSpan(const std::vector<int>& midiNotes) {
    if (midiNotes.empty()) {
        return 0;
    }
    
    auto [minIt, maxIt] = std::minmax_element(midiNotes.begin(), midiNotes.end());
    return *maxIt - *minIt;
}

bool isCloseVoicing(const std::vector<int>& midiNotes) {
    return calculateSpan(midiNotes) <= 12;
}

std::vector<int> getAdjacentIntervals(const std::vector<int>& midiNotes) {
    std::vector<int> intervals;
    
    if (midiNotes.size() < 2) {
        return intervals;
    }
    
    // Ensure sorted
    std::vector<int> sorted = midiNotes;
    std::sort(sorted.begin(), sorted.end());
    
    intervals.reserve(sorted.size() - 1);
    for (size_t i = 0; i < sorted.size() - 1; ++i) {
        intervals.push_back(sorted[i + 1] - sorted[i]);
    }
    
    return intervals;
}

} // namespace VoicingAnalyzer
} // namespace ChordDetection
