#include "ChordDisplayComponent.h"
#include "../Version.h"

ChordDisplayComponent::ChordDisplayComponent()
    : midiActivity_(false),
      showDebugPanel_(false),
      chordNameFont_(juce::Font(juce::FontOptions().withHeight(54.0f)).boldened()),
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
    // Professional synth design background
    g.fillAll(juce::Colour(0xff121317));
    
    // Border
    g.setColour(juce::Colour(0xff252833));
    g.drawRect(getLocalBounds(), 1);
    
    // MIDI Activity LED (top-right corner of the whole container)
    auto ledRect = getLocalBounds().removeFromTop(30).removeFromRight(30).reduced(8);
    if (midiActivity_)
    {
        g.setColour(juce::Colour(0xff00ff88)); // Neon green active
        g.fillEllipse(ledRect.toFloat());
        g.setColour(juce::Colour(0xffa6ffa6));
        g.drawEllipse(ledRect.toFloat().reduced(2), 1.5f);
    }
    else
    {
        g.setColour(juce::Colour(0xff1c202a)); // Dark passive
        g.fillEllipse(ledRect.toFloat());
        g.setColour(juce::Colour(0xff3a3f4d));
        g.drawEllipse(ledRect.toFloat().reduced(1), 1.0f);
    }

    auto fullBounds = getLocalBounds().reduced(15);
    
    if (showDebugPanel_)
    {
        // Split: Left for Chord name, Right for Debug Info
        float leftWidth = fullBounds.getWidth() * 0.58f;
        auto leftArea = fullBounds.removeFromLeft(static_cast<int>(leftWidth));
        auto rightArea = fullBounds; // remaining is Right Panel
        
        // --- LEFT ZONE: Main Chord Display (adaptive font for long names) ---
        auto chordNameArea = leftArea.removeFromTop(juce::roundToInt(leftArea.getHeight() * 0.65f));

        float chordFontSizeL = 54.0f;
        int nameLenL = chordNameString_.length();
        if (nameLenL > 8)  chordFontSizeL = 44.0f;
        if (nameLenL > 11) chordFontSizeL = 36.0f;
        if (nameLenL > 14) chordFontSizeL = 28.0f;
        auto adaptiveFontL = chordNameFont_.withHeight(chordFontSizeL);

        g.setFont(adaptiveFontL);
        g.setColour(juce::Colour(0xff00d4ff).withAlpha(0.18f));
        g.drawFittedText(chordNameString_, chordNameArea.translated(0, 1), juce::Justification::centred, 1, 0.5f);
        g.drawFittedText(chordNameString_, chordNameArea.translated(0, -1), juce::Justification::centred, 1, 0.5f);

        g.setColour(juce::Colours::white);
        g.drawFittedText(chordNameString_, chordNameArea, juce::Justification::centred, 1, 0.5f);

        leftArea.removeFromTop(5);

        g.setColour(juce::Colour(0xff00d4ff));
        g.setFont(detailFont_.boldened());
        g.drawFittedText(descriptionString_, leftArea, juce::Justification::centredTop, 2, 0.75f);
        
        // Split line
        g.setColour(juce::Colour(0xff252833));
        g.drawLine(static_cast<float>(rightArea.getX() - 10), static_cast<float>(fullBounds.getY()),
                   static_cast<float>(rightArea.getX() - 10), static_cast<float>(fullBounds.getBottom()), 1.5f);
                   
        // --- RIGHT ZONE: Technical/Info Panels ---
        // Let's draw clean info metrics
        int labelX = rightArea.getX() + 10;
        int valueX = rightArea.getRight() - 10;
        int itemY = rightArea.getY() + 15;
        int itemHeight = 26;
        
        g.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)).boldened());
        
        // Row width available for the value (right-justified)
        int rowW = valueX - labelX;

        // 1. Position / Inversion
        g.setColour(juce::Colour(0xff5c6273));
        g.drawText("POSITION", labelX, itemY, 100, itemHeight, juce::Justification::left);
        g.setColour(juce::Colours::white);
        juce::String posStr = currentChord_ ? juce::String(currentChord_->position) : "N/A";
        g.drawFittedText(posStr, labelX, itemY, rowW, itemHeight, juce::Justification::right, 1, 0.7f);
        
        itemY += itemHeight;
        
        // 2. Voicing Type
        g.setColour(juce::Colour(0xff5c6273));
        g.drawText("VOICING", labelX, itemY, 100, itemHeight, juce::Justification::left);
        g.setColour(juce::Colours::white);
        juce::String voicingStr = "N/A";
        if (currentChord_)
        {
            switch (currentChord_->voicingType)
            {
                case ChordDetection::VoicingType::Close: voicingStr = "Close"; break;
                case ChordDetection::VoicingType::Open: voicingStr = "Open"; break;
                case ChordDetection::VoicingType::Drop2: voicingStr = "Drop 2"; break;
                case ChordDetection::VoicingType::Drop3: voicingStr = "Drop 3"; break;
                case ChordDetection::VoicingType::Rootless: voicingStr = "Rootless"; break;
                default: voicingStr = "Unknown"; break;
            }
        }
        g.drawFittedText(voicingStr, labelX, itemY, rowW, itemHeight, juce::Justification::right, 1, 0.7f);
        
        itemY += itemHeight;
        
