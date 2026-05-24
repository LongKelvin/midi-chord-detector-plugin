#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../chord_detection/detector/NoteUtils.h"
#include <vector>
#include <algorithm>

// Bravura.otf must be added to CMakeLists.txt via juce_add_binary_data:
//   juce_add_binary_data(BinaryData SOURCES fonts/Bravura.otf)
// Download: https://github.com/steinbergmedia/bravura/releases
#include <BinaryData.h>

/**
 * NotationComponent — Professional music engraving display using SMuFL / Bravura.
 *
 * ── Key layout corrections (this revision) ──────────────────────────────────
 *
 *  1. stepH formula: usableH / 28  (was /46 — too small, clamped to 6.5 px).
 *     New range: 7–9.5 px, giving legible Bravura noteheads at typical panel sizes.
 *
 *  2. centreY (Middle C pixel position) is COMPUTED, not a naive 50% of H.
 *     The treble clef glyph extends 16 steps above centreY (anchor at G4=+4,
 *     curl top at +6S = +12 steps above G4 → 16 total steps above centreY).
 *     centreY is pushed down so the clef top sits just below the header label,
 *     while the bass staff bottom stays inside the component.
 *
 *  3. Staff lines are bounded to [staffStartX, staffEndX] — NOT full component
 *     width. Full-width lines made the notation look like a grid widget.
 *
 *  4. Clef, notehead, and accidental rects are all derived from S (= 2×stepH),
 *     so everything scales together at any panel height.
 *
 * ── Coordinate system ───────────────────────────────────────────────────────
 *   stepH  = half of one staff space (S/2). One diatonic step = one stepH.
 *   centreY = pixel Y of Middle C (step 0, C4).
 *
 *   step +10 = F5  (top treble line)
 *   step  +4 = G4  ← G line (treble clef anchor)
 *   step  +2 = E4  (bottom treble line)
 *   step   0 = C4  Middle C  (ledger line in gap)
 *   step  -2 = A3  (top bass line)
 *   step  -4 = F3  ← F line (bass clef anchor)
 *   step -10 = G2  (bottom bass line)
 *
 * ── Bravura font sizing ──────────────────────────────────────────────────────
 *   1 staff space = 0.25 em in Bravura → fontSize = S × 4 = stepH × 8.
 *
 * ── Public API (unchanged) ──────────────────────────────────────────────────
 *   void setNotes(const std::vector<int>& notes)
 *   void setChordName(const juce::String& name)
 */
