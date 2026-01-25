/*
  ==============================================================================

    ChromaVector.h
    
    Weighted Chroma Vector for H-WCTM (Hybrid Weighted Chroma-Template Matching)
    
    Based on research: "Weighted Chroma Template Matching with Bass Prioritization"
    
    Key concepts:
    - 12-dimensional float vector representing pitch class energy
    - Bass boosting (lowest note × 3.0)
    - Register weighting (melody attenuation)
    - Velocity scaling
    - Decay for sustained notes

  ==============================================================================
*/

#pragma once

#include <array>
#include <cmath>
#include <algorithm>

namespace ChordDetection
{

// ============================================================================
// Weighting Constants (as per research)
// ============================================================================

struct ChromaWeights
{
    // Bass boost for lowest note in voicing
    float bassBoost = 3.0f;
    
    // Register-based attenuation
    // Notes below middle C (60) = full weight (harmony region)
    int harmonyUpperBound = 60;
    float harmonyWeight = 1.0f;
    
    // Notes 60-72 = mid weight (chord voicing region)
    int midUpperBound = 72;
    float midWeight = 1.0f;
    
    // Notes 72-84 = reduced weight (melody region)
    int melodyUpperBound = 84;
    float melodyWeight = 0.5f;
    
    // Notes above 84 = very low weight (sparkle/ornaments)
    float highWeight = 0.2f;
    
    // Sustained note decay (weight multiplier per 100ms)
    float sustainDecayRate = 0.85f;
    
    // Minimum weight after decay
    float minSustainWeight = 0.2f;
};

// Default weights from research
constexpr ChromaWeights DEFAULT_CHROMA_WEIGHTS = {};

// ============================================================================
// ChromaVector - Weighted 12-bin pitch class profile
// ============================================================================

class ChromaVector
{
public:
    ChromaVector()
    {
        clear();
    }
    
    /**
     * Clear all bins to zero
     */
    void clear()
    {
        bins_.fill(0.0f);
        totalEnergy_ = 0.0f;
    }
    
    /**
     * Add a note to the chroma vector with weighting
     * 
     * @param midiNote MIDI note number (0-127)
     * @param velocity Velocity (0.0 - 1.0)
     * @param isLowestNote Whether this is the bass note
     * @param weights Weighting configuration
     * @param sustainAge Age of sustained note in ms (0 = just played)
     */
    void addNote(int midiNote, float velocity, bool isLowestNote,
                 const ChromaWeights& weights = DEFAULT_CHROMA_WEIGHTS,
                 double sustainAge = 0.0)
    {
        if (midiNote < 0 || midiNote > 127)
            return;
        
        int pitchClass = midiNote % 12;
        
        // Start with velocity-based weight
        float weight = velocity;
        
        // Apply register weighting
        if (midiNote < weights.harmonyUpperBound)
        {
            weight *= weights.harmonyWeight;
        }
        else if (midiNote < weights.midUpperBound)
        {
            weight *= weights.midWeight;
        }
        else if (midiNote < weights.melodyUpperBound)
        {
            weight *= weights.melodyWeight;
        }
        else
        {
            weight *= weights.highWeight;
        }
        
        // Apply bass boost
        if (isLowestNote)
        {
            weight *= weights.bassBoost;
        }
        
        // Apply sustain decay if note is sustained
        if (sustainAge > 0.0)
        {
            // Exponential decay every 100ms
            int decaySteps = static_cast<int>(sustainAge / 100.0);
            float decayMultiplier = std::pow(weights.sustainDecayRate, static_cast<float>(decaySteps));
            decayMultiplier = std::max(decayMultiplier, weights.minSustainWeight);
            weight *= decayMultiplier;
        }
        
        // Accumulate into bin
        bins_[pitchClass] += weight;
        totalEnergy_ += weight;
    }
    
