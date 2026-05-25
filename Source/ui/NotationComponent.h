#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../chord_detection/detector/NoteUtils.h"
#include <vector>
#include <algorithm>
#include <cmath>

class NotationComponent : public juce::Component
{
public:
    NotationComponent()
    {
        addAndMakeVisible(labelsToggle_);
        labelsToggle_.setButtonText("LABELS: ON");
        styleToggleButton();
        labelsToggle_.onClick = [this]
        {
            showLabels_ = !showLabels_;
            labelsToggle_.setButtonText(showLabels_ ? "LABELS: ON" : "LABELS: OFF");
            styleToggleButton();
            repaint();
        };
    }

    ~NotationComponent() override = default;

    void setNotes(const std::vector<int>& notes)
    {
        if (notes_ != notes) { notes_ = notes; repaint(); }
    }

    void setChordName(const juce::String& name)
    {
        if (chordName_ != name) { chordName_ = name; repaint(); }
    }

    void resized() override
    {
        labelsToggle_.setBounds(getWidth() - 100, 8, 88, 22);
    }

    void paint(juce::Graphics& g) override
    {
        const float W = static_cast<float>(getWidth());
        const float H = static_cast<float>(getHeight());

        const juce::Colour bgBase  (0xff141722);
        const juce::Colour panelBg (0xff1e2230);
        const juce::Colour staffCol(0xff4a5568);
        const juce::Colour ink     (0xffffffff);
        const juce::Colour cyanAcc (0xff00ccdd);

        // ── 1. Chrome background + header ────────────────────────────────────
        g.fillAll(bgBase);
        g.setColour(staffCol);
        g.setFont(juce::Font(juce::FontOptions().withHeight(10.5f)).boldened());
        g.drawText("NOTATION", 14, 10, 100, 14, juce::Justification::left, true);

        // ── 2. Panel ──────────────────────────────────────────────────────────
        const float panelX = 10.0f;
        const float panelY = 30.0f;
        const float panelW = W - 20.0f;
        const float panelH = H - 40.0f;

        g.setColour(panelBg);
        g.fillRoundedRectangle(panelX, panelY, panelW, panelH, 5.0f);
        g.setColour(juce::Colour(0xff2a3040));
        g.drawRoundedRectangle(panelX, panelY, panelW, panelH, 5.0f, 1.0f);

        // ── 3. Grand staff geometry (always computed) ─────────────────────────
        // topPad: space inside panel above treble top line (chord name + breathing)
        const float topPad    = 22.0f;
        const float botPad    = 14.0f;
        const float availH    = panelH - topPad - botPad;
        const float S         = juce::jlimit(8.0f, 14.0f, availH / 12.0f);
        const float stepH     = S * 0.5f;

        // trebleBotY = E4 (bottom treble line)   bassTopY = A3 (top bass line)
        // Traditional grand staff gap = 2.5S  (Middle C sits ~1S below trebleBotY)
        const float trebleBotY = panelY + topPad + 4.0f * S;
        const float trebleTopY = trebleBotY - 4.0f * S;
        const float bassTopY   = trebleBotY + 2.5f * S;
        const float bassBotY   = bassTopY   + 4.0f * S;

        // Horizontal layout — staff spans full panel width minus margins
        const float clefW      = S * 2.8f;
        const float hPad       = S * 0.9f;
        const float staffLeft  = panelX + hPad;
        const float staffRight = panelX + panelW - hPad - (showLabels_ ? S * 4.5f : S * 0.5f);
        const float noteBaseX  = staffLeft + clefW + S * 0.6f;
        const float noteW      = S * 1.3f;
        const float lineThk    = juce::jmax(0.8f, S * 0.10f);

        // ── 4. Staff lines (always drawn) ─────────────────────────────────────
        g.setColour(staffCol);
        for (int i = 0; i <= 4; ++i)
        {
            g.drawLine(staffLeft, trebleBotY - i * S, staffRight, trebleBotY - i * S, lineThk);
            g.drawLine(staffLeft, bassTopY   + i * S, staffRight, bassTopY   + i * S, lineThk);
        }
        // Opening barline spans both staves
        g.drawLine(staffLeft,              trebleTopY, staffLeft,              bassBotY, lineThk * 1.3f);
        // Closing double barline
        g.drawLine(staffRight,             trebleTopY, staffRight,             bassBotY, lineThk * 0.9f);
        g.drawLine(staffRight + S * 0.28f, trebleTopY, staffRight + S * 0.28f, bassBotY, lineThk * 2.8f);

        // Piano brace
        g.setColour(ink);
        drawVectorBrace(g, staffLeft - S * 0.25f, trebleTopY, bassBotY, S);

        // ── 5. Clef glyphs (always drawn) ─────────────────────────────────────
        g.setColour(staffCol.brighter(0.5f));
        // Treble clef — render rect anchored so G4 line sits at ~35% of glyph height
        g.setFont(juce::Font(juce::FontOptions().withHeight(S * 4.6f)));
        g.drawText(juce::String::charToString(0x1D11E),
                   juce::roundToInt(staffLeft + S * 0.1f),
                   juce::roundToInt(trebleBotY - S * 3.0f),
                   juce::roundToInt(S * 2.8f),
                   juce::roundToInt(S * 6.0f),
                   juce::Justification::centredLeft, false);
        // Bass clef — anchored to F3 line (bassTopY + S)
        g.setFont(juce::Font(juce::FontOptions().withHeight(S * 3.0f)));
        g.drawText(juce::String::charToString(0x1D122),
                   juce::roundToInt(staffLeft + S * 0.15f),
                   juce::roundToInt(bassTopY  - S * 0.3f),
                   juce::roundToInt(S * 2.8f),
                   juce::roundToInt(S * 2.8f),
                   juce::Justification::centredLeft, false);

        // ── 6. Empty state hint (staff is already visible) ────────────────────
        if (notes_.empty())
        {
            const float midY = (trebleBotY + bassTopY) * 0.5f;
            g.setColour(staffCol.withAlpha(0.45f));
            g.setFont(juce::Font(juce::FontOptions().withHeight(10.5f)));
            g.drawText("Play MIDI notes",
                       juce::roundToInt(noteBaseX),
                       juce::roundToInt(midY - 9.0f),
                       juce::roundToInt(staffRight - noteBaseX),
                       18, juce::Justification::centred, true);
            return;
        }

        // ── 7. Build note render list ─────────────────────────────────────────
        struct RenderNote {
            int  midiNote, renderStep;
            bool isSharp, isFlat, hasAcc, needs8va, needs8vb;
            float x, y, accX;
        };

        std::vector<RenderNote> rNotes;
        for (int note : notes_)
        {
            RenderNote rn;
            rn.midiNote   = note;
            rn.renderStep = getWhiteKeyStep(note);

            const int  pc         = note % 12;
            const bool preferFlat = (pc == 10 || pc == 3 || pc == 8);
            const bool isBlack    = (pc == 1  || pc == 3 || pc == 6 || pc == 8 || pc == 10);
            rn.isSharp  = isBlack && !preferFlat;
            rn.isFlat   = isBlack &&  preferFlat;
            rn.hasAcc   = rn.isSharp || rn.isFlat;
            rn.needs8va = rn.needs8vb = false;

            while (rn.renderStep >=  13) { rn.renderStep -= 7; rn.needs8va = true; }
            while (rn.renderStep <= -13) { rn.renderStep += 7; rn.needs8vb = true; }

            rn.y = (rn.renderStep >= 0)
                 ? trebleBotY - static_cast<float>(rn.renderStep - 2) * stepH
                 : bassTopY   - static_cast<float>(rn.renderStep + 2) * stepH;

            rNotes.push_back(rn);
        }

        std::sort(rNotes.begin(), rNotes.end(),
                  [](const auto& a, const auto& b) { return a.midiNote < b.midiNote; });

        // ── 8. Horizontal stagger for adjacent intervals ───────────────────────
        for (size_t i = 0; i < rNotes.size(); ++i)
        {
            const float xOff = (i > 0 && std::abs(rNotes[i].y - rNotes[i-1].y) < S * 0.9f)
                               ? noteW + S * 0.15f : 0.0f;
            rNotes[i].x    = noteBaseX + xOff;
            rNotes[i].accX = noteBaseX + xOff - S * 1.3f;
            if (i > 0 && rNotes[i].hasAcc && rNotes[i-1].hasAcc
                && std::abs(rNotes[i].y - rNotes[i-1].y) < S * 2.2f)
                rNotes[i].accX -= S * 0.9f;
        }

        // ── 9. Chord name (above treble top, inside panel) ────────────────────
        if (chordName_.isNotEmpty())
        {
            const float nameSz = juce::jmax(11.0f, S * 1.05f);
            g.setColour(ink);
            g.setFont(juce::Font(juce::FontOptions().withHeight(nameSz)).boldened());
            g.drawText(chordName_,
                       juce::roundToInt(noteBaseX - S * 0.5f),
                       juce::roundToInt(panelY + 5.0f),
                       juce::roundToInt(panelW * 0.7f),
                       juce::roundToInt(nameSz + 4.0f),
                       juce::Justification::centredLeft, false);
        }

        // ── 10. 8va / 8vb bracket bounds ──────────────────────────────────────
        float min8vaX = 1e6f, max8vaX = -1e6f, min8vaY = 1e6f;
        float min8vbX = 1e6f, max8vbX = -1e6f, max8vbY = -1e6f;
        for (const auto& rn : rNotes)
        {
            if (rn.needs8va) { min8vaX = juce::jmin(min8vaX, rn.x); max8vaX = juce::jmax(max8vaX, rn.x); min8vaY = juce::jmin(min8vaY, rn.y); }
            if (rn.needs8vb) { min8vbX = juce::jmin(min8vbX, rn.x); max8vbX = juce::jmax(max8vbX, rn.x); max8vbY = juce::jmax(max8vbY, rn.y); }
        }

        // ── 11. Noteheads, accidentals, ledger lines ──────────────────────────
        for (const auto& rn : rNotes)
        {
            const float lhw = noteW * 0.5f + S * 0.32f;
            const float lx0 = rn.x + noteW * 0.5f - lhw;
            const float lx1 = rn.x + noteW * 0.5f + lhw;

            g.setColour(staffCol);

            // Middle C ledger line (drawn for notes at steps −1, 0, +1)
            if (rn.renderStep >= -1 && rn.renderStep <= 1)
                g.drawLine(lx0, trebleBotY + S, lx1, trebleBotY + S, lineThk * 1.4f);

            // Extra ledger lines above treble top (steps +12, +14, …)
            if (rn.renderStep > 10)
                for (int s = 12; s <= rn.renderStep + (rn.renderStep % 2 != 0 ? 1 : 0); s += 2)
                    g.drawLine(lx0, trebleBotY - static_cast<float>(s - 2) * stepH,
                               lx1, trebleBotY - static_cast<float>(s - 2) * stepH, lineThk * 1.4f);

            // Extra ledger lines below bass bottom (steps −12, −14, …)
            if (rn.renderStep < -10)
                for (int s = -12; s >= rn.renderStep - (std::abs(rn.renderStep) % 2 != 0 ? 1 : 0); s -= 2)
                    g.drawLine(lx0, bassTopY + static_cast<float>(-s - 2) * stepH,
                               lx1, bassTopY + static_cast<float>(-s - 2) * stepH, lineThk * 1.4f);

            g.setColour(ink);
            if (rn.isSharp) drawVectorSharp(g, rn.accX, rn.y, S);
            if (rn.isFlat)  drawVectorFlat (g, rn.accX, rn.y, S);
            drawVectorWholeNote(g, rn.x, rn.y, S, noteW);
        }

        // ── 12. 8va / 8vb brackets ────────────────────────────────────────────
        auto drawOctaveLine = [&](bool is8va, float yPos, float startX, float endX)
        {
            g.setColour(cyanAcc);
            g.setFont(juce::Font(juce::FontOptions().withHeight(S * 1.4f)).italicised());
            g.drawText(is8va ? "8va" : "8vb",
                       juce::roundToInt(startX - S * 0.4f),
                       juce::roundToInt(yPos - S * 0.85f),
                       juce::roundToInt(S * 2.4f), juce::roundToInt(S),
                       juce::Justification::centredLeft, false);
            const float lineY = yPos - S * 0.2f;
            float cx = startX + S * 1.6f;
            while (cx < endX + noteW) {
                g.drawLine(cx, lineY, juce::jmin(cx + S * 0.4f, endX + noteW), lineY, lineThk);
                cx += S * 0.72f;
            }
            g.drawLine(endX + noteW, lineY, endX + noteW,
                       lineY + (is8va ? S * 0.7f : -S * 0.7f), lineThk);
        };

        if (min8vaX <= max8vaX) drawOctaveLine(true,  min8vaY - S * 2.0f, min8vaX, max8vaX);
        if (min8vbX <= max8vbX) drawOctaveLine(false, max8vbY + S * 2.0f, min8vbX, max8vbX);

        // ── 13. Pitch labels ───────────────────────────────────────────────────
        if (showLabels_)
        {
            const float lsz        = juce::jmax(9.0f, S * 1.05f);
            const float labelBaseX = staffRight + S * 0.6f;
            g.setFont(juce::Font(juce::FontOptions().withHeight(lsz)).boldened());
            float prevY = -9999.0f;
            float curLX = labelBaseX;
            for (const auto& rn : rNotes)
            {
                curLX = (std::abs(rn.y - prevY) < lsz * 1.3f) ? curLX + S * 2.0f : labelBaseX;
                prevY = rn.y;
                const int pc  = rn.midiNote % 12;
                const int oct = (rn.midiNote / 12) - 1;
                juce::String nm = juce::String(ChordDetection::NoteUtils::getNoteName(pc, true))
                                  + juce::String(oct);
                g.setColour(cyanAcc);
                g.drawText(nm,
                           juce::roundToInt(curLX),
                           juce::roundToInt(rn.y - lsz * 0.5f),
                           juce::roundToInt(S * 3.5f),
                           juce::roundToInt(lsz + 2.0f),
                           juce::Justification::centredLeft, false);
            }
        }
    }

private:
    std::vector<int>  notes_;
    juce::String      chordName_;
    juce::TextButton  labelsToggle_{ "LABELS" };
    bool              showLabels_ = true;

