#include "HarmonicMemory.h"
#include <cstring>
#include <algorithm>

namespace ChordDetection
{

HarmonicMemory::HarmonicMemory()
    : writeIndex_(0)
    , frameCount_(0)
    , memoryWindowMs_(DEFAULT_MEMORY_WINDOW_MS)
    , decayHalfLifeMs_(DEFAULT_DECAY_HALF_LIFE_MS)
{
    for (auto& frame : frames_)
        frame.clear();
}

void HarmonicMemory::addFrame(
    double currentTimeMs,
    const ChordHypothesis* candidates,
    int candidateCount
)
{
    // First, prune old frames
    pruneOldFrames(currentTimeMs);
    
    // Don't add empty frames
    if (candidateCount <= 0 || candidates == nullptr)
        return;
    
    // Write to next slot
    HarmonicFrame& frame = frames_[writeIndex_];
    frame.timestamp = currentTimeMs;
    frame.candidateCount = std::min(candidateCount, MAX_CANDIDATES_PER_FRAME);
    
    for (int i = 0; i < frame.candidateCount; ++i)
    {
        frame.candidates[i] = candidates[i];
    }
    
    // Advance write index (circular)
    writeIndex_ = (writeIndex_ + 1) % MAX_HARMONIC_FRAMES;
    
    if (frameCount_ < MAX_HARMONIC_FRAMES)
        ++frameCount_;
}

void HarmonicMemory::pruneOldFrames(double currentTimeMs)
{
    // Mark frames outside window as empty
    for (int i = 0; i < frameCount_; ++i)
    {
        HarmonicFrame& frame = frames_[i];
        if (frame.candidateCount > 0)
        {
            if (currentTimeMs - frame.timestamp > memoryWindowMs_)
            {
                frame.clear();
            }
        }
    }
    
    // Recount active frames
    int activeCount = 0;
    for (int i = 0; i < MAX_HARMONIC_FRAMES; ++i)
    {
        if (frames_[i].candidateCount > 0)
            ++activeCount;
    }
    frameCount_ = activeCount;
}

int HarmonicMemory::getReinforcedCandidates(ReinforcedCandidate* outCandidates) const
{
    int candidateCount = 0;
    
    // Get current time from newest frame
    double currentTime = getNewestFrameTime();
    if (currentTime <= 0.0)
        return 0;
    
    // Process all frames
    for (int fIdx = 0; fIdx < MAX_HARMONIC_FRAMES; ++fIdx)
    {
        const HarmonicFrame& frame = frames_[fIdx];
        if (frame.isEmpty())
            continue;
        
        // Calculate decay weight for this frame
        float weight = calculateDecayWeight(frame.timestamp, currentTime);
        
        // Process each candidate in the frame
        for (int cIdx = 0; cIdx < frame.candidateCount; ++cIdx)
        {
            const ChordHypothesis& hyp = frame.candidates[cIdx];
            if (!hyp.isValid())
                continue;
            
            // Find or create entry for this chord
            int idx = findOrAddCandidate(hyp, outCandidates, candidateCount);
            if (idx < 0)
                continue;  // Max candidates reached
            
            ReinforcedCandidate& rc = outCandidates[idx];
            
            // Add weighted score
            rc.reinforcementScore += hyp.baseScore * weight;
            rc.frameCount++;
            
            // Update timing
            if (rc.firstSeenTime == 0.0 || frame.timestamp < rc.firstSeenTime)
                rc.firstSeenTime = frame.timestamp;
            if (frame.timestamp > rc.lastSeenTime)
                rc.lastSeenTime = frame.timestamp;
        }
    }
    
    // Sort by reinforcement score (descending)
    std::sort(outCandidates, outCandidates + candidateCount,
        [](const ReinforcedCandidate& a, const ReinforcedCandidate& b) {
            return a.reinforcementScore > b.reinforcementScore;
        });
    
    return candidateCount;
}

ChordHypothesis HarmonicMemory::getMostStableCandidate() const
{
    std::array<ReinforcedCandidate, MAX_REINFORCED_CANDIDATES> candidates;
    int count = getReinforcedCandidates(candidates.data());
    
    if (count == 0)
        return ChordHypothesis();
    
    // Find candidate with best combination of reinforcement and stability
    const ReinforcedCandidate* best = &candidates[0];
    float bestCombinedScore = 0.0f;
    
    for (int i = 0; i < count; ++i)
    {
        const ReinforcedCandidate& rc = candidates[i];
        
        // Combined score: weighted sum of reinforcement and stability
        float stabilityFactor = std::min(1.0f, rc.getStability() / 10.0f);
        float combinedScore = rc.reinforcementScore * 0.7f + stabilityFactor * 0.3f;
        
        if (combinedScore > bestCombinedScore)
        {
            bestCombinedScore = combinedScore;
            best = &rc;
        }
    }
    
    return best->hypothesis;
}

void HarmonicMemory::clear()
{
    for (auto& frame : frames_)
        frame.clear();
    
    writeIndex_ = 0;
    frameCount_ = 0;
}

int HarmonicMemory::getFrameCount() const
{
    int count = 0;
    for (int i = 0; i < MAX_HARMONIC_FRAMES; ++i)
    {
        if (frames_[i].candidateCount > 0)
            ++count;
    }
    return count;
}

double HarmonicMemory::getOldestFrameTime() const
{
    double oldest = 0.0;
    for (int i = 0; i < MAX_HARMONIC_FRAMES; ++i)
    {
        if (frames_[i].candidateCount > 0)
        {
            if (oldest == 0.0 || frames_[i].timestamp < oldest)
                oldest = frames_[i].timestamp;
        }
    }
    return oldest;
}

double HarmonicMemory::getNewestFrameTime() const
{
    double newest = 0.0;
    for (int i = 0; i < MAX_HARMONIC_FRAMES; ++i)
    {
        if (frames_[i].candidateCount > 0)
        {
            if (frames_[i].timestamp > newest)
                newest = frames_[i].timestamp;
        }
    }
    return newest;
}

float HarmonicMemory::calculateDecayWeight(double frameTime, double currentTime) const
{
    double deltaT = currentTime - frameTime;
    if (deltaT <= 0.0)
        return 1.0f;
    
    // Exponential decay: weight = exp(-Δt / halfLife)
    // At halfLife, weight = 0.5
    // At 2*halfLife, weight = 0.25
    double exponent = -deltaT / decayHalfLifeMs_;
    return static_cast<float>(std::exp(exponent));
}

int HarmonicMemory::findOrAddCandidate(
    const ChordHypothesis& hyp,
    ReinforcedCandidate* candidates,
    int& count
) const
{
    // Look for existing entry with same chord
    for (int i = 0; i < count; ++i)
    {
        if (candidates[i].hypothesis.isSameChord(hyp))
            return i;
    }
    
    // Not found - add new entry if space available
    if (count >= MAX_REINFORCED_CANDIDATES)
        return -1;
    
    candidates[count].hypothesis = hyp;
    candidates[count].reinforcementScore = 0.0f;
    candidates[count].frameCount = 0;
    candidates[count].firstSeenTime = 0.0;
    candidates[count].lastSeenTime = 0.0;
    
    return count++;
}

} // namespace ChordDetection
