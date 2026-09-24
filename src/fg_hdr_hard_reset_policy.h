#pragma once

// Portable policy for r31's HDR/SDR DLSS-G hard reset. The runtime owns the
// Windows/Streamline calls; this header keeps the state-transition rules testable.
namespace control_fg_hdr_hard_reset {

enum class Stage : unsigned int {
    Idle = 0,
    OffQueuedWaitPresent = 1,
    ResourcesFreedWaitBridge = 2,
};

static constexpr unsigned long long kNoBridgeGracePresents = 30ull;
static constexpr unsigned int kNoBridgeFreshFramesRequired = 2u;

inline bool Pending(Stage stage) noexcept {
    return stage != Stage::Idle;
}

inline bool CanFreeAfterPresent(Stage stage, bool apiOff, bool presentSucceeded) noexcept {
    return stage == Stage::OffQueuedWaitPresent && apiOff && presentSucceeded;
}

inline bool CanRearm(Stage stage,
                     unsigned long long resetBridgeGeneration,
                     unsigned long long currentBridgeGeneration,
                     unsigned long long settledBridgeGeneration,
                     bool displayTransitionPending) noexcept {
    return stage == Stage::ResourcesFreedWaitBridge &&
           currentBridgeGeneration != resetBridgeGeneration &&
           settledBridgeGeneration == currentBridgeGeneration &&
           !displayTransitionPending;
}

inline unsigned long long NoBridgeGraceAnchor(unsigned long long freePresent,
                                              unsigned long long displayChangePresent) noexcept {
    return freePresent > displayChangePresent ? freePresent : displayChangePresent;
}

inline bool NoBridgeGraceElapsed(unsigned long long present,
                                 unsigned long long freePresent,
                                 unsigned long long displayChangePresent) noexcept {
    const unsigned long long anchor = NoBridgeGraceAnchor(freePresent, displayChangePresent);
    return anchor != 0ull && present >= anchor &&
           (present - anchor) >= kNoBridgeGracePresents;
}

inline unsigned int NextNoBridgeFreshFrameCount(Stage stage,
                                                unsigned long long resetBridgeGeneration,
                                                unsigned long long currentBridgeGeneration,
                                                bool displayTransitionPending,
                                                bool inputsReady,
                                                unsigned long long present,
                                                unsigned long long freePresent,
                                                unsigned long long displayChangePresent,
                                                unsigned long long lastFreshPresent,
                                                unsigned int currentCount) noexcept {
    if (stage != Stage::ResourcesFreedWaitBridge ||
        currentBridgeGeneration != resetBridgeGeneration ||
        !displayTransitionPending ||
        !inputsReady ||
        !NoBridgeGraceElapsed(present, freePresent, displayChangePresent)) {
        return 0u;
    }
    if (currentCount == 0u || present != lastFreshPresent + 1ull) return 1u;
    return currentCount < kNoBridgeFreshFramesRequired ? currentCount + 1u : currentCount;
}

inline bool CanRearmWithoutBridge(Stage stage,
                                  unsigned long long resetBridgeGeneration,
                                  unsigned long long currentBridgeGeneration,
                                  bool displayTransitionPending,
                                  unsigned long long present,
                                  unsigned long long freePresent,
                                  unsigned long long displayChangePresent,
                                  unsigned int freshFrames) noexcept {
    return stage == Stage::ResourcesFreedWaitBridge &&
           currentBridgeGeneration == resetBridgeGeneration &&
           displayTransitionPending &&
           NoBridgeGraceElapsed(present, freePresent, displayChangePresent) &&
           freshFrames >= kNoBridgeFreshFramesRequired;
}

} // namespace control_fg_hdr_hard_reset
