#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "core/ChordDetectorEngine.h"
#include <atomic>
#include <memory>

//==============================================================================
/**
 * MidiChordDetectorAudioProcessor
 * 
 * VST3 MIDI Effect plugin processor
 * 
 * Uses the new layered chord detection architecture:
 * - CandidateGenerator: Enumerate chord possibilities
 * - HarmonicMemory: Temporal reasoning (rolling chords, arpeggios)
 * - ConfidenceScorer: Multi-factor scoring
 * - ChordResolver: Final chord selection
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
     * Get the most recent resolved chord (safe for UI thread)
     * Returns a copy of the current chord result
     */
    ChordDetection::ResolvedChord getCurrentChord() const;
    
    /**
     * Check if a new chord has been detected since last check
     */
    bool hasNewChord() const;
    
    /**
     * Check if MIDI activity was detected since last check
     */
    bool hasMidiActivity() const;
    
    /**
     * Access engine configuration (for potential parameter exposure)
     */
    ChordDetection::EngineConfig getEngineConfig() const;
    void setEngineConfig(const ChordDetection::EngineConfig& config);

private:
    //==============================================================================
    // Audio thread members
    ChordDetection::ChordDetectorEngine chordEngine_;
    
    double sampleRate_;
    double currentTimeMs_;
    
    // Settings
    bool passMidiThrough_;
    
    // Cross-thread communication (lock-free)
    // Audio thread writes, UI thread reads
    std::atomic<ChordDetection::ResolvedChord*> currentChord_;
    ChordDetection::ResolvedChord chordBuffer_[2];  // Double buffer
    int chordBufferIndex_;
    
    mutable std::atomic<bool> newChordAvailable_;  // mutable for hasNewChord() const
    mutable std::atomic<bool> midiActivityFlag_;   // mutable for hasMidiActivity() const
    
    // Helper methods
    void processMidiMessage(const juce::MidiMessage& message);
    void updateChordDetection();
    void publishChordResult(const ChordDetection::ResolvedChord& chord);
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiChordDetectorAudioProcessor)
};
