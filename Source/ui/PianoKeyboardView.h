/*
  ==============================================================================

    PianoKeyboardView.h
    
    A visual piano keyboard component that displays key states.
    Designed to be driven by external MIDI state - does not process MIDI directly.
    
    Features:
    - Configurable key range (default: C2-C7, 5 octaves)
    - Real-time safe state updates via atomic flags
    - Smooth key highlighting with configurable colors
    - Independent and reusable component

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <atomic>

/**
 * A visual piano keyboard component that displays which keys are pressed.
 * 
 * This component is purely a view - it renders keyboard state but does not
 * process MIDI events directly. The controller (e.g., MainComponent) is
 * responsible for calling setKeyState() based on MIDI input.
 * 
 * Thread Safety:
 * - setKeyState(), clearAllKeys() can be called from any thread (real-time safe)
 * - Paint and other UI methods run on the message thread
 * - Uses atomic operations for lock-free state updates
 */
class PianoKeyboardView : public juce::Component,
                          public juce::Timer
{
public:
    //==========================================================================
    // Configuration
    //==========================================================================
    
    /// Default key range (MIDI note numbers)
    static constexpr int kDefaultLowestNote = 21;   // A0 (standard 88-key piano)
    static constexpr int kDefaultHighestNote = 108;  // C8 (standard 88-key piano)
    
    /// Total possible MIDI notes
    static constexpr int kTotalMidiNotes = 128;
    
    //==========================================================================
    // Construction / Destruction
    //==========================================================================
    
    PianoKeyboardView();
    ~PianoKeyboardView() override;
    
    //==========================================================================
    // Key Range Configuration
    //==========================================================================
    
    /**
     * Set the visible key range.
     * @param lowestNote  MIDI note number of the lowest visible key
     * @param highestNote MIDI note number of the highest visible key
     */
    void setKeyRange(int lowestNote, int highestNote);
    
    /** Get the lowest visible note. */
    [[nodiscard]] int getLowestNote() const noexcept { return lowestNote_; }
    
    /** Get the highest visible note. */
    [[nodiscard]] int getHighestNote() const noexcept { return highestNote_; }
    
    //==========================================================================
    // Key State Control (Thread-Safe, Real-Time Safe)
    //==========================================================================
    
    /**
     * Set the pressed state of a key.
     * 
     * This method is real-time safe and can be called from any thread,
     * including MIDI callback threads.
     * 
     * @param midiNoteNumber MIDI note number (0-127)
     * @param isPressed      true = key down, false = key up
     */
    void setKeyState(int midiNoteNumber, bool isPressed) noexcept;
    
    /**
     * Clear all key states (all keys up).
     * 
     * Real-time safe, can be called from any thread.
     */
    void clearAllKeys() noexcept;
    
    /**
     * Check if a specific key is currently pressed.
     * @param midiNoteNumber MIDI note number (0-127)
     * @return true if the key is pressed
     */
    [[nodiscard]] bool isKeyPressed(int midiNoteNumber) const noexcept;
    
    //==========================================================================
    // Appearance Configuration
    //==========================================================================
    
    /** Set the color for pressed white keys. */
    void setPressedWhiteKeyColour(juce::Colour colour);
    
    /** Set the color for pressed black keys. */
    void setPressedBlackKeyColour(juce::Colour colour);
    
    /** Set the color for unpressed white keys. */
    void setWhiteKeyColour(juce::Colour colour);
    
    /** Set the color for unpressed black keys. */
    void setBlackKeyColour(juce::Colour colour);
    
    /** Set the outline color for keys. */
    void setKeyOutlineColour(juce::Colour colour);
    
    //==========================================================================
    // Component Overrides
    //==========================================================================
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    //==========================================================================
    // Timer Callback
    //==========================================================================
    
    void timerCallback() override;

private:
    //==========================================================================
    // Internal Types
    //==========================================================================
    
    struct KeyGeometry
    {
        juce::Rectangle<float> bounds;
        int midiNote;
        bool isBlackKey;
    };
    
    //==========================================================================
    // Internal Methods
    //==========================================================================
    
    void calculateKeyGeometry();
    void drawWhiteKeys(juce::Graphics& g);
    void drawBlackKeys(juce::Graphics& g);
    
    [[nodiscard]] static bool isBlackKey(int noteNumber) noexcept;
    [[nodiscard]] int countWhiteKeysInRange() const noexcept;
    [[nodiscard]] int getWhiteKeyIndex(int midiNote) const noexcept;
    
    //==========================================================================
    // State
    //==========================================================================
    
    /// Key pressed states (atomic for thread-safe access)
    std::array<std::atomic<bool>, kTotalMidiNotes> keyStates_;
    
    /// Flag indicating state has changed (for efficient repainting)
    std::atomic<bool> stateChanged_{false};
    
    /// Visible key range
    int lowestNote_ = kDefaultLowestNote;
    int highestNote_ = kDefaultHighestNote;
    
    /// Cached key geometry (recalculated on resize)
    std::vector<KeyGeometry> whiteKeyGeometry_;
    std::vector<KeyGeometry> blackKeyGeometry_;
    
    //==========================================================================
    // Appearance
    //==========================================================================
    
    juce::Colour whiteKeyColour_        = juce::Colour(0xffeeeeee);
    juce::Colour blackKeyColour_        = juce::Colour(0xff1a1a1a);
    juce::Colour pressedWhiteKeyColour_ = juce::Colour(0xff00ccff);  // Cyan
    juce::Colour pressedBlackKeyColour_ = juce::Colour(0xff0088bb);  // Darker cyan
    juce::Colour keyOutlineColour_      = juce::Colour(0xff333333);
    
    //==========================================================================
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoKeyboardView)
};
