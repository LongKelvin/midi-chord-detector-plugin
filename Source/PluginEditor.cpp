#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MidiChordDetectorAudioProcessorEditor::MidiChordDetectorAudioProcessorEditor (MidiChordDetectorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Set editor size to accommodate keyboard
    setSize (500, 450);
    
    // Add chord display component
    addAndMakeVisible(chordDisplay_);
    
    // Add piano keyboard (full 88 keys)
    pianoKeyboard_.setKeyRange(21, 108);  // A0 to C8
    pianoKeyboard_.setPressedWhiteKeyColour(juce::Colour(0xff00ccff));
    pianoKeyboard_.setPressedBlackKeyColour(juce::Colour(0xff0088bb));
    addAndMakeVisible(pianoKeyboard_);
    
    // Start timer for UI updates (30 Hz)
    startTimerHz(30);
}

MidiChordDetectorAudioProcessorEditor::~MidiChordDetectorAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void MidiChordDetectorAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xff202020));
    
    // Title bar
    g.setColour(juce::Colour(0xff303030));
    g.fillRect(0, 0, getWidth(), 40);
    
    // Title text
    g.setColour(juce::Colours::white);
    auto titleFont = juce::Font(juce::FontOptions().withHeight(16.0f)).boldened();
    g.setFont(titleFont);
    g.drawText("MIDI Chord Detector", 10, 10, getWidth() - 20, 20,
               juce::Justification::centredLeft);
}

void MidiChordDetectorAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Reserve space for title bar
    bounds.removeFromTop(50);
    
    // Reserve space for piano keyboard at bottom
    auto keyboardArea = bounds.removeFromBottom(120);
    pianoKeyboard_.setBounds(keyboardArea.reduced(10));
    
    // Chord display takes remaining space
    chordDisplay_.setBounds(bounds.reduced(10));
}

void MidiChordDetectorAudioProcessorEditor::timerCallback()
{
    // Always read the latest chord state
    auto chord = audioProcessor.getCurrentChord();
    chordDisplay_.setChord(chord);
    
    // Check MIDI activity
    bool hasMidi = audioProcessor.hasMidiActivity();
    chordDisplay_.setMidiActivity(hasMidi);
    
    // Update piano keyboard with current notes
    auto currentNotes = audioProcessor.getCurrentNotes();
    
    // First clear all keys, then set pressed ones
    // (More efficient than tracking state changes)
    static std::vector<int> previousNotes;
    if (currentNotes != previousNotes)
    {
        pianoKeyboard_.clearAllKeys();
        for (int note : currentNotes)
        {
            pianoKeyboard_.setKeyState(note, true);
        }
        previousNotes = currentNotes;
    }
}
