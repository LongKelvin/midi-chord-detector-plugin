#include "TimeWindowBuffer.h"
#include <algorithm>

namespace ChordDetection
{

TimeWindowBuffer::TimeWindowBuffer()
    : writeIndex_(0)
    , windowSizeMs_(DEFAULT_TIME_WINDOW_MS)
{
    for (auto& note : buffer_)
    {
        note = TimedNote();
    }
}

void TimeWindowBuffer::addNote(int midiNote, double currentTimeMs)
{
    if (midiNote < 0 || midiNote >= 128)
        return;
    
    // Check if note already exists and update it
    for (auto& note : buffer_)
    {
        if (note.isActive && note.midiNote == midiNote)
        {
            note.timestamp = currentTimeMs;
            return;
        }
    }
    
    // Find an inactive slot or overwrite oldest
    int targetIndex = -1;
    
    // First, try to find an inactive slot
    for (int i = 0; i < MAX_TIME_WINDOW_NOTES; ++i)
    {
        if (!buffer_[i].isActive)
        {
            targetIndex = i;
            break;
        }
    }
    
    // If no inactive slot, use circular write index
    if (targetIndex == -1)
    {
        targetIndex = writeIndex_;
        writeIndex_ = (writeIndex_ + 1) % MAX_TIME_WINDOW_NOTES;
    }
    
    // Add the note
    buffer_[targetIndex].midiNote = static_cast<int8_t>(midiNote);
    buffer_[targetIndex].timestamp = currentTimeMs;
    buffer_[targetIndex].isActive = true;
}

void TimeWindowBuffer::removeNote(int midiNote)
{
    if (midiNote < 0 || midiNote >= 128)
        return;
    
    for (auto& note : buffer_)
    {
        if (note.isActive && note.midiNote == midiNote)
        {
            note.isActive = false;
            break;
        }
    }
}

int TimeWindowBuffer::getNotesInWindow(double currentTimeMs, double windowMs,
                                      int* outNotes, int maxNotes) const
{
    if (!outNotes || maxNotes <= 0)
        return 0;
    
    int count = 0;
    const double cutoffTime = currentTimeMs - windowMs;
    
    for (const auto& note : buffer_)
    {
        if (note.isActive && note.timestamp >= cutoffTime)
        {
            // Check if not already in output (avoid duplicates)
            bool alreadyAdded = false;
            for (int i = 0; i < count; ++i)
            {
                if (outNotes[i] == note.midiNote)
                {
                    alreadyAdded = true;
                    break;
                }
            }
            
            if (!alreadyAdded && count < maxNotes)
            {
                outNotes[count++] = note.midiNote;
            }
        }
    }
    
    return count;
}

void TimeWindowBuffer::clear()
{
    for (auto& note : buffer_)
    {
        note.isActive = false;
    }
    writeIndex_ = 0;
}

void TimeWindowBuffer::setWindowSize(double windowMs)
{
    if (windowMs > 0.0 && windowMs <= 1000.0) // Clamp to reasonable range
    {
        windowSizeMs_ = windowMs;
    }
}

void TimeWindowBuffer::removeOldNotes(double currentTimeMs)
{
    const double cutoffTime = currentTimeMs - windowSizeMs_;
    
    for (auto& note : buffer_)
    {
        if (note.isActive && note.timestamp < cutoffTime)
        {
            note.isActive = false;
        }
    }
}

} // namespace ChordDetection
