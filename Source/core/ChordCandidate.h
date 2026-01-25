#pragma once

#include "ChordTypes.h"
#include <cstdint>

namespace ChordDetection
{

/**
 * ChordCandidate - Represents a detected chord result
 * 
 * This structure is designed for atomic pointer swapping between
 * audio and UI threads. Keep it small and POD-like.
 */
struct ChordCandidate
{
    // Chord identification
    int rootNote;           // 0-11 (pitch class)
    int bassNote;           // 0-11 (pitch class)
    int chordTypeIndex;     // Index into CHORD_DEFINITIONS array
    
    // Scoring and confidence
    float score;            // Weighted match score
    float confidence;       // 0.0 - 1.0
    
    // Inversion info
    int8_t inversionType;   // 0=root, 1=1st, 2=2nd, 3=3rd, -1=slash
    int8_t bassIntervalFromRoot; // Semitones from root to bass
    
    // Playback info
    int8_t activeNoteCount;
    int8_t lowestMidiNote;
    int8_t highestMidiNote;
    
    // Voice separation (note counts)
    int8_t bassNoteCount;
    int8_t chordNoteCount;
    int8_t melodyNoteCount;
    
    // Flags
    bool isValid;
    bool hasOptionalIntervals;
    
    // Timing
    double timestamp;       // For UI refresh throttling
    
    ChordCandidate()
        : rootNote(0)
        , bassNote(0)
        , chordTypeIndex(-1)
        , score(0.0f)
        , confidence(0.0f)
        , inversionType(0)
        , bassIntervalFromRoot(0)
        , activeNoteCount(0)
        , lowestMidiNote(0)
        , highestMidiNote(0)
        , bassNoteCount(0)
        , chordNoteCount(0)
        , melodyNoteCount(0)
        , isValid(false)
        , hasOptionalIntervals(false)
        , timestamp(0.0)
    {}
    
    /**
     * Get full chord name (e.g., "Cmaj7/E")
     */
    void getChordName(char* buffer, int bufferSize) const
    {
        if (!isValid || chordTypeIndex < 0)
        {
            snprintf(buffer, bufferSize, "N.C.");
            return;
        }
        
        const ChordDefinition& def = CHORD_DEFINITIONS[chordTypeIndex];
        const char* rootName = NOTE_NAMES[rootNote];
        
        if (bassIntervalFromRoot == 0)
        {
            // Root position
            snprintf(buffer, bufferSize, "%s%s", rootName, def.symbol);
        }
        else
        {
            // Slash chord
            const char* bassName = NOTE_NAMES[bassNote];
            snprintf(buffer, bufferSize, "%s%s/%s", rootName, def.symbol, bassName);
        }
    }
    
    /**
     * Get chord description (full name)
     */
    const char* getChordDescription() const
    {
        if (!isValid || chordTypeIndex < 0)
            return "No Chord";
        
        return CHORD_DEFINITIONS[chordTypeIndex].fullName;
    }
    
    /**
     * Get inversion text
     */
    const char* getInversionText() const
    {
        if (!isValid)
            return "N/A";
        
        switch (inversionType)
        {
            case 0: return "Root Position";
            case 1: return "1st Inversion";
            case 2: return "2nd Inversion";
            case 3: return "3rd Inversion";
            case -1: return "Slash Chord";
            default: return "Unknown";
        }
    }
};

/**
 * VoiceBuckets - Temporary structure for voice separation
 * Used during chord detection, not stored.
 */
struct VoiceBuckets
{
    std::array<int8_t, 8> bassNotes;
    int bassCount;
    
    std::array<int8_t, 16> chordNotes;
    int chordCount;
    
    std::array<int8_t, 8> melodyNotes;
    int melodyCount;
    
    VoiceBuckets()
        : bassCount(0), chordCount(0), melodyCount(0)
    {
        bassNotes.fill(-1);
        chordNotes.fill(-1);
        melodyNotes.fill(-1);
    }
    
    void clear()
    {
        bassCount = 0;
        chordCount = 0;
        melodyCount = 0;
    }
};

} // namespace ChordDetection
