/*
  ==============================================================================

    MainComponent.h
    
    Main component for the standalone MIDI Chord Detector application.
    Provides MIDI device selection, real-time chord display, and debug logging.
    
    Uses the new optimized pattern-based chord detection algorithm.

  ==============================================================================
*/

#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "../chord_detection/api/JuceChordDetector.h"

//==============================================================================
class MainComponent : public juce::Component,
                      public juce::Timer,
                      private juce::MidiInputCallback
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    // MIDI callback
    void handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage& message) override;
    
    // UI setup helpers
    void setupMidiDeviceSelector();
    void refreshMidiDevices();
    void openMidiDevice (int deviceIndex);
    void closeMidiDevice();
    
    // Debug logging
    void addLogMessage (const juce::String& message);
    void logChordDetection (const std::shared_ptr<ChordDetection::ChordCandidate>& chord);
    void logMidiMessage (const juce::MidiMessage& message);
    
    // Format helpers
    juce::String noteNumberToName (int noteNumber) const;
    juce::String getActiveNotesString() const;
    
    //==============================================================================
    // Core engine - using new pattern-based detection
    ChordDetection::JuceChordDetector chordDetector;
    std::shared_ptr<ChordDetection::ChordCandidate> currentChord;
    std::atomic<bool> chordUpdated { false };
    
    // MIDI
    std::unique_ptr<juce::MidiInput> midiInput;
    juce::StringArray midiDeviceNames;
    juce::Array<juce::MidiDeviceInfo> midiDevices;
    int currentDeviceIndex = -1;
    
    // Thread safety
    juce::CriticalSection midiLock;
    juce::CriticalSection logLock;
    
    //==============================================================================
    // UI Components
    
    // Header section
    juce::Label titleLabel;
    juce::Label versionLabel;
    
    // MIDI device section
    juce::Label midiDeviceLabel;
    juce::ComboBox midiDeviceSelector;
    juce::TextButton refreshButton { "Refresh" };
    
    // Chord display section
    juce::Label chordLabel;
    juce::Label chordNameLabel;
    juce::Label confidenceLabel;
    juce::Label bassNoteLabel;
    
    // Active notes section
    juce::Label notesLabel;
    juce::Label activeNotesLabel;
    juce::Label pitchClassLabel;
    
    // Debug section
    juce::Label debugLabel;
    juce::TextEditor logDisplay;
    juce::ToggleButton enableLogging { "Enable Debug Logging" };
    juce::ToggleButton logMidiEvents { "Log MIDI Events" };
    juce::TextButton clearLogButton { "Clear Log" };
    
    // Status bar
    juce::Label statusLabel;
    
    //==============================================================================
    // State
    std::vector<juce::String> logMessages;
    static constexpr int MAX_LOG_LINES = 500;
    bool isLoggingEnabled = true;
    bool isMidiLoggingEnabled = true;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
