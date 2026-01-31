#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "ui/ChordDisplayComponent.h"
#include "ui/PianoKeyboardView.h"

//==============================================================================
/**
 * MidiChordDetectorAudioProcessorEditor
 * 
 * Plugin UI window
 * 
 * Responsibilities:
 * - Display chord information
 * - Poll processor for chord updates (via timer)
 * - Provide clean, professional interface
 * 
 * Design:
 * - Dark theme
 * - Large chord display
 * - Automatic updates (30 Hz refresh)
 * - No user controls initially (Phase 1)
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
    
    // Reference to processor
    MidiChordDetectorAudioProcessor& audioProcessor;
    
    // UI components
    ChordDisplayComponent chordDisplay_;
    PianoKeyboardView pianoKeyboard_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiChordDetectorAudioProcessorEditor)
};
