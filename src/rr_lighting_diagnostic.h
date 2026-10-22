#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace control_rr_diagnostic {
// CPU observations only. No resource access, allocation, waits or file I/O.
// Monotonic totals are never reset by the UI: concurrent hook calls cannot be lost.
enum class Event : std::size_t {
    HooksInstalled, BroadA, BroadB, BroadNative, BroadBypass,
    ReflectionEntry, ReflectionNative, ReflectionClamp, ReflectionRaw,
    GIOption, GIBypassSelected, GIRelease, GIValidated,
    EvaluationEntry, RRAttempt, RRSuccess, RRFailure, SRReturn, EvaluationRejected, Count
};
inline constexpr const char* kEventNames[] = {
    "hooks_installed", "broad_a", "broad_b", "broad_native", "broad_bypass",
    "reflection_entry", "reflection_native", "reflection_clamp", "reflection_raw",
    "gi_option", "gi_bypass_selected", "gi_release", "gi_validated",
    "evaluation_entry", "rr_attempt", "rr_success", "rr_failure", "sr_return", "evaluation_rejected"
};
inline constexpr auto kCount = static_cast<std::size_t>(Event::Count);
static_assert(sizeof(kEventNames)/sizeof(kEventNames[0]) == kCount);
static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
class Counters {
    std::array<std::atomic<std::uint64_t>, kCount> values_{};
public:
    void Record(Event event) noexcept {
        values_[static_cast<std::size_t>(event)].fetch_add(1, std::memory_order_relaxed);
    }
    std::array<std::uint64_t, kCount> Snapshot() const noexcept {
        std::array<std::uint64_t, kCount> result{};
        for (std::size_t i=0; i<kCount; ++i) result[i]=values_[i].load(std::memory_order_relaxed);
        return result;
    }
};
inline Counters counters;
inline std::atomic<unsigned> lastEffects{0}, lastEvaluationResult{0};
inline std::atomic<bool> lastSelected{false};
inline void Record(Event event) noexcept { counters.Record(event); }
} // namespace control_rr_diagnostic
