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
    , activeNotesBufferIndex_(0)
{
    // Initialize chord buffers with nullptr (empty state)
    chordBuffer_[0] = nullptr;
    chordBuffer_[1] = nullptr;
    
    // Initialize active notes buffers
    activeNotesBuffer_[0] = std::vector<int>();
    activeNotesBuffer_[1] = std::vector<int>();
    
    sustainPedalDown_ = false;
    sustainedNotes_.reset();
    
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

    // Prepare the hosted sound-engine instrument (if any) for playback.
    instrumentHost_.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
}

void MidiChordDetectorAudioProcessor::releaseResources()
{
    // Clear state when playback stops
    chordDetector_.clearNotes();

    instrumentHost_.releaseResources();
}

bool MidiChordDetectorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // VST3 Instrument: Accept mono or stereo output. Output carries the
    // hosted sound-engine instrument's audio (see instrumentHost_), or
    // silence if none is loaded.
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
    
    // Copy incoming MIDI to preserve for output (explicit pass-through for host compatibility)
    juce::MidiBuffer processedMidi;
    
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
        processedMidi.addEvent(message, samplePosition);
    }
    
    // Publish chord result if notes changed
    if (chordMayHaveChanged)
    {
        auto chord = chordDetector_.getCurrentChord();
        publishChordResult(chord);
    }

    // Drive the sound engine with the same notes powering chord detection,
    // so what's heard always matches what's displayed. No-op if no
    // instrument is loaded.
    instrumentHost_.renderNextBlock(buffer, midiMessages);

    // Swap the processed MIDI buffer to output
    midiMessages.swapWith(processedMidi);
}

void MidiChordDetectorAudioProcessor::processMidiMessage(const juce::MidiMessage& message)
{
    if (message.isController() && message.getControllerNumber() == 64)
    {
        const bool pedalNowDown = (message.getControllerValue() >= 64);

        if (!pedalNowDown && sustainPedalDown_)
        {
            // Release: flush all sustained notes
            for (int i = 0; i < 128; ++i)
            {
                if (sustainedNotes_.test(static_cast<size_t>(i)))
                {
                    chordDetector_.removeNote(i);
                }
            }
            sustainedNotes_.reset();
        }

        sustainPedalDown_ = pedalNowDown;
    }
    else if (message.isNoteOn())
    {
        sustainedNotes_.reset(static_cast<size_t>(message.getNoteNumber()));
        chordDetector_.addNote(message.getNoteNumber());
    }
    else if (message.isNoteOff())
    {
        if (sustainPedalDown_)
        {
            sustainedNotes_.set(static_cast<size_t>(message.getNoteNumber()));
        }
        else
        {
            chordDetector_.removeNote(message.getNoteNumber());
        }
    }
    else if (message.isController())
    {
        // All notes off (CC 123)
        if (message.getControllerNumber() == 123)
        {
            sustainedNotes_.reset();
            sustainPedalDown_ = false;
            chordDetector_.clearNotes();
        }
    }
    else if (message.isAllNotesOff() || message.isAllSoundOff())
    {
        sustainedNotes_.reset();
        sustainPedalDown_ = false;
        chordDetector_.clearNotes();
    }
}

void MidiChordDetectorAudioProcessor::publishChordResult(const std::shared_ptr<ChordDetection::ChordCandidate>& chord)
{
    // Double-buffer technique for lock-free communication.
    // Compute the write index from the current read index atomically so the UI
    // thread never sees a torn value.
    int readIndex  = chordBufferIndex_.load(std::memory_order_acquire);
    int writeIndex = 1 - readIndex;
    chordBuffer_[writeIndex] = chord;

    // Publish the raw pointer atomically so the UI can validate liveness.
    currentChordPtr_.store(chordBuffer_[writeIndex].get(), std::memory_order_release);

    // Flip the buffer index last — this is the commit point seen by UI.
    chordBufferIndex_.store(writeIndex, std::memory_order_release);

    // Swap active notes buffer in the same lock-free double-buffered way
    int notesReadIndex = activeNotesBufferIndex_.load(std::memory_order_acquire);
    int notesWriteIndex = 1 - notesReadIndex;
    activeNotesBuffer_[notesWriteIndex] = chordDetector_.getCurrentNotes();
    activeNotesBufferIndex_.store(notesWriteIndex, std::memory_order_release);

    newChordAvailable_.store(true, std::memory_order_release);
}

std::shared_ptr<ChordDetection::ChordCandidate> MidiChordDetectorAudioProcessor::getCurrentChord() const
{
    // Acquire the current buffer index atomically before reading the shared_ptr.
    int index = chordBufferIndex_.load(std::memory_order_acquire);
    return chordBuffer_[index];
}

bool MidiChordDetectorAudioProcessor::hasNewChord() const
{
    return newChordAvailable_.exchange(false, std::memory_order_acq_rel);
}

bool MidiChordDetectorAudioProcessor::hasMidiActivity() const
{
    return midiActivityFlag_.exchange(false, std::memory_order_acq_rel);
}
std::vector<int> MidiChordDetectorAudioProcessor::getCurrentNotes() const
{
    int index = activeNotesBufferIndex_.load(std::memory_order_acquire);
    return activeNotesBuffer_[index];
}
void MidiChordDetectorAudioProcessor::setSlashChordMode(ChordDetection::SlashChordMode mode)
{
    chordDetector_.setSlashChordMode(mode);
    publishChordResult(chordDetector_.getCurrentChord());
}

void MidiChordDetectorAudioProcessor::setMinimumNotes(int minNotes)
{
    chordDetector_.setMinimumNotes(minNotes);
    publishChordResult(chordDetector_.getCurrentChord());
}

//==============================================================================
bool MidiChordDetectorAudioProcessor::loadInstrumentPlugin(const juce::File& pluginFile, juce::String& errorMessage)
{
    return instrumentHost_.loadPlugin(pluginFile, errorMessage);
}

void MidiChordDetectorAudioProcessor::unloadInstrumentPlugin()
{
    instrumentHost_.unloadPlugin();
}

bool MidiChordDetectorAudioProcessor::hasInstrumentLoaded() const
{
    return instrumentHost_.isLoaded();
}

juce::String MidiChordDetectorAudioProcessor::getLoadedInstrumentName() const
{
    return instrumentHost_.getLoadedPluginName();
}

void MidiChordDetectorAudioProcessor::showInstrumentEditor()
{
    auto name = instrumentHost_.getLoadedPluginName();
    instrumentHost_.showEditorWindow(name.isNotEmpty() ? "Sound Engine - " + name : "Sound Engine");
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
    // Persist the hosted sound-engine instrument (if any), including its own
    // patch/state, so the DAW project restores it on reload.
    // TODO: Persist our own settings (slash chord mode, minimum notes) too.
    instrumentHost_.getStateInformation(destData);
}

void MidiChordDetectorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    instrumentHost_.setStateInformation(data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidiChordDetectorAudioProcessor();
}
