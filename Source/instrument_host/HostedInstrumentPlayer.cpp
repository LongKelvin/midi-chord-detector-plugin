#include "HostedInstrumentPlayer.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace InstrumentHost {

//==============================================================================
// Thin DocumentWindow that owns and displays the hosted plugin's own native
// editor. Closing it just hides it -- the editor component (and the hosted
// instance) keep running so patches/state aren't lost by closing the window.
class HostedInstrumentPlayer::EditorWindow : public juce::DocumentWindow
{
public:
    EditorWindow(const juce::String& title, juce::AudioProcessorEditor* editor)
        : juce::DocumentWindow(title, juce::Colours::darkgrey, juce::DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(editor, true);
        setResizable(editor->isResizable(), false);
        centreWithSize(getWidth(), getHeight());
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }
};

//==============================================================================
HostedInstrumentPlayer::HostedInstrumentPlayer()
{
    // Only formats enabled via JUCE_PLUGINHOST_* compile definitions are
    // actually registered here -- this plugin only enables VST3.
    formatManager_.addDefaultFormats();
}

HostedInstrumentPlayer::~HostedInstrumentPlayer()
{
    closeEditorWindow();
    unloadPlugin();
}

//==============================================================================
void HostedInstrumentPlayer::prepare(double sampleRate, int maximumBlockSize, int numOutputChannels)
{
    sampleRate_ = sampleRate;
    blockSize_ = maximumBlockSize;
    numOutputChannels_ = juce::jmax(1, numOutputChannels);

    scratchBuffer_.setSize(numOutputChannels_, blockSize_);
    scratchMidi_.ensureSize(2048);

    const juce::SpinLock::ScopedLockType lock(swapLock_);
    if (instance_ != nullptr)
        instance_->prepareToPlay(sampleRate_, blockSize_);
}

void HostedInstrumentPlayer::releaseResources()
{
    const juce::SpinLock::ScopedLockType lock(swapLock_);
    if (instance_ != nullptr)
        instance_->releaseResources();
}

//==============================================================================
bool HostedInstrumentPlayer::loadPlugin(const juce::File& pluginFile, juce::String& errorMessage)
{
    if (! pluginFile.exists())
    {
        errorMessage = "\"" + pluginFile.getFullPathName() + "\" does not exist.";
        return false;
    }

    juce::OwnedArray<juce::PluginDescription> found;
    for (auto* format : formatManager_.getFormats())
        format->findAllTypesForFile(found, pluginFile.getFullPathName());

    if (found.isEmpty())
    {
        errorMessage = "No supported instrument was found in \"" + pluginFile.getFullPathName() + "\".";
        return false;
    }

    // A bundle can expose several sub-plugins (e.g. an FX + instrument pair);
    // prefer the first one that identifies itself as an instrument.
    const juce::PluginDescription* chosen = found.getFirst();
    for (auto* description : found)
    {
        if (description->isInstrument)
        {
            chosen = description;
            break;
        }
    }

    auto newInstance = formatManager_.createPluginInstance(*chosen, sampleRate_, blockSize_, errorMessage);
    if (newInstance == nullptr)
        return false;

    newInstance->prepareToPlay(sampleRate_, blockSize_);
    applyPendingStateToInstance(*newInstance);

    // Close any editor bound to the previous instance before swapping it out.
    closeEditorWindow();

    {
        const juce::SpinLock::ScopedLockType lock(swapLock_);
        if (instance_ != nullptr)
            instance_->releaseResources();
        instance_ = std::move(newInstance);
        loadedDescription_ = *chosen;
    }

    return true;
}

void HostedInstrumentPlayer::unloadPlugin()
{
    closeEditorWindow();

    const juce::SpinLock::ScopedLockType lock(swapLock_);
    if (instance_ != nullptr)
    {
        instance_->releaseResources();
        instance_.reset();
    }
    loadedDescription_ = {};
}

void HostedInstrumentPlayer::applyPendingStateToInstance(juce::AudioPluginInstance& newInstance)
{
    if (pendingInstanceState_.getSize() > 0)
    {
        newInstance.setStateInformation(pendingInstanceState_.getData(),
                                         static_cast<int>(pendingInstanceState_.getSize()));
        pendingInstanceState_.reset();
    }
}

//==============================================================================
bool HostedInstrumentPlayer::isLoaded() const noexcept
{
    const juce::SpinLock::ScopedLockType lock(swapLock_);
    return instance_ != nullptr;
}

juce::String HostedInstrumentPlayer::getLoadedPluginName() const
{
    const juce::SpinLock::ScopedLockType lock(swapLock_);
    return instance_ != nullptr ? loadedDescription_.name : juce::String();
}

//==============================================================================
void HostedInstrumentPlayer::showEditorWindow(const juce::String& windowTitle)
{
    juce::AudioPluginInstance* activeInstance = nullptr;
    {
        const juce::SpinLock::ScopedLockType lock(swapLock_);
        activeInstance = instance_.get();
    }

    if (activeInstance == nullptr)
        return;

    if (editorWindow_ != nullptr)
    {
        editorWindow_->setVisible(true);
        editorWindow_->toFront(true);
        return;
    }

    if (auto* editor = activeInstance->createEditorIfNeeded())
        editorWindow_ = std::make_unique<EditorWindow>(windowTitle, editor);
}

void HostedInstrumentPlayer::closeEditorWindow()
{
    editorWindow_.reset();
}

//==============================================================================
void HostedInstrumentPlayer::renderNextBlock(juce::AudioBuffer<float>& hostBuffer, const juce::MidiBuffer& midiIn)
{
    const juce::SpinLock::ScopedTryLockType tryLock(swapLock_);

    // No instrument loaded, or one is being swapped in/out on the message
    // thread right now -- leave hostBuffer as-is (silence) for this block
    // rather than blocking the audio thread.
    if (! tryLock.isLocked() || instance_ == nullptr)
        return;

    const int numSamples = hostBuffer.getNumSamples();
    const int hostedChannels = juce::jmax(1, instance_->getTotalNumOutputChannels());

    scratchBuffer_.setSize(hostedChannels, numSamples, false, false, true);
    scratchBuffer_.clear();

    scratchMidi_.clear();
    scratchMidi_.addEvents(midiIn, 0, numSamples, 0);

    instance_->processBlock(scratchBuffer_, scratchMidi_);

    const int outChannels = hostBuffer.getNumChannels();
    for (int channel = 0; channel < outChannels; ++channel)
    {
        const int sourceChannel = juce::jmin(channel, hostedChannels - 1);
        hostBuffer.addFrom(channel, 0, scratchBuffer_, sourceChannel, 0, numSamples);
    }
}

//==============================================================================
void HostedInstrumentPlayer::getStateInformation(juce::MemoryBlock& destData) const
{
    juce::ValueTree state("HostedInstrument");

    const juce::SpinLock::ScopedLockType lock(swapLock_);
    if (instance_ != nullptr)
    {
        state.setProperty("pluginName", loadedDescription_.name, nullptr);
        state.setProperty("pluginFile", loadedDescription_.fileOrIdentifier, nullptr);
        state.setProperty("pluginFormat", loadedDescription_.pluginFormatName, nullptr);
        state.setProperty("pluginUid", static_cast<int>(loadedDescription_.uniqueId), nullptr);

        juce::MemoryBlock instanceState;
        instance_->getStateInformation(instanceState);
        state.setProperty("instanceState", instanceState.toBase64Encoding(), nullptr);
    }

    if (auto xml = state.createXml())
        juce::AudioProcessor::copyXmlToBinary(*xml, destData);
}

void HostedInstrumentPlayer::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(juce::AudioProcessor::getXmlFromBinary(data, sizeInBytes));
    if (xml == nullptr)
        return;

    juce::ValueTree state = juce::ValueTree::fromXml(*xml);
    const juce::String pluginFile = state.getProperty("pluginFile").toString();
    if (pluginFile.isEmpty())
        return;

    juce::MemoryBlock instanceState;
    instanceState.fromBase64Encoding(state.getProperty("instanceState").toString());
    pendingInstanceState_ = instanceState;

    juce::String errorMessage;
    if (! loadPlugin(juce::File(pluginFile), errorMessage))
    {
        // The saved instrument couldn't be found/loaded on this machine
        // (moved, uninstalled, different plugin folder, ...). Drop the
        // pending state -- the chord detector still works standalone.
        pendingInstanceState_.reset();
        juce::Logger::writeToLog("HostedInstrumentPlayer: failed to restore \"" + pluginFile + "\": " + errorMessage);
    }
}

} // namespace InstrumentHost
