/*
  ==============================================================================

    MainComponent.cpp
    
    Main component implementation for the standalone MIDI Chord Detector.
    Uses the new optimized pattern-based chord detection algorithm.

  ==============================================================================
*/

#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
    : chordDetector(ChordDetection::SlashChordMode::Auto)
{
    setSize (800, 700);
    
    // Pre-allocate log buffer to avoid reallocations
    logMessages.reserve(MAX_LOG_LINES);
    
    // Configure the chord detector with defaults
    chordDetector.setMinimumNotes(2);
    chordDetector.setSlashChordMode(ChordDetection::SlashChordMode::Auto);
    
    //==========================================================================
    // Title section
    titleLabel.setText ("MIDI Chord Detector", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (28.0f, juce::Font::bold));
    titleLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (titleLabel);
    
    versionLabel.setText ("v2.0.0 - Standalone Debug Mode", juce::dontSendNotification);
    versionLabel.setFont (juce::Font (14.0f, juce::Font::italic));
    versionLabel.setJustificationType (juce::Justification::centred);
    versionLabel.setColour (juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible (versionLabel);
    
    //==========================================================================
    // MIDI device section
    midiDeviceLabel.setText ("MIDI Input Device:", juce::dontSendNotification);
    midiDeviceLabel.setFont (juce::Font (16.0f, juce::Font::bold));
    addAndMakeVisible (midiDeviceLabel);
    
    midiDeviceSelector.onChange = [this] {
        int selectedIndex = midiDeviceSelector.getSelectedItemIndex();
        // Account for the "-- Select MIDI Device --" placeholder at index 0
        if (selectedIndex > 0 && selectedIndex <= midiDevices.size())
        {
            openMidiDevice (selectedIndex - 1); // Adjust for placeholder offset
        }
        else
        {
            closeMidiDevice();
        }
    };
    addAndMakeVisible (midiDeviceSelector);
    
    refreshButton.onClick = [this] { refreshMidiDevices(); };
    addAndMakeVisible (refreshButton);
    
    //==========================================================================
    // Chord display section
    chordLabel.setText ("Detected Chord:", juce::dontSendNotification);
    chordLabel.setFont (juce::Font (16.0f, juce::Font::bold));
    addAndMakeVisible (chordLabel);
    
    chordNameLabel.setText ("N.C.", juce::dontSendNotification);
    chordNameLabel.setFont (juce::Font (72.0f, juce::Font::bold));
    chordNameLabel.setJustificationType (juce::Justification::centred);
    chordNameLabel.setColour (juce::Label::textColourId, juce::Colours::cyan);
    addAndMakeVisible (chordNameLabel);
    
    confidenceLabel.setText ("Confidence: --", juce::dontSendNotification);
    confidenceLabel.setFont (juce::Font (16.0f));
    confidenceLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (confidenceLabel);
    
    bassNoteLabel.setText ("Bass: --", juce::dontSendNotification);
    bassNoteLabel.setFont (juce::Font (16.0f));
    bassNoteLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (bassNoteLabel);
    
    alternativesLabel.setText ("", juce::dontSendNotification);
    alternativesLabel.setFont (juce::Font (12.0f));
    alternativesLabel.setJustificationType (juce::Justification::centred);
    alternativesLabel.setColour (juce::Label::textColourId, juce::Colour (0xff66aacc));  // Light blue
    addAndMakeVisible (alternativesLabel);
    
    //==========================================================================
    // Active notes section
    notesLabel.setText ("Active Notes:", juce::dontSendNotification);
    notesLabel.setFont (juce::Font (16.0f, juce::Font::bold));
    addAndMakeVisible (notesLabel);
    
    activeNotesLabel.setText ("None", juce::dontSendNotification);
    activeNotesLabel.setFont (juce::Font (14.0f));
    activeNotesLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (activeNotesLabel);
    
    pitchClassLabel.setText ("Pitch Classes: {}", juce::dontSendNotification);
    pitchClassLabel.setFont (juce::Font (14.0f));
    pitchClassLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (pitchClassLabel);
    
    //==========================================================================
    // Debug section
    debugLabel.setText ("Debug Log:", juce::dontSendNotification);
    debugLabel.setFont (juce::Font (16.0f, juce::Font::bold));
    addAndMakeVisible (debugLabel);
    
    enableLogging.setToggleState (true, juce::dontSendNotification);
    enableLogging.onClick = [this] { isLoggingEnabled = enableLogging.getToggleState(); };
    addAndMakeVisible (enableLogging);
    
    logMidiEvents.setToggleState (true, juce::dontSendNotification);
    logMidiEvents.onClick = [this] { isMidiLoggingEnabled = logMidiEvents.getToggleState(); };
    addAndMakeVisible (logMidiEvents);
    
    clearLogButton.onClick = [this] {
        juce::ScopedLock sl (logLock);
        logMessages.clear();
        logDisplay.clear();
    };
    addAndMakeVisible (clearLogButton);
    
    logDisplay.setMultiLine (true);
    logDisplay.setReadOnly (true);
    logDisplay.setScrollbarsShown (true);
    logDisplay.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
    logDisplay.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff1e1e1e));
    logDisplay.setColour (juce::TextEditor::textColourId, juce::Colours::lightgreen);
    addAndMakeVisible (logDisplay);
    
    //==========================================================================
    // Status bar
    statusLabel.setText ("No MIDI device connected", juce::dontSendNotification);
    statusLabel.setFont (juce::Font (14.0f));
    statusLabel.setColour (juce::Label::textColourId, juce::Colours::orange);
    addAndMakeVisible (statusLabel);
    
    //==========================================================================
    // Initialize MIDI
    setupMidiDeviceSelector();
    
    // Start the timer for UI updates
    startTimerHz (30); // 30 fps
    
    addLogMessage ("=== MIDI Chord Detector v3.0.0 Started ===");
    addLogMessage ("Engine: Pattern-based detection with 100+ chord types");
}

