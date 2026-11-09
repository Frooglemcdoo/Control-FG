#pragma once
#include <atomic>
namespace control_rr_diffuse_source {
// R361: native broad diffuse temporal/spatial filtering is required upstream
// of character composition. RR selection alone is not a reason to skip it.
// The old policy is available only for the bounded TEST JESSE comparison.
inline std::atomic<bool> legacyBypass{false};
inline bool Preserve(bool diagnosticPreserve, bool legacy) noexcept {
    return diagnosticPreserve || !legacy;
}
inline bool Preserve(bool diagnosticPreserve) noexcept {
    return Preserve(diagnosticPreserve,legacyBypass.load(std::memory_order_acquire));
}
}
