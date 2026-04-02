#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "chord_detection/api/JuceChordDetector.h"
#include <atomic>
#include <memory>

//==============================================================================
/**
 * MidiChordDetectorAudioProcessor
 * 
 * VST3 MIDI Effect plugin processor using optimized pattern-based chord detection.
 * 
 * The new algorithm:
 * - Pattern-based detection with 100+ chord types
 * - Optimized scoring for accurate inversion detection
 * - Slash chord support with auto/always/never modes
 * - Voicing classification (close, open, drop-2, drop-3, rootless)
 * 
 * Responsibilities:
 * - Parse MIDI input from host
 * - Update note state (note on/off, sustain pedal)
 * - Trigger chord detection
 * - Store detected chord atomically for UI thread
 * - Pass MIDI through unchanged
 * 
 * Real-time safety:
 * - All processBlock code is real-time safe
 * - No allocations in audio thread
 * - Atomic pointer swap for UI communication
 * - No locks or mutexes
 */
class MidiChordDetectorAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    MidiChordDetectorAudioProcessor();
    ~MidiChordDetectorAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    //==============================================================================
    // UI Thread Access
    
    /**
     * Get the most recent detected chord (safe for UI thread)
     * Returns the current ChordCandidate, may be nullptr if no chord detected
     */
    std::shared_ptr<ChordDetection::ChordCandidate> getCurrentChord() const;
    
    /**
     * Check if a new chord has been detected since last check
     */
    bool hasNewChord() const;
    
    /**
     * Check if MIDI activity was detected since last check
     */
    bool hasMidiActivity() const;
    
    /**
     * Set slash chord mode
     */
    void setSlashChordMode(ChordDetection::SlashChordMode mode);
    
    /**
     * Set minimum notes for detection
     */
    void setMinimumNotes(int minNotes);

private:
    //==============================================================================
    // Chord detector using new pattern-based algorithm
    ChordDetection::JuceChordDetector chordDetector_;
    
    double sampleRate_;
    
    // Settings
    bool passMidiThrough_;
    
    // Cross-thread communication (lock-free)
    // Audio thread writes, UI thread reads
    std::atomic<ChordDetection::ChordCandidate*> currentChordPtr_;
    std::shared_ptr<ChordDetection::ChordCandidate> chordBuffer_[2];  // Double buffer
    std::atomic<int> chordBufferIndex_;  // written by audio thread, read by UI thread
    
    mutable std::atomic<bool> newChordAvailable_;  // mutable for hasNewChord() const
    mutable std::atomic<bool> midiActivityFlag_;   // mutable for hasMidiActivity() const
    
    // Helper methods
    void processMidiMessage(const juce::MidiMessage& message);
    void publishChordResult(const std::shared_ptr<ChordDetection::ChordCandidate>& chord);
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiChordDetectorAudioProcessor)
};
