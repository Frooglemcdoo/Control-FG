#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>

namespace control_rr {

// Portable policy/encoding used by the Windows-only option isolation hook.
// The real Control RR option stays at the game's baseline value (OFF). Only
// the three reviewed render decisions and the jitter byte consult mod-owned
// state, so removing the proxy cannot leave RR rendering state behind.
using RipByteCompare6 = std::array<std::uint8_t, 6>;

inline bool NativeRenderOption(bool hookEnabled, bool selected, bool stopped) noexcept {
    return hookEnabled && selected && !stopped;
}

inline bool RedirectRipByteCompare(const RipByteCompare6& original,
                                   std::uintptr_t site,
                                   std::uintptr_t expectedTarget,
                                   std::uintptr_t replacementTarget,
                                   RipByteCompare6& replacement) noexcept {
    if (original[0] != 0x38u || original[1] != 0x1du) return false; // cmp byte ptr [rip+disp32], bl
    std::int32_t oldDisp = 0;
    std::memcpy(&oldDisp, original.data() + 2, sizeof(oldDisp));
    const auto after = site + original.size();
    const auto decoded = static_cast<std::uintptr_t>(static_cast<std::intptr_t>(after) + oldDisp);
    if (decoded != expectedTarget) return false;

    const auto delta = static_cast<std::int64_t>(replacementTarget) - static_cast<std::int64_t>(after);
    if (delta < (std::numeric_limits<std::int32_t>::min)() ||
        delta > (std::numeric_limits<std::int32_t>::max)()) return false;
    const auto newDisp = static_cast<std::int32_t>(delta);
    replacement = original;
    std::memcpy(replacement.data() + 2, &newDisp, sizeof(newDisp));
    return true;
}

inline std::uintptr_t DecodeRipByteCompareTarget(const RipByteCompare6& instruction,
                                                 std::uintptr_t site) noexcept {
    if (instruction[0] != 0x38u || instruction[1] != 0x1du) return 0;
    std::int32_t disp = 0;
    std::memcpy(&disp, instruction.data() + 2, sizeof(disp));
    return static_cast<std::uintptr_t>(static_cast<std::intptr_t>(site + instruction.size()) + disp);
}

} // namespace control_rr
