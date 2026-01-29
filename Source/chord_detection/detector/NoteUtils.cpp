// NoteUtils.cpp
// Note name utilities implementation

#include "NoteUtils.h"
#include <algorithm>
#include <regex>
#include <stdexcept>

namespace ChordDetection {
namespace NoteUtils {

int parseNoteString(const std::string& noteStr) {
    std::string processed = noteStr;
    
    // Convert to uppercase
    std::transform(processed.begin(), processed.end(), processed.begin(),
        [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    
    // Replace ASCII accidentals with Unicode
    for (const auto& [oldStr, newStr] : kEnharmonicMap) {
        std::string oldUpper = oldStr;
        std::transform(oldUpper.begin(), oldUpper.end(), oldUpper.begin(),
            [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        
        size_t pos = 0;
        while ((pos = processed.find(oldUpper, pos)) != std::string::npos) {
            processed.replace(pos, oldUpper.length(), newStr);
            pos += newStr.length();
        }
    }
    
    // Parse note name and optional octave using regex
    std::regex noteRegex("([A-G][♯♭#b]?)(\\d?)");
    std::smatch match;
    
    if (!std::regex_match(processed, match, noteRegex)) {
        throw std::runtime_error("Invalid note format: " + noteStr);
    }
    
    std::string noteName = match[1].str();
    int octave = match[2].str().empty() ? 4 : std::stoi(match[2].str());
    
    // Replace remaining ASCII accidentals
    size_t pos = noteName.find('#');
    if (pos != std::string::npos) {
        noteName.replace(pos, 1, "♯");
    }
    pos = noteName.find('b');
    if (pos != std::string::npos) {
        noteName.replace(pos, 1, "♭");
    }
    
    // Find pitch class by matching note name
    int pitchClass = -1;
    
    // Try sharp names first
    for (size_t i = 0; i < kNoteNamesSharp.size(); ++i) {
        if (kNoteNamesSharp[i] == noteName) {
            pitchClass = static_cast<int>(i);
            break;
        }
    }
    
    // Try flat names if not found
    if (pitchClass < 0) {
        for (size_t i = 0; i < kNoteNamesFlat.size(); ++i) {
            if (kNoteNamesFlat[i] == noteName) {
                pitchClass = static_cast<int>(i);
                break;
            }
        }
    }
    
    if (pitchClass < 0) {
        throw std::runtime_error("Unknown note: " + noteName);
    }
    
    // Calculate MIDI number (C4 = 60)
    int midi = pitchClass + (octave + 1) * kOctaveSize;
    
    if (midi < kMinMidiNote || midi > kMaxMidiNote) {
        throw std::runtime_error("MIDI note out of range: " + std::to_string(midi));
    }
    
    return midi;
}

} // namespace NoteUtils
} // namespace ChordDetection
