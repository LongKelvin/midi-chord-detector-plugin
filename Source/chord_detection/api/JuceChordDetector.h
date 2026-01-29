// JuceChordDetector.h
// JUCE-friendly wrapper for the Optimized Chord Detector
// Easy integration with JUCE audio applications

#pragma once

#include "../detector/ChordDetector.h"
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <functional>

namespace ChordDetection {

/**
 * JUCE-friendly wrapper for chord detection
 * 
 * Usage in JUCE:
 * 
 * // In your audio processor or component:
 * JuceChordDetector detector;
 * 
 * // From MIDI buffer:
 * for (const auto metadata : midiBuffer) {
 *     auto message = metadata.getMessage();
 *     if (message.isNoteOn()) {
 *         detector.addNote(message.getNoteNumber());
 *     }
 *     else if (message.isNoteOff()) {
 *         detector.removeNote(message.getNoteNumber());
 *     }
 * }
 * 
 * // Get current chord:
 * auto chord = detector.getCurrentChord();
 * if (chord) {
 *     DBG("Detected: " + juce::String(chord->chordName));
 * }
 */
class JuceChordDetector {
public:
    explicit JuceChordDetector(SlashChordMode mode = SlashChordMode::Auto)
        : detector_(false, mode)
    {}
    
    ~JuceChordDetector() = default;
    
    // ========================================================================
    // MIDI NOTE TRACKING
    // ========================================================================
    
    /**
     * Add a note to the current chord (call on NoteOn)
     */
    void addNote(int midiNoteNumber) {
        if (midiNoteNumber >= 0 && midiNoteNumber <= 127) {
            currentNotes_.insert(midiNoteNumber);
            updateDetection();
        }
    }
    
    /**
     * Remove a note from the current chord (call on NoteOff)
     */
    void removeNote(int midiNoteNumber) {
        currentNotes_.erase(midiNoteNumber);
        updateDetection();
    }
    
    /**
     * Clear all currently held notes
     */
    void clearNotes() {
        currentNotes_.clear();
        currentChord_ = nullptr;
    }
    
    /**
     * Set notes directly (useful for non-realtime detection)
     */
    void setNotes(const std::vector<int>& notes) {
        currentNotes_.clear();
        for (int note : notes) {
            if (note >= 0 && note <= 127) {
                currentNotes_.insert(note);
            }
        }
        updateDetection();
    }
    
    // ========================================================================
    // JUCE MIDI BUFFER PROCESSING
    // ========================================================================
    
    /**
     * Process a JUCE MIDI buffer and update chord detection
     */
    void processMidiBuffer(const juce::MidiBuffer& midiBuffer) {
        for (const auto metadata : midiBuffer) {
            auto message = metadata.getMessage();
            
            if (message.isNoteOn()) {
                addNote(message.getNoteNumber());
            }
            else if (message.isNoteOff()) {
                removeNote(message.getNoteNumber());
            }
            else if (message.isAllNotesOff() || message.isAllSoundOff()) {
                clearNotes();
            }
        }
    }
    
    // ========================================================================
    // CHORD DETECTION
    // ========================================================================
    
    /**
     * Get the currently detected chord (returns nullptr if no valid chord)
     */
    [[nodiscard]] std::shared_ptr<ChordCandidate> getCurrentChord() const noexcept {
        return currentChord_;
    }
    
    /**
     * Detect chord from specific MIDI notes (doesn't affect current state)
     */
    std::shared_ptr<ChordCandidate> detectChord(const std::vector<int>& notes) {
        return detector_.detectChord(notes);
    }
    
    /**
     * Get currently held MIDI note numbers
     */
    [[nodiscard]] std::vector<int> getCurrentNotes() const {
        return std::vector<int>(currentNotes_.begin(), currentNotes_.end());
    }
    
    /**
     * Check if any notes are currently held
     */
    [[nodiscard]] bool hasNotes() const noexcept {
        return !currentNotes_.empty();
    }
    
    /**
     * Get number of currently held notes
     */
    [[nodiscard]] int getNoteCount() const noexcept {
        return static_cast<int>(currentNotes_.size());
    }
    
    // ========================================================================
    // FORMATTING & DISPLAY
    // ========================================================================
    
    /**
     * Get chord name as JUCE String
     */
    [[nodiscard]] juce::String getChordName() const {
        if (currentChord_) {
            return juce::String(currentChord_->chordName);
        }
        return juce::String();
    }
    
    /**
     * Get root note name as JUCE String
     */
    [[nodiscard]] juce::String getRootName() const {
        if (currentChord_) {
            return juce::String(currentChord_->rootName);
        }
        return juce::String();
    }
    