MainComponent::~MainComponent()
{
    stopTimer();
    closeMidiDevice();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff2d2d2d));
    
    // Draw section separators
    g.setColour (juce::Colours::grey.withAlpha (0.3f));
    
    // Line below MIDI device section
    g.drawHorizontalLine (130, 20.0f, (float) getWidth() - 20.0f);
    
    // Line below chord display section
    g.drawHorizontalLine (340, 20.0f, (float) getWidth() - 20.0f);
    
    // Line below active notes section
    g.drawHorizontalLine (430, 20.0f, (float) getWidth() - 20.0f);
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds().reduced (20);
    
    // Title section
    titleLabel.setBounds (bounds.removeFromTop (35));
    versionLabel.setBounds (bounds.removeFromTop (20));
    bounds.removeFromTop (15); // Spacing
    
    // MIDI device section
    midiDeviceLabel.setBounds (bounds.removeFromTop (25));
    auto midiRow = bounds.removeFromTop (30);
    refreshButton.setBounds (midiRow.removeFromRight (80));
    midiRow.removeFromRight (10);
    midiDeviceSelector.setBounds (midiRow);
    bounds.removeFromTop (20); // Spacing
    
    // Chord display section
    chordLabel.setBounds (bounds.removeFromTop (25));
    chordNameLabel.setBounds (bounds.removeFromTop (100));
    
    auto infoRow = bounds.removeFromTop (25);
    confidenceLabel.setBounds (infoRow.removeFromLeft (infoRow.getWidth() / 2));
    bassNoteLabel.setBounds (infoRow);
    
    // Alternatives row
    alternativesLabel.setBounds (bounds.removeFromTop (20));
    bounds.removeFromTop (10); // Spacing
    
    // Active notes section
    notesLabel.setBounds (bounds.removeFromTop (25));
    activeNotesLabel.setBounds (bounds.removeFromTop (25));
    pitchClassLabel.setBounds (bounds.removeFromTop (25));
    bounds.removeFromTop (15); // Spacing
    
    // Debug section
    debugLabel.setBounds (bounds.removeFromTop (25));
    
    auto checkboxRow = bounds.removeFromTop (30);
    enableLogging.setBounds (checkboxRow.removeFromLeft (180));
    logMidiEvents.setBounds (checkboxRow.removeFromLeft (150));
    clearLogButton.setBounds (checkboxRow.removeFromRight (80));
    
    bounds.removeFromTop (5);
    
    // Status bar at bottom
    statusLabel.setBounds (bounds.removeFromBottom (25));
    bounds.removeFromBottom (5);
    
    // Log display takes remaining space
    logDisplay.setBounds (bounds);
}

