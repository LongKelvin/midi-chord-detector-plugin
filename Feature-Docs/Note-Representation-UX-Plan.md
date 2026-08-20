# Note Representation UX — Analysis & Plan

**Branch:** `feature/note-representation-ux` (off `main`, current as of 2026-08-20)
**Design reference:** [Role-Coded Note Language mockup](https://claude.ai/code/artifact/577c296a-f67b-449a-a37c-344be0504e61)

## 1. Analysis — current state

Three components independently render "which notes are on right now," and none of them agree with each other or with the chord engine:

| Component | File | What it shows | What it's missing |
|---|---|---|---|
| Piano keyboard | `Source/ui/PianoKeyboardView.h/.cpp` | Binary pressed/unpressed key, one flat accent color (`#00d4ff`) for every pressed key, black or white | No note identity on the key itself, no distinction between a note you're physically holding vs. one only ringing because the sustain pedal is down, no velocity, no relationship to the detected chord |
| Active Notes Monitor | `Source/ui/PlayedNotesComponent.h` | Row of capsule tokens: note name + MIDI number, identical cyan outline for all | Hard `break` cutoff with no scroll/overflow affordance (wide chords — 9th/11th/13th — can be silently truncated off-panel), no role/velocity/sustain info, capsules don't reflect held-vs-sustained |
| Grand staff notation | `Source/ui/NotationComponent.h` | Correctly engraved noteheads, accidentals, ledger lines, 8va/8vb — visually the most polished surface already | All noteheads are the same ink color regardless of function; nothing ties a notehead back to "this is the root" or "this is the 7th" |

**The real gap is architectural, not cosmetic.** `ChordDetector` already computes exactly the semantic data needed — `ChordCandidate::degrees` ("R", "3", "5", "♭7", …) is index-aligned with `pitchClasses`/`noteNames`/`noteNumbers` (`Source/chord_detection/detector/ChordCandidate.h:64-73`) — but `PluginEditor::timerCallback` (`Source/PluginEditor.cpp:189-199`) only ever forwards a raw `std::vector<int>` of MIDI notes into `setNotes()` on all three components. The role information is computed once, displayed as text in the analysis panel, and then thrown away before it reaches the keyboard, monitor, or staff. Similarly, `MidiChordDetectorAudioProcessor` never captures note-on **velocity** at all (confirmed: no `velocity` reference anywhere in `Source/PluginProcessor.*`), and while `sustainedNotes_` is tracked as a bitset (`Source/PluginProcessor.h:119`), that sustain-vs-actively-held distinction never survives past the processor into the UI.

So: three visually inconsistent, semantically flat renderings of the same information, sitting one field away from a much richer picture the engine already has.

## 2. Design direction

**Signature idea:** one role-color language — Root / 3rd / 5th / 7th / Tension(9-13-alt) / Non-chord-tone — applied identically across the keyboard, the monitor, and the staff, plus a shared held-vs-sustained visual state (filled = actively held, hollow/dashed = ringing via pedal only). This is the single aesthetic risk worth taking here; everything else stays close to the shipped dark chrome so the change reads as a refinement, not a reskin.

Palette (dark chrome unchanged; new role accents only):
| Role | Color | Rationale |
|---|---|---|
| Root | `#ffb84d` amber | warm "home" anchor, distinct from every existing UI accent |
| 3rd | `#00d4ff` cyan | keep the app's existing signature accent — it already means "chord quality" |
| 5th | `#34d399` teal-green | stable/supportive, low visual weight |
| 7th | `#b388ff` violet | color/extension |
| 9/11/13/alt | `#ff5fa8` magenta | upper tension, most "spicy" |
| Non-chord tone | `#4a5568` slate | already `staffCol` in `NotationComponent` — reuse, don't invent |

Full mockup with side-by-side before/after for all three surfaces: see the artifact link above.

## 3. TODO — scoped work items

Ordered so each step is independently buildable/testable and maps to one `feat(scope):` commit per our new [Git-Workflow.md](../Docs/Git-Workflow.md).

- [ ] **`feat(plugin)` Plumb note-role data to the editor.** Extend `MidiChordDetectorAudioProcessor` (or a small new struct) so the UI thread can ask "what role does MIDI note N have in the current chord?" — a lookup built from `ChordCandidate::pitchClasses`/`degrees` each time a chord is (re)detected. Keep this off the audio thread's hot path (build the lookup once per chord change, not per `processBlock`).
- [ ] **`feat(plugin)` Track note-on velocity.** Capture velocity in `processMidiMessage` alongside existing note on/off handling; expose per-note velocity (0-127) the same way `getCurrentNotes()` exposes note numbers today.
- [ ] **`feat(plugin)` Expose held-vs-sustained per note.** `sustainedNotes_` already exists on the processor; surface it per-note (e.g. alongside role/velocity) so UI components can distinguish "finger down" from "pedal only."
- [ ] **`feat(ui)` Define a shared `NoteRole` enum + color table.** One small header (e.g. `Source/ui/NoteRoleColors.h`) mapping degree strings → `NoteRole` → `juce::Colour`, so the three components can't drift out of sync with each other. Single source of truth for the palette above.
- [ ] **`feat(ui)` Role-color the piano keyboard.** Extend `PianoKeyboardView::setKeyState` (or add a parallel call) to accept a role + held/sustained flag per note; render root with a bright ring, sustained-only keys hollow instead of filled.
- [ ] **`feat(ui)` Role-color + velocity-bar the Active Notes Monitor.** Add role tag and a thin velocity-intensity bar to each capsule; replace the hard `break` overflow with either horizontal scroll or wrap-to-second-row so wide voicings (9ths/11ths/13ths) are never silently cut off.
- [ ] **`feat(ui)` Role-color notation noteheads.** Tint noteheads in `NotationComponent::paint` by role, outline (not fill) the root notehead to keep it readable next to accidentals.
- [ ] **`test(tests)` / manual DAW pass.** No detector logic changes, so `Source/tests/` coverage is unaffected — validate visually via the standalone app (`MidiChordDetectorStandalone`) across a spread of chord types (triads, rootless voicings, wide 13th chords) before touching the VST3 target.
- [ ] **`docs(ui)`** Update `Docs/Technical-Design.md` §4.7 and the README screenshot once the new look is final.

## 3b. Shipped: grand-staff engraving redesign (SMuFL)

Landed on `feature/note-representation-ux` ahead of the role-color plumbing above, in response to a direct ask to match real notation-software engraving quality. Kept the grand-staff layout (per the staff-layout decision above) and rebuilt its rendering fidelity rather than switching to a single treble staff.

**Root cause found:** `fonts/Bravura.otf` was already bundled and embedded via `BinaryData` (see `CMakeLists.txt`), but nothing in the codebase actually loaded or used it — `NotationComponent` hand-drew noteheads/accidentals as vector paths and rendered clefs via generic Unicode musical-symbol codepoints (`U+1D11E`/`U+1D122`) on whatever font the host OS happened to fall back to. The asset was present but unused.

**What changed:**
- **`Source/ui/MusicFont.h`** (new) — loads the bundled Bravura typeface from `BinaryData` and exposes the exact SMuFL codepoints used (verified against the authoritative `w3c/smufl` `glyphnames.json`, not guessed): `noteheadWhole` (U+E0A2), `accidentalSharp`/`accidentalFlat` (U+E262/E260), and the jazz chord-symbol glyphs `csymDiminished`/`csymHalfDiminished`/`csymAugmented`/`csymMajorSeventh` (U+E870–E873) — the ° / ø / + / △ marks the reference image uses.
- **Noteheads & accidentals** now render as real Bravura glyphs instead of hand-drawn paths.
- **Chord symbol typography** — new `tokenizeChordSymbol()`/`drawChordSymbol()`: root at full size, quality marker (`m`, or a °/ø/+/△ glyph) at full size immediately after it, extensions/alterations (7, 9, ♭13, ♯11, …) superscripted — matching how real notation software sets jazz chord symbols. Tokenizer parses the existing `ChordFormatter` output text (no changes to `ChordPatterns.cpp` or the detector needed); unrecognized suffixes fall back to plain superscripted text, so new patterns don't require updating a second lookup table.
- **Fixed a real clustering bug:** the old stacked-seconds logic only ever compared a note to its immediate predecessor and picked one of two fixed x-offsets, so a chain of 3+ consecutive seconds (e.g. a tight C-D-E-F-G cluster) piled notes 2 and 3 onto the same offset column instead of zig-zagging. Rewritten as a proper chain-aware alternating zig-zag, verified visually against a synthetic 5-note cluster.
- Font-size tuning: Bravura declares an exaggerated font ascent/descent (±2012 units on a 1000-unit em, to fit tall multi-staff glyphs like braces without clipping), so a naive "4 staff-spaces = font height" assumption rendered glyphs ~4x too small. Empirically corrected via the visual-QA loop below.

**How it was verified:** no live JUCE preview is available in this environment, so a temporary, git-revertible harness was added to the standalone app's `MainComponent.cpp` (plus a temporary `BinaryData` link in `CMakeLists.txt`) that renders `NotationComponent` off-screen for a batch of representative chords (`Cmaj7`, `Cm7♭5`, `Cdim7`, `Caug`, `Cm11`, `C7♯9♭13`, `C13♭9`, and a synthetic 5-note tight cluster) to PNG snapshots, then exits. Both temporary files were reverted (`git checkout --`) after use — they never landed on the branch. The plugin and standalone targets both build clean (no errors, no warnings) after the change.

**Known follow-ups (tracked, not blocking):** accidental vertical alignment relative to the staff line/space could be tightened further; long chord symbols (e.g. `C7♯9♭13`) can visually crowd the topmost notehead on tall/wide voicings — needs a responsive `topPad` or smaller chord-symbol sizing at high note counts.

## 3c. Shipped: clef position accuracy + clef/note margin

Follow-up fix after user testing found two issues in a real chord (screenshot: F2 C3 F3 G#3 C4 D#4 G4 across both staves):

1. **Clef position was inaccurate.** The treble/bass clefs were still drawn with the old generic-Unicode-glyph + guessed-bounding-box approach (deliberately left unchanged in §3b to limit risk) — never converted to Bravura, unlike noteheads/accidentals. Root cause of "not quite right" position: a centred bounding box is the wrong anchor model for clefs, whose ink is *not* symmetric about their design baseline (verified via `fontTools`: gClef bbox is -658..+1098 em-units around baseline, fClef is -635..+262) — SMuFL clefs are meant to be placed at an explicit baseline (gClef's baseline = bottom treble line; fClef's baseline = top bass line), not centred in a box. Fixed by adding a `GlyphAnchor::baseline` mode to `drawGlyph()` (uses `Graphics::drawSingleLineText`) and switching both clefs to real Bravura glyphs anchored that way. Side effect: this also fixed an unrelated overflow the user's screenshot showed (a high note rendering outside the panel's top border) — that was downstream of the same imprecise old clef/layout math, not a separate bug.
2. **Notes crowded the clef.** `clefW` (horizontal space reserved for the clef) was a flat guessed constant (`S*2.8`); now computed from the clefs' actual Bravura advance widths (`MusicFont::Metrics::gClefAdvanceEm`/`fClefAdvanceEm`, from the font's own `hmtx` table) plus explicit margin, and the clef-to-first-note gap was widened (`noteBaseX` gap `S*0.6` → `S*1.6`).

Verified visually (temporary QA harness, reverted after use) against the exact chord from the bug report plus isolated single-note clef zoom shots. Plugin target builds clean.

**Correction (user caught this in review):** the baseline anchor itself was off by one staff line in both directions — gClef's spiral hugs the **G4 line (2nd line from the bottom)**, not the bottom line (E4); fClef's two dots straddle the **F3 line (2nd line from the top)**, not the top line (A3). Fixed by offsetting the baseline Y by one staff-line-spacing (`trebleBotY - S` / `bassTopY + S`) from the previous (wrong) anchor. Re-verified against isolated G4-only and F3-only renders plus a `D/C` chord matching the reference image the user provided.

## 4. Explicitly out of scope for this branch

- The `Source/sound_engine/` work (separate untracked feature, unrelated to display).
- Any change to chord-detection scoring/pattern logic — this is purely a display-layer enhancement consuming data the detector already produces.
