#include "ChordDetector.h"
#include <algorithm>
#include <cstring>
#include <cmath>

namespace ChordDetection
{

ChordDetector::ChordDetector()
    : minimumScore_(MIN_ACCEPTABLE_SCORE)
{
    noteBuffer_.fill(-1);
    pitchClassSet_.reset();
    pitchClassWeights_.fill(0.0f);
}

void ChordDetector::noteOn(int midiNote, double currentTimeMs)
{
    timeWindow_.addNote(midiNote, currentTimeMs);
}

void ChordDetector::noteOff(int midiNote, double currentTimeMs)
{
    (void)currentTimeMs; // Unused parameter
    timeWindow_.removeNote(midiNote);
}

void ChordDetector::reset()
{
    timeWindow_.clear();
}

void ChordDetector::setTimeWindow(double windowMs)
{
    timeWindow_.setWindowSize(windowMs);
}

void ChordDetector::setMinimumScore(float score)
{
    minimumScore_ = score;
}

ChordCandidate ChordDetector::detectChord(
    const MidiNoteState& noteState,
    double currentTimeMs,
    bool useTimeWindow,
    int minNotes
)
{
    ChordCandidate result;
    
    // Get active notes (either from state or time window)
    int noteCount = 0;
    
    if (useTimeWindow)
    {
        noteCount = timeWindow_.getNotesInWindow(
            currentTimeMs,
            timeWindow_.getWindowSize(),
            noteBuffer_.data(),
            static_cast<int>(noteBuffer_.size())
        );
    }
    else
    {
        noteCount = noteState.getActiveNotes(
            noteBuffer_.data(),
            static_cast<int>(noteBuffer_.size())
        );
    }
    
    // Need minimum notes for a chord
    if (noteCount < minNotes)
    {
        return result; // isValid = false
    }
    
    // Sort notes for consistent processing
    std::sort(noteBuffer_.begin(), noteBuffer_.begin() + noteCount);
    
    // Voice separation
    VoiceBuckets voices;
    separateVoices(noteBuffer_.data(), noteCount, voices);
    
    // Use chord tones for analysis (ignore melody)
    int analysisNoteCount = voices.bassCount + voices.chordCount;
    if (analysisNoteCount < minNotes)
    {
        return result;
    }
    
    // Build analysis note array
    std::array<int, 32> analysisNotes;
    int analysisIndex = 0;
    
    for (int i = 0; i < voices.bassCount; ++i)
        analysisNotes[analysisIndex++] = voices.bassNotes[i];
    for (int i = 0; i < voices.chordCount; ++i)
        analysisNotes[analysisIndex++] = voices.chordNotes[i];
    
    // Get pitch class set
    std::bitset<12> playedPitchClasses = getPitchClassSet(
        analysisNotes.data(),
        analysisNoteCount
    );
    
    // Find lowest note (bass)
    int lowestNote = analysisNotes[0];
    for (int i = 1; i < analysisNoteCount; ++i)
    {
        if (analysisNotes[i] < lowestNote)
            lowestNote = analysisNotes[i];
    }
    
    int bassPC = lowestNote % 12;
    
    // Test all root candidates
    float bestScore = -1.0f;
    int bestRoot = -1;
    int bestChordIndex = -1;
    
    for (int rootPC = 0; rootPC < 12; ++rootPC)
    {
        // Normalize pitch classes relative to this root
        std::bitset<12> relativePCs;
        relativePCs.reset();
        
        for (int pc = 0; pc < 12; ++pc)
        {
            if (playedPitchClasses[pc])
            {
                int relativePC = (pc - rootPC + 12) % 12;
                relativePCs[relativePC] = true;
            }
        }
        
        // Test against all chord definitions
        for (int chordIndex = 0; chordIndex < CHORD_DEFINITION_COUNT; ++chordIndex)
        {
            const ChordDefinition& def = CHORD_DEFINITIONS[chordIndex];
            
            float score = scoreChordCandidate(relativePCs, def);
            
            // Apply context adjustments
            score = adjustScoreForContext(def.symbol, score);
            
            // Apply priority boost
            score *= def.priority;
            
            if (score > bestScore)
            {
                bestScore = score;
                bestRoot = rootPC;
                bestChordIndex = chordIndex;
            }
        }
    }
    
    // Check if best match meets minimum threshold
    if (bestScore < minimumScore_ || bestChordIndex < 0)
    {
        return result;
    }
    
    // Build result
    result.isValid = true;
    result.rootNote = bestRoot;
    result.bassNote = bassPC;
    result.chordTypeIndex = bestChordIndex;
    result.score = bestScore;
    result.confidence = std::min(bestScore, 1.0f);
    result.timestamp = currentTimeMs;
    
    // Calculate bass interval from root
    result.bassIntervalFromRoot = (bassPC - bestRoot + 12) % 12;
    
    // Determine inversion
    result.inversionType = determineInversion(
        result.bassIntervalFromRoot,
        CHORD_DEFINITIONS[bestChordIndex]
    );
    
    // Fill in note count info
    result.activeNoteCount = static_cast<int8_t>(noteCount);
    result.lowestMidiNote = static_cast<int8_t>(static_cast<uint8_t>(noteBuffer_[0]));
    result.highestMidiNote = static_cast<int8_t>(static_cast<uint8_t>(noteBuffer_[noteCount - 1]));
    result.bassNoteCount = static_cast<int8_t>(voices.bassCount);
    result.chordNoteCount = static_cast<int8_t>(voices.chordCount);
    result.melodyNoteCount = static_cast<int8_t>(voices.melodyCount);
    
    return result;
}

void ChordDetector::separateVoices(
    const int* midiNotes,
    int noteCount,
    VoiceBuckets& outVoices
) const
{
    outVoices.clear();
    
    for (int i = 0; i < noteCount; ++i)
    {
        int note = midiNotes[i];
        
        if (note < BASS_MAX_NOTE)
        {
            // Bass register
            if (outVoices.bassCount < 8)
                outVoices.bassNotes[outVoices.bassCount++] = static_cast<int8_t>(note);
        }
        else if (note >= MELODY_MIN_NOTE)
        {
            // Melody register (ignored for chord detection)
            if (outVoices.melodyCount < 8)
                outVoices.melodyNotes[outVoices.melodyCount++] = static_cast<int8_t>(note);
        }
        else
        {
            // Chord tone register
            if (outVoices.chordCount < 16)
                outVoices.chordNotes[outVoices.chordCount++] = static_cast<int8_t>(note);
        }
    }
}

std::bitset<12> ChordDetector::getPitchClassSet(
    const int* midiNotes,
    int noteCount
) const
{
    std::bitset<12> pcs;
    pcs.reset();
    
    for (int i = 0; i < noteCount; ++i)
    {
        int pc = midiNotes[i] % 12;
        pcs[pc] = true;
    }
    
    return pcs;
}

float ChordDetector::scoreChordCandidate(
    const std::bitset<12>& playedPCs,
    const ChordDefinition& def
) const
{
    // Count matched core intervals
    float coreScore = 0.0f;
    float totalCoreWeight = 0.0f;
    int matchedCoreCount = 0;
    
    for (int i = 0; i < def.coreCount; ++i)
    {
        int interval = def.coreIntervals[i];
        if (interval < 0) break;
        
        float weight = INTERVAL_WEIGHTS[interval % 12];
        totalCoreWeight += weight;
        
        if (playedPCs[interval])
        {
            coreScore += weight;
            ++matchedCoreCount;
        }
    }
    
    // Require all core intervals for complex chords
    if (def.coreCount > 3)
    {
        bool allCoreMatched = true;
        for (int i = 0; i < def.coreCount; ++i)
        {
            int interval = def.coreIntervals[i];
            if (interval < 0) break;
            if (!playedPCs[interval])
            {
                allCoreMatched = false;
                break;
            }
        }
        
        if (!allCoreMatched)
            return 0.0f;
    }
    
    // Base score from core matches
    float baseScore = (totalCoreWeight > 0.0f) ? (coreScore / totalCoreWeight) : 0.0f;
    
    // Bonus for optional intervals
    float optionalScore = 0.0f;
    for (int i = 0; i < def.optionalCount; ++i)
    {
        int interval = def.optionalIntervals[i];
        if (interval < 0) break;
        
        if (playedPCs[interval])
        {
            float weight = INTERVAL_WEIGHTS[interval % 12];
            optionalScore += weight;
        }
    }
    
    if (def.optionalCount > 0 && totalCoreWeight > 0.0f)
    {
        baseScore += (optionalScore / totalCoreWeight) * OPTIONAL_INTERVAL_WEIGHT_FACTOR;
    }
    
    // Penalty for extra notes (not in core or optional)
    int extraNoteCount = 0;
    for (int pc = 0; pc < 12; ++pc)
    {
        if (!playedPCs[pc]) continue;
        
        bool isCore = false;
        bool isOptional = false;
        
        for (int i = 0; i < def.coreCount && def.coreIntervals[i] >= 0; ++i)
        {
            if (def.coreIntervals[i] == pc)
            {
                isCore = true;
                break;
            }
        }
        
        if (!isCore)
        {
            for (int i = 0; i < def.optionalCount && def.optionalIntervals[i] >= 0; ++i)
            {
                if (def.optionalIntervals[i] == pc)
                {
                    isOptional = true;
                    break;
                }
            }
        }
        
        if (!isCore && !isOptional)
            ++extraNoteCount;
    }
    
    float penalty = std::min(
        static_cast<float>(extraNoteCount) * EXTRA_NOTE_PENALTY_PER_NOTE,
        MAX_EXTRA_NOTE_PENALTY
    );
    
    float finalScore = std::max(baseScore - penalty, 0.0f);
    
    return finalScore;
}

int8_t ChordDetector::determineInversion(
    int bassIntervalFromRoot,
    const ChordDefinition& def
) const
{
    if (bassIntervalFromRoot == 0)
        return 0; // Root position
    
    // Check if bass is in core intervals
    for (int i = 0; i < def.coreCount; ++i)
    {
        if (def.coreIntervals[i] < 0) break;
        
        if (def.coreIntervals[i] == bassIntervalFromRoot)
        {
            // Found in core - determine inversion number
            return static_cast<int8_t>(i);
        }
    }
    
    // Bass is not a core tone - it's a slash chord
    return -1;
}

float ChordDetector::adjustScoreForContext(
    const char* chordType,
    float baseScore
) const
{
    // Boost dominant 7th chords (common in jazz)
    if (strcmp(chordType, "7") == 0 ||
        strcmp(chordType, "9") == 0 ||
        strcmp(chordType, "13") == 0 ||
        strcmp(chordType, "7b9") == 0 ||
        strcmp(chordType, "7#9") == 0)
    {
        return baseScore * 1.1f;
    }
    
    return baseScore;
}

} // namespace ChordDetection
