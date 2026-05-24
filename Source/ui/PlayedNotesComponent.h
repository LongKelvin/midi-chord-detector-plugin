#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../chord_detection/detector/NoteUtils.h"
#include <vector>

/**
 * PlayedNotesComponent
 *
 * Renders currently held or sustained notes as horizontal capsule tokens (synth-style).
 */
class PlayedNotesComponent : public juce::Component
{
public:
    PlayedNotesComponent() {}
    ~PlayedNotesComponent() override = default;

    void paint (juce::Graphics& g) override
    {
        // Dark charcoal styling
        g.fillAll(juce::Colour(0xff121317));

        // Border outline
        g.setColour(juce::Colour(0xff252833));
        g.drawRect(getLocalBounds(), 1);

        // Header label
        g.setColour(juce::Colour(0xffabafbc));
        g.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)).boldened());
        g.drawText("ACTIVE NOTES MONITOR", 12, 0, 150, getHeight(), juce::Justification::centredLeft);

        // Draw note capsules
        if (notes_.empty())
        {
            g.setColour(juce::Colour(0xff454a5c));
            g.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
            g.drawText("No keys pressed", 180, 0, getWidth() - 200, getHeight(), juce::Justification::centredLeft);
            return;
        }

        // Draw a horizontal row of capsules starting from layout offset
        int startX = 180;
        int capsuleHeight = 28;
        int capsuleWidth = 62;
        int margin = 6;
        int currentX = startX;
        
        int centerY = (getHeight() - capsuleHeight) / 2;

        for (int note : notes_)
        {
            if (currentX + capsuleWidth > getWidth() - 12)
                break; // Don't overflow the UI bounds

            auto capsuleBounds = juce::Rectangle<float>(static_cast<float>(currentX), static_cast<float>(centerY), 
                                                        static_cast<float>(capsuleWidth), static_cast<float>(capsuleHeight));

            // Background filling: Sleek semi-translucent dark gradient with neon cyan highlights
            g.setColour(juce::Colour(0xff181b24));
            g.fillRoundedRectangle(capsuleBounds, 5.0f);

            // Glowing cyan outline
            g.setColour(juce::Colour(0xff00d4ff).withAlpha(0.6f));
            g.drawRoundedRectangle(capsuleBounds, 5.0f, 1.2f);

            // Draw Note string (e.g. "C#4")
            int pc = note % 12;
            int octave = (note / 12) - 1;
            juce::String nameStr = juce::String(ChordDetection::NoteUtils::getNoteName(pc, true)) + juce::String(octave);

            g.setColour(juce::Colours::white);
            g.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)).boldened());
            g.drawText(nameStr, capsuleBounds.withHeight(16.0f), juce::Justification::centred, false);

            // Draw Midi number representation underneath
            g.setColour(juce::Colour(0xff8c90a1));
            g.setFont(juce::Font(juce::FontOptions().withHeight(8.0f)));
            juce::String midiValStr = "#" + juce::String(note);
            g.drawText(midiValStr, capsuleBounds.removeFromBottom(10.0f), juce::Justification::centred, false);

            currentX += capsuleWidth + margin;
        }
    }

    void resized() override {}

    void setNotes (const std::vector<int>& notes)
    {
        if (notes_ != notes)
        {
            notes_ = notes;
            repaint();
        }
    }

private:
    std::vector<int> notes_;
};
