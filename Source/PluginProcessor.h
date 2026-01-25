#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "core/MidiNoteState.h"
#include "core/ChordDetector.h"
#include "core/ChordCandidate.h"
#include <atomic>
#include <memory>

//==============================================================================
/**
 * MidiChordDetectorAudioProcessor
 * 
 * VST3 MIDI Effect plugin processor
 * 
 * Responsibilities:
 * - Parse MIDI input from host
 * - Update note state (note on/off, sustain pedal)
 * - Trigger chord detection
 * - Store detected chord atomically for UI thread
 * - Pass MIDI through unchanged (optional)
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
     * Returns a copy of the current chord candidate
     */
    ChordDetection::ChordCandidate getCurrentChord() const;
    
    /**
     * Check if a new chord has been detected since last check
     */
    bool hasNewChord() const;
    
    /**
     * Check if MIDI activity was detected since last check
     */
    bool hasMidiActivity() const;

private:
    //==============================================================================
    // Audio thread members
    ChordDetection::MidiNoteState noteState_;
    ChordDetection::ChordDetector chordDetector_;
    
    double sampleRate_;
    double currentTimeMs_;
    
    // Settings
    bool passMidiThrough_;
    int minNotesForChord_;
    bool useTimeWindow_;
    
    // Cross-thread communication (lock-free)
    // Audio thread writes, UI thread reads
    std::atomic<ChordDetection::ChordCandidate*> currentChord_;
    ChordDetection::ChordCandidate chordBuffer_[2];  // Double buffer
    int chordBufferIndex_;
    
    mutable std::atomic<bool> newChordAvailable_;  // mutable for hasNewChord() const
    mutable std::atomic<bool> midiActivityFlag_;    // mutable for hasMidiActivity() const
    
    // Helper methods
    void processMidiMessage(const juce::MidiMessage& message);
    void updateChordDetection();
    void publishChordResult(const ChordDetection::ChordCandidate& chord);
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiChordDetectorAudioProcessor)
};
