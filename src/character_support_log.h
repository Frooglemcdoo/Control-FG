#pragma once
#include <cstring>

namespace control_character_support {
// Only bounded lifecycle events. Never admit per-frame RR/guide diagnostics.
inline bool IsLifecycleEvent(const char* format) noexcept {
    if (!format) return false;
    const char* const markers[] = {
        "RR_CHARACTER_SUPPORT_R418",
        "RR_CHARACTER_BASE_ALBEDO_R417",
        "RR_HAIR_OPACITY_READY",
        // Bounded pipeline creation / first-and-final binding results only.
        "GI_FROZEN_R426",
        "GI_FROZEN_R428",
        "GI_FROZEN_R429",
        "PROBE", "GI_FAULT_R467", "GI_AUTO_R467", "GI_AUTO_R468",
        "GI_TOGGLE_R308",
        "GI_RAYS_R439",
        "GI_COST_R440A",
        "GI_AUTO_R448",
        "GI_AUTO_R449",
        "GI_AUTO_R450",
        "GI_AUTO_R451",
        "GI_AUTO_R452",
        "GI_AUTO_R453",
        "GI_AUTO_R454",
        "GI_AUTO_R466", "GI_AUTO_R465", "GI_AUTO_R464", "GI_AUTO_R463", "GI_AUTO_R462", "GI_AUTO_R460", "GI_AUTO_R459", "GI_AUTO_R458", "GI_AUTO_R457", "GI_AUTO_R456", "GI_AUTO_R455"
    };
    for (const char* marker : markers) {
        const auto n = std::strlen(marker);
        if (std::strncmp(format, marker, n) == 0 &&
            (format[n] == '\0' || format[n] == ' ')) return true;
    }
    return false;
}
}
