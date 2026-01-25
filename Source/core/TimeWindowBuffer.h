#pragma once

#include "ChordTypes.h"
#include <array>
#include <cstdint>

namespace ChordDetection
{

/**
 * TimeWindowBuffer - Tracks notes played within a time window
 * 
 * Implements a circular buffer for efficient rolled chord detection.
 * Keeps the last N note events with timestamps.
 * Real-time safe - no allocations, fixed size.
 */
class TimeWindowBuffer
{
public:
    struct TimedNote
    {
        int8_t midiNote;
        double timestamp;  // milliseconds
        bool isActive;
        
        TimedNote() : midiNote(-1), timestamp(0.0), isActive(false) {}
    };
    
    TimeWindowBuffer();
    ~TimeWindowBuffer() = default;
    
    /**
     * Add a note event to the buffer
     */
    void addNote(int midiNote, double currentTimeMs);
    
    /**
     * Remove a note from the buffer
     */
    void removeNote(int midiNote);
    
    /**
     * Get all notes within the time window
     * Returns count of notes added to outNotes array
     */
    int getNotesInWindow(double currentTimeMs, double windowMs, 
                        int* outNotes, int maxNotes) const;
    
    /**
     * Clear all notes
     */
    void clear();
    
    /**
     * Set the time window size in milliseconds
     */
    void setWindowSize(double windowMs);
    
    double getWindowSize() const { return windowSizeMs_; }
    
private:
    std::array<TimedNote, MAX_TIME_WINDOW_NOTES> buffer_;
    int writeIndex_;
    double windowSizeMs_;
    
    void removeOldNotes(double currentTimeMs);
};

} // namespace ChordDetection
