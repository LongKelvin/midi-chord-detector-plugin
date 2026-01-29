// ChordFormatter.cpp
// Chord result formatting implementation

#include "ChordFormatter.h"
#include "ChordTypes.h"
#include <sstream>
#include <iomanip>

namespace ChordDetection {
namespace ChordFormatter {

std::string formatDetailed(
    const std::shared_ptr<ChordCandidate>& result,
    const std::map<std::string, ChordPattern>& patterns)
{
    if (!result) {
        return "No chord detected";
    }
    
    std::stringstream ss;
    
    const std::string separator(70, '=');
    
    ss << "\n" << separator << "\n";
    ss << "CHORD DETECTION RESULT\n";
    ss << separator << "\n";
    
    // Chord name and confidence
    ss << "\nCHORD: " << result->chordName << "\n";
    ss << formatConfidenceIndicator(result->confidence)
       << " Confidence: " << std::fixed << std::setprecision(1)
       << (result->confidence * 100.0f) << "% (Score: "
       << std::fixed << std::setprecision(0) << result->score << ")\n";
    
    // Position and root
    ss << "Position: " << result->position << "\n";
    ss << "Root: " << result->rootName << "\n";
    
    // Quality (from pattern database)
    auto patternIt = patterns.find(result->chordType);
    if (patternIt != patterns.end()) {
        ss << "Quality: " << patternIt->second.quality << "\n";
    }
    
    // Voicing type
    ss << "Voicing: " << voicingTypeToString(result->voicingType) << "\n";
    
    // MIDI notes
    ss << "\nMIDI Notes: " << formatNoteList(result->noteNumbers) << "\n";
    
    // Note names
    ss << "Note Names: " << formatNoteList(result->noteNames) << "\n";
    
    // Intervals
    ss << "Intervals: " << formatNoteList(result->intervals) << "\n";
    
    // Degrees
    ss << "Degrees: " << formatNoteList(result->degrees) << "\n";
    
    ss << separator << "\n";
    
    return ss.str();
}

std::string formatChordName(const std::shared_ptr<ChordCandidate>& result) {
    if (!result || result->chordName.empty()) {
        return "N/A";
    }
    return result->chordName;
}

std::string formatConfidenceIndicator(float confidence) {
    if (confidence > 0.8f) {
        return "[HIGH]";
    } else if (confidence > 0.6f) {
        return "[MED]";
    }
    return "[LOW]";
}

} // namespace ChordFormatter
} // namespace ChordDetection
