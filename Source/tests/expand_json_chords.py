import json

# Load the original test suite
with open('chord_tests.json', 'r') as f:
    data = json.load(f)

# New categories to add with comprehensive chord types
new_categories = [
    {
        "category": "Suspended Chords",
        "tests": [
            {"name": "C sus2", "notes": ["C4", "D4", "G4"], "expected": "Csus2", "minConfidence": 0.6},
            {"name": "D sus2", "notes": ["D4", "E4", "A4"], "expected": "Dsus2", "minConfidence": 0.6},
            {"name": "F sus2", "notes": ["F4", "G4", "C5"], "expected": "Fsus2", "minConfidence": 0.6},
            {"name": "G sus2", "notes": ["G4", "A4", "D5"], "expected": "Gsus2", "minConfidence": 0.6},
            {"name": "C sus4", "notes": ["C4", "F4", "G4"], "expected": "Csus4", "minConfidence": 0.6},
            {"name": "D sus4", "notes": ["D4", "G4", "A4"], "expected": "Dsus4", "minConfidence": 0.6},
            {"name": "F sus4", "notes": ["F4", "Bb4", "C5"], "expected": "Fsus4", "minConfidence": 0.6},
            {"name": "G sus4", "notes": ["G4", "C5", "D5"], "expected": "Gsus4", "minConfidence": 0.6},
            {"name": "A sus4", "notes": ["A4", "D5", "E5"], "expected": "Asus4", "minConfidence": 0.6},
            {"name": "Bb sus4", "notes": ["Bb4", "Eb5", "F5"], "expected": "Bbsus4", "minConfidence": 0.6},
        ]
    },
    {
        "category": "Minor Major Seventh Chords",
        "tests": [
            {"name": "C Minor Major 7", "notes": ["C4", "Eb4", "G4", "B4"], "expected": "Cm(maj7)", "minConfidence": 0.5},
            {"name": "D Minor Major 7", "notes": ["D4", "F4", "A4", "C#5"], "expected": "Dm(maj7)", "minConfidence": 0.5},
            {"name": "E Minor Major 7", "notes": ["E4", "G4", "B4", "D#5"], "expected": "Em(maj7)", "minConfidence": 0.5},
            {"name": "F Minor Major 7", "notes": ["F4", "Ab4", "C5", "E5"], "expected": "Fm(maj7)", "minConfidence": 0.5},
            {"name": "G Minor Major 7", "notes": ["G4", "Bb4", "D5", "F#5"], "expected": "Gm(maj7)", "minConfidence": 0.5},
            {"name": "A Minor Major 7", "notes": ["A4", "C5", "E5", "G#5"], "expected": "Am(maj7)", "minConfidence": 0.5},
        ]
    },
    {
        "category": "Half-Diminished Seventh Chords",
        "tests": [
            {"name": "C Half-Diminished 7", "notes": ["C4", "Eb4", "Gb4", "Bb4"], "expected": "Cm7b5", "minConfidence": 0.5},
            {"name": "D Half-Diminished 7", "notes": ["D4", "F4", "Ab4", "C5"], "expected": "Dm7b5", "minConfidence": 0.5},
            {"name": "E Half-Diminished 7", "notes": ["E4", "G4", "Bb4", "D5"], "expected": "Em7b5", "minConfidence": 0.5},
            {"name": "F Half-Diminished 7", "notes": ["F4", "Ab4", "B4", "Eb5"], "expected": "Fm7b5", "minConfidence": 0.5},
            {"name": "G Half-Diminished 7", "notes": ["G4", "Bb4", "Db5", "F5"], "expected": "Gm7b5", "minConfidence": 0.5},
            {"name": "A Half-Diminished 7", "notes": ["A4", "C5", "Eb5", "G5"], "expected": "Am7b5", "minConfidence": 0.5},
            {"name": "B Half-Diminished 7", "notes": ["B4", "D5", "F5", "A5"], "expected": "Bm7b5", "minConfidence": 0.5},
        ]
    },
    {
        "category": "Diminished Seventh Chords",
        "tests": [
            {"name": "C Diminished 7", "notes": ["C4", "Eb4", "Gb4", "A4"], "expected": "Cdim7", "minConfidence": 0.5},
            {"name": "D Diminished 7", "notes": ["D4", "F4", "Ab4", "B4"], "expected": "Ddim7", "minConfidence": 0.5},
            {"name": "E Diminished 7", "notes": ["E4", "G4", "Bb4", "Db5"], "expected": "Edim7", "minConfidence": 0.5},
            {"name": "F Diminished 7", "notes": ["F4", "Ab4", "B4", "D5"], "expected": "Fdim7", "minConfidence": 0.5},
            {"name": "G Diminished 7", "notes": ["G4", "Bb4", "Db5", "E5"], "expected": "Gdim7", "minConfidence": 0.5},
            {"name": "A Diminished 7", "notes": ["A4", "C5", "Eb5", "Gb5"], "expected": "Adim7", "minConfidence": 0.5},
        ]
    },
    {
        "category": "Augmented Seventh Chords",
        "tests": [
            {"name": "C Augmented Major 7", "notes": ["C4", "E4", "G#4", "B4"], "expected": "Caugmaj7", "minConfidence": 0.5},
            {"name": "D Augmented Major 7", "notes": ["D4", "F#4", "A#4", "C#5"], "expected": "Daugmaj7", "minConfidence": 0.5},
            {"name": "F Augmented Major 7", "notes": ["F4", "A4", "C#5", "E5"], "expected": "Faugmaj7", "minConfidence": 0.5},
            {"name": "C Augmented 7", "notes": ["C4", "E4", "G#4", "Bb4"], "expected": "C7#5", "minConfidence": 0.5},
            {"name": "D Augmented 7", "notes": ["D4", "F#4", "A#4", "C5"], "expected": "D7#5", "minConfidence": 0.5},
            {"name": "G Augmented 7", "notes": ["G4", "B4", "D#5", "F5"], "expected": "G7#5", "minConfidence": 0.5},
        ]
    },
    {
        "category": "Sixth Chords",
        "tests": [
            {"name": "C Major 6", "notes": ["C4", "E4", "G4", "A4"], "expected": "C6", "minConfidence": 0.6},
            {"name": "D Major 6", "notes": ["D4", "F#4", "A4", "B4"], "expected": "D6", "minConfidence": 0.6},
            {"name": "F Major 6", "notes": ["F4", "A4", "C5", "D5"], "expected": "F6", "minConfidence": 0.6},
            {"name": "G Major 6", "notes": ["G4", "B4", "D5", "E5"], "expected": "G6", "minConfidence": 0.6},
            {"name": "C Minor 6", "notes": ["C4", "Eb4", "G4", "A4"], "expected": "Cm6", "minConfidence": 0.6},
            {"name": "D Minor 6", "notes": ["D4", "F4", "A4", "B4"], "expected": "Dm6", "minConfidence": 0.6},
            {"name": "E Minor 6", "notes": ["E4", "G4", "B4", "C#5"], "expected": "Em6", "minConfidence": 0.6},
            {"name": "A Minor 6", "notes": ["A4", "C5", "E5", "F#5"], "expected": "Am6", "minConfidence": 0.6},
        ]
    },
    {
        "category": "Six Nine Chords",
        "tests": [
            {"name": "C Major 6/9", "notes": ["C4", "E4", "G4", "A4", "D5"], "expected": "C6/9", "minConfidence": 0.5},
            {"name": "D Major 6/9", "notes": ["D4", "F#4", "A4", "B4", "E5"], "expected": "D6/9", "minConfidence": 0.5},
            {"name": "F Major 6/9", "notes": ["F4", "A4", "C5", "D5", "G5"], "expected": "F6/9", "minConfidence": 0.5},
            {"name": "G Major 6/9", "notes": ["G4", "B4", "D5", "E5", "A5"], "expected": "G6/9", "minConfidence": 0.5},
            {"name": "C Minor 6/9", "notes": ["C4", "Eb4", "G4", "A4", "D5"], "expected": "Cm6/9", "minConfidence": 0.5},
            {"name": "D Minor 6/9", "notes": ["D4", "F4", "A4", "B4", "E5"], "expected": "Dm6/9", "minConfidence": 0.5},
        ]
    },
    {
        "category": "Altered Dominant Chords",
        "tests": [
            {"name": "C Dominant 7b9", "notes": ["C3", "E4", "G4", "Bb4", "Db5"], "expected": "C7b9", "minConfidence": 0.5},
            {"name": "D Dominant 7b9", "notes": ["D3", "F#4", "A4", "C5", "Eb5"], "expected": "D7b9", "minConfidence": 0.5},
            {"name": "G Dominant 7b9", "notes": ["G3", "B4", "D5", "F5", "Ab5"], "expected": "G7b9", "minConfidence": 0.5},
            {"name": "C Dominant 7#9", "notes": ["C3", "E4", "G4", "Bb4", "D#5"], "expected": "C7#9", "minConfidence": 0.5},
            {"name": "D Dominant 7#9", "notes": ["D3", "F#4", "A4", "C5", "E#5"], "expected": "D7#9", "minConfidence": 0.5},
            {"name": "G Dominant 7#9", "notes": ["G3", "B4", "D5", "F5", "A#5"], "expected": "G7#9", "minConfidence": 0.5},
            {"name": "C Dominant 7b5", "notes": ["C4", "E4", "Gb4", "Bb4"], "expected": "C7b5", "minConfidence": 0.5},
            {"name": "F Dominant 7b5", "notes": ["F4", "A4", "B4", "Eb5"], "expected": "F7b5", "minConfidence": 0.5},
            {"name": "C Dominant 7b9b5", "notes": ["C3", "E4", "Gb4", "Bb4", "Db5"], "expected": "C7b9b5", "minConfidence": 0.4},
            {"name": "C Dominant 7#9b5", "notes": ["C3", "E4", "Gb4", "Bb4", "D#5"], "expected": "C7#9b5", "minConfidence": 0.4},
            {"name": "C Dominant 7b9#5", "notes": ["C3", "E4", "G#4", "Bb4", "Db5"], "expected": "C7b9#5", "minConfidence": 0.4},
            {"name": "C Dominant 7#9#5", "notes": ["C3", "E4", "G#4", "Bb4", "D#5"], "expected": "C7#9#5", "minConfidence": 0.4},
        ]
    },
    {
        "category": "Dominant Eleventh Chords",
        "tests": [
            {"name": "C Dominant 11", "notes": ["C3", "E4", "G4", "Bb4", "D5", "F5"], "expected": "C11", "minConfidence": 0.4},
            {"name": "D Dominant 11", "notes": ["D3", "F#4", "A4", "C5", "E5", "G5"], "expected": "D11", "minConfidence": 0.4},
            {"name": "F Dominant 11", "notes": ["F3", "A4", "C5", "Eb5", "G5", "Bb5"], "expected": "F11", "minConfidence": 0.4},
            {"name": "G Dominant 11", "notes": ["G3", "B4", "D5", "F5", "A5", "C6"], "expected": "G11", "minConfidence": 0.4},
            {"name": "C Dominant 7#11", "notes": ["C3", "E4", "G4", "Bb4", "F#5"], "expected": "C7#11", "minConfidence": 0.5},
            {"name": "D Dominant 7#11", "notes": ["D3", "F#4", "A4", "C5", "G#5"], "expected": "D7#11", "minConfidence": 0.5},
            {"name": "G Dominant 7#11", "notes": ["G3", "B4", "D5", "F5", "C#6"], "expected": "G7#11", "minConfidence": 0.5},
        ]
    },
    {
        "category": "Minor Eleventh Chords",
        "tests": [
            {"name": "C Minor 11", "notes": ["C3", "Eb4", "G4", "Bb4", "D5", "F5"], "expected": "Cm11", "minConfidence": 0.4},
            {"name": "D Minor 11", "notes": ["D3", "F4", "A4", "C5", "E5", "G5"], "expected": "Dm11", "minConfidence": 0.4},
            {"name": "E Minor 11", "notes": ["E3", "G4", "B4", "D5", "F#5", "A5"], "expected": "Em11", "minConfidence": 0.4},
            {"name": "A Minor 11", "notes": ["A3", "C5", "E5", "G5", "B5", "D6"], "expected": "Am11", "minConfidence": 0.4},
        ]
    },
    {
        "category": "Major Eleventh Chords",
        "tests": [
            {"name": "C Major 11", "notes": ["C3", "E4", "G4", "B4", "D5", "F5"], "expected": "Cmaj11", "minConfidence": 0.4},
            {"name": "D Major 11", "notes": ["D3", "F#4", "A4", "C#5", "E5", "G5"], "expected": "Dmaj11", "minConfidence": 0.4},
            {"name": "F Major 11", "notes": ["F3", "A4", "C5", "E5", "G5", "Bb5"], "expected": "Fmaj11", "minConfidence": 0.4},
            {"name": "C Major 7#11", "notes": ["C3", "E4", "G4", "B4", "F#5"], "expected": "Cmaj7#11", "minConfidence": 0.5},
            {"name": "D Major 7#11", "notes": ["D3", "F#4", "A4", "C#5", "G#5"], "expected": "Dmaj7#11", "minConfidence": 0.5},
            {"name": "F Major 7#11", "notes": ["F3", "A4", "C5", "E5", "B5"], "expected": "Fmaj7#11", "minConfidence": 0.5},
        ]
    },
    {
        "category": "Dominant Thirteenth Chords",
        "tests": [
            {"name": "C Dominant 13", "notes": ["C3", "E4", "G4", "Bb4", "D5", "A5"], "expected": "C13", "minConfidence": 0.4},
            {"name": "D Dominant 13", "notes": ["D3", "F#4", "A4", "C5", "E5", "B5"], "expected": "D13", "minConfidence": 0.4},
            {"name": "F Dominant 13", "notes": ["F3", "A4", "C5", "Eb5", "G5", "D6"], "expected": "F13", "minConfidence": 0.4},
            {"name": "G Dominant 13", "notes": ["G3", "B4", "D5", "F5", "A5", "E6"], "expected": "G13", "minConfidence": 0.4},
            {"name": "C Dominant 13b9", "notes": ["C3", "E4", "G4", "Bb4", "Db5", "A5"], "expected": "C13b9", "minConfidence": 0.4},
            {"name": "C Dominant 13#9", "notes": ["C3", "E4", "G4", "Bb4", "D#5", "A5"], "expected": "C13#9", "minConfidence": 0.4},
            {"name": "C Dominant 13#11", "notes": ["C3", "E4", "G4", "Bb4", "F#5", "A5"], "expected": "C13#11", "minConfidence": 0.4},
        ]
    },
    {
        "category": "Major Thirteenth Chords",
        "tests": [
            {"name": "C Major 13", "notes": ["C3", "E4", "G4", "B4", "D5", "A5"], "expected": "Cmaj13", "minConfidence": 0.4},
            {"name": "D Major 13", "notes": ["D3", "F#4", "A4", "C#5", "E5", "B5"], "expected": "Dmaj13", "minConfidence": 0.4},
            {"name": "F Major 13", "notes": ["F3", "A4", "C5", "E5", "G5", "D6"], "expected": "Fmaj13", "minConfidence": 0.4},
            {"name": "G Major 13", "notes": ["G3", "B4", "D5", "F#5", "A5", "E6"], "expected": "Gmaj13", "minConfidence": 0.4},
        ]
    },
    {
        "category": "Minor Thirteenth Chords",
        "tests": [
            {"name": "C Minor 13", "notes": ["C3", "Eb4", "G4", "Bb4", "D5", "A5"], "expected": "Cm13", "minConfidence": 0.4},
            {"name": "D Minor 13", "notes": ["D3", "F4", "A4", "C5", "E5", "B5"], "expected": "Dm13", "minConfidence": 0.4},
            {"name": "E Minor 13", "notes": ["E3", "G4", "B4", "D5", "F#5", "C#6"], "expected": "Em13", "minConfidence": 0.4},
            {"name": "A Minor 13", "notes": ["A3", "C5", "E5", "G5", "B5", "F#6"], "expected": "Am13", "minConfidence": 0.4},
        ]
    },
    {
        "category": "Added Tone Chords",
        "tests": [
            {"name": "C add9", "notes": ["C4", "E4", "G4", "D5"], "expected": "Cadd9", "minConfidence": 0.6},
            {"name": "D add9", "notes": ["D4", "F#4", "A4", "E5"], "expected": "Dadd9", "minConfidence": 0.6},
            {"name": "F add9", "notes": ["F4", "A4", "C5", "G5"], "expected": "Fadd9", "minConfidence": 0.6},
            {"name": "G add9", "notes": ["G4", "B4", "D5", "A5"], "expected": "Gadd9", "minConfidence": 0.6},
            {"name": "C minor add9", "notes": ["C4", "Eb4", "G4", "D5"], "expected": "Cmadd9", "minConfidence": 0.6},
            {"name": "D minor add9", "notes": ["D4", "F4", "A4", "E5"], "expected": "Dmadd9", "minConfidence": 0.6},
            {"name": "E minor add9", "notes": ["E4", "G4", "B4", "F#5"], "expected": "Emadd9", "minConfidence": 0.6},
            {"name": "A minor add9", "notes": ["A4", "C5", "E5", "B5"], "expected": "Amadd9", "minConfidence": 0.6},
        ]
    },
    {
        "category": "Suspended Seventh Chords",
        "tests": [
            {"name": "C Dominant 7sus4", "notes": ["C4", "F4", "G4", "Bb4"], "expected": "C7sus4", "minConfidence": 0.6},
            {"name": "D Dominant 7sus4", "notes": ["D4", "G4", "A4", "C5"], "expected": "D7sus4", "minConfidence": 0.6},
            {"name": "F Dominant 7sus4", "notes": ["F4", "Bb4", "C5", "Eb5"], "expected": "F7sus4", "minConfidence": 0.6},
            {"name": "G Dominant 7sus4", "notes": ["G4", "C5", "D5", "F5"], "expected": "G7sus4", "minConfidence": 0.6},
            {"name": "C Dominant 9sus4", "notes": ["C3", "F4", "G4", "Bb4", "D5"], "expected": "C9sus4", "minConfidence": 0.5},
            {"name": "D Dominant 9sus4", "notes": ["D3", "G4", "A4", "C5", "E5"], "expected": "D9sus4", "minConfidence": 0.5},
            {"name": "G Dominant 9sus4", "notes": ["G3", "C5", "D5", "F5", "A5"], "expected": "G9sus4", "minConfidence": 0.5},
            {"name": "C Dominant 13sus4", "notes": ["C3", "F4", "G4", "Bb4", "D5", "A5"], "expected": "C13sus4", "minConfidence": 0.4},
        ]
    },
    {
        "category": "Slash Chords - Major over Major",
        "tests": [
            {"name": "C/E (1st inversion)", "notes": ["E4", "G4", "C5"], "expected": "C/E", "minConfidence": 0.6},
            {"name": "C/G (2nd inversion)", "notes": ["G3", "C4", "E4"], "expected": "C/G", "minConfidence": 0.6},
            {"name": "D/F# (1st inversion)", "notes": ["F#4", "A4", "D5"], "expected": "D/F#", "minConfidence": 0.6},
            {"name": "F/A (1st inversion)", "notes": ["A3", "C4", "F4"], "expected": "F/A", "minConfidence": 0.6},
            {"name": "G/B (1st inversion)", "notes": ["B3", "D4", "G4"], "expected": "G/B", "minConfidence": 0.6},
            {"name": "G/D (2nd inversion)", "notes": ["D4", "G4", "B4"], "expected": "G/D", "minConfidence": 0.6},
        ]
    },
    {
        "category": "Slash Chords - Minor Inversions",
        "tests": [
            {"name": "Cm/Eb (1st inversion)", "notes": ["Eb4", "G4", "C5"], "expected": "Cm/Eb", "minConfidence": 0.6},
            {"name": "Cm/G (2nd inversion)", "notes": ["G3", "C4", "Eb4"], "expected": "Cm/G", "minConfidence": 0.6},
            {"name": "Dm/F (1st inversion)", "notes": ["F4", "A4", "D5"], "expected": "Dm/F", "minConfidence": 0.6},
            {"name": "Em/G (1st inversion)", "notes": ["G3", "B3", "E4"], "expected": "Em/G", "minConfidence": 0.6},
            {"name": "Am/C (1st inversion)", "notes": ["C4", "E4", "A4"], "expected": "Am/C", "minConfidence": 0.6},
            {"name": "Am/E (2nd inversion)", "notes": ["E4", "A4", "C5"], "expected": "Am/E", "minConfidence": 0.6},
        ]
    },
    {
        "category": "Slash Chords - Seventh Inversions",
        "tests": [
            {"name": "Cmaj7/E (1st inversion)", "notes": ["E4", "G4", "B4", "C5"], "expected": "Cmaj7/E", "minConfidence": 0.5},
            {"name": "Cmaj7/G (2nd inversion)", "notes": ["G3", "B3", "C4", "E4"], "expected": "Cmaj7/G", "minConfidence": 0.5},
            {"name": "Cmaj7/B (3rd inversion)", "notes": ["B3", "C4", "E4", "G4"], "expected": "Cmaj7/B", "minConfidence": 0.5},
            {"name": "C7/E (1st inversion)", "notes": ["E4", "G4", "Bb4", "C5"], "expected": "C7/E", "minConfidence": 0.5},
            {"name": "C7/G (2nd inversion)", "notes": ["G3", "Bb3", "C4", "E4"], "expected": "C7/G", "minConfidence": 0.5},
            {"name": "C7/Bb (3rd inversion)", "notes": ["Bb3", "C4", "E4", "G4"], "expected": "C7/Bb", "minConfidence": 0.5},
        ]
    },
    {
        "category": "Dominant Ninth Extended Alterations",
        "tests": [
            {"name": "C Dominant 9b5", "notes": ["C3", "E4", "Gb4", "Bb4", "D5"], "expected": "C9b5", "minConfidence": 0.4},
            {"name": "C Dominant 9#5", "notes": ["C3", "E4", "G#4", "Bb4", "D5"], "expected": "C9#5", "minConfidence": 0.4},
            {"name": "C Dominant 9#11", "notes": ["C3", "E4", "G4", "Bb4", "D5", "F#5"], "expected": "C9#11", "minConfidence": 0.4},
            {"name": "D Dominant 9#11", "notes": ["D3", "F#4", "A4", "C5", "E5", "G#5"], "expected": "D9#11", "minConfidence": 0.4},
        ]
    },
    {
        "category": "Polychords and Upper Structures",
        "tests": [
            {"name": "D/C (D triad over C)", "notes": ["C3", "D4", "F#4", "A4"], "expected": "D/C", "minConfidence": 0.4},
            {"name": "E/C (E triad over C)", "notes": ["C3", "E4", "G#4", "B4"], "expected": "E/C", "minConfidence": 0.4},
            {"name": "Bb/C (Bb triad over C)", "notes": ["C3", "Bb3", "D4", "F4"], "expected": "Bb/C", "minConfidence": 0.4},
            {"name": "Dm/C (Dm triad over C)", "notes": ["C3", "D4", "F4", "A4"], "expected": "Dm/C", "minConfidence": 0.4},
            {"name": "F#/C (F# triad over C)", "notes": ["C3", "F#4", "A#4", "C#5"], "expected": "F#/C", "minConfidence": 0.4},
        ]
    },
    {
        "category": "Quartal Harmony",
        "tests": [
            {"name": "C Quartal Triad", "notes": ["C4", "F4", "Bb4"], "expected": "C-F-Bb", "minConfidence": 0.5},
            {"name": "D Quartal Triad", "notes": ["D4", "G4", "C5"], "expected": "D-G-C", "minConfidence": 0.5},
            {"name": "G Quartal Triad", "notes": ["G4", "C5", "F5"], "expected": "G-C-F", "minConfidence": 0.5},
            {"name": "C Quartal Seventh", "notes": ["C4", "F4", "Bb4", "E5"], "expected": "C-F-Bb-E", "minConfidence": 0.4},
            {"name": "D So What Chord", "notes": ["D4", "G4", "C5", "F5", "A5"], "expected": "DSoWhat", "minConfidence": 0.4},
        ]
    },
    {
        "category": "Chromatic and Color Chords",
        "tests": [
            {"name": "C Major Cluster", "notes": ["C4", "D4", "E4", "G4"], "expected": "Ccluster", "minConfidence": 0.4},
            {"name": "C Minor Cluster", "notes": ["C4", "Db4", "Eb4"], "expected": "Cmcluster", "minConfidence": 0.4},
            {"name": "C Tone Cluster", "notes": ["C4", "D4", "E4"], "expected": "Ctone", "minConfidence": 0.4},
        ]
    },
    {
        "category": "Advanced Jazz Voicings",
        "tests": [
            {"name": "C Half-Diminished 9", "notes": ["C4", "Eb4", "Gb4", "Bb4", "D5"], "expected": "Cm7b5(9)", "minConfidence": 0.4},
            {"name": "D Half-Diminished 9", "notes": ["D4", "F4", "Ab4", "C5", "E5"], "expected": "Dm7b5(9)", "minConfidence": 0.4},
            {"name": "C Minor 11b5", "notes": ["C3", "Eb4", "Gb4", "Bb4", "D5", "F5"], "expected": "Cm11b5", "minConfidence": 0.4},
            {"name": "C Dominant 7b13", "notes": ["C3", "E4", "G4", "Bb4", "Ab5"], "expected": "C7b13", "minConfidence": 0.4},
            {"name": "C Dominant 13b9b13", "notes": ["C3", "E4", "G4", "Bb4", "Db5", "Ab5"], "expected": "C13b9b13", "minConfidence": 0.3},
        ]
    },
]

# Add new categories to the existing data
data["chords"].extend(new_categories)

# Update metadata
data["metadata"]["version"] = "3.0"
data["metadata"]["description"] = "H-WCTM Comprehensive Chord Detection Test Suite - Full Jazz & Advanced Harmony"
data["metadata"]["generatedDate"] = "2026-01-29"

# Save the expanded test suite
output_path = '/home/claude/chord_tests_expanded.json'
with open(output_path, 'w') as f:
    json.dump(data, f, indent=2)

print(f"✅ Expanded chord test suite created successfully!")
print(f"📊 Total categories: {len(data['chords'])}")
print(f"📝 Total test cases: {sum(len(cat['tests']) for cat in data['chords'])}")
print(f"💾 Saved to: {output_path}")

# Print category summary
print("\n📋 Categories Added:")
for cat in new_categories:
    print(f"  - {cat['category']}: {len(cat['tests'])} tests")
