// ChordTypes.h
// Core types, enums, and constants for chord detection
//
// This header contains fundamental definitions used throughout
// the chord detection system.

#pragma once

#include <cstdint>

namespace ChordDetection {

// ============================================================================
// CONSTANTS
// ============================================================================

inline constexpr int kPitchClassCount = 12;
inline constexpr int kMaxMidiNote = 127;
inline constexpr int kMinMidiNote = 0;
inline constexpr int kMaxIntervals = 24;
inline constexpr int kDefaultMinimumNotes = 2;
inline constexpr float kMinimumScoreThreshold = 80.0f;
inline constexpr size_t kMaxChordHistorySize = 10;

// MIDI constants
inline constexpr int kMiddleC = 60;  // C4
inline constexpr int kOctaveSize = 12;

// ============================================================================
// ENUMS
// ============================================================================

/**
 * Voicing type classification for chord analysis.
 * Describes how chord tones are distributed across the pitch range.
 */
enum class VoicingType : uint8_t {
    Close,     // All notes within one octave
    Open,      // Notes spread beyond one octave
    Rootless,  // Root omitted (common in jazz)
    Drop2,     // 2nd voice from top dropped an octave
    Drop3,     // 3rd voice from top dropped an octave
    Unknown    // Unable to classify
};

/**
 * Slash chord notation mode.
 * Controls how inversions and bass notes are displayed.
 */
enum class SlashChordMode : uint8_t {
    Auto,    // Smart detection: show slash for non-chord tones in bass
    Always,  // Always show /bass notation
    Never    // Use traditional inversion names only
};

// ============================================================================
// BACKWARD COMPATIBILITY ALIASES (deprecated - use enum class values)
// ============================================================================

[[deprecated("Use VoicingType::Close")]]
inline constexpr VoicingType CLOSE = VoicingType::Close;

[[deprecated("Use VoicingType::Open")]]
inline constexpr VoicingType OPEN = VoicingType::Open;

[[deprecated("Use VoicingType::Rootless")]]
inline constexpr VoicingType ROOTLESS = VoicingType::Rootless;

[[deprecated("Use VoicingType::Drop2")]]
inline constexpr VoicingType DROP_2 = VoicingType::Drop2;

[[deprecated("Use VoicingType::Drop3")]]
inline constexpr VoicingType DROP_3 = VoicingType::Drop3;

[[deprecated("Use VoicingType::Unknown")]]
inline constexpr VoicingType UNKNOWN = VoicingType::Unknown;

[[deprecated("Use SlashChordMode::Auto")]]
inline constexpr SlashChordMode AUTO = SlashChordMode::Auto;

[[deprecated("Use SlashChordMode::Always")]]
inline constexpr SlashChordMode ALWAYS = SlashChordMode::Always;

[[deprecated("Use SlashChordMode::Never")]]
inline constexpr SlashChordMode NEVER = SlashChordMode::Never;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Convert VoicingType to string for display.
 */
[[nodiscard]] inline const char* voicingTypeToString(VoicingType type) noexcept {
    switch (type) {
        case VoicingType::Close:    return "close";
        case VoicingType::Open:     return "open";
        case VoicingType::Rootless: return "rootless";
        case VoicingType::Drop2:    return "drop_2";
        case VoicingType::Drop3:    return "drop_3";
        default:                    return "unknown";
    }
}

/**
 * Convert SlashChordMode to string for display.
 */
[[nodiscard]] inline const char* slashChordModeToString(SlashChordMode mode) noexcept {
    switch (mode) {
        case SlashChordMode::Auto:   return "auto";
        case SlashChordMode::Always: return "always";
        case SlashChordMode::Never:  return "never";
        default:                     return "unknown";
    }
}

} // namespace ChordDetection
