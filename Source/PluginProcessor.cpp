#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MidiChordDetectorAudioProcessor::MidiChordDetectorAudioProcessor()
     : AudioProcessor (BusesProperties()
                       // VST3 Instrument requires audio output bus (outputs silence)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                       )
    , chordDetector_(ChordDetection::SlashChordMode::Auto)
    , sampleRate_(44100.0)
    , passMidiThrough_(true)
    , chordBufferIndex_(0)
{
    // Initialize chord buffers with nullptr (empty state)
    chordBuffer_[0] = nullptr;
    chordBuffer_[1] = nullptr;
    
    currentChordPtr_.store(nullptr, std::memory_order_release);
    newChordAvailable_.store(false, std::memory_order_release);
    midiActivityFlag_.store(false, std::memory_order_release);
    
    // Configure detector with defaults
    chordDetector_.setMinimumNotes(2);
    chordDetector_.setSlashChordMode(ChordDetection::SlashChordMode::Auto);
}

MidiChordDetectorAudioProcessor::~MidiChordDetectorAudioProcessor()
{
}

//==============================================================================
const juce::String MidiChordDetectorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MidiChordDetectorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MidiChordDetectorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MidiChordDetectorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MidiChordDetectorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MidiChordDetectorAudioProcessor::getNumPrograms()
{
    return 1;
}

int MidiChordDetectorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MidiChordDetectorAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused(index);
}

const juce::String MidiChordDetectorAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused(index);
    return {};
}

void MidiChordDetectorAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void MidiChordDetectorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    
    sampleRate_ = sampleRate;
    
    // Reset detector state
    chordDetector_.clearNotes();
}

void MidiChordDetectorAudioProcessor::releaseResources()
{
    // Clear state when playback stops
    chordDetector_.clearNotes();
}

bool MidiChordDetectorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // VST3 Instrument: Accept mono or stereo output (we output silence anyway)
    const auto& mainOutput = layouts.getMainOutputChannelSet();
    return mainOutput == juce::AudioChannelSet::mono()
        || mainOutput == juce::AudioChannelSet::stereo();
}

void MidiChordDetectorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                     juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    // Clear audio buffers (we don't process audio, only MIDI)
    buffer.clear();
    
    // Reuse pre-allocated buffer for real-time safety (no heap allocation)
    preallocatedMidiBuffer_.clear();
    
    // Track if chord changed this block
    bool chordMayHaveChanged = false;
    
    // Process incoming MIDI messages
    for (const auto metadata : midiMessages)
    {
        const juce::MidiMessage& message = metadata.getMessage();
        const int samplePosition = metadata.samplePosition;
        
        // Signal MIDI activity to UI
        midiActivityFlag_.store(true, std::memory_order_release);
        
        // Analyze the MIDI message for chord detection
        processMidiMessage(message);
        
        if (message.isNoteOn() || message.isNoteOff() || 
            message.isController())
        {
            chordMayHaveChanged = true;
        }
        
        // Add message to output buffer (explicit pass-through)
        preallocatedMidiBuffer_.addEvent(message, samplePosition);
    }
    
    // Publish chord result if notes changed
    if (chordMayHaveChanged)
    {
        auto chord = chordDetector_.getCurrentChord();
        publishChordResult(chord);
    }
    
    // Swap the processed MIDI buffer to output
    midiMessages.swapWith(preallocatedMidiBuffer_);
}

void MidiChordDetectorAudioProcessor::processMidiMessage(const juce::MidiMessage& message)
{
    if (message.isNoteOn())
    {
        chordDetector_.addNote(message.getNoteNumber());
    }
    else if (message.isNoteOff())
    {
        chordDetector_.removeNote(message.getNoteNumber());
    }
    else if (message.isController())
    {
        // All notes off (CC 123)
        if (message.getControllerNumber() == 123)
        {
            chordDetector_.clearNotes();
        }
    }
    else if (message.isAllNotesOff() || message.isAllSoundOff())
    {
        chordDetector_.clearNotes();
    }
}

void MidiChordDetectorAudioProcessor::publishChordResult(const std::shared_ptr<ChordDetection::ChordCandidate>& chord)
{
    // Double-buffer technique for lock-free communication
    // Write to the buffer not currently being read
    int writeIndex = 1 - chordBufferIndex_;
    chordBuffer_[writeIndex] = chord;
    
    // Atomic swap to make new chord available
    currentChordPtr_.store(chordBuffer_[writeIndex].get(), std::memory_order_release);
    chordBufferIndex_ = writeIndex;
    
    newChordAvailable_.store(true, std::memory_order_release);
}

std::shared_ptr<ChordDetection::ChordCandidate> MidiChordDetectorAudioProcessor::getCurrentChord() const
{
    // Return the current chord from the detector
    // This is safe because we're returning a shared_ptr copy
    return chordBuffer_[chordBufferIndex_];
}

bool MidiChordDetectorAudioProcessor::hasNewChord() const
{
    return newChordAvailable_.exchange(false, std::memory_order_acq_rel);
}

bool MidiChordDetectorAudioProcessor::hasMidiActivity() const
{
    return midiActivityFlag_.exchange(false, std::memory_order_acq_rel);
}

void MidiChordDetectorAudioProcessor::setSlashChordMode(ChordDetection::SlashChordMode mode)
{
    chordDetector_.setSlashChordMode(mode);
}

void MidiChordDetectorAudioProcessor::setMinimumNotes(int minNotes)
{
    chordDetector_.setMinimumNotes(minNotes);
}

//==============================================================================
bool MidiChordDetectorAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* MidiChordDetectorAudioProcessor::createEditor()
{
    return new MidiChordDetectorAudioProcessorEditor (*this);
}

//==============================================================================
void MidiChordDetectorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Save plugin state (parameters, settings)
    juce::ignoreUnused(destData);
    
    // TODO: Implement state saving if needed
    // For now, plugin has no persistent state
}

void MidiChordDetectorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Restore plugin state
    juce::ignoreUnused(data, sizeInBytes);
    
    // TODO: Implement state restoration if needed
}

//==============================================================================
// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidiChordDetectorAudioProcessor();
}
