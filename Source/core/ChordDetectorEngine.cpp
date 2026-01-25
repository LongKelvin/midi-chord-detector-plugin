#include "ChordDetectorEngine.h"
#include <cmath>

namespace ChordDetection
{

ChordDetectorEngine::ChordDetectorEngine()
    : lastProcessTime_(0.0)
    , minimumNotes_(2)
{
}

void ChordDetectorEngine::noteOn(int noteNumber, float velocity, double currentTimeMs)
{
    noteState_.noteOn(noteNumber, velocity, currentTimeMs);
}

void ChordDetectorEngine::noteOff(int noteNumber, double currentTimeMs)
{
    (void)currentTimeMs;  // Timestamp not needed for note-off
    noteState_.noteOff(noteNumber);
}

void ChordDetectorEngine::setSustainPedal(bool isPressed, double currentTimeMs)
{
    (void)currentTimeMs;
    noteState_.setSustainPedal(isPressed);
}

void ChordDetectorEngine::allNotesOff(double currentTimeMs)
{
    (void)currentTimeMs;
    noteState_.allNotesOff();
    harmonicMemory_.clear();
    chordResolver_.reset();
    currentChord_ = ResolvedChord();
}

ResolvedChord ChordDetectorEngine::detectChord(double currentTimeMs)
{
    lastProcessTime_ = currentTimeMs;
    
    // Check if we have enough notes
    int noteCount = noteState_.getActiveNoteCount();
    if (noteCount < minimumNotes_)
    {
        // Not enough notes - clear chord
        currentChord_ = ResolvedChord();
        currentChord_.timestamp = currentTimeMs;
        currentChord_.buildDisplayName();
        return currentChord_;
    }
    
    // Step 1: Generate candidates from current note state
    int candidateCount = candidateGenerator_.generateCandidatesFromNoteState(
        noteState_,
        currentTimeMs,
        candidateBuffer_.data()
    );
    
    // Step 2: Add candidates to harmonic memory
    harmonicMemory_.addFrame(currentTimeMs, candidateBuffer_.data(), candidateCount);
    
    // Step 3: Get temporally reinforced candidates
    int reinforcedCount = harmonicMemory_.getReinforcedCandidates(reinforcedBuffer_.data());
    
    if (reinforcedCount == 0)
    {
        // No reinforced candidates - might be too early
        // Use the best raw candidate
        if (candidateCount > 0)
        {
            // Convert single candidate to reinforced format
            reinforcedBuffer_[0].hypothesis = candidateBuffer_[0];
            reinforcedBuffer_[0].reinforcementScore = candidateBuffer_[0].baseScore;
            reinforcedBuffer_[0].frameCount = 1;
            reinforcedBuffer_[0].firstSeenTime = currentTimeMs;
            reinforcedBuffer_[0].lastSeenTime = currentTimeMs;
            reinforcedCount = 1;
        }
        else
        {
            currentChord_ = ResolvedChord();
            currentChord_.timestamp = currentTimeMs;
            currentChord_.buildDisplayName();
            return currentChord_;
        }
    }
    
    // Step 4: Build scoring context
    ScoringContext context = buildScoringContext(currentTimeMs);
    
    // Step 5: Score candidates
    int scoredCount = confidenceScorer_.scoreAllCandidates(
        reinforcedBuffer_.data(),
        reinforcedCount,
        context,
        scoredBuffer_.data()
    );
    
    // Step 6: Resolve final chord
    currentChord_ = chordResolver_.resolve(
        scoredBuffer_.data(),
        scoredCount,
        context,
        currentTimeMs
    );
    
    return currentChord_;
}

void ChordDetectorEngine::reset()
{
    noteState_.allNotesOff();
    harmonicMemory_.clear();
    chordResolver_.reset();
    currentChord_ = ResolvedChord();
    lastProcessTime_ = 0.0;
}

void ChordDetectorEngine::setConfig(const EngineConfig& config)
{
    minimumNotes_ = config.minimumNotes;
    
    harmonicMemory_.setMemoryWindowMs(config.memoryWindowMs);
    harmonicMemory_.setDecayHalfLifeMs(config.decayHalfLifeMs);
    
    confidenceScorer_.setMinimumConfidence(config.minimumConfidence);
    
    ResolutionConfig resConfig = chordResolver_.getConfig();
    resConfig.minimumConfidence = config.minimumConfidence;
    resConfig.allowSlashChords = config.allowSlashChords;
    chordResolver_.setConfig(resConfig);
    
    candidateGenerator_.setMinimumNoteCount(config.minimumNotes);
}

EngineConfig ChordDetectorEngine::getConfig() const
{
    EngineConfig config;
    config.minimumNotes = minimumNotes_;
    config.memoryWindowMs = harmonicMemory_.getMemoryWindowMs();
    config.decayHalfLifeMs = harmonicMemory_.getDecayHalfLifeMs();
    config.minimumConfidence = confidenceScorer_.getMinimumConfidence();
    config.allowSlashChords = chordResolver_.getConfig().allowSlashChords;
    return config;
}

ScoringContext ChordDetectorEngine::buildScoringContext(double currentTimeMs) const
{
    ScoringContext context;
    context.currentTimeMs = currentTimeMs;
    
    // Get bass pitch class
    int lowestNote = noteState_.getLowestNote();
    context.bassPitchClass = (lowestNote >= 0) ? (lowestNote % 12) : -1;
    
    // Count notes
    context.totalNoteCount = noteState_.getActiveNoteCount();
    
    // Calculate average velocity
    if (context.totalNoteCount > 0)
    {
        float totalVelocity = 0.0f;
        std::array<ActiveNote, 32> notes;
        int count = noteState_.getActiveNotesDetailed(notes.data(), 32);
        
        for (int i = 0; i < count; ++i)
        {
            totalVelocity += notes[i].velocity;
        }
        context.averageVelocity = totalVelocity / static_cast<float>(count);
    }
    else
    {
        context.averageVelocity = 0.5f;
    }
    
    // Previous chord info
    const ResolvedChord& prev = chordResolver_.getPreviousChord();
    if (prev.isValid())
    {
        context.previousChord = prev.chord;
        context.hasPreviousChord = true;
    }
    
    return context;
}

} // namespace ChordDetection
