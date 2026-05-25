#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "ui/ChordDisplayComponent.h"
#include "ui/PianoKeyboardView.h"
#include "ui/NotationComponent.h"
#include "ui/PlayedNotesComponent.h"

//==============================================================================
/**
 * MidiChordDetectorAudioProcessorEditor
 * 
 * Plugin UI window with modern modular, toggleable workstation sections.
 */
class MidiChordDetectorAudioProcessorEditor : public juce::AudioProcessorEditor,
                                               private juce::Timer
{
public:
    MidiChordDetectorAudioProcessorEditor (MidiChordDetectorAudioProcessor&);
    ~MidiChordDetectorAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // Timer callback for polling chord updates
    void timerCallback() override;
    
    // Helper to style buttons in synth-style
    void styleToggle (juce::TextButton& button, bool active);
    
    // Reference to processor
    MidiChordDetectorAudioProcessor& audioProcessor;
    
    // UI components
    ChordDisplayComponent chordDisplay_;
    NotationComponent notation_;
    PlayedNotesComponent playedNotes_;
    PianoKeyboardView pianoKeyboard_;
    
    // Toolbar toggle buttons
    juce::TextButton keyboardToggle_{ "KEYBOARD" };
    juce::TextButton notationToggle_{ "NOTATION" };
    juce::TextButton playedNotesToggle_{ "MONITOR" };
    juce::TextButton debugToggle_{ "DEBUG" };
    
    // Toggle States
    bool showKeyboard_ = true;
    bool showNotation_ = true;
    bool showPlayedNotes_ = true;
    bool showDebugPanel_ = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiChordDetectorAudioProcessorEditor)
};