#if defined(DEVELOPER_DEBUG_MODE) && (DEVELOPER_DEBUG_MODE == 1)
        // ====== DEVELOPER DEBUG BUILD =====
        // 3. Confidence
        g.setColour(juce::Colour(0xff5c6273));
        g.drawText("CONFIDENCE", labelX, itemY, 100, itemHeight, juce::Justification::left);
        g.setColour(juce::Colour(0xff00d4ff));
        juce::String confStr = currentChord_ ? juce::String(juce::roundToInt(currentChord_->confidence * 100.0f)) + "%" : "0%";
        g.drawFittedText(confStr, labelX, itemY, rowW, itemHeight, juce::Justification::right, 1, 0.7f);
        
        itemY += itemHeight;
        
        // 4. Score
        g.setColour(juce::Colour(0xff5c6273));
        g.drawText("RAW SCORE", labelX, itemY, 100, itemHeight, juce::Justification::left);
        g.setColour(juce::Colours::white);
        juce::String scoreStr = currentChord_ ? juce::String(currentChord_->score, 2) : "0.00";
        g.drawFittedText(scoreStr, labelX, itemY, rowW, itemHeight, juce::Justification::right, 1, 0.7f);

        itemY += itemHeight;

        // 5. Active pitch classes (as raw integers)
        g.setColour(juce::Colour(0xff5c6273));
        g.drawText("PITCH CLASSES", labelX, itemY, 100, itemHeight, juce::Justification::left);
        g.setColour(juce::Colour(0xffabafbc));
        juce::String pitchesStr;
        if (currentChord_ && !currentChord_->pitchClasses.empty())
        {
            for (size_t i = 0; i < currentChord_->pitchClasses.size(); ++i)
            {
                pitchesStr += juce::String(currentChord_->pitchClasses[i]);
                if (i < currentChord_->pitchClasses.size() - 1)
                    pitchesStr += ",";
            }
        }
        else
        {
            pitchesStr = "-";
        }
        g.drawFittedText(pitchesStr, labelX, itemY, rowW, itemHeight, juce::Justification::right, 1, 0.7f);