class NotationComponent : public juce::Component
{
public:
    NotationComponent()
    {
        addAndMakeVisible (labelsToggle_);
        labelsToggle_.setButtonText ("LABELS: ON");
        labelsToggle_.setColour (juce::TextButton::buttonColourId,  juce::Colour (0xff1c202a));
        labelsToggle_.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff8c90a1));
        labelsToggle_.setColour (juce::TextButton::textColourOnId,  juce::Colour (0xff00d4ff));
        labelsToggle_.setColour (juce::ComboBox::outlineColourId,   juce::Colour (0xff252833));
        labelsToggle_.onClick = [this]
        {
            showLabels_ = !showLabels_;
            labelsToggle_.setButtonText (showLabels_ ? "LABELS: ON" : "LABELS: OFF");
            styleToggleButton();
            repaint();
        };
        styleToggleButton();
    }

    ~NotationComponent() override = default;

    // =========================================================================
    //  Public API
    // =========================================================================

    void setNotes (const std::vector<int>& notes)
    {
        if (notes_ != notes) { notes_ = notes; repaint(); }
    }

    void setChordName (const juce::String& name)
    {
        if (chordName_ != name) { chordName_ = name; repaint(); }
    }

    // =========================================================================
    //  Component overrides
    // =========================================================================

    void resized() override
    {
        labelsToggle_.setBounds (getWidth() - 105, 8, 92, 22);
    }

    void paint (juce::Graphics& g) override
    {
        const float W = static_cast<float> (getWidth());
        const float H = static_cast<float> (getHeight());

        // ------------------------------------------------------------------
        // 1.  Background
        // ------------------------------------------------------------------
        g.fillAll (juce::Colour (0xff0e1018));
        g.setColour (juce::Colour (0xff1e2336));
        g.drawRect (getLocalBounds(), 1);

        // Header label (top-left, 8px from top)
        g.setColour (juce::Colour (0xff5a6480));
        g.setFont (juce::Font (juce::FontOptions().withHeight (10.0f)).boldened());
        g.drawText ("ENGRAVED MUSIC NOTATION", 14, 8, 240, 16,
                    juce::Justification::left, true);

        // ------------------------------------------------------------------
        // 2.  stepH — half a staff space, in pixels.
        //
        //   Divide usable height by 28 to leave room for the treble clef tail
        //   (extends ~6 staff spaces above G4) and bottom breathing room.
        //   Clamp 7–9.5: Bravura glyphs look best in this range.
        // ------------------------------------------------------------------
        const float topPad    = 8.0f;   // just the header row
        const float botPad    = 8.0f;
        const float usableH   = H - topPad - botPad;
        const float stepH     = juce::jlimit (7.0f, 9.5f, usableH / 28.0f);
        const float S         = stepH * 2.0f;   // one full staff space

        // ------------------------------------------------------------------
        // 3.  centreY — pixel Y of Middle C (step 0).
        //
        //   The treble clef glyph's topmost point sits at:
        //     centreY − 4·stepH  (G4 anchor)  −  6·S  (glyph top offset)
        //   = centreY − 4·stepH − 12·stepH
        //   = centreY − 16·stepH
        //
        //   We want that to be just below the header (topPad + 20 px clearance).
        //   So:  centreY_min = topPad + 20 + 16·stepH
        //
        //   Simultaneously, the bass staff bottom (step −10) must stay inside:
        //     centreY_max = H − botPad − 6 − 10·stepH
        //
        //   centreY = clamp(visual_centre, min, max).
        // ------------------------------------------------------------------
        const float centreY_min    = topPad + 20.0f + 16.0f * stepH;
        const float centreY_max    = H - botPad - 6.0f - 10.0f * stepH;
        const float centreY_visual = H * 0.50f;
        const float centreY        = juce::jlimit (centreY_min, centreY_max, centreY_visual);

        // ------------------------------------------------------------------
        // 4.  Horizontal bounds: staff lines are bounded, NOT full-width.
        // ------------------------------------------------------------------
        const float clefZoneW  = S * 3.5f;    // space for clef glyphs on the left
        const float staffStartX = clefZoneW;
        const float staffEndX   = W - 18.0f;  // right margin

        // ------------------------------------------------------------------
        // 5.  Staff lines (5 treble + 5 bass, 1 px)
        // ------------------------------------------------------------------
        const juce::Colour staffColour (0xff4c5880);
        g.setColour (staffColour);

        for (int i = 1; i <= 5; ++i)
        {
            float ty = centreY - static_cast<float> (i * 2) * stepH;
            float by = centreY + static_cast<float> (i * 2) * stepH;
            g.drawLine (staffStartX, ty, staffEndX, ty, 1.0f);
            g.drawLine (staffStartX, by, staffEndX, by, 1.0f);
        }

        // Opening barline (left edge of staff system)
        g.drawLine (staffStartX,
                    centreY - 10.0f * stepH,
                    staffStartX,
                    centreY -  2.0f * stepH, 1.2f);
        g.drawLine (staffStartX,
                    centreY +  2.0f * stepH,
                    staffStartX,
                    centreY + 10.0f * stepH, 1.2f);

        // Closing barline
        g.drawLine (staffEndX,
                    centreY - 10.0f * stepH,
                    staffEndX,
                    centreY -  2.0f * stepH, 1.2f);
        g.drawLine (staffEndX,
                    centreY +  2.0f * stepH,
                    staffEndX,
                    centreY + 10.0f * stepH, 1.2f);

        // ------------------------------------------------------------------
        // 6.  Bravura font  (fontSize = S × 4, from SMuFL spec)
        // ------------------------------------------------------------------
        const float   fontSize  = S * 4.0f;
        juce::Font    musicFont = getMusicFont (fontSize);
        const juce::Colour glyphColour (0xffdedad0);

        // ------------------------------------------------------------------
        // 7.  Clef glyphs
        //
        //   Treble (U+E050):
        //     Bravura origin = G line (step +4).
        //     Glyph bbox: top = origin − 6S, bottom = origin + 4S → height = 10S.
        //     Width (practical): ~2.2S.
        //
        //   Bass (U+E062):
        //     Bravura origin = F line (step −4).
        //     Glyph bbox: top = origin − 1.16S, height ≈ 1.64S.
        //     Width (practical): ~2.2S.
        // ------------------------------------------------------------------
        g.setFont (musicFont);
        g.setColour (glyphColour);

        // Treble clef
        {
            float anchorY = centreY - 4.0f * stepH;  // G4
            float glyphX  = staffStartX - clefZoneW + 2.0f;
            float glyphY  = anchorY - 6.0f * S;
            float glyphW  = S * 2.4f;
            float glyphH  = S * 10.0f;
            g.drawText (juce::String::charToString (0xE050),
                        juce::roundToInt (glyphX), juce::roundToInt (glyphY),
                        juce::roundToInt (glyphW), juce::roundToInt (glyphH),
                        juce::Justification::centredLeft, false);
        }

        // Bass clef
        {
            float anchorY = centreY + 4.0f * stepH;  // F3
            float glyphX  = staffStartX - clefZoneW + 2.0f;
            float glyphY  = anchorY - 1.16f * S;
            float glyphW  = S * 2.4f;
            float glyphH  = S * 1.64f;
            g.drawText (juce::String::charToString (0xE062),
                        juce::roundToInt (glyphX), juce::roundToInt (glyphY),
                        juce::roundToInt (glyphW), juce::roundToInt (glyphH),
                        juce::Justification::centredLeft, false);
        }

        // Staff section micro-labels
        g.setColour (juce::Colour (0xff3e4a66));
        g.setFont (juce::Font (juce::FontOptions().withHeight (8.0f)).boldened());
        g.drawText ("TREBLE", juce::roundToInt (staffStartX - clefZoneW + 2),
                    juce::roundToInt (centreY - 13.5f * stepH), 40, 12,
                    juce::Justification::left, true);
        g.drawText ("BASS", juce::roundToInt (staffStartX - clefZoneW + 2),
                    juce::roundToInt (centreY + 11.5f * stepH), 40, 12,
                    juce::Justification::left, true);

        // ------------------------------------------------------------------
        // 8.  Empty state
        // ------------------------------------------------------------------
        if (notes_.empty())
        {
            g.setColour (juce::Colour (0xff3e4352));
            g.setFont (juce::Font (juce::FontOptions().withHeight (11.5f)));
            g.drawText ("Play MIDI notes to preview real-time chord engraving",
                        juce::Rectangle<int> (juce::roundToInt (staffStartX), 0,
                                              juce::roundToInt (staffEndX - staffStartX),
                                              static_cast<int> (H)),
                        juce::Justification::centred, true);
            return;
        }

        // ------------------------------------------------------------------
        // 9.  Build sorted render list
        // ------------------------------------------------------------------
        struct RenderNote
        {
            int   midiNote;
            int   step;
            bool  isSharp;
            bool  isFlat;
            float y;
            float x = 0.0f;
        };

        std::vector<RenderNote> rNotes;
        rNotes.reserve (notes_.size());
        for (int note : notes_)
        {
            int  pc         = note % 12;
            bool preferFlat = (pc == 10 || pc == 3 || pc == 8);
            bool isBlack    = (pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10);
            RenderNote rn;
            rn.midiNote = note;
            rn.step     = getWhiteKeyStep (note);
            rn.isSharp  = isBlack && !preferFlat;
            rn.isFlat   = isBlack &&  preferFlat;
            rn.y        = centreY - static_cast<float> (rn.step) * stepH;
            rNotes.push_back (rn);
        }
        std::sort (rNotes.begin(), rNotes.end(),
                   [] (const RenderNote& a, const RenderNote& b)
                   { return a.midiNote < b.midiNote; });

        // ------------------------------------------------------------------
        // 10.  Notehead dimensions (Bravura: width=1.68S, height=1.0S)
        // ------------------------------------------------------------------
        const float noteW = S * 1.68f;
        const float noteH = S * 1.00f;

        // ------------------------------------------------------------------
        // 11.  Horizontal layout
        //   Accidental column (if any): 0.7S to the left of the notehead.
        //   noteXBase is shifted right by half that when accidentals exist,
        //   so the whole cluster (acc + notehead + label) looks centred.
        // ------------------------------------------------------------------
        const float accColW = S * 0.75f;
        const float contentCX = staffStartX + (staffEndX - staffStartX) * 0.50f;
        bool anyAcc = std::any_of (rNotes.begin(), rNotes.end(),
                                   [] (const RenderNote& r)
                                   { return r.isSharp || r.isFlat; });
        const float noteXBase = contentCX + (anyAcc ? accColW * 0.50f : 0.0f);

        for (auto& rn : rNotes)
            rn.x = noteXBase;

        // ------------------------------------------------------------------
        // 12.  Second-interval stagger (standard engraving rule):
        //   When two notes occupy adjacent diatonic steps (diff == 1),
        //   displace the upper note right by ~one notehead width.
        // ------------------------------------------------------------------
        for (int i = 1; i < static_cast<int> (rNotes.size()); ++i)
        {
            if (rNotes[i].step - rNotes[i - 1].step == 1)
                rNotes[i].x = noteXBase + noteW * 0.92f;
        }

        // ------------------------------------------------------------------
        // 13.  Chord name above treble staff
        // ------------------------------------------------------------------
        if (chordName_.isNotEmpty())
        {
            float nameSize = juce::jmax (13.0f, S * 1.15f);
            float nameY    = centreY - 10.0f * stepH - nameSize - 4.0f;
            g.setColour (glyphColour);
            g.setFont (juce::Font (juce::FontOptions().withHeight (nameSize).withStyle ("Bold")));
            g.drawText (chordName_,
                        juce::roundToInt (noteXBase - 70.0f),
                        juce::roundToInt (nameY),
                        140, juce::roundToInt (nameSize + 4.0f),
                        juce::Justification::centred, false);
        }

        // ------------------------------------------------------------------
        // 14.  Render: ledger lines → accidental → notehead → label
        // ------------------------------------------------------------------
        const float ledgerOverhang = noteW * 0.26f;

        for (const auto& rn : rNotes)
        {
            const float lx0 = rn.x - noteW * 0.5f - ledgerOverhang;
            const float lx1 = rn.x + noteW * 0.5f + ledgerOverhang;

            // ── Ledger lines ──────────────────────────────────────────────
            g.setColour (staffColour);

            // Middle C: draw when note is on step 0, or one step either side
            if (rn.step >= -1 && rn.step <= 1)
                g.drawLine (lx0, centreY, lx1, centreY, 1.0f);

            // Above treble (≥ step 12): every even step from 12 up
            if (rn.step >= 12)
                for (int s = 12; s <= rn.step; s += 2)
                    g.drawLine (lx0, centreY - s * stepH,
                                lx1, centreY - s * stepH, 1.0f);

            // Below bass (≤ step −12): every even step from −12 down
            if (rn.step <= -12)
                for (int s = -12; s >= rn.step; s -= 2)
                    g.drawLine (lx0, centreY - s * stepH,
                                lx1, centreY - s * stepH, 1.0f);

            // ── Accidental ────────────────────────────────────────────────
            if (rn.isSharp || rn.isFlat)
            {
                // Sharp bbox: height ≈ 3S, Flat bbox: height ≈ 3S
                const float accH  = S * 3.2f;
                const float accW  = S * 1.0f;
                const float accX  = rn.x - noteW * 0.5f - accColW + S * 0.05f;
                const float accY  = rn.y - accH * 0.50f;
                juce::juce_wchar ch = rn.isSharp
                    ? static_cast<juce::juce_wchar> (0xE262)
                    : static_cast<juce::juce_wchar> (0xE260);

                g.setFont (musicFont);
                g.setColour (glyphColour);
                g.drawText (juce::String::charToString (ch),
                            juce::roundToInt (accX), juce::roundToInt (accY),
                            juce::roundToInt (accW), juce::roundToInt (accH),
                            juce::Justification::centredLeft, false);
            }

            // ── Whole notehead (U+E0A2) ───────────────────────────────────
            //   Bravura bbox: x=[0..1.68S], y=[−0.5S..+0.5S] from origin.
            //   Centre the rect on (rn.x, rn.y).
            {
                g.setFont (musicFont);
                g.setColour (glyphColour);
                g.drawText (juce::String::charToString (0xE0A2),
                            juce::roundToInt (rn.x - noteW * 0.5f),
                            juce::roundToInt (rn.y - noteH * 0.5f),
                            juce::roundToInt (noteW),
                            juce::roundToInt (noteH),
                            juce::Justification::centredLeft, false);
            }

            // ── Pitch label ───────────────────────────────────────────────
            if (showLabels_)
            {
                juce::Font lf (juce::FontOptions().withHeight (juce::jmax (8.5f, stepH * 1.5f)));
                g.setFont (lf);
                g.setColour (juce::Colour (0xff7a8298));
                int pc     = rn.midiNote % 12;
                int octave = (rn.midiNote / 12) - 1;
                juce::String nm = juce::String (ChordDetection::NoteUtils::getNoteName (pc, true))
                                  + juce::String (octave);
                g.drawText (nm,
                            juce::roundToInt (rn.x + noteW * 0.5f + 5.0f),
                            juce::roundToInt (rn.y - lf.getHeight() * 0.5f),
                            42, juce::roundToInt (lf.getHeight() + 2.0f),
                            juce::Justification::centredLeft, false);
            }
        }
    }

