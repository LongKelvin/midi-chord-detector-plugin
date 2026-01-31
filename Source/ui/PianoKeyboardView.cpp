/*
  ==============================================================================

    PianoKeyboardView.cpp
    
    Implementation of the visual piano keyboard component.

  ==============================================================================
*/

#include "PianoKeyboardView.h"

//==============================================================================
// Construction / Destruction
//==============================================================================

PianoKeyboardView::PianoKeyboardView()
{
    // Initialize all key states to false (not pressed)
    for (auto& state : keyStates_)
    {
        state.store(false, std::memory_order_relaxed);
    }
    
    // Start timer for checking state changes
    startTimerHz(60); // 60 fps refresh for smooth updates
}

PianoKeyboardView::~PianoKeyboardView()
{
    stopTimer();
}

//==============================================================================
// Key Range Configuration
//==============================================================================

void PianoKeyboardView::setKeyRange(int lowestNote, int highestNote)
{
    // Clamp to valid MIDI range
    lowestNote_ = juce::jlimit(0, 127, lowestNote);
    highestNote_ = juce::jlimit(0, 127, highestNote);
    
    // Ensure lowest <= highest
    if (lowestNote_ > highestNote_)
        std::swap(lowestNote_, highestNote_);
    
    // Recalculate geometry
    calculateKeyGeometry();
    repaint();
}

//==============================================================================
// Key State Control
//==============================================================================

void PianoKeyboardView::setKeyState(int midiNoteNumber, bool isPressed) noexcept
{
    if (midiNoteNumber >= 0 && midiNoteNumber < kTotalMidiNotes)
    {
        keyStates_[static_cast<size_t>(midiNoteNumber)].store(isPressed, std::memory_order_release);
        stateChanged_.store(true, std::memory_order_release);
    }
}

void PianoKeyboardView::clearAllKeys() noexcept
{
    for (auto& state : keyStates_)
    {
        state.store(false, std::memory_order_relaxed);
    }
    // Memory fence to ensure all stores are visible
    std::atomic_thread_fence(std::memory_order_release);
    stateChanged_.store(true, std::memory_order_release);
}

bool PianoKeyboardView::isKeyPressed(int midiNoteNumber) const noexcept
{
    if (midiNoteNumber >= 0 && midiNoteNumber < kTotalMidiNotes)
    {
        return keyStates_[static_cast<size_t>(midiNoteNumber)].load(std::memory_order_acquire);
    }
    return false;
}

//==============================================================================
// Appearance Configuration
//==============================================================================

void PianoKeyboardView::setPressedWhiteKeyColour(juce::Colour colour)
{
    pressedWhiteKeyColour_ = colour;
    repaint();
}

void PianoKeyboardView::setPressedBlackKeyColour(juce::Colour colour)
{
    pressedBlackKeyColour_ = colour;
    repaint();
}

void PianoKeyboardView::setWhiteKeyColour(juce::Colour colour)
{
    whiteKeyColour_ = colour;
    repaint();
}

void PianoKeyboardView::setBlackKeyColour(juce::Colour colour)
{
    blackKeyColour_ = colour;
    repaint();
}

void PianoKeyboardView::setKeyOutlineColour(juce::Colour colour)
{
    keyOutlineColour_ = colour;
    repaint();
}

//==============================================================================
// Component Overrides
//==============================================================================

void PianoKeyboardView::paint(juce::Graphics& g)
{
    // Draw background
    g.fillAll(juce::Colour(0xff1a1a1a));
    
    // Draw white keys first (they're behind black keys)
    drawWhiteKeys(g);
    
    // Draw black keys on top
    drawBlackKeys(g);
}

void PianoKeyboardView::resized()
{
    calculateKeyGeometry();
}

//==============================================================================
// Timer Callback
//==============================================================================

void PianoKeyboardView::timerCallback()
{
    // Only repaint if state has changed
    if (stateChanged_.exchange(false, std::memory_order_acquire))
    {
        repaint();
    }
}

//==============================================================================
// Internal Methods
//==============================================================================