    void styleToggleButton()
    {
        labelsToggle_.setColour(juce::TextButton::buttonColourId, showLabels_ ? juce::Colour(0xff00ccdd).withAlpha(0.2f) : juce::Colour(0xff2a3040));
        labelsToggle_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff667788));
        labelsToggle_.setColour(juce::TextButton::textColourOnId,  juce::Colour(0xff00ccdd));
    }

    // =================================================================================
    // 100% VECTOR GRAPHICS - GUARANTEES PERFECT "HENLE" LOOK WITHOUT FONTS
    // =================================================================================
    void drawVectorWholeNote(juce::Graphics& g, float x, float y, float S, float w) const
    {
        juce::Path p, inner;
        p.addEllipse(0, 0, w, S);
        inner.addEllipse(w * 0.35f, S * 0.15f, w * 0.3f, S * 0.7f);
        p.setUsingNonZeroWinding(false);
        p.addPath(inner);
        // Rotate slightly for the classical chisel-pen look
        p.applyTransform(juce::AffineTransform::rotation(-0.2f, w * 0.5f, S * 0.5f).translated(x, y - S * 0.5f));
        g.fillPath(p);
    }

    void drawVectorSharp(juce::Graphics& g, float x, float y, float S) const
    {
        juce::Path p;
        float w = S * 0.9f;
        float h = S * 2.8f;
        float thk = S * 0.12f;
        // Verticals
        p.addRectangle(0, S * 0.4f, thk, h * 0.8f);
        p.addRectangle(w * 0.6f, 0, thk, h * 0.8f);
        // Slants
        juce::Path slant;
        slant.startNewSubPath (-S * 0.2f, S * 1.1f);
        slant.lineTo           (w  * 0.8f, S * 0.8f);
        slant.lineTo           (w  * 0.8f, S * 1.1f);
        slant.lineTo           (-S * 0.2f, S * 1.4f);
        slant.closeSubPath();
        p.addPath(slant);
        slant.applyTransform(juce::AffineTransform::translation(0, S * 0.8f));
        p.addPath(slant);
        p.applyTransform(juce::AffineTransform::translation(x, y - S * 1.5f));
        g.fillPath(p);
    }

    void drawVectorFlat(juce::Graphics& g, float x, float y, float S) const
    {
        juce::Path p;
        float w = S * 0.8f;
        p.addRectangle(0, 0, S * 0.12f, S * 2.5f);
        // Teardrop bezier
        p.startNewSubPath(0, S * 1.5f);
        p.cubicTo(w * 1.2f, S * 1.0f, w * 1.3f, S * 2.2f, 0, S * 2.4f);
        p.cubicTo(w * 0.6f, S * 2.1f, w * 0.4f, S * 1.8f, 0, S * 1.8f);
        p.applyTransform(juce::AffineTransform::translation(x, y - S * 1.8f));
        g.fillPath(p);
    }

    void drawVectorBrace(juce::Graphics& g, float x, float topY, float botY, float S) const
    {
        juce::Path p;
        float midY = (topY + botY) * 0.5f;
        // Beautiful elegant piano brace bezier curve
        p.startNewSubPath(x, topY);
        p.cubicTo(x - S*0.8f, topY, x - S*0.8f, midY - S*0.5f, x - S*1.8f, midY);
        p.cubicTo(x - S*0.8f, midY + S*0.5f, x - S*0.8f, botY, x, botY);
        p.cubicTo(x - S*0.5f, botY, x - S*0.5f, midY + S*0.5f, x - S*1.2f, midY);
        p.cubicTo(x - S*0.5f, midY - S*0.5f, x - S*0.5f, topY, x, topY);
        g.fillPath(p);
    }

    static int getWhiteKeyStep(int midiNote) noexcept
    {
        const int pc = midiNote % 12;
        const int octave = (midiNote / 12) - 5; 
        int base = 0;
        switch (pc) {
            case 0: case 1:  base = 0; break;
            case 2: case 3:  base = 1; break;
            case 4:          base = 2; break;
            case 5: case 6:  base = 3; break;
            case 7: case 8:  base = 4; break;
            case 9: case 10: base = 5; break;
            case 11:         base = 6; break;
        }
        return base + octave * 7;
    }
};