// HostedInstrumentPlayer.h
// In-process VST3 sub-hosting: lets the chord detector drive a third-party
// sound-generating instrument (Kontakt, Groove Agent, HALion, ...) so the
// plugin can produce audible output without implementing its own synth engine.
//
// See Docs/Technical-Design.md "Sound Engine (Hosted Instrument)" for the
// architecture rationale.

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>

namespace InstrumentHost {

/**
 * Hosts a single third-party VST3 instrument in-process.
 *
 * Design:
 * - Exactly one hosted instrument at a time; loading a new one replaces
 *   whatever was previously loaded.
 * - The same MIDI stream that drives chord detection is forwarded to the
 *   hosted instrument; its rendered audio is mixed into this plugin's own
 *   output buffer, so "what you play" is exactly "what you hear."
 * - Loading/unloading/instantiation happens on the message thread.
 *   renderNextBlock() is real-time safe: it uses a non-blocking try-lock
 *   and simply renders silence for that block if a swap is in flight —
 *   the same brief-glitch tradeoff any DAW makes when you swap a plugin
 *   while the transport is rolling.
 *
 * Thread-safety: prepare(), releaseResources(), loadPlugin(), unloadPlugin(),
 * showEditorWindow() and closeEditorWindow() must be called from the message
 * thread. renderNextBlock() must only be called from the audio thread.
 */
class HostedInstrumentPlayer
{
public:
    HostedInstrumentPlayer();
    ~HostedInstrumentPlayer();

    /** Allocates scratch buffers and (re)prepares any loaded instrument. Call from prepareToPlay(). */
    void prepare(double sampleRate, int maximumBlockSize, int numOutputChannels);

    /** Releases the hosted instrument's resources. Call from releaseResources(). */
    void releaseResources();

    /**
     * Scans a VST3 file/bundle and loads its (first) instrument sub-plugin,
     * replacing any instrument currently loaded. Must be called from the
     * message thread.
     *
     * @return true on success. On failure, errorMessage explains why and
     *         any previously loaded instrument stays loaded.
     */
    bool loadPlugin(const juce::File& pluginFile, juce::String& errorMessage);

    /** Unloads the currently hosted instrument, if any. Message thread only. */
    void unloadPlugin();

    [[nodiscard]] bool isLoaded() const noexcept;
    [[nodiscard]] juce::String getLoadedPluginName() const;

    /** Opens (or brings to front) the hosted plugin's own native editor window. */
    void showEditorWindow(const juce::String& windowTitle);

    /** Closes the hosted plugin's editor window, if open. */
    void closeEditorWindow();

    /**
     * Real-time safe. Forwards midiIn to the hosted instrument and mixes its
     * rendered audio into hostBuffer (additive — hostBuffer should already
     * hold this plugin's own output, typically silence). Channel-count
     * mismatches between the hosted instrument and hostBuffer are adapted
     * (e.g. a mono instrument is duplicated across all output channels).
     * No-op if no instrument is loaded.
     */
    void renderNextBlock(juce::AudioBuffer<float>& hostBuffer, const juce::MidiBuffer& midiIn);

    /** Serializes {plugin identity + plugin's own state} for the host's project file. */
    void getStateInformation(juce::MemoryBlock& destData) const;

    /** Restores a previously saved hosted instrument. Message thread only. */
    void setStateInformation(const void* data, int sizeInBytes);

private:
    class EditorWindow;

    void applyPendingStateToInstance(juce::AudioPluginInstance& newInstance);

    juce::AudioPluginFormatManager formatManager_;
    std::unique_ptr<juce::AudioPluginInstance> instance_;
    juce::PluginDescription loadedDescription_;
    std::unique_ptr<EditorWindow> editorWindow_;

    // Guards instance_ against concurrent access between the message thread
    // (load/unload) and the audio thread (renderNextBlock's try-lock).
    mutable juce::SpinLock swapLock_;

    double sampleRate_ = 44100.0;
    int blockSize_ = 512;
    int numOutputChannels_ = 2;

    juce::AudioBuffer<float> scratchBuffer_;
    juce::MidiBuffer scratchMidi_;

    // State restored from the host project before an instance exists for it
    // yet (e.g. right after setStateInformation triggers loadPlugin). Applied
    // once the instance is created, then cleared.
    juce::MemoryBlock pendingInstanceState_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HostedInstrumentPlayer)
};

} // namespace InstrumentHost