    /**
     * Get the energy at a specific pitch class
     */
    float operator[](int pitchClass) const
    {
        if (pitchClass < 0 || pitchClass >= 12)
            return 0.0f;
        return bins_[pitchClass];
    }
    
    /**
     * Get the raw bins array
     */
    const std::array<float, 12>& getBins() const { return bins_; }
    
    /**
     * Get total energy (sum of all bins)
     */
    float getTotalEnergy() const { return totalEnergy_; }
    
    /**
     * Get the magnitude (L2 norm) of the vector
     */
    float getMagnitude() const
    {
        float sumSquares = 0.0f;
        for (int i = 0; i < 12; ++i)
        {
            sumSquares += bins_[i] * bins_[i];
        }
        return std::sqrt(sumSquares);
    }
    
    /**
     * Normalize the vector to unit magnitude
     */
    void normalize()
    {
        float mag = getMagnitude();
        if (mag > 0.0001f)
        {
            for (int i = 0; i < 12; ++i)
            {
                bins_[i] /= mag;
            }
            totalEnergy_ = 1.0f;
        }
    }
    
    /**
     * Get a shifted version of this vector (for root transposition)
     * Shifts so that pitch class 'root' becomes index 0
     * 
     * Example: If C-E-G is in bins[0,4,7] and root=4 (E),
     * result will have E at index 0, so intervals are relative to E.
     */
    ChromaVector shifted(int root) const
    {
        ChromaVector result;
        for (int i = 0; i < 12; ++i)
        {
            // For interval i relative to root, get the absolute pitch class
            int srcIdx = (i + root) % 12;
            result.bins_[i] = bins_[srcIdx];
        }
        result.totalEnergy_ = totalEnergy_;
        return result;
    }
    
    /**
     * Calculate cosine similarity with another vector
     * Returns value between 0.0 (orthogonal) and 1.0 (identical direction)
     */
    float cosineSimilarity(const ChromaVector& other) const
    {
        float dot = 0.0f;
        float magA = 0.0f;
        float magB = 0.0f;
        
        for (int i = 0; i < 12; ++i)
        {
            dot += bins_[i] * other.bins_[i];
            magA += bins_[i] * bins_[i];
            magB += other.bins_[i] * other.bins_[i];
        }
        
        magA = std::sqrt(magA);
        magB = std::sqrt(magB);
        
        if (magA < 0.0001f || magB < 0.0001f)
            return 0.0f;
        
        return dot / (magA * magB);
    }
    
    /**
     * Calculate cosine similarity with a raw float array template
     */
    float cosineSimilarityWithTemplate(const float* templateBins) const
    {
        float dot = 0.0f;
        float magA = 0.0f;
        float magB = 0.0f;
        
        for (int i = 0; i < 12; ++i)
        {
            dot += bins_[i] * templateBins[i];
            magA += bins_[i] * bins_[i];
            magB += templateBins[i] * templateBins[i];
        }
        
        magA = std::sqrt(magA);
        magB = std::sqrt(magB);
        
        if (magA < 0.0001f || magB < 0.0001f)
            return 0.0f;
        
        return dot / (magA * magB);
    }
    
    /**
     * Check if vector is empty (no energy)
     */
    bool isEmpty() const
    {
        return totalEnergy_ < 0.0001f;
    }
    
    /**
     * Get the pitch class with maximum energy
     */
    int getMaxBin() const
    {
        int maxIdx = 0;
        float maxVal = bins_[0];
        for (int i = 1; i < 12; ++i)
        {
            if (bins_[i] > maxVal)
            {
                maxVal = bins_[i];
                maxIdx = i;
            }
        }
        return maxIdx;
    }
    
    /**
     * Count how many bins have non-zero energy
     */
    int countActiveBins() const
    {
        int count = 0;
        for (int i = 0; i < 12; ++i)
        {
            if (bins_[i] > 0.0001f)
                ++count;
        }
        return count;
    }

private:
    std::array<float, 12> bins_;
    float totalEnergy_;
};

} // namespace ChordDetection
