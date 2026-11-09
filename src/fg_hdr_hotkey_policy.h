#pragma once

namespace control_fg_hdr_hotkey {

enum class Stage : unsigned int {
    Idle = 0,
    InterceptedWaitOffPresent = 1,
    OffQueuedWaitPresentReturn = 2,
    OffCommittedWaitKeyRelease = 3,
    ReplayedWaitHdrTransition = 4,
};

enum class ReplayMode : unsigned int {
    None = 0,
    BOnlyModifiersStillDown = 1,
    FullChordAfterRelease = 2,
};

inline bool ShouldHoldFG(Stage stage) noexcept {
    return stage != Stage::Idle;
}

inline bool ReadyToReplay(Stage stage) noexcept {
    return stage == Stage::OffCommittedWaitKeyRelease;
}

inline ReplayMode ChooseReplayMode(Stage stage, bool bDown, bool winDown, bool altDown) noexcept {
    if (!ReadyToReplay(stage) || bDown) return ReplayMode::None;
    if (winDown && altDown) return ReplayMode::BOnlyModifiersStillDown;
    if (!winDown && !altDown) return ReplayMode::FullChordAfterRelease;
    return ReplayMode::None;
}

inline bool BridgeCanTakeOwnership(Stage stage,
                                   unsigned long long interceptedGeneration,
                                   unsigned long long currentGeneration) noexcept {
    return stage == Stage::ReplayedWaitHdrTransition &&
           currentGeneration != interceptedGeneration;
}

} // namespace control_fg_hdr_hotkey
