/*
  ==============================================================================

    MusicFont.h

    Loads the bundled Bravura SMuFL (Standard Music Font Layout) typeface and
    exposes the glyph codepoints NotationComponent needs. Bravura ships as
    fonts/Bravura.otf and is embedded via BinaryData (see CMakeLists.txt).

    Codepoints below are taken verbatim from the SMuFL specification's
    glyphnames.json (https://github.com/w3c/smufl) — every SMuFL-conformant
    font, including Bravura, maps these Private Use Area code points to the
    same glyph shapes, so this file has no Bravura-specific guesswork in it.

  ==============================================================================
*/

#pragma once

#include <juce_graphics/juce_graphics.h>
#include <BinaryData.h>

namespace MusicFont
{
    //==========================================================================
    // SMuFL glyph codepoints (subset used by NotationComponent)
    //==========================================================================
    namespace Glyph
    {
        // Clefs
        constexpr juce::juce_wchar gClef              = 0xE050;
        constexpr juce::juce_wchar fClef              = 0xE062;

        // Noteheads
        constexpr juce::juce_wchar noteheadWhole      = 0xE0A2;

        // Accidentals
        constexpr juce::juce_wchar accidentalFlat     = 0xE260;
        constexpr juce::juce_wchar accidentalNatural  = 0xE261;
        constexpr juce::juce_wchar accidentalSharp    = 0xE262;

        // Chord symbol quality glyphs ("csym" range, SMuFL E870-E87C)
        constexpr juce::juce_wchar csymDiminished     = 0xE870;  // °
        constexpr juce::juce_wchar csymHalfDiminished = 0xE871;  // ø
        constexpr juce::juce_wchar csymAugmented      = 0xE872;  // +
        constexpr juce::juce_wchar csymMajorSeventh   = 0xE873;  // △
    }

    //==========================================================================
    // Metrics needed to lay glyphs out correctly (from Bravura's own font
    // tables — verified with fontTools against fonts/Bravura.otf, not guessed)
    //==========================================================================
    namespace Metrics
    {
        // Bravura declares an exaggerated font ascent/descent (±2012 units on
        // a 1000-unit em, ~4.024 em total line height) so tall glyphs that
        // span a whole staff or more — braces, brackets — never get clipped.
        // JUCE sizes a Font by that declared line height, not the raw em, so
        // asking for height = 4 * (staff space) — the usual "SMuFL glyphs
        // are 4 staff-spaces tall" rule of thumb — undersizes every glyph by
        // roughly 4x. This is the empirically-verified correction factor
        // (see Feature-Docs/Note-Representation-UX-Plan.md for how it was
        // derived): fontSize = staffSpace * kStaffSpacesToFontSize.
        constexpr float kStaffSpacesToFontSize = 8.0f;

        // Glyph advance widths, as a fraction of em, for reserving layout
        // space without needing to re-measure them at paint time.
        constexpr float gClefAdvanceEm = 0.671f;
        constexpr float fClefAdvanceEm = 0.684f;
    }

    /**
     * Returns the shared Bravura typeface, loading it from BinaryData on first use.
     * Safe to call from the message thread only (as with all JUCE font/graphics APIs).
     */
    inline juce::Typeface::Ptr getBravuraTypeface()
    {
        static juce::Typeface::Ptr typeface =
            juce::Typeface::createSystemTypefaceFor(BinaryData::Bravura_otf,
                                                      (size_t) BinaryData::Bravura_otfSize);
        return typeface;
    }

    /** A juce::Font set to Bravura at the given point size, ready for drawText()/GlyphArrangement. */
    inline juce::Font bravuraFont(float pointHeight)
    {
        return juce::Font(juce::FontOptions(getBravuraTypeface())).withHeight(pointHeight);
    }
}
