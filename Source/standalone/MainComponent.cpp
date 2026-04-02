/*
  ==============================================================================

    MainComponent.cpp
    
    Main component implementation for the standalone MIDI Chord Detector.
    Uses the new optimized pattern-based chord detection algorithm.

  ==============================================================================
*/

#include "MainComponent.h"
#include "../Version.h"

//==============================================================================
MainComponent::MainComponent()
    : chordDetector(ChordDetection::SlashChordMode::Auto)
{
    setSize (800, 700);
    
    // Configure the chord detector with defaults
    chordDetector.setMinimumNotes(2);
    chordDetector.setSlashChordMode(ChordDetection::SlashChordMode::Auto);
    
    //==========================================================================
    // Title section
    titleLabel.setText ("MIDI Chord Detector", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (28.0f, juce::Font::bold));
    titleLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (titleLabel);
    
    versionLabel.setText (juce::String(MidiChordDetector::Version::VERSION_STRING_WITH_V) + " - Standalone Debug Mode", juce::dontSendNotification);
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
        logDisplay.clear();
        logLineCount_ = 0;
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
    
    addLogMessage ("=== MIDI Chord Detector " + juce::String(MidiChordDetector::Version::VERSION_STRING_WITH_V) + " Started ===");
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
    bounds.removeFromTop (15); // Spacing
    
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
    if (!chordUpdated.exchange (false))
        return;

    // -------------------------------------------------------------------------
    // Snapshot everything we need under one brief lock.
    // getCurrentNotes() reads notesBitset_ which is written by the MIDI thread
    // under midiLock \u2014 we MUST hold the lock here to avoid a data race.
    // -------------------------------------------------------------------------
    std::shared_ptr<ChordDetection::ChordCandidate> chord;
    std::vector<int> notes;
    {
        juce::ScopedLock sl (midiLock);
        chord = currentChord;
        notes = chordDetector.getCurrentNotes();  // safe: we hold midiLock
    }

    // -------------------------------------------------------------------------
    // Update chord display (no lock held)
    // -------------------------------------------------------------------------
    if (chord)
    {
        chordNameLabel.setText (juce::String (chord->chordName), juce::dontSendNotification);
        chordNameLabel.setColour (juce::Label::textColourId, juce::Colours::cyan);

        confidenceLabel.setText ("Confidence: " + juce::String (chord->confidence * 100.0f, 1) + "%",
                                 juce::dontSendNotification);

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
    }
    else
    {
        chordNameLabel.setText ("N.C.", juce::dontSendNotification);
        chordNameLabel.setColour (juce::Label::textColourId, juce::Colours::grey);
        confidenceLabel.setText ("Confidence: --", juce::dontSendNotification);
        bassNoteLabel.setText ("Bass: --", juce::dontSendNotification);
    }

    // -------------------------------------------------------------------------
    // Build active-notes string from snapshot (no lock, no allocation)
    // -------------------------------------------------------------------------
    juce::String activeStr;
    if (notes.empty())
    {
        activeStr = "None";
    }
    else
    {
        for (size_t i = 0; i < notes.size(); ++i)
        {
            if (i > 0) activeStr += ", ";
            activeStr += noteNumberToName (notes[i]);
        }
    }
    activeNotesLabel.setText (activeStr, juce::dontSendNotification);

    // -------------------------------------------------------------------------
    // Build pitch-class string using std::bitset<12> \u2014 zero heap allocation.
    // -------------------------------------------------------------------------
    std::bitset<12> pcBits;
    for (int n : notes)
        pcBits.set (static_cast<size_t> (n % 12));

    juce::String pcStr = "Pitch Classes: {";
    bool first = true;
    for (int pc = 0; pc < 12; ++pc)
    {
        if (pcBits.test (static_cast<size_t> (pc)))
        {
            if (!first) pcStr += ", ";
            pcStr += juce::String (pc);
            first = false;
        }
    }
    pcStr += "}";
    pitchClassLabel.setText (pcStr, juce::dontSendNotification);
}

//==============================================================================
void MainComponent::handleIncomingMidiMessage (juce::MidiInput* /*source*/, const juce::MidiMessage& message)
{
    // Log raw MIDI first — no lock needed, logMidiMessage is self-contained.
    if (isMidiLoggingEnabled)
        logMidiMessage (message);

    bool noteChanged = false;
    std::shared_ptr<ChordDetection::ChordCandidate> chord;

    {
        juce::ScopedLock sl (midiLock);

        // -----------------------------------------------------------------------
        // CC 64 — sustain pedal (mirrors PluginProcessor logic)
        // -----------------------------------------------------------------------
        if (message.isController() && message.getControllerNumber() == 64)
        {
            const bool pedalNowDown = (message.getControllerValue() >= 64);

            if (!pedalNowDown && sustainPedalDown_)
            {
                // Release: flush all deferred NoteOffs
                for (int i = 0; i < 128; ++i)
                    if (sustainedNotes_.test (static_cast<size_t> (i)))
                        chordDetector.removeNote (i);

                sustainedNotes_.reset();
                noteChanged = true;
            }

            sustainPedalDown_ = pedalNowDown;
        }
        // -----------------------------------------------------------------------
        // NoteOn — always add, cancel any pending removal for this note
        // -----------------------------------------------------------------------
        else if (message.isNoteOn())
        {
            sustainedNotes_.reset (static_cast<size_t> (message.getNoteNumber()));
            chordDetector.addNote (message.getNoteNumber());
            noteChanged = true;
        }
        // -----------------------------------------------------------------------
        // NoteOff — defer while pedal is held
        // -----------------------------------------------------------------------
        else if (message.isNoteOff())
        {
            if (sustainPedalDown_)
            {
                sustainedNotes_.set (static_cast<size_t> (message.getNoteNumber()));
                // Do not remove from detector; hold the display.
            }
            else
            {
                chordDetector.removeNote (message.getNoteNumber());
                noteChanged = true;
            }
        }
        // -----------------------------------------------------------------------
        // Emergency reset (CC123 / All Notes Off) — ignore pedal state
        // -----------------------------------------------------------------------
        else if ((message.isController() && message.getControllerNumber() == 123)
                 || message.isAllNotesOff() || message.isAllSoundOff())
        {
            sustainedNotes_.reset();
            sustainPedalDown_ = false;
            chordDetector.clearNotes();
            noteChanged = true;
        }

        if (noteChanged)
            currentChord = chordDetector.getCurrentChord();

        chord = currentChord;  // local copy, safe to use after lock releases
    }

    // Log chord detection outside the lock — no lock inversion risk.
    if (noteChanged && isLoggingEnabled)
        logChordDetection (chord);

    if (noteChanged)
        chordUpdated.store (true, std::memory_order_release);
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
    juce::String line = "[" + time.formatted ("%H:%M:%S.")
                      + juce::String (time.getMilliseconds()).paddedLeft ('0', 3)
                      + "] " + message;

    // Post to the message thread — zero blocking on the MIDI callback thread.
    juce::MessageManager::callAsync ([this, line]() {
        // Trim the TextEditor when it exceeds the line limit.
        ++logLineCount_;
        if (logLineCount_ > MAX_LOG_LINES)
        {
            logDisplay.clear();
            logLineCount_ = 1;
        }
        logDisplay.moveCaretToEnd();
        logDisplay.insertTextAtCaret (line + "\n");
    });
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
