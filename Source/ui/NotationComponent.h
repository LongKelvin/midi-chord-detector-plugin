#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../chord_detection/detector/NoteUtils.h"
#include "MusicFont.h"
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

        // Once S hits its clamp, a taller panel stops growing the staff
        // itself but still has real height to spend — split that leftover
        // evenly between "room above the treble staff" (higher notes, right
        // hand) and "room below the bass staff" (lower notes, left hand) by
        // nudging the whole staff block down from the top of the panel,
        // instead of leaving it all pinned to the bottom margin.
        const float nominalBlockH = 10.5f * S; // treble(4S) + gap(2.5S) + bass(4S)
        const float verticalSlack = juce::jmax(0.0f, availH - nominalBlockH);
        const float topOffset     = topPad + verticalSlack * 0.5f;

        // trebleBotY = E4 (bottom treble line)   bassTopY = A3 (top bass line)
        // Traditional grand staff gap = 2.5S  (Middle C sits ~1S below trebleBotY)
        const float trebleBotY = panelY + topOffset + 4.0f * S;
        const float trebleTopY = trebleBotY - 4.0f * S;
        const float bassTopY   = trebleBotY + 2.5f * S;
        const float bassBotY   = bassTopY   + 4.0f * S;

        // Horizontal layout — staff spans full panel width minus margins
        const float clefFontSz = S * MusicFont::Metrics::kStaffSpacesToFontSize;
        const float clefW      = S * 0.5f
                                + juce::jmax(MusicFont::Metrics::gClefAdvanceEm, MusicFont::Metrics::fClefAdvanceEm) * clefFontSz;
        const float hPad       = S * 0.9f;
        const float staffLeft  = panelX + hPad;
        const float staffRight = panelX + panelW - hPad - (showLabels_ ? S * 4.5f : S * 0.5f);
        // Extra breathing room between the clef and the first notehead column
        // so wide/accidental-heavy chords don't crowd or overlap the clef.
        const float noteBaseX  = staffLeft + clefW + S * 1.6f;
        const float noteW      = S * 1.3f;
        const float lineThk    = juce::jmax(0.8f, S * 0.10f);

        // ── 3b. Ledger-line budget (drives when 8va/8vb kicks in) ─────────────
        // Two different MIDI pitches must never render at the same staff
        // position. Below the staff, that's automatic (getWhiteKeyStep() is
        // monotonic in pitch), but 8va/8vb folds a note down by a full octave
        // (7 diatonic steps) — so it only stays safe while it's used as a
        // last resort, once real ledger-line space runs out, rather than a
        // fixed step count applied regardless of how tall the panel actually
        // is. Compute how many ledger steps actually fit in the space this
        // panel has above the treble staff and below the bass staff, and
        // only fold once a note goes beyond that.
        const float topReserve   = S * 1.8f;  // keep the chord symbol clear of ledger lines
        const float botReserve   = S * 0.4f;
        const float topRoomPx    = juce::jmax(0.0f, trebleTopY - (panelY + topReserve));
        const float botRoomPx    = juce::jmax(0.0f, (panelY + panelH - botReserve) - bassBotY);
        const int   maxStepAbove = 10 + static_cast<int>(std::floor(topRoomPx / stepH));  // treble top line = step 10
        const int   minStepBelow = -10 - static_cast<int>(std::floor(botRoomPx / stepH)); // bass bottom line = step -10

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
        // Real Bravura glyphs, positioned at their SMuFL-standard baseline:
        // gClef's spiral curls around the G4 line — the 2nd line from the
        // bottom of the treble staff, not the bottom line itself — and
        // fClef's two dots straddle the F3 line — the 2nd line from the top
        // of the bass staff, not the top line itself.
        g.setColour(staffCol.brighter(0.5f));
        drawGlyph(g, MusicFont::Glyph::gClef, staffLeft + S * 0.15f, trebleBotY - S, clefFontSz, GlyphAnchor::baseline);
        drawGlyph(g, MusicFont::Glyph::fClef, staffLeft + S * 0.15f, bassTopY   + S, clefFontSz, GlyphAnchor::baseline);

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

            // Fold only once a note goes past the ledger-line room this panel
            // actually has (see §3b) — not a fixed step count. Genuinely
            // distinct pitches keep genuinely distinct positions until we
            // truly run out of vertical space to draw them in.
            while (rn.renderStep > maxStepAbove) { rn.renderStep -= 7; rn.needs8va = true; }
            while (rn.renderStep < minStepBelow) { rn.renderStep += 7; rn.needs8vb = true; }

            rn.y = (rn.renderStep >= 0)
                 ? trebleBotY - static_cast<float>(rn.renderStep - 2) * stepH
                 : bassTopY   - static_cast<float>(rn.renderStep + 2) * stepH;

            rNotes.push_back(rn);
        }

        std::sort(rNotes.begin(), rNotes.end(),
                  [](const auto& a, const auto& b) { return a.midiNote < b.midiNote; });

        // ── 8. Horizontal stagger for adjacent intervals (chain-aware zig-zag) ─
        // Engraving convention for stacked seconds: each note that collides with
        // its immediate neighbour flips to the opposite side, so a run of three+
        // consecutive seconds (e.g. C-D-E clustered tight) zig-zags left/right
        // instead of every colliding note piling onto the same offset column.
        bool offsetSide = false;
        for (size_t i = 0; i < rNotes.size(); ++i)
        {
            const bool collidesWithPrev = (i > 0) &&
                (std::abs(rNotes[i].y - rNotes[i-1].y) < S * 0.9f);
            offsetSide = collidesWithPrev ? !offsetSide : false;

            const float xOff = offsetSide ? noteW + S * 0.15f : 0.0f;
            rNotes[i].x    = noteBaseX + xOff;
            // Accidentals stack in their own column(s) immediately left of the
            // leftmost notehead column; a note that's a close neighbour of the
            // previous one and also carries an accidental gets pushed one
            // column further left so the two accidentals don't collide.
            rNotes[i].accX = noteBaseX - S * 1.3f;
            if (i > 0 && rNotes[i].hasAcc && rNotes[i-1].hasAcc
                && std::abs(rNotes[i].y - rNotes[i-1].y) < S * 2.2f)
                rNotes[i].accX = rNotes[i-1].accX - S * 0.9f;
        }

        // ── 9. Chord symbol (above treble top, inside panel) ───────────────────
        // Real jazz notation typography: root at full size, quality marker
        // (m, °, ø, +, △) at full size immediately after it, and any
        // extension/alteration digits (7, 9, ♭13, ♯11, ...) superscripted.
        if (chordName_.isNotEmpty())
        {
            const float nameSz = juce::jmax(11.0f, S * 1.05f);
            drawChordSymbol(g, chordName_, noteBaseX - S * 0.5f, panelY + 5.0f, nameSz, ink);
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
            if (rn.isSharp) drawGlyph(g, MusicFont::Glyph::accidentalSharp, rn.accX + S * 0.65f, rn.y, S * MusicFont::Metrics::kStaffSpacesToFontSize);
            if (rn.isFlat)  drawGlyph(g, MusicFont::Glyph::accidentalFlat,  rn.accX + S * 0.55f, rn.y, S * MusicFont::Metrics::kStaffSpacesToFontSize);
            drawGlyph(g, MusicFont::Glyph::noteheadWhole, rn.x + noteW * 0.5f, rn.y, S * MusicFont::Metrics::kStaffSpacesToFontSize);
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
    // SMuFL glyph rendering (Bravura) — noteheads, accidentals, chord-quality marks
    // =================================================================================

    /**
     * How a SMuFL glyph's draw position is interpreted:
     * - centred: (x, y) is the glyph's visual centre. Correct for glyphs whose
     *   ink is roughly symmetric about their design baseline (noteheads,
     *   accidentals, chord-quality marks).
     * - baseline: (x, y) is the glyph's left edge at its SMuFL design
     *   baseline. Required for glyphs whose ink sits mostly above or below
     *   their baseline (clefs) — centring those in a box misplaces them,
     *   since the box centre isn't where the glyph's reference line is.
     */
    enum class GlyphAnchor { centred, baseline };

    /** Draws a single SMuFL glyph at the given font size (px), per `anchor`. */
    void drawGlyph(juce::Graphics& g, juce::juce_wchar glyph, float x, float y, float fontSize,
                   GlyphAnchor anchor = GlyphAnchor::centred) const
    {
        g.setFont(MusicFont::bravuraFont(fontSize));
        const auto text = juce::String::charToString(glyph);
        if (anchor == GlyphAnchor::baseline)
        {
            g.drawSingleLineText(text, juce::roundToInt(x), juce::roundToInt(y));
        }
        else
        {
            auto box = juce::Rectangle<float>(fontSize * 1.6f, fontSize * 1.1f).withCentre({ x, y });
            g.drawText(text, box, juce::Justification::centred, false);
        }
    }

    /**
     * One piece of a rendered chord symbol: either a plain text run (root name,
     * bare "m" for minor, extension digits/alterations) or a single Bravura
     * glyph (°, ø, +, △). `superscript` runs are drawn smaller and raised,
     * matching how real notation software sets extensions/alterations.
     */
    struct ChordSymbolRun
    {
        juce::String    text;
        juce::juce_wchar glyph = 0;
        bool            isGlyph = false;
        bool            superscript = false;
    };

    /**
     * Tokenizes a formatted chord name (e.g. "Cmaj7", "Fm7♭5", "Bdim7/D") into
     * root / quality / extension / slash-bass runs for jazz-style typesetting.
     * Operates purely on the text produced by ChordFormatter — the quality
     * keywords below ("maj", "dim", "aug", "m7♭5", "m11♭5") are exactly the
     * suffix vocabulary used in Source/chord_detection/detector/ChordPatterns.cpp,
     * so a new pattern's suffix simply falls through to the generic case
     * (superscripted as plain text) rather than needing this list updated too.
     */
    std::vector<ChordSymbolRun> tokenizeChordSymbol(const juce::String& chordName) const
    {
        std::vector<ChordSymbolRun> runs;
        if (chordName.isEmpty())
            return runs;

        juce::String main = chordName;
        juce::String bass;
        const int slashPos = chordName.indexOfChar('/');
        if (slashPos > 0)
        {
            main = chordName.substring(0, slashPos);
            bass = chordName.substring(slashPos + 1);
        }

        // Root: a letter A-G optionally followed by a single ♯/♭ accidental.
        int rootLen = main.isNotEmpty() ? 1 : 0;
        if (rootLen > 0 && main.length() > 1)
        {
            const juce::juce_wchar c2 = main[1];
            if (c2 == (juce::juce_wchar) 0x266F || c2 == (juce::juce_wchar) 0x266D) // ♯ / ♭
                rootLen = 2;
        }
        const juce::String root   = main.substring(0, rootLen);
        const juce::String suffix = main.substring(rootLen);

        juce::String quality;             // baseline-size marker ("m", or empty if glyph used)
        juce::juce_wchar qualityGlyph = 0; // 0 = no glyph, use `quality` text instead
        juce::String extension;           // superscripted remainder

        if (suffix == "m7♭5")            { qualityGlyph = MusicFont::Glyph::csymHalfDiminished; }
        else if (suffix == "m11♭5")       { qualityGlyph = MusicFont::Glyph::csymHalfDiminished; extension = "11"; }
        else if (suffix == "maj7")             { qualityGlyph = MusicFont::Glyph::csymMajorSeventh; }
        else if (suffix.startsWith("maj"))     { qualityGlyph = MusicFont::Glyph::csymMajorSeventh; extension = suffix.substring(3); }
        else if (suffix == "dim7")              { qualityGlyph = MusicFont::Glyph::csymDiminished; extension = "7"; }
        else if (suffix == "dim")               { qualityGlyph = MusicFont::Glyph::csymDiminished; }
        else if (suffix == "aug7")               { qualityGlyph = MusicFont::Glyph::csymAugmented; extension = "7"; }
        else if (suffix == "aug")                { qualityGlyph = MusicFont::Glyph::csymAugmented; }
        else if (suffix.startsWith("m") && !suffix.startsWith("maj"))
        {
            quality = "m";
            extension = suffix.substring(1);
        }
        else
        {
            extension = suffix; // dominant family, sus/add/alt/quartal, power chords: all superscripted
        }

        runs.push_back({ root, 0, false, false });
        if (qualityGlyph != 0)
            runs.push_back({ {}, qualityGlyph, true, false });
        else if (quality.isNotEmpty())
            runs.push_back({ quality, 0, false, false });
        if (extension.isNotEmpty())
            runs.push_back({ extension, 0, false, true });
        if (bass.isNotEmpty())
        {
            runs.push_back({ "/", 0, false, false });
            runs.push_back({ bass, 0, false, false });
        }
        return runs;
    }

    /** Lays out and draws a tokenized chord symbol starting at (x, yTop). */
    void drawChordSymbol(juce::Graphics& g, const juce::String& chordName,
                          float x, float yTop, float nameSz, juce::Colour colour) const
    {
        const auto runs = tokenizeChordSymbol(chordName);
        const float superSz   = nameSz * 0.62f;
        const float superRise = nameSz * 0.22f; // raise superscripted extensions above the baseline
        float cursorX = x;

        g.setColour(colour);
        for (const auto& run : runs)
        {
            if (run.isGlyph)
            {
                const float glyphSz = nameSz * 4.6f;
                drawGlyph(g, run.glyph, cursorX + glyphSz * 0.34f, yTop + nameSz * 0.42f, glyphSz);
                cursorX += glyphSz * 0.68f;
                continue;
            }

            const float sz = run.superscript ? superSz : nameSz;
            const float y  = run.superscript ? yTop - superRise : yTop;
            auto font = juce::Font(juce::FontOptions(sz)).boldened();
            g.setFont(font);
            const float w = juce::GlyphArrangement::getStringWidth(font, run.text) + 1.0f;
            g.drawText(run.text, juce::roundToInt(cursorX), juce::roundToInt(y),
                       juce::roundToInt(w) + 2, juce::roundToInt(sz + 4.0f),
                       juce::Justification::centredLeft, false);
            cursorX += w;
        }
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