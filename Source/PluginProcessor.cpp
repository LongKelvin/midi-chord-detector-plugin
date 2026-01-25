#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MidiChordDetectorAudioProcessor::MidiChordDetectorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
    , sampleRate_(44100.0)
    , currentTimeMs_(0.0)
    , passMidiThrough_(true)
    , minNotesForChord_(2)
    , useTimeWindow_(true)
    , chordBufferIndex_(0)
{
    // Initialize chord buffers with empty "N.C." state
    chordBuffer_[0] = ChordDetection::ChordCandidate();
    chordBuffer_[1] = ChordDetection::ChordCandidate();
    chordBuffer_[0].isValid = false;  // Ensure it shows "N.C."
    chordBuffer_[1].isValid = false;
    
    currentChord_.store(&chordBuffer_[0], std::memory_order_release);
    newChordAvailable_.store(false, std::memory_order_release);
    midiActivityFlag_.store(false, std::memory_order_release);
    
    // Do an initial chord detection to establish baseline
    updateChordDetection();
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
    currentTimeMs_ = 0.0;
    
    // Reset state
    noteState_.allNotesOff();
    chordDetector_.reset();
}

void MidiChordDetectorAudioProcessor::releaseResources()
{
    // Clear state when playback stops
    noteState_.allNotesOff();
    chordDetector_.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MidiChordDetectorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    // Check input/output match for non-synth plugins
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void MidiChordDetectorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                     juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    // Clear audio buffers (we don't process audio, only MIDI)
    buffer.clear();
    
    // Update time
    currentTimeMs_ += (buffer.getNumSamples() / sampleRate_) * 1000.0;
    
    // Copy incoming MIDI to preserve for output (explicit pass-through for host compatibility)
    juce::MidiBuffer processedMidi;
    
    // Process incoming MIDI messages
    bool notesChanged = false;
    
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
            notesChanged = true;
        }
        
        // Add message to output buffer (explicit pass-through)
        processedMidi.addEvent(message, samplePosition);
    }
    
    // Update chord detection if notes changed OR if we have active notes
    // (to handle sustain and time window)
    if (notesChanged || noteState_.getActiveNoteCount() > 0)
    {
        updateChordDetection();
    }
    
    // Swap the processed MIDI buffer to output
    // This ensures MIDI is explicitly passed through for all hosts (including Cubase MIDI FX)
    midiMessages.swapWith(processedMidi);
}

void MidiChordDetectorAudioProcessor::processMidiMessage(const juce::MidiMessage& message)
{
    if (message.isNoteOn())
    {
        int note = message.getNoteNumber();
        float velocity = message.getFloatVelocity();
        
        noteState_.noteOn(note, velocity);
        chordDetector_.noteOn(note, currentTimeMs_);
    }
    else if (message.isNoteOff())
    {
        int note = message.getNoteNumber();
        
        noteState_.noteOff(note);
        chordDetector_.noteOff(note, currentTimeMs_);
    }
    else if (message.isController())
    {
        // Sustain pedal (CC 64)
        if (message.getControllerNumber() == 64)
        {
            bool sustained = message.getControllerValue() >= 64;
            noteState_.setSustainPedal(sustained);
        }
        // All notes off (CC 123)
        else if (message.getControllerNumber() == 123)
        {
            noteState_.allNotesOff();
            chordDetector_.reset();
        }
    }
    else if (message.isAllNotesOff() || message.isAllSoundOff())
    {
        noteState_.allNotesOff();
        chordDetector_.reset();
    }
}

void MidiChordDetectorAudioProcessor::updateChordDetection()
{
    // Detect chord from current note state
    auto chord = chordDetector_.detectChord(
        noteState_,
        currentTimeMs_,
        useTimeWindow_,
        minNotesForChord_
    );
    
    // Publish result to UI thread
    publishChordResult(chord);
}

void MidiChordDetectorAudioProcessor::publishChordResult(const ChordDetection::ChordCandidate& chord)
{
    // Double-buffer technique for lock-free communication
    // Write to the buffer not currently being read
    int writeIndex = 1 - chordBufferIndex_;
    chordBuffer_[writeIndex] = chord;
    
    // Atomic swap to make new chord available
    currentChord_.store(&chordBuffer_[writeIndex], std::memory_order_release);
    chordBufferIndex_ = writeIndex;
    
    newChordAvailable_.store(true, std::memory_order_release);
}

ChordDetection::ChordCandidate MidiChordDetectorAudioProcessor::getCurrentChord() const
{
    auto* chordPtr = currentChord_.load(std::memory_order_acquire);
    return *chordPtr;
}

bool MidiChordDetectorAudioProcessor::hasNewChord() const
{
    return newChordAvailable_.exchange(false, std::memory_order_acq_rel);
}

bool MidiChordDetectorAudioProcessor::hasMidiActivity() const
{
    return midiActivityFlag_.exchange(false, std::memory_order_acq_rel);
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
