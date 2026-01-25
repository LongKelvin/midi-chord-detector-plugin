#pragma once

#include <bitset>
#include <array>
#include <cstdint>

namespace ChordDetection
{

/**
 * MidiNoteState - Real-time safe MIDI note tracking
 * 
 * Uses a bitset for O(1) note on/off operations and fixed-size arrays
 * for velocity tracking. No heap allocations, suitable for audio thread.
 * 
 * Thread safety: Should only be accessed from the audio thread.
 */
class MidiNoteState
{
public:
    MidiNoteState();
    ~MidiNoteState() = default;
    
    // Note control
    void noteOn(int noteNumber, float velocity);
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
     * Get velocity of a note (0.0 if not active)
     */
    float getNoteVelocity(int noteNumber) const;
    
    // Sustain pedal handling
    void setSustainPedal(bool isPressed);
    bool isSustainPedalPressed() const;
    
    /**
     * Called when sustain pedal is released
     * Removes all notes that were released while sustain was on
     */
    void processSustainRelease();
    
private:
    std::bitset<128> activeNotes_;
    std::bitset<128> sustainedNotesPendingRelease_;
    std::array<float, 128> velocities_;
    bool sustainPedalPressed_;
    int activeNoteCount_;
    
    void updateActiveNoteCount();
};

} // namespace ChordDetection
