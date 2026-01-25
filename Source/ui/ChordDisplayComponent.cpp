#include "ChordDisplayComponent.h"

ChordDisplayComponent::ChordDisplayComponent()
    : midiActivity_(false),
      chordNameFont_(juce::Font(juce::FontOptions().withHeight(48.0f)).boldened()),
      detailFont_(juce::Font(juce::FontOptions().withHeight(14.0f)))
{
    
    chordNameString_ = "N.C.";
    descriptionString_ = "No Chord";
    inversionString_ = "";
    confidenceString_ = "";
}

ChordDisplayComponent::~ChordDisplayComponent()
{
}

void ChordDisplayComponent::paint (juce::Graphics& g)
{
    // Dark background
    g.fillAll(juce::Colour(0xff1a1a1a));
    
    // Border
    g.setColour(juce::Colour(0xff404040));
    g.drawRect(getLocalBounds(), 1);
    
    // MIDI Activity LED (top-right corner)
    auto ledBounds = getLocalBounds().removeFromTop(30).removeFromRight(30).reduced(8);
    if (midiActivity_)
    {
        g.setColour(juce::Colours::green);
        g.fillEllipse(ledBounds.toFloat());
        g.setColour(juce::Colours::lightgreen);
        g.drawEllipse(ledBounds.toFloat().reduced(2), 2.0f);
    }
    else
    {
        g.setColour(juce::Colour(0xff404040));
        g.fillEllipse(ledBounds.toFloat());
        g.setColour(juce::Colour(0xff606060));
        g.drawEllipse(ledBounds.toFloat().reduced(2), 1.0f);
    }
    
    auto bounds = getLocalBounds().reduced(20);
    
    // Chord name (large, centered)
    g.setColour(juce::Colours::white);
    g.setFont(chordNameFont_);
    
    auto chordNameArea = bounds.removeFromTop(80);
    g.drawText(chordNameString_, chordNameArea,
               juce::Justification::centred, true);
    
    bounds.removeFromTop(10); // Spacing
    
    // Description
    g.setColour(juce::Colour(0xffcccccc));
    g.setFont(detailFont_);
    
    auto descArea = bounds.removeFromTop(20);
    g.drawText(descriptionString_, descArea,
               juce::Justification::centred, true);
    
    bounds.removeFromTop(20); // Spacing
    
    // Inversion info
    if (!inversionString_.isEmpty())
    {
        g.setColour(juce::Colour(0xff999999));
        auto invArea = bounds.removeFromTop(18);
        g.drawText(inversionString_, invArea,
                   juce::Justification::centred, true);
    }
    
    // Confidence
    if (!confidenceString_.isEmpty())
    {
        bounds.removeFromTop(5);
        g.setColour(juce::Colour(0xff888888));
        auto confArea = bounds.removeFromTop(18);
        g.drawText(confidenceString_, confArea,
                   juce::Justification::centred, true);
    }
}

void ChordDisplayComponent::resized()
{
    // Layout is handled in paint()
}

void ChordDisplayComponent::setChord(const ChordDetection::ChordCandidate& chord)
{
    currentChord_ = chord;
    updateDisplayStrings();
    repaint();
}

void ChordDisplayComponent::clearChord()
{
    currentChord_ = ChordDetection::ChordCandidate();
    updateDisplayStrings();
    repaint();
}

void ChordDisplayComponent::setMidiActivity(bool active)
{
    if (midiActivity_ != active)
    {
        midiActivity_ = active;
        repaint();
    }
}

void ChordDisplayComponent::updateDisplayStrings()
{
    if (!currentChord_.isValid)
    {
        chordNameString_ = "N.C.";
        descriptionString_ = "No Chord";
        inversionString_ = "";
        confidenceString_ = "";
        return;
    }
    
    // Chord name
    char chordNameBuffer[64];
    currentChord_.getChordName(chordNameBuffer, sizeof(chordNameBuffer));
    chordNameString_ = juce::String(chordNameBuffer);
    
    // Description
    descriptionString_ = juce::String(currentChord_.getChordDescription());
    
    // Inversion
    inversionString_ = juce::String(currentChord_.getInversionText());
    
    // Confidence
    int confidencePercent = static_cast<int>(currentChord_.confidence * 100.0f);
    confidenceString_ = juce::String("Confidence: ") + juce::String(confidencePercent) + "%";
}
