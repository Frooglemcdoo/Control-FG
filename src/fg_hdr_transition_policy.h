#pragma once
#include <cstdint>

namespace control_fg_hdr_transition {
static constexpr std::uint32_t kFreshFramesRequired = 2;


inline bool DisplayDomainFlip(bool baselineKnown, bool previousHdr, bool currentHdr) noexcept {
    return baselineKnown && previousHdr != currentHdr;
}


inline bool BridgeAlreadyOwnsObservedDisplayFlip(std::uint64_t observedBridgeGeneration,
                                                 std::uint64_t currentBridgeGeneration,
                                                 bool bridgeHdrActive,
                                                 bool observedDisplayHdrActive) noexcept {
    // If the bridge generation advanced since the display domain was last observed
    // and the bridge now already matches the newly observed domain, ResizeBuffers/
    // bridge handling got there first. Treat the output flip as a late duplicate.
    return currentBridgeGeneration != observedBridgeGeneration &&
           bridgeHdrActive == observedDisplayHdrActive;
}

inline bool HoldForPreResizeDisplayTransition(bool pending,
                                               std::uint64_t currentBridgeGeneration,
                                               std::uint64_t detectedBridgeGeneration) noexcept {
    return pending && currentBridgeGeneration == detectedBridgeGeneration;
}

inline bool BridgeTransitionObserved(bool pending,
                                     std::uint64_t currentBridgeGeneration,
                                     std::uint64_t detectedBridgeGeneration) noexcept {
    return pending && currentBridgeGeneration != detectedBridgeGeneration;
}

inline bool NeedsCommittedOffPresent(bool fgWasActive) noexcept {
    return fgWasActive;
}

inline bool QuiesceCommitted(bool fgWasActive, bool offQueued, bool realPresentSucceeded) noexcept {
    return !fgWasActive || (offQueued && realPresentSucceeded);
}

inline bool TransitionPending(std::uint64_t transitionGeneration, std::uint64_t settledGeneration) noexcept {
    return transitionGeneration != settledGeneration;
}

inline std::uint32_t NextFreshFrameCount(bool inputsReady,
                                         std::uint64_t transitionGeneration,
                                         std::uint64_t observedGeneration,
                                         std::uint64_t present,
                                         std::uint64_t lastPresent,
                                         std::uint32_t currentCount) noexcept {
    if (!inputsReady || present == 0) return 0;
    if (transitionGeneration != observedGeneration || currentCount == 0 || present != lastPresent + 1) return 1;
    return currentCount < kFreshFramesRequired ? currentCount + 1 : currentCount;
}

inline bool WarmupComplete(std::uint32_t freshFrameCount) noexcept {
    return freshFrameCount >= kFreshFramesRequired;
}

inline bool CanEnable(bool requested,
                      std::uint64_t transitionGeneration,
                      std::uint64_t settledGeneration) noexcept {
    return requested && !TransitionPending(transitionGeneration, settledGeneration);
}
}
