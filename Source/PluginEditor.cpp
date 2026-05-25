#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Version.h"

//==============================================================================
MidiChordDetectorAudioProcessorEditor::MidiChordDetectorAudioProcessorEditor (MidiChordDetectorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Wider window to comfortably display the full 88-key piano (52 white keys)
    // 1800px → (1800-24) / 52 ≈ 34 px per white key
    setSize (1800, 676);
    setResizable (true, true);
    setResizeLimits (1500, 546, 2560, 1400);
    
    // Add sub-components
    addAndMakeVisible(chordDisplay_);
    addAndMakeVisible(notation_);
    addAndMakeVisible(playedNotes_);
    
    // Add piano keyboard (full, beautiful 5-octave range)
    pianoKeyboard_.setKeyRange(21, 108);  // A0 to C8 (full 88-key piano)
    addAndMakeVisible(pianoKeyboard_);
    
    // Setup toggle buttons
    addAndMakeVisible(keyboardToggle_);
    addAndMakeVisible(notationToggle_);
    addAndMakeVisible(playedNotesToggle_);
    
#if defined(DEVELOPER_DEBUG_MODE) && (DEVELOPER_DEBUG_MODE == 1)
    debugToggle_.setButtonText("DEBUG");
#else
    debugToggle_.setButtonText("INFO");
#endif
    addAndMakeVisible(debugToggle_);
    
    // Configure buttons in synth-style
    styleToggle(keyboardToggle_, showKeyboard_);
    styleToggle(notationToggle_, showNotation_);
    styleToggle(playedNotesToggle_, showPlayedNotes_);
    styleToggle(debugToggle_, showDebugPanel_);
    
    // Button Click Listeners (modular toggle system)
    keyboardToggle_.onClick = [this] {
        showKeyboard_ = !showKeyboard_;
        styleToggle(keyboardToggle_, showKeyboard_);
        resized();
        repaint();
    };
    
    notationToggle_.onClick = [this] {
        showNotation_ = !showNotation_;
        styleToggle(notationToggle_, showNotation_);
        resized();
        repaint();
    };
    
    playedNotesToggle_.onClick = [this] {
        showPlayedNotes_ = !showPlayedNotes_;
        styleToggle(playedNotesToggle_, showPlayedNotes_);
        resized();
        repaint();
    };
    
    debugToggle_.onClick = [this] {
        showDebugPanel_ = !showDebugPanel_;
        chordDisplay_.setDebugMode(showDebugPanel_);
        styleToggle(debugToggle_, showDebugPanel_);
        resized();
        repaint();
    };
    
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
    // Matte charcoal workstation background
    g.fillAll(juce::Colour(0xff121317));
    
    // Title bar area background
    g.setColour(juce::Colour(0xff181b24));
    g.fillRect(0, 0, getWidth(), 45);
    
    // Separator line
    g.setColour(juce::Colour(0xff252833));
    g.drawLine(0.0f, 45.0f, static_cast<float>(getWidth()), 45.0f, 1.5f);
    
    // Title text
    g.setColour(juce::Colours::white);
    auto titleFont = juce::Font(juce::FontOptions().withHeight(14.0f)).boldened();
    g.setFont(titleFont);
    g.drawText("CHORD DETECTOR", 15, 0, 150, 45, juce::Justification::centredLeft);
    
    // Version label next to title
    g.setColour(juce::Colour(0xff5c6273));
    g.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
    juce::String verStr = juce::String("v") + MidiChordDetector::Version::VERSION_STRING 
                          + " | By " + MidiChordDetector::Version::AUTHOR_NAME 
                          + " | DAW MIDI PLUGIN";
    g.drawText(verStr, 155, 0, 400, 45, juce::Justification::centredLeft);
}

void MidiChordDetectorAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Top Title/Toolbar Area
    auto topArea = bounds.removeFromTop(45);
    
    // Position toolbar buttons on the right side of topArea
    int buttonWidth = 110;
    int buttonHeight = 26;
    int padding = 8;
    
    auto btnArea = topArea.removeFromRight(480).reduced(0, (topArea.getHeight() - buttonHeight) / 2);
    debugToggle_.setBounds(btnArea.removeFromRight(buttonWidth).reduced(padding / 2, 0));
    playedNotesToggle_.setBounds(btnArea.removeFromRight(buttonWidth).reduced(padding / 2, 0));
    notationToggle_.setBounds(btnArea.removeFromRight(buttonWidth).reduced(padding / 2, 0));
    keyboardToggle_.setBounds(btnArea.removeFromRight(buttonWidth).reduced(padding / 2, 0));
    
    // Apply layout boundaries with clean padding
    bounds.reduce(12, 10);
    
    // 1. Keyboard Component (bottom slot)
    if (showKeyboard_)
    {
        auto keyboardArea = bounds.removeFromBottom(115);
        pianoKeyboard_.setBounds(keyboardArea);
        pianoKeyboard_.setVisible(true);
        bounds.removeFromBottom(10); // Spacing
    }
    else
    {
        pianoKeyboard_.setVisible(false);
    }
    
    // 2. Played Notes Monitor (bottom center slot)
    if (showPlayedNotes_)
    {
        auto monitorArea = bounds.removeFromBottom(48);
        playedNotes_.setBounds(monitorArea);
        playedNotes_.setVisible(true);
        bounds.removeFromBottom(10); // Spacing
    }
    else
    {
        playedNotes_.setVisible(false);
    }
    
    // 3. Middle split: Notation Panel takes up right side of slot, ChordDisplay takes left
    if (showNotation_)
    {
        auto notationWidth = bounds.getWidth() * 0.44f;
        auto notationArea = bounds.removeFromRight(static_cast<int>(notationWidth));
        notation_.setBounds(notationArea);
        notation_.setVisible(true);
        bounds.removeFromRight(10); // Spacing
    }
    else
    {
        notation_.setVisible(false);
    }
    
    // Chord display takes remaining space!
    chordDisplay_.setBounds(bounds);
}

void MidiChordDetectorAudioProcessorEditor::timerCallback()
{
    // Always read the latest chord state from processor
    auto chord = audioProcessor.getCurrentChord();
    chordDisplay_.setChord(chord);
    
    // Check MIDI activity
    bool hasMidi = audioProcessor.hasMidiActivity();
    chordDisplay_.setMidiActivity(hasMidi);
    
    // Update active notes for components
    auto currentNotes = audioProcessor.getCurrentNotes();
    
    // Feed notes to child sub-components
    notation_.setNotes(currentNotes);
    playedNotes_.setNotes(currentNotes);
    
    // Set keyboard view keys
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

void MidiChordDetectorAudioProcessorEditor::styleToggle (juce::TextButton& button, bool active)
{
    button.setClickingTogglesState(false);
    
    // Softer, premium DAW console contrast matching Cubase dark design language
    if (active)
    {
        // Polished slate-cyan button background
        button.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff162a33)); 
        button.setColour(juce::TextButton::textColourOnId, juce::Colour(0xff00d4ff));
        button.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00d4ff));
        button.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff00d4ff));
    }
    else
    {
        // Smooth charcoal workstation background
        button.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1e2129));
        button.setColour(juce::TextButton::textColourOnId, juce::Colour(0xff8c90a1));
        button.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8c90a1));
        button.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff2e3340));
    }
}
