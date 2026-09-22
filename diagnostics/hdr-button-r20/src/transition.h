#pragma once
#include <cstdint>

namespace hdrbutton {

enum class Phase : uint32_t {
    Idle,
    RequestOff,
    WaitOffCommit,
    SetSystemHdr,
    WaitGameHdr,
    WaitFreshHdr,
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
    case Phase::RequestOff: return "request_fg_off";
    case Phase::WaitOffCommit: return "wait_fg_off_commit";
    case Phase::SetSystemHdr: return "set_windows_hdr";
    case Phase::WaitGameHdr: return "wait_game_hdr";
    case Phase::WaitFreshHdr: return "wait_fresh_hdr";
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
    uint32_t sourceSelection{};
    uint64_t startedMs{};
    uint64_t basePresent{};
    uint64_t baseFreeCount{};
    uint64_t baseGeneratedCount{};
    uint64_t drainStartPresent{};
    uint64_t freshAaBase{};
    const char* error{"none"};

    bool active() const noexcept {
        return phase != Phase::Idle && phase != Phase::Complete && phase != Phase::Failed;
    }

    void fail(const char* why) noexcept {
        phase = Phase::Failed;
        error = why;
    }
};

}
