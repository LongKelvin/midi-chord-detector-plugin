#include "ChordDisplayComponent.h"

ChordDisplayComponent::ChordDisplayComponent()
    : midiActivity_(false),
      hasAlternatives_(false),
      chordNameFont_(juce::Font(juce::FontOptions().withHeight(48.0f)).boldened()),
      detailFont_(juce::Font(juce::FontOptions().withHeight(14.0f))),
      alternativeFont_(juce::Font(juce::FontOptions().withHeight(12.0f)))
{
    
    chordNameString_ = "N.C.";
    descriptionString_ = "No Chord";
    inversionString_ = "";
    confidenceString_ = "";
    alternativesString_ = "";
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
    
    bounds.removeFromTop(15); // Spacing
    
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
    
    // Alternatives section (if available)
    if (hasAlternatives_ && !alternativesString_.isEmpty())
    {
        bounds.removeFromTop(15); // Spacing
        
        // Draw separator line
        g.setColour(juce::Colour(0xff404040));
        g.drawHorizontalLine(bounds.getY(), bounds.getX() + 20.0f, bounds.getRight() - 20.0f);
        
        bounds.removeFromTop(10);
        
        // "Also possible:" label
        g.setColour(juce::Colour(0xff777777));
        g.setFont(alternativeFont_);
        
        auto labelArea = bounds.removeFromTop(16);
        g.drawText("Also possible:", labelArea, juce::Justification::centred, true);
        
        bounds.removeFromTop(5);
        
        // Alternative chords
        g.setColour(juce::Colour(0xff66aacc));  // Light blue for alternatives
        auto altArea = bounds.removeFromTop(40);  // Allow for multi-line
        g.drawFittedText(alternativesString_, altArea, juce::Justification::centred, 3);
    }
}

void ChordDisplayComponent::resized()
{
    // Layout is handled in paint()
}

void ChordDisplayComponent::setChord(const std::shared_ptr<ChordDetection::ChordCandidate>& chord)
{
    // Only update and repaint if chord actually changed
    bool chordChanged = false;
    
    if (!currentChord_ && !chord) {
        // Both null - no change
        return;
    }
    else if (!currentChord_ || !chord) {
        // One is null, one isn't - changed
        chordChanged = true;
    }
    else {
        // Both valid - compare chord names
        chordChanged = (currentChord_->chordName != chord->chordName);
    }
    
    if (chordChanged) {
        currentChord_ = chord;
        updateDisplayStrings();
        repaint();
    }
}

void ChordDisplayComponent::clearChord()
{
    currentChord_ = nullptr;
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
    if (!currentChord_)
    {
        chordNameString_ = "N.C.";
        descriptionString_ = "No Chord";
        inversionString_ = "";
        confidenceString_ = "";
        alternativesString_ = "";
        hasAlternatives_ = false;
        return;
    }
    
    // Chord name
    chordNameString_ = juce::String(currentChord_->chordName);
    
    // Quality description - capitalize first letter
    std::string quality = currentChord_->chordType;
    if (!quality.empty())
    {
        quality[0] = static_cast<char>(std::toupper(quality[0]));
    }
    descriptionString_ = juce::String(quality);
    
    // Position/inversion
    inversionString_ = juce::String(currentChord_->position);
    
    // Confidence with ambiguity indicator
    int confidencePercent = static_cast<int>(currentChord_->confidence * 100.0f);
    confidenceString_ = juce::String("Confidence: ") + juce::String(confidencePercent) + "%";
    
    if (currentChord_->isAmbiguous)
    {
        confidenceString_ += " (ambiguous)";
    }
    
    // Build alternatives string
    hasAlternatives_ = currentChord_->hasAlternatives();
    if (hasAlternatives_)
    {
        juce::StringArray altStrings;
        for (const auto& alt : currentChord_->alternatives)
        {
            juce::String altStr = juce::String(alt.chordName);
            if (!alt.relationship.empty())
            {
                altStr += " (" + juce::String(alt.relationship) + ")";
            }
            altStrings.add(altStr);
        }
        alternativesString_ = altStrings.joinIntoString("  |  ");
    }
    else
    {
        alternativesString_ = "";
    }
}
