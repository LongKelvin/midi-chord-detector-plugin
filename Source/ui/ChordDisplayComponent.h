#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "core/ChordResolver.h"

/**
 * ChordDisplayComponent - Displays detected chord information
 * 
 * Shows:
 * - Large chord name (e.g., "Cmaj7/E")
 * - Chord description ("Major 7th")
 * - Inversion type
 * - Confidence score
 * 
 * Design:
 * - Dark background with light text
 * - Large, readable font for chord name
 * - Smaller text for details
 * - Clean, professional appearance
 */
class ChordDisplayComponent : public juce::Component
{
public:
    ChordDisplayComponent();
    ~ChordDisplayComponent() override;
    
    void paint (juce::Graphics&) override;
    void resized() override;
    
    void setMidiActivity(bool active);
    
    /**
     * Update the displayed chord
     * Call from UI thread when new chord is available
     */
    void setChord(const ChordDetection::ResolvedChord& chord);
    
    /**
     * Clear the display (show "N.C.")
     */
    void clearChord();

private:
    ChordDetection::ResolvedChord currentChord_;
    
    juce::Font chordNameFont_;
    juce::Font detailFont_;
    
    // Cached strings to avoid allocations in paint()
    juce::String chordNameString_;
    juce::String descriptionString_;
    juce::String inversionString_;
    juce::String confidenceString_;
    
    bool midiActivity_;
    
    void updateDisplayStrings();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordDisplayComponent)
};