void MainComponent::timerCallback()
{
    if (chordUpdated.exchange (false))
    {
        // Update chord display
        std::shared_ptr<ChordDetection::ChordCandidate> chord;
        {
            juce::ScopedLock sl (midiLock);
            chord = currentChord;
        }
        
        if (chord)
        {
            juce::String chordName = juce::String (chord->chordName);
            
            chordNameLabel.setText (chordName, juce::dontSendNotification);
            chordNameLabel.setColour (juce::Label::textColourId, juce::Colours::cyan);
            
            // Show confidence with ambiguity indicator
            juce::String confStr = "Confidence: " + juce::String (chord->confidence * 100.0f, 1) + "%";
            if (chord->isAmbiguous)
            {
                confStr += " (ambiguous)";
            }
            confidenceLabel.setText (confStr, juce::dontSendNotification);
            
            if (!chord->pitchClasses.empty())
            {
                static const char* rootNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
                int bassPC = chord->pitchClasses[0];
                bassNoteLabel.setText ("Bass: " + juce::String (rootNames[bassPC % 12]), juce::dontSendNotification);
            }
            else
            {
                bassNoteLabel.setText ("Bass: --", juce::dontSendNotification);
            }
            
            // Update alternatives display
            if (chord->hasAlternatives())
            {
                juce::StringArray altStrings;
                for (const auto& alt : chord->alternatives)
                {
                    juce::String altStr = juce::String (alt.chordName);
                    if (!alt.relationship.empty())
                    {
                        altStr += " (" + juce::String (alt.relationship) + ")";
                    }
                    altStrings.add (altStr);
                }
                alternativesLabel.setText ("Also: " + altStrings.joinIntoString (" | "), juce::dontSendNotification);
            }
            else
            {
                alternativesLabel.setText ("", juce::dontSendNotification);
            }
        }
        else
        {
            chordNameLabel.setText ("N.C.", juce::dontSendNotification);
            chordNameLabel.setColour (juce::Label::textColourId, juce::Colours::grey);
            confidenceLabel.setText ("Confidence: --", juce::dontSendNotification);
            bassNoteLabel.setText ("Bass: --", juce::dontSendNotification);
            alternativesLabel.setText ("", juce::dontSendNotification);
        }
        
        // Update active notes display
        activeNotesLabel.setText (getActiveNotesString(), juce::dontSendNotification);
        
        // Update pitch classes
        auto notes = chordDetector.getCurrentNotes();
        std::set<int> pitchClassSet;
        for (int note : notes)
        {
            pitchClassSet.insert(note % 12);
        }
        
        juce::String pcStr = "Pitch Classes: {";
        bool first = true;
        for (int pc : pitchClassSet)
        {
            if (!first) pcStr += ", ";
            pcStr += juce::String(pc);
            first = false;
        }
        pcStr += "}";
        pitchClassLabel.setText (pcStr, juce::dontSendNotification);
    }
}

