#pragma once
#include <atomic>
#include <cstdint>

namespace native_lighting {
enum class Mode : unsigned { Native, DisableBakedGi, DisableDirect };
inline constexpr unsigned kGiBit = 1, kDirectBit = 2, kAllBits = 3;
inline const char* ModeName(Mode mode) noexcept {
    switch (mode) {
    case Mode::DisableBakedGi: return "gi_off";
    case Mode::DisableDirect: return "direct_off";
    default: return "native";
    }
}
inline bool Disables(Mode mode, unsigned kind) noexcept {
    return (mode == Mode::DisableBakedGi && kind == kGiBit) ||
           (mode == Mode::DisableDirect && kind == kDirectBit);
}

// A selection and its observation masks share one atomic word. Late worker
// recordings cannot mark a newer selection as confirmed.
class LiveSelection {
    std::atomic<uint64_t> state_{0};
public:
    uint64_t Read() const noexcept { return state_.load(std::memory_order_acquire); }
    static Mode GetMode(uint64_t state) noexcept { return static_cast<Mode>(state & 3); }
    static uint64_t Sequence(uint64_t state) noexcept { return state >> 4; }
    static unsigned Observed(uint64_t state) noexcept { return unsigned((state >> 2) & 3); }
    uint64_t Select(Mode mode) noexcept {
        auto old = Read();
        uint64_t next;
        do { next = ((Sequence(old) + 1) << 4) | static_cast<unsigned>(mode); }
        while (!state_.compare_exchange_weak(old, next, std::memory_order_acq_rel));
        return next;
    }
    bool Observe(uint64_t selected, unsigned kind) noexcept {
        auto current = Read();
        while ((current & ~uint64_t(12)) == (selected & ~uint64_t(12))) {
            const auto next = current | (uint64_t(kind & kAllBits) << 2);
            if (next == current) return false;
            if (state_.compare_exchange_weak(current, next, std::memory_order_acq_rel)) return true;
        }
        return false;
    }
};
} // namespace native_lighting
