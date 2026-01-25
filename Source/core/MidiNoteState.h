#pragma once

#include <bitset>
#include <array>
#include <cstdint>

namespace ChordDetection
{

/**
 * ActiveNote - Extended note information with timing
 * 
 * As per spec: "Extend current code to store timestamps and velocity"
 */
struct ActiveNote
{
    int midiNote;           // 0-127
    int pitchClass;         // midiNote % 12
    float velocity;         // 0.0-1.0
    double noteOnTime;      // Timestamp in milliseconds
    
    ActiveNote()
        : midiNote(-1)
        , pitchClass(-1)
        , velocity(0.0f)
        , noteOnTime(0.0)
    {}
    
    ActiveNote(int note, float vel, double time)
        : midiNote(note)
        , pitchClass(note % 12)
        , velocity(vel)
        , noteOnTime(time)
    {}
    
    bool isValid() const { return midiNote >= 0 && midiNote < 128; }
};

/**
 * MidiNoteState - Real-time safe MIDI note tracking
 * 
 * Uses a bitset for O(1) note on/off operations and fixed-size arrays
 * for velocity tracking. No heap allocations, suitable for audio thread.
 * 
 * Extended with timestamps for harmonic memory system.
 * 
 * Thread safety: Should only be accessed from the audio thread.
 */
class MidiNoteState
{
public:
    MidiNoteState();
    ~MidiNoteState() = default;
    
    // Note control (with timestamps)
    void noteOn(int noteNumber, float velocity, double currentTimeMs = 0.0);
    void noteOff(int noteNumber);
    void allNotesOff();
    
    // Query methods
    bool isNoteActive(int noteNumber) const;
    int getActiveNoteCount() const;
    
    /**
     * Get all active notes as an array
     * Returns the number of active notes filled into the output array
     * maxNotes should be at least 128 for safety
     */
    int getActiveNotes(int* outNotes, int maxNotes) const;
    
    /**
     * Get all active notes with full information (including timestamps)
     * Returns the number of notes filled into the output array
     */
    int getActiveNotesDetailed(ActiveNote* outNotes, int maxNotes) const;
    
    /**
     * Get velocity of a note (0.0 if not active)
     */
    float getNoteVelocity(int noteNumber) const;
    
    /**
     * Get note-on timestamp (0.0 if not active)
     */
    double getNoteOnTime(int noteNumber) const;
    
    /**
     * Get detailed info for a specific note
     */
    ActiveNote getNoteInfo(int noteNumber) const;
    
    // Sustain pedal handling
    void setSustainPedal(bool isPressed);
    bool isSustainPedalPressed() const;
    
    /**
     * Check if a specific note is being held by sustain pedal
     * (i.e., key released but still sounding due to sustain)
     */
    bool isNoteSustained(int noteNumber) const;
    
    /**
     * Get note-on timestamp (0.0 if not active)
     * Alias for getNoteOnTime for consistency
     */
    double getNoteOnsetTime(int noteNumber) const { return getNoteOnTime(noteNumber); }
    
    /**
     * Called when sustain pedal is released
     * Removes all notes that were released while sustain was on
     */
    void processSustainRelease();
    
    /**
     * Get pitch class set of all active notes
     */
    std::bitset<12> getPitchClassSet() const;
    
    /**
     * Get the lowest active note (for bass detection)
     * Returns -1 if no notes active
     */
    int getLowestNote() const;
    
    /**
     * Get the highest active note (for melody detection)
     * Returns -1 if no notes active
     */
    int getHighestNote() const;
    
private:
    std::bitset<128> activeNotes_;
    std::bitset<128> sustainedNotesPendingRelease_;
    std::array<float, 128> velocities_;
    std::array<double, 128> noteOnTimes_;
    bool sustainPedalPressed_;
    int activeNoteCount_;
    
    void updateActiveNoteCount();
};

} // namespace ChordDetection