//==============================================================================
void MainComponent::handleIncomingMidiMessage (juce::MidiInput* /*source*/, const juce::MidiMessage& message)
{
    // Log MIDI message
    if (isMidiLoggingEnabled)
    {
        logMidiMessage (message);
    }
    
    // Process the message through the chord detector
    {
        juce::ScopedLock sl (midiLock);
        
        if (message.isNoteOn())
        {
            chordDetector.addNote (message.getNoteNumber());
        }
        else if (message.isNoteOff())
        {
            chordDetector.removeNote (message.getNoteNumber());
        }
        else if (message.isController() && message.getControllerNumber() == 123)
        {
            // All notes off
            chordDetector.clearNotes();
        }
        else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            chordDetector.clearNotes();
        }
        
        // Get current chord
        currentChord = chordDetector.getCurrentChord();
    }
    
    // Log chord detection
    if (isLoggingEnabled && (message.isNoteOn() || message.isNoteOff()))
    {
        juce::ScopedLock sl (midiLock);
        logChordDetection (currentChord);
    }
    
    chordUpdated = true;
}

//==============================================================================
void MainComponent::setupMidiDeviceSelector()
{
    refreshMidiDevices();
}

void MainComponent::refreshMidiDevices()
{
    int previousSelection = midiDeviceSelector.getSelectedItemIndex();
    juce::String previousDeviceName;
    if (previousSelection > 0 && previousSelection <= midiDeviceNames.size())
        previousDeviceName = midiDeviceNames[previousSelection - 1];
    
    midiDeviceSelector.clear();
    midiDeviceNames.clear();
    midiDevices = juce::MidiInput::getAvailableDevices();
    
    midiDeviceSelector.addItem ("-- Select MIDI Device --", 1);
    
    int newSelectionIndex = -1;
    for (int i = 0; i < midiDevices.size(); i++)
    {
        midiDeviceNames.add (midiDevices[i].name);
        midiDeviceSelector.addItem (midiDevices[i].name, i + 2);
        
        if (midiDevices[i].name == previousDeviceName)
            newSelectionIndex = i;
    }
    
    if (midiDevices.isEmpty())
    {
        addLogMessage ("No MIDI input devices found");
        statusLabel.setText ("No MIDI devices found", juce::dontSendNotification);
        statusLabel.setColour (juce::Label::textColourId, juce::Colours::red);
    }
    else
    {
        addLogMessage ("Found " + juce::String (midiDevices.size()) + " MIDI device(s)");
        
        if (newSelectionIndex >= 0)
        {
            // Re-select previously selected device (add 1 for placeholder offset)
            midiDeviceSelector.setSelectedItemIndex (newSelectionIndex + 1, juce::sendNotification);
        }
        else
        {
            midiDeviceSelector.setSelectedItemIndex (0, juce::dontSendNotification);
        }
    }
}

void MainComponent::openMidiDevice (int deviceIndex)
{
    closeMidiDevice();
    
    if (deviceIndex < 0 || deviceIndex >= midiDevices.size())
        return;
    
    const auto& deviceInfo = midiDevices[deviceIndex];
    midiInput = juce::MidiInput::openDevice (deviceInfo.identifier, this);
    
    if (midiInput != nullptr)
    {
        midiInput->start();
        currentDeviceIndex = deviceIndex;
        
        addLogMessage ("Opened MIDI device: " + deviceInfo.name);
        statusLabel.setText ("Connected: " + deviceInfo.name, juce::dontSendNotification);
        statusLabel.setColour (juce::Label::textColourId, juce::Colours::lightgreen);
    }
    else
    {
        addLogMessage ("ERROR: Failed to open MIDI device: " + deviceInfo.name);
        statusLabel.setText ("Failed to open: " + deviceInfo.name, juce::dontSendNotification);
        statusLabel.setColour (juce::Label::textColourId, juce::Colours::red);
    }
}

void MainComponent::closeMidiDevice()
{
    if (midiInput != nullptr)
    {
        midiInput->stop();
        midiInput.reset();
        
        if (currentDeviceIndex >= 0 && currentDeviceIndex < midiDevices.size())
        {
            addLogMessage ("Closed MIDI device: " + midiDevices[currentDeviceIndex].name);
        }
        
        currentDeviceIndex = -1;
        statusLabel.setText ("No MIDI device connected", juce::dontSendNotification);
        statusLabel.setColour (juce::Label::textColourId, juce::Colours::orange);
    }
}

