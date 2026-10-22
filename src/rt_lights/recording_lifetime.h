#pragma once
#include <cstdint>

namespace rt_lights {
// Metadata only. The owner serializes updates from recording/reset/submission
// hooks. A fence is necessary but insufficient: a closed command list may be
// submitted again until Reset succeeds, so it continues to own its resources.
class RecordingLifetime {
    std::uint64_t generation = 0, lastFence = 0;
    bool detached = true, submitted = false, quarantined = false;
public:
    bool Begin(std::uint64_t id) noexcept {
        if (!id || generation || quarantined) return false;
        generation = id; lastFence = 0;
        detached = submitted = false;
        return true;
    }
    bool Submit(std::uint64_t id, std::uint64_t fence) noexcept {
        if (quarantined || detached || id != generation || !generation ||
            !fence || fence == UINT64_MAX || fence <= lastFence) return false;
        submitted = true; lastFence = fence;
        return true;
    }
    bool Reset(std::uint64_t id) noexcept {
        if (quarantined || id != generation || !generation || detached) return false;
        detached = true;
        return true;
    }
    bool Retire(std::uint64_t completed) noexcept {
        if (quarantined || !generation || !detached || completed == UINT64_MAX ||
            (submitted && completed < lastFence)) return false;
        generation = lastFence = 0;
        submitted = false;
        return true;
    }
    void Quarantine() noexcept { quarantined = true; }
    bool Detached() const noexcept { return detached; }
    bool Submitted() const noexcept { return submitted; }
    bool Available() const noexcept { return !generation && !quarantined; }
    std::uint64_t Generation() const noexcept { return generation; }
};
}
