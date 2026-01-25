#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MidiChordDetectorAudioProcessorEditor::MidiChordDetectorAudioProcessorEditor (MidiChordDetectorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Set editor size
    setSize (400, 300);
    
    // Add chord display component
    addAndMakeVisible(chordDisplay_);
    
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
    g.setFont(juce::Font(16.0f, juce::Font::bold));
    g.drawText("MIDI Chord Detector", 10, 10, getWidth() - 20, 20,
               juce::Justification::centredLeft);
}

void MidiChordDetectorAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Reserve space for title bar
    bounds.removeFromTop(50);
    
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
}
