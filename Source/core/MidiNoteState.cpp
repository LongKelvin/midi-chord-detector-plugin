#include "MidiNoteState.h"
#include <algorithm>

namespace ChordDetection
{

MidiNoteState::MidiNoteState()
    : sustainPedalPressed_(false)
    , activeNoteCount_(0)
{
    activeNotes_.reset();
    sustainedNotesPendingRelease_.reset();
    velocities_.fill(0.0f);
}

void MidiNoteState::noteOn(int noteNumber, float velocity)
{
    if (noteNumber < 0 || noteNumber >= 128)
        return;
    
    if (!activeNotes_[noteNumber])
    {
        activeNotes_[noteNumber] = true;
        ++activeNoteCount_;
    }
    
    velocities_[noteNumber] = velocity;
    
    // If this note was pending release from sustain, remove it from that set
    sustainedNotesPendingRelease_[noteNumber] = false;
}

void MidiNoteState::noteOff(int noteNumber)
{
    if (noteNumber < 0 || noteNumber >= 128)
        return;
    
    if (sustainPedalPressed_)
    {
        // Mark for release when sustain pedal lifts
        if (activeNotes_[noteNumber])
        {
            sustainedNotesPendingRelease_[noteNumber] = true;
        }
    }
    else
    {
        // Immediate release
        if (activeNotes_[noteNumber])
        {
            activeNotes_[noteNumber] = false;
            --activeNoteCount_;
        }
        velocities_[noteNumber] = 0.0f;
        sustainedNotesPendingRelease_[noteNumber] = false;
    }
}

void MidiNoteState::allNotesOff()
{
    activeNotes_.reset();
    sustainedNotesPendingRelease_.reset();
    velocities_.fill(0.0f);
    activeNoteCount_ = 0;
}

bool MidiNoteState::isNoteActive(int noteNumber) const
{
    if (noteNumber < 0 || noteNumber >= 128)
        return false;
    return activeNotes_[noteNumber];
}

int MidiNoteState::getActiveNoteCount() const
{
    return activeNoteCount_;
}

int MidiNoteState::getActiveNotes(int* outNotes, int maxNotes) const
{
    int count = 0;
    for (int i = 0; i < 128 && count < maxNotes; ++i)
    {
        if (activeNotes_[i])
        {
            outNotes[count++] = i;
        }
    }
    return count;
}

float MidiNoteState::getNoteVelocity(int noteNumber) const
{
    if (noteNumber < 0 || noteNumber >= 128)
        return 0.0f;
    return velocities_[noteNumber];
}

void MidiNoteState::setSustainPedal(bool isPressed)
{
    bool wasPressed = sustainPedalPressed_;
    sustainPedalPressed_ = isPressed;
    
    // If pedal was just released, process pending note releases
    if (wasPressed && !isPressed)
    {
        processSustainRelease();
    }
}

bool MidiNoteState::isSustainPedalPressed() const
{
    return sustainPedalPressed_;
}

void MidiNoteState::processSustainRelease()
{
    for (int i = 0; i < 128; ++i)
    {
        if (sustainedNotesPendingRelease_[i])
        {
            if (activeNotes_[i])
            {
                activeNotes_[i] = false;
                --activeNoteCount_;
            }
            velocities_[i] = 0.0f;
            sustainedNotesPendingRelease_[i] = false;
        }
    }
}

} // namespace ChordDetection