private:
    // =========================================================================
    //  State
    // =========================================================================
    std::vector<int>  notes_;
    juce::String      chordName_;
    juce::TextButton  labelsToggle_ { "LABELS" };
    bool              showLabels_ = true;

    // =========================================================================
    //  Bravura — static singleton, loaded once from BinaryData.
    //
    //  CMakeLists.txt (add once):
    //    juce_add_binary_data(BinaryData SOURCES fonts/Bravura.otf)
    //  Download: https://github.com/steinbergmedia/bravura/releases
    //  Place at: <project_root>/fonts/Bravura.otf
    // =========================================================================
    static juce::Typeface::Ptr getBravuraTypeface()
    {
        static juce::Typeface::Ptr instance =
            juce::Typeface::createSystemTypefaceFor (
                BinaryData::Bravura_otf, BinaryData::Bravura_otfSize);
        return instance;
    }

    static juce::Font getMusicFont (float size)
    {
        return juce::Font (juce::FontOptions (getBravuraTypeface()).withHeight (size));
    }

    void styleToggleButton()
    {
        labelsToggle_.setColour (juce::TextButton::buttonColourId,
                                 showLabels_ ? juce::Colour (0xff142830)
                                             : juce::Colour (0xff181b24));
        labelsToggle_.setColour (juce::ComboBox::outlineColourId,
                                 showLabels_ ? juce::Colour (0xff005a73)
                                             : juce::Colour (0xff252833));
    }

    // =========================================================================
    //  C4 (MIDI 60) = step 0.  Each octave = 7 steps.
    //  Accidentals share the step of their lower diatonic neighbour.
    // =========================================================================
    static int getWhiteKeyStep (int midiNote) noexcept
    {
        const int pc     = midiNote % 12;
        const int octave = (midiNote / 12) - 5;
        int base = 0;
        switch (pc)
        {
            case 0: case 1:  base = 0; break;
            case 2: case 3:  base = 1; break;
            case 4:          base = 2; break;
            case 5: case 6:  base = 3; break;
            case 7: case 8:  base = 4; break;
            case 9: case 10: base = 5; break;
            case 11:         base = 6; break;
            default: break;
        }
        return base + octave * 7;
    }
};

