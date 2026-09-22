#pragma once
#include <cstdint>

namespace hdrbutton {

static constexpr uint64_t kTransitionTimeoutMs = 60000;
static constexpr uint64_t kFailureRecoveryDelayMs = 1000;
static constexpr uint64_t kFreshDomainFramesRequired = 3;

enum class Phase : uint32_t {
    Idle,
    RequestOff,
    WaitOffCommit,
    SetSystemHdr,
    WaitSystemHdr,
    SetGameHdr,
    WaitDomain,
    WaitFreshDomain,
    RestoreFg,
    WaitFgResume,
    Complete,
    Failed
};

inline const wchar_t* ButtonText(Phase phase, bool hdr, bool supported) noexcept {
    if (!supported) return L"HDR N/A";
    if (phase != Phase::Idle && phase != Phase::Complete && phase != Phase::Failed) return L"HDR...";
    if (phase == Phase::Failed) return L"HDR ERR";
    return hdr ? L"HDR ON" : L"HDR OFF";
}

inline const char* PhaseName(Phase p) noexcept {
    switch (p) {
    case Phase::Idle: return "idle";
    case Phase::RequestOff: return "request_dlssg_hard_reset";
    case Phase::WaitOffCommit: return "wait_dlssg_resources_freed";
    case Phase::SetSystemHdr: return "set_windows_hdr_direct";
    case Phase::WaitSystemHdr: return "wait_windows_hdr";
    case Phase::SetGameHdr: return "set_control_hdr";
    case Phase::WaitDomain: return "wait_hdr_domain";
    case Phase::WaitFreshDomain: return "wait_fresh_domain";
    case Phase::RestoreFg: return "confirm_saved_fg_selection";
    case Phase::WaitFgResume: return "wait_fg_resume";
    case Phase::Complete: return "complete";
    case Phase::Failed: return "failed";
    }
    return "unknown";
}

struct State {
    Phase phase{Phase::Idle};
    bool sourceHdr{};
    bool targetHdr{};
    bool sourceFgWasEnabled{};
    bool failureUnwindStarted{};
    uint32_t sourceSelection{};
    uint64_t startedMs{};
    uint64_t phaseStartedMs{};
    uint64_t basePresent{};
    uint64_t baseGeneratedCount{};
    uint64_t baseFreeCount{};
    uint64_t freshAaBase{};
    uint64_t freshPresentBase{};
    const char* error{"none"};

    bool active() const noexcept {
        return phase != Phase::Idle && phase != Phase::Complete && phase != Phase::Failed;
    }

    void fail(const char* why, uint64_t nowMs = 0) noexcept {
        phase = Phase::Failed;
        error = why;
        phaseStartedMs = nowMs;
    }
};

inline bool TransitionTimedOut(uint64_t startedMs, uint64_t nowMs) noexcept {
    return nowMs >= startedMs && (nowMs - startedMs) > kTransitionTimeoutMs;
}

inline bool ThirtySecondStallIsSafe(uint64_t startedMs) noexcept {
    return !TransitionTimedOut(startedMs, startedMs + 30000);
}

}