#else
        // ====== DAW PRODUCTION BUILD =====
        // 3. Root note representation
        g.setColour(juce::Colour(0xff5c6273));
        g.drawText("ROOT NOTE", labelX, itemY, 100, itemHeight, juce::Justification::left);
        g.setColour(juce::Colour(0xff00d4ff));
        juce::String rootStr = currentChord_ ? juce::String(currentChord_->rootName) : "N/A";
        g.drawFittedText(rootStr, labelX, itemY, rowW, itemHeight, juce::Justification::right, 1, 0.7f);
        
        itemY += itemHeight;

        // 4. Chord Degrees
        g.setColour(juce::Colour(0xff5c6273));
        g.drawText("INTERVALS", labelX, itemY, 100, itemHeight, juce::Justification::left);
        g.setColour(juce::Colours::white);
        juce::String degStr = "-";
        if (currentChord_ && !currentChord_->degrees.empty())
        {
            degStr = "";
            for (size_t i = 0; i < currentChord_->degrees.size(); ++i)
            {
                degStr += juce::String(currentChord_->degrees[i]);
                if (i < currentChord_->degrees.size() - 1)
                    degStr += ", ";
            }
        }
        // Allow two lines for long interval lists (e.g. 1, b3, 5, b7, 9, 11, 13)
        g.drawFittedText(degStr, labelX, itemY, rowW, itemHeight * 2, juce::Justification::topRight, 2, 0.8f);
        itemY += itemHeight;

        itemY += itemHeight;

        // 5. Active pitches (rendered by pitch names)
        g.setColour(juce::Colour(0xff5c6273));
        g.drawText("PITCH HEADS", labelX, itemY, 100, itemHeight, juce::Justification::left);
        g.setColour(juce::Colour(0xffabafbc));
        juce::String notesPlayedStr = "-";
        if (currentChord_ && !currentChord_->noteNames.empty())
        {
            notesPlayedStr = "";
            for (size_t i = 0; i < currentChord_->noteNames.size(); ++i)
            {
                notesPlayedStr += juce::String(currentChord_->noteNames[i]);
                if (i < currentChord_->noteNames.size() - 1)
                    notesPlayedStr += ", ";
            }
        }
        g.drawFittedText(notesPlayedStr, labelX, itemY, rowW, itemHeight * 2, juce::Justification::topRight, 2, 0.8f);
#endif
    }
    else
    {
        // --- MINIMALIST CENTRALIZED ZONE ---
        auto centerArea = fullBounds;
        
        // Large chord name — auto-scales font to fit any chord name length
        auto chordNameArea = centerArea.removeFromTop(juce::roundToInt(centerArea.getHeight() * 0.65f));

        // Adaptive font: shrink for long names so they never overflow
        float chordFontSize = 54.0f;
        int nameLen = chordNameString_.length();
        if (nameLen > 8)  chordFontSize = 44.0f;
        if (nameLen > 11) chordFontSize = 36.0f;
        if (nameLen > 14) chordFontSize = 28.0f;
        auto adaptiveFont = chordNameFont_.withHeight(chordFontSize);

        // Subtle glow effect
        g.setColour(juce::Colour(0xff00d4ff).withAlpha(0.18f));
        g.setFont(adaptiveFont);
        g.drawFittedText(chordNameString_, chordNameArea.translated(0, 1), juce::Justification::centred, 1, 0.5f);
        g.drawFittedText(chordNameString_, chordNameArea.translated(0, -1), juce::Justification::centred, 1, 0.5f);

        g.setColour(juce::Colours::white);
        g.drawFittedText(chordNameString_, chordNameArea, juce::Justification::centred, 1, 0.5f);

        // Spacing
        centerArea.removeFromTop(5);

        // Quality description — fitted so long types don't overflow
        g.setColour(juce::Colour(0xff00d4ff));
        g.setFont(detailFont_.boldened());
        g.drawFittedText(descriptionString_, centerArea, juce::Justification::centredTop, 2, 0.75f);
    }
}

void ChordDisplayComponent::resized()
{
}

void ChordDisplayComponent::setChord(const std::shared_ptr<ChordDetection::ChordCandidate>& chord)
{
    currentChord_ = chord;
    updateDisplayStrings();
    repaint();
}

void ChordDisplayComponent::clearChord()
{
    currentChord_ = nullptr;
    updateDisplayStrings();
    repaint();
}

void ChordDisplayComponent::setDebugMode(bool show)
{
    if (showDebugPanel_ != show)
    {
        showDebugPanel_ = show;
        repaint();
    }
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
    
    inversionString_ = {};
    confidenceString_ = {};
}
