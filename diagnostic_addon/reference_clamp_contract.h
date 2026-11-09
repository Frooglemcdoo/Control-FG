#pragma once
#include <cstdint>

namespace controlfg_clamp_ref_contract {
inline constexpr std::uint32_t TargetShaderCRC = 0x600347E7u;
inline constexpr char BuildId[] = "ControlFG-RenoDXClampRef-r21w-api18-renodx120663347";

enum StatusBits : std::uint32_t {
    Registered = 1u << 0,
    ShaderReady = 1u << 1,
    TargetSeen = 1u << 2,
    ReplacementRequested = 1u << 3,
    ReplacementConfirmed = 1u << 4,
    TargetInitializedUnreplaced = 1u << 5,
};

inline constexpr std::uint32_t ReadyMask =
    Registered | ShaderReady | TargetSeen | ReplacementRequested | ReplacementConfirmed;
inline constexpr std::uint32_t RejectMask = TargetInitializedUnreplaced;

inline constexpr bool Ready(
    bool buildIdMatches,
    std::uint32_t status,
    std::uint64_t requests,
    std::uint64_t confirms,
    std::uint32_t targetCrc,
    std::uint32_t replacementCrc) noexcept {
    return buildIdMatches &&
           targetCrc == TargetShaderCRC && replacementCrc != 0u &&
           (status & ReadyMask) == ReadyMask && (status & RejectMask) == 0u &&
           requests != 0u && confirms != 0u;
}
}