//==============================================================================
void MainComponent::addLogMessage (const juce::String& message)
{
    if (!isLoggingEnabled) return;
    
    auto time = juce::Time::getCurrentTime();
    juce::String timestamp = time.formatted ("%H:%M:%S.") + juce::String (time.getMilliseconds()).paddedLeft ('0', 3);
    juce::String logLine = "[" + timestamp + "] " + message;
    
    {
        juce::ScopedLock sl (logLock);
        
        // Use circular buffer pattern for O(1) insertion
        if (logMessages.size() < MAX_LOG_LINES)
        {
            logMessages.push_back (logLine);
        }
        else
        {
            // Replace oldest entry at logWriteIndex_ position
            logMessages[logWriteIndex_] = logLine;
        }
        logWriteIndex_ = (logWriteIndex_ + 1) % MAX_LOG_LINES;
    }
    
    // Update UI (must be on message thread)
    // Throttle UI updates to prevent overwhelming the text editor
    auto now = juce::Time::getMillisecondCounterHiRes();
    if (now - lastLogUpdateTime_ > 50.0) // Max 20 updates per second
    {
        lastLogUpdateTime_ = now;
        juce::MessageManager::callAsync ([this, logLine]() {
            // Trim text editor if it gets too long to prevent unbounded growth
            if (logDisplay.getText().length() > MAX_LOG_LINES * 100)
            {
                auto text = logDisplay.getText();
                // Keep only the last half
                int startPos = text.length() / 2;
                while (startPos < text.length() && text[startPos] != '\n')
                    ++startPos;
                if (startPos < text.length())
                    logDisplay.setText(text.substring(startPos + 1));
            }
            logDisplay.moveCaretToEnd();
            logDisplay.insertTextAtCaret (logLine + "\n");
        });
    }
}

void MainComponent::logChordDetection (const std::shared_ptr<ChordDetection::ChordCandidate>& chord)
{
    juce::String msg;
    
    if (chord)
    {
        msg = "CHORD: " + juce::String (chord->chordName);
        msg += " (confidence=" + juce::String (chord->confidence * 100.0f, 1) + "%)";
    }
    else
    {
        msg = "CHORD: N.C. (no chord)";
    }
    
    addLogMessage (msg);
}

void MainComponent::logMidiMessage (const juce::MidiMessage& message)
{
    juce::String msg;
    
    if (message.isNoteOn())
    {
        msg = "NOTE ON:  " + noteNumberToName (message.getNoteNumber()) 
            + " (note=" + juce::String (message.getNoteNumber()) 
            + ", vel=" + juce::String (message.getVelocity()) + ")";
    }
    else if (message.isNoteOff())
    {
        msg = "NOTE OFF: " + noteNumberToName (message.getNoteNumber())
            + " (note=" + juce::String (message.getNoteNumber()) + ")";
    }
    else if (message.isController())
    {
        if (message.getControllerNumber() == 64)
        {
            msg = "SUSTAIN:  " + juce::String (message.getControllerValue() >= 64 ? "DOWN" : "UP");
        }
        else
        {
            msg = "CC:       " + juce::String (message.getControllerNumber()) 
                + " = " + juce::String (message.getControllerValue());
        }
    }
    else if (message.isPitchWheel())
    {
        msg = "PITCH:    " + juce::String (message.getPitchWheelValue());
    }
    else
    {
        return; // Don't log other messages
    }
    
    addLogMessage (msg);
}

juce::String MainComponent::noteNumberToName (int noteNumber) const
{
    static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    int octave = (noteNumber / 12) - 1;
    int note = noteNumber % 12;
    return juce::String (noteNames[note]) + juce::String (octave);
}

juce::String MainComponent::getActiveNotesString() const
{
    auto notes = chordDetector.getCurrentNotes();
    
    if (notes.empty())
        return "None";
    
    juce::String result;
    for (size_t i = 0; i < notes.size(); i++)
    {
        if (i > 0) result += ", ";
        result += noteNumberToName (notes[i]);
    }
    
    return result;
}
