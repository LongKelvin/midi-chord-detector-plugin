// ChordPatterns.cpp
// Chord pattern database implementation

#include "ChordPatterns.h"
#include <algorithm>

namespace ChordDetection {
namespace ChordPatterns {

void initializePatterns(std::map<std::string, ChordPattern>& patterns) {
    patterns.clear();
    
    // ========================================================================
    // TRIADS
    // ========================================================================
    
    patterns["major"] = ChordPattern(
        {0, 4, 7}, 100, {0, 4, 7}, {}, {4, 7}, "{root}", "major"
    );
    
    patterns["minor"] = ChordPattern(
        {0, 3, 7}, 100, {0, 3, 7}, {}, {3, 7}, "{root}m", "minor"
    );
    
    patterns["diminished"] = ChordPattern(
        {0, 3, 6}, 100, {0, 3, 6}, {}, {3, 6}, "{root}dim", "diminished"
    );
    
    patterns["augmented"] = ChordPattern(
        {0, 4, 8}, 100, {0, 4, 8}, {}, {4, 8}, "{root}aug", "augmented"
    );
    
    patterns["sus2"] = ChordPattern(
        {0, 2, 7}, 95, {0, 2, 7}, {}, {2, 7}, "{root}sus2", "suspended"
    );
    
    patterns["sus4"] = ChordPattern(
        {0, 5, 7}, 95, {0, 5, 7}, {}, {5, 7}, "{root}sus4", "suspended"
    );
    
    patterns["power5"] = ChordPattern(
        {0, 7}, 80, {0, 7}, {}, {7}, "{root}5", "power"
    );
    
    // ========================================================================
    // SEVENTH CHORDS
    // ========================================================================
    
    patterns["major7"] = ChordPattern(
        {0, 4, 7, 11}, 115, {0, 4, 11}, {7}, {4, 11}, "{root}maj7", "major"
    );
    
    patterns["minor7"] = ChordPattern(
        {0, 3, 7, 10}, 115, {0, 3, 10}, {7}, {3, 10}, "{root}m7", "minor"
    );
    
    patterns["dominant7"] = ChordPattern(
        {0, 4, 7, 10}, 115, {0, 4, 10}, {7}, {4, 10}, "{root}7", "dominant"
    );
    
    patterns["diminished7"] = ChordPattern(
        {0, 3, 6, 9}, 115, {0, 3, 6, 9}, {}, {3, 6, 9}, "{root}dim7", "diminished"
    );
    
    patterns["half-diminished7"] = ChordPattern(
        {0, 3, 6, 10}, 115, {0, 3, 6, 10}, {}, {3, 6, 10}, "{root}m7♭5", "half-diminished"
    );
    
    patterns["augmented7"] = ChordPattern(
        {0, 4, 8, 10}, 110, {0, 4, 8, 10}, {}, {4, 8, 10}, "{root}aug7", "augmented"
    );
    
    patterns["augmented-major7"] = ChordPattern(
        {0, 4, 8, 11}, 110, {0, 4, 8, 11}, {}, {4, 8, 11}, "{root}+maj7", "augmented"
    );
    
    patterns["minor-major7"] = ChordPattern(
        {0, 3, 7, 11}, 110, {0, 3, 11}, {7}, {3, 11}, "{root}m(maj7)", "minor"
    );
    
    patterns["7sus4"] = ChordPattern(
        {0, 5, 7, 10}, 108, {0, 5, 10}, {7}, {5, 10}, "{root}7sus4", "suspended"
    );
    
    // ========================================================================
    // SIXTH CHORDS
    // ========================================================================
    
    patterns["major6"] = ChordPattern(
        {0, 4, 7, 9}, 105, {0, 4, 9}, {7}, {4, 9}, "{root}6", "major"
    );
    
    patterns["minor6"] = ChordPattern(
        {0, 3, 7, 9}, 105, {0, 3, 9}, {7}, {3, 9}, "{root}m6", "minor"
    );
    
    patterns["6/9"] = ChordPattern(
        {0, 4, 7, 9, 14}, 110, {0, 4, 9, 14}, {7}, {4, 9, 14}, "{root}6/9", "major"
    );
    
    patterns["minor6/9"] = ChordPattern(
        {0, 3, 7, 9, 14}, 110, {0, 3, 9, 14}, {7}, {3, 9, 14}, "{root}m6/9", "minor"
    );
    
    // ========================================================================
    // NINTH CHORDS
    // ========================================================================
    
    patterns["major9"] = ChordPattern(
        {0, 4, 7, 11, 14}, 125, {0, 4, 11, 14}, {7}, {4, 11, 14}, "{root}maj9", "major"
    );
    
    patterns["minor9"] = ChordPattern(
        {0, 3, 7, 10, 14}, 125, {0, 3, 10, 14}, {7}, {3, 10, 14}, "{root}m9", "minor"
    );
    
    patterns["dominant9"] = ChordPattern(
        {0, 4, 7, 10, 14}, 125, {0, 4, 10, 14}, {7}, {4, 10, 14}, "{root}9", "dominant"
    );
    
    patterns["dominant7b9"] = ChordPattern(
        {0, 4, 7, 10, 13}, 120, {0, 4, 10, 13}, {7}, {4, 10, 13}, "{root}7♭9", "dominant"
    );
    
    patterns["dominant7#9"] = ChordPattern(
        {0, 4, 7, 10, 15}, 120, {0, 4, 10, 15}, {7}, {4, 10, 15}, "{root}7♯9", "dominant"
    );
    
    patterns["minor-major9"] = ChordPattern(
        {0, 3, 7, 11, 14}, 120, {0, 3, 11, 14}, {7}, {3, 11, 14}, "{root}m(maj9)", "minor"
    );
    
    // ========================================================================
    // ELEVENTH CHORDS
    // ========================================================================
    
    patterns["major11"] = ChordPattern(
        {0, 4, 7, 11, 14, 17}, 130, {0, 4, 11, 14, 17}, {7}, {4, 11, 14, 17}, "{root}maj11", "major"
    );
    
    patterns["minor11"] = ChordPattern(
        {0, 3, 7, 10, 14, 17}, 130, {0, 3, 10, 14, 17}, {7}, {3, 10, 14, 17}, "{root}m11", "minor"
    );
    
    patterns["dominant11"] = ChordPattern(
        {0, 4, 7, 10, 14, 17}, 130, {0, 4, 10, 14, 17}, {7}, {4, 10, 14, 17}, "{root}11", "dominant"
    );
    
    patterns["dominant7#11"] = ChordPattern(
        {0, 4, 7, 10, 18}, 125, {0, 4, 10, 18}, {7}, {4, 10, 18}, "{root}7♯11", "dominant"
    );
    
    patterns["major7#11"] = ChordPattern(
        {0, 4, 7, 11, 18}, 125, {0, 4, 11, 18}, {7}, {4, 11, 18}, "{root}maj7♯11", "major"
    );
    
    patterns["major9#11"] = ChordPattern(
        {0, 4, 7, 11, 14, 18}, 130, {0, 4, 11, 14, 18}, {7}, {4, 11, 14, 18}, "{root}maj9♯11", "major"
    );
    
    patterns["minor11b5"] = ChordPattern(
        {0, 3, 6, 10, 14, 17}, 125, {0, 3, 6, 10, 14, 17}, {}, {3, 6, 10, 14, 17}, "{root}m11♭5", "half-diminished"
    );
    
    // ========================================================================
    // THIRTEENTH CHORDS
    // ========================================================================
    
    patterns["major13"] = ChordPattern(
        {0, 4, 7, 11, 14, 21}, 135, {0, 4, 11, 21}, {7, 14}, {4, 11, 21}, "{root}maj13", "major"
    );
    
    patterns["minor13"] = ChordPattern(
        {0, 3, 7, 10, 14, 21}, 135, {0, 3, 10, 21}, {7, 14}, {3, 10, 21}, "{root}m13", "minor"
    );
    
    patterns["dominant13"] = ChordPattern(
        {0, 4, 7, 10, 14, 21}, 135, {0, 4, 10, 21}, {7, 14}, {4, 10, 21}, "{root}13", "dominant"
    );
    
    patterns["dominant13#11"] = ChordPattern(
        {0, 4, 7, 10, 18, 21}, 135, {0, 4, 10, 18, 21}, {7}, {4, 10, 18, 21}, "{root}13♯11", "dominant"
    );
    
    patterns["dominant7b13"] = ChordPattern(
        {0, 4, 7, 10, 20}, 125, {0, 4, 10, 20}, {7}, {4, 10, 20}, "{root}7♭13", "dominant"
    );
    
    patterns["dominant13b9"] = ChordPattern(
        {0, 4, 7, 10, 13, 21}, 130, {0, 4, 10, 13, 21}, {7}, {4, 10, 13, 21}, "{root}13♭9", "dominant"
    );
    
    patterns["dominant13#9"] = ChordPattern(
        {0, 4, 7, 10, 15, 21}, 130, {0, 4, 10, 15, 21}, {7}, {4, 10, 15, 21}, "{root}13♯9", "dominant"
    );
    
    // ========================================================================
    // ALTERED DOMINANTS
    // ========================================================================
    
    patterns["dominant7b5"] = ChordPattern(
        {0, 4, 6, 10}, 118, {0, 4, 6, 10}, {}, {4, 6, 10}, "{root}7♭5", "dominant"
    );
    
    patterns["dominant7#5"] = ChordPattern(
        {0, 4, 8, 10}, 118, {0, 4, 8, 10}, {}, {4, 8, 10}, "{root}7♯5", "dominant"
    );
    
    patterns["dominant7b5b9"] = ChordPattern(
        {0, 4, 6, 10, 13}, 122, {0, 4, 6, 10, 13}, {}, {4, 6, 10, 13}, "{root}7♭5♭9", "dominant"
    );
    
    patterns["dominant7#5b9"] = ChordPattern(
        {0, 4, 8, 10, 13}, 122, {0, 4, 8, 10, 13}, {}, {4, 8, 10, 13}, "{root}7♯5♭9", "dominant"
    );
    
    patterns["dominant7b5#9"] = ChordPattern(
        {0, 4, 6, 10, 15}, 122, {0, 4, 6, 10, 15}, {}, {4, 6, 10, 15}, "{root}7♭5♯9", "dominant"
    );
    
    patterns["dominant7#5#9"] = ChordPattern(
        {0, 4, 8, 10, 15}, 122, {0, 4, 8, 10, 15}, {}, {4, 8, 10, 15}, "{root}7♯5♯9", "dominant"
    );
    
    patterns["altered"] = ChordPattern(
        {0, 4, 6, 10, 13}, 120, {0, 4, 10}, {6, 8, 13, 15}, {4, 10}, "{root}7alt", "dominant"
    );
    
    patterns["dominant7#5#9b13"] = ChordPattern(
        {0, 4, 8, 10, 15, 20}, 128, {0, 4, 8, 10, 15, 20}, {}, {4, 8, 10, 15, 20}, "{root}7♯5♯9♭13", "dominant"
    );
    
    patterns["dominant9#11"] = ChordPattern(
        {0, 4, 7, 10, 14, 18}, 130, {0, 4, 10, 14, 18}, {7}, {4, 10, 14, 18}, "{root}9♯11", "dominant"
    );
    
    patterns["dominant9b13"] = ChordPattern(
        {0, 4, 7, 10, 14, 20}, 130, {0, 4, 10, 14, 20}, {7}, {4, 10, 14, 20}, "{root}9♭13", "dominant"
    );
    
    patterns["dominant7#9#11"] = ChordPattern(
        {0, 4, 7, 10, 15, 18}, 128, {0, 4, 10, 15, 18}, {7}, {4, 10, 15, 18}, "{root}7♯9♯11", "dominant"
    );
    
    patterns["dominant7b9#11"] = ChordPattern(
        {0, 4, 7, 10, 13, 18}, 128, {0, 4, 10, 13, 18}, {7}, {4, 10, 13, 18}, "{root}7♭9♯11", "dominant"
    );
    
    patterns["dominant7b9b13"] = ChordPattern(
        {0, 4, 7, 10, 13, 20}, 128, {0, 4, 10, 13, 20}, {7}, {4, 10, 13, 20}, "{root}7♭9♭13", "dominant"
    );
    
    patterns["dominant7#9b13"] = ChordPattern(
        {0, 4, 7, 10, 15, 20}, 128, {0, 4, 10, 15, 20}, {7}, {4, 10, 15, 20}, "{root}7♯9♭13", "dominant"
    );
    
    // ========================================================================
    // ADD CHORDS
    // ========================================================================
    
    patterns["add9"] = ChordPattern(
        {0, 4, 7, 14}, 105, {0, 4, 7, 14}, {}, {4, 7, 14}, "{root}add9", "major"
    );
    
    patterns["minor-add9"] = ChordPattern(
        {0, 3, 7, 14}, 105, {0, 3, 7, 14}, {}, {3, 7, 14}, "{root}m(add9)", "minor"
    );
    
    patterns["add11"] = ChordPattern(
        {0, 4, 7, 17}, 100, {0, 4, 7, 17}, {}, {4, 7, 17}, "{root}add11", "major"
    );
    
    patterns["add#11"] = ChordPattern(
        {0, 4, 7, 18}, 100, {0, 4, 7, 18}, {}, {4, 7, 18}, "{root}add♯11", "major"
    );
    
    // ========================================================================
    // SPECIAL CHORDS
    // ========================================================================
    
    patterns["major7#5"] = ChordPattern(
        {0, 4, 8, 11}, 115, {0, 4, 8, 11}, {}, {4, 8, 11}, "{root}maj7♯5", "augmented"
    );
    
    patterns["minor7b5"] = ChordPattern(
        {0, 3, 6, 10}, 115, {0, 3, 6, 10}, {}, {3, 6, 10}, "{root}m7♭5", "half-diminished"
    );
    
    patterns["quartal"] = ChordPattern(
        {0, 5, 10}, 90, {0, 5, 10}, {}, {5, 10}, "{root}quartal", "quartal"
    );
    
    patterns["quartal-7"] = ChordPattern(
        {0, 5, 10, 15}, 95, {0, 5, 10, 15}, {}, {5, 10, 15}, "{root}quartal7", "quartal"
    );
}

void buildIntervalIndex(
    const std::map<std::string, ChordPattern>& patterns,
    std::map<std::vector<int>, std::vector<std::string>>& index)
{
    index.clear();
    
    for (const auto& [chordType, pattern] : patterns) {
        // Create sorted, unique interval signature
        std::vector<int> signature = pattern.intervals;
        std::sort(signature.begin(), signature.end());
        
        auto last = std::unique(signature.begin(), signature.end());
        signature.erase(last, signature.end());
        
        // Add to index
        index[signature].push_back(chordType);
    }
}

} // namespace ChordPatterns
} // namespace ChordDetection