    /**
     * Get chord quality (major, minor, dominant, etc.)
     */
    [[nodiscard]] juce::String getQuality() const {
        if (currentChord_) {
            auto& patterns = detector_.getChordPatterns();
            auto it = patterns.find(currentChord_->chordType);
            if (it != patterns.end()) {
                return juce::String(it->second.quality);
            }
        }
        return juce::String();
    }
    
    /**
     * Get confidence score (0.0 to 1.0)
     */
    [[nodiscard]] float getConfidence() const noexcept {
        if (currentChord_) {
            return currentChord_->confidence;
        }
        return 0.0f;
    }
    
    /**
     * Get formatted output as JUCE String
     */
    [[nodiscard]] juce::String getFormattedOutput() const {
        return juce::String(detector_.formatOutput(currentChord_));
    }
    
    // ========================================================================
    // CONFIGURATION
    // ========================================================================
    
    /**
     * Set slash chord display mode
     */
    void setSlashChordMode(SlashChordMode mode) {
        detector_.setSlashChordMode(mode);
        updateDetection();
    }
    
    /**
     * Enable/disable context-aware detection
     */
    void setEnableContext(bool enable) {
        detector_.setEnableContext(enable);
    }
    
    /**
     * Set minimum notes required for detection (default: 2)
     */
    void setMinimumNotes(int minNotes) {
        minimumNotes_ = juce::jmax(2, minNotes);
        updateDetection();
    }
    
    // ========================================================================
    // CALLBACKS (for GUI updates, etc.)
    // ========================================================================
    
    /**
     * Callback type for chord change notifications
     * Parameters: (newChord, previousChord)
     */
    using ChordChangeCallback = std::function<void(
        std::shared_ptr<ChordCandidate>, 
        std::shared_ptr<ChordCandidate>
    )>;
    
    /**
     * Set callback to be notified when chord changes
     */
    void setChordChangeCallback(ChordChangeCallback callback) {
        chordChangeCallback_ = callback;
    }
    
private:
    void updateDetection() {
        if (currentNotes_.size() < static_cast<size_t>(minimumNotes_)) {
            if (currentChord_ != nullptr) {
                auto previousChord = currentChord_;
                currentChord_ = nullptr;
                
                if (chordChangeCallback_) {
                    chordChangeCallback_(currentChord_, previousChord);
                }
            }
            return;
        }
        
        std::vector<int> notes(currentNotes_.begin(), currentNotes_.end());
        auto newChord = detector_.detectChord(notes);
        
        // Check if chord actually changed
        bool changed = false;
        if (!currentChord_ && newChord) {
            changed = true;
        }
        else if (currentChord_ && !newChord) {
            changed = true;
        }
        else if (currentChord_ && newChord) {
            changed = (currentChord_->chordName != newChord->chordName);
        }
        
        if (changed) {
            auto previousChord = currentChord_;
            currentChord_ = newChord;
            
            if (chordChangeCallback_) {
                chordChangeCallback_(currentChord_, previousChord);
            }
        }
    }
    
    OptimizedChordDetector detector_;
    std::set<int> currentNotes_;
    std::shared_ptr<ChordCandidate> currentChord_;
    int minimumNotes_ = 2;
    ChordChangeCallback chordChangeCallback_;
};

// ============================================================================
// JUCE VALUE TREE HELPERS (for saving/loading chord data)
// ============================================================================

/**
 * Convert ChordCandidate to JUCE ValueTree for serialization
 */
inline juce::ValueTree chordToValueTree(const std::shared_ptr<ChordCandidate>& chord) {
    if (!chord) {
        return juce::ValueTree("Chord");
    }
    
    juce::ValueTree tree("Chord");
    tree.setProperty("name", juce::String(chord->chordName), nullptr);
    tree.setProperty("root", chord->root, nullptr);
    tree.setProperty("rootName", juce::String(chord->rootName), nullptr);
    tree.setProperty("type", juce::String(chord->chordType), nullptr);
    tree.setProperty("position", juce::String(chord->position), nullptr);
    tree.setProperty("score", chord->score, nullptr);
    tree.setProperty("confidence", chord->confidence, nullptr);
    
    // Store note numbers as string
    juce::String notesStr;
    for (size_t i = 0; i < chord->noteNumbers.size(); ++i) {
        if (i > 0) notesStr += " ";
        notesStr += juce::String(chord->noteNumbers[i]);
    }
    tree.setProperty("notes", notesStr, nullptr);
    
    return tree;
}

} // namespace ChordDetection