void PianoKeyboardView::calculateKeyGeometry()
{
    whiteKeyGeometry_.clear();
    blackKeyGeometry_.clear();
    
    if (getWidth() <= 0 || getHeight() <= 0)
        return;
    
    // Count white keys in the range
    int whiteKeyCount = countWhiteKeysInRange();
    if (whiteKeyCount <= 0)
        return;
    
    // Calculate dimensions
    float keyboardWidth = static_cast<float>(getWidth());
    float keyboardHeight = static_cast<float>(getHeight());
    
    float whiteKeyWidth = keyboardWidth / static_cast<float>(whiteKeyCount);
    float blackKeyWidth = whiteKeyWidth * 0.65f;
    float blackKeyHeight = keyboardHeight * 0.62f;
    
    // Reserve space for efficiency
    whiteKeyGeometry_.reserve(static_cast<size_t>(whiteKeyCount));
    blackKeyGeometry_.reserve(static_cast<size_t>(whiteKeyCount)); // Upper bound
    
    // Calculate geometry for each key in range
    int whiteKeyIndex = 0;
    
    for (int note = lowestNote_; note <= highestNote_; ++note)
    {
        if (isBlackKey(note))
        {
            // Black key - positioned relative to previous white key
            float xPos = static_cast<float>(whiteKeyIndex) * whiteKeyWidth - (blackKeyWidth * 0.5f);
            
            // Adjust position based on which black key it is within the octave
            // Black keys are not evenly spaced - C#/Db and D#/Eb are closer together,
            // F#/Gb, G#/Ab, and A#/Bb are closer together
            int noteInOctave = note % 12;
            float offset = 0.0f;
            
            // Fine-tune black key positions for realistic appearance
            switch (noteInOctave)
            {
                case 1:  offset = -0.08f; break;  // C#
                case 3:  offset =  0.08f; break;  // D#
                case 6:  offset = -0.10f; break;  // F#
                case 8:  offset =  0.00f; break;  // G#
                case 10: offset =  0.10f; break;  // A#
                default: break;
            }
            
            xPos += offset * whiteKeyWidth;
            
            KeyGeometry geo;
            geo.bounds = juce::Rectangle<float>(xPos, 0.0f, blackKeyWidth, blackKeyHeight);
            geo.midiNote = note;
            geo.isBlackKey = true;
            blackKeyGeometry_.push_back(geo);
        }
        else
        {
            // White key
            float xPos = static_cast<float>(whiteKeyIndex) * whiteKeyWidth;
            
            KeyGeometry geo;
            geo.bounds = juce::Rectangle<float>(xPos, 0.0f, whiteKeyWidth, keyboardHeight);
            geo.midiNote = note;
            geo.isBlackKey = false;
            whiteKeyGeometry_.push_back(geo);
            
            ++whiteKeyIndex;
        }
    }
}

void PianoKeyboardView::drawWhiteKeys(juce::Graphics& g)
{
    for (const auto& key : whiteKeyGeometry_)
    {
        bool pressed = isKeyPressed(key.midiNote);
        
        // Fill
        g.setColour(pressed ? pressedWhiteKeyColour_ : whiteKeyColour_);
        g.fillRect(key.bounds);
        
        // Outline
        g.setColour(keyOutlineColour_);
        g.drawRect(key.bounds, 1.0f);
        
        // Add subtle gradient for 3D effect
        if (!pressed)
        {
            juce::ColourGradient gradient(
                whiteKeyColour_.brighter(0.1f), key.bounds.getX(), key.bounds.getY(),
                whiteKeyColour_.darker(0.15f), key.bounds.getX(), key.bounds.getBottom(),
                false);
            g.setGradientFill(gradient);
            g.fillRect(key.bounds.reduced(1.0f));
        }
        else
        {
            // Pressed key gradient (inverted, appears pushed down)
            juce::ColourGradient gradient(
                pressedWhiteKeyColour_.darker(0.1f), key.bounds.getX(), key.bounds.getY(),
                pressedWhiteKeyColour_.brighter(0.1f), key.bounds.getX(), key.bounds.getBottom(),
                false);
            g.setGradientFill(gradient);
            g.fillRect(key.bounds.reduced(1.0f));
        }
    }
}

void PianoKeyboardView::drawBlackKeys(juce::Graphics& g)
{
    for (const auto& key : blackKeyGeometry_)
    {
        bool pressed = isKeyPressed(key.midiNote);
        
        // Fill
        g.setColour(pressed ? pressedBlackKeyColour_ : blackKeyColour_);
        g.fillRoundedRectangle(key.bounds, 2.0f);
        
        // Add 3D effect
        if (!pressed)
        {
            // Highlight on top
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            auto highlightRect = key.bounds.withHeight(3.0f);
            g.fillRoundedRectangle(highlightRect, 1.0f);
            
            // Shadow
            g.setColour(juce::Colours::black.withAlpha(0.3f));
            auto shadowRect = key.bounds.withTrimmedTop(key.bounds.getHeight() - 4.0f);
            g.fillRoundedRectangle(shadowRect, 1.0f);
        }
        else
        {
            // Pressed effect - slight glow
            g.setColour(pressedBlackKeyColour_.brighter(0.3f).withAlpha(0.3f));
            g.drawRoundedRectangle(key.bounds.expanded(1.0f), 3.0f, 2.0f);
        }
        
        // Outline
        g.setColour(keyOutlineColour_);
        g.drawRoundedRectangle(key.bounds, 2.0f, 1.0f);
    }
}

bool PianoKeyboardView::isBlackKey(int noteNumber) noexcept
{
    // Black keys are C#, D#, F#, G#, A# (positions 1, 3, 6, 8, 10 in octave)
    int noteInOctave = noteNumber % 12;
    return noteInOctave == 1 || noteInOctave == 3 || 
           noteInOctave == 6 || noteInOctave == 8 || noteInOctave == 10;
}

int PianoKeyboardView::countWhiteKeysInRange() const noexcept
{
    int count = 0;
    for (int note = lowestNote_; note <= highestNote_; ++note)
    {
        if (!isBlackKey(note))
            ++count;
    }
    return count;
}

int PianoKeyboardView::getWhiteKeyIndex(int midiNote) const noexcept
{
    int index = 0;
    for (int note = lowestNote_; note < midiNote; ++note)
    {
        if (!isBlackKey(note))
            ++index;
    }
    return index;
}
