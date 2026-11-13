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
        "PROBE",
        "GI_TOGGLE_R308",
        "GI_FILTER_R435",
        "gi_temporal_adaptive"
    };
    for (const char* marker : markers) {
        const auto n = std::strlen(marker);
        if (std::strncmp(format, marker, n) == 0 &&
            (format[n] == '\0' || format[n] == ' ')) return true;
    }
    return false;
}
}
