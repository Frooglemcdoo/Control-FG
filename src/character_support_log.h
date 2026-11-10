#pragma once
#include <cstring>

namespace control_character_support {
// Only bounded lifecycle events. Never admit per-frame RR/guide diagnostics.
inline bool IsLifecycleEvent(const char* format) noexcept {
    if (!format) return false;
    const char* const markers[] = {
        "RR_CHARACTER_SUPPORT_R418",
        "RR_CHARACTER_BASE_ALBEDO_R417",
        "RR_HAIR_OPACITY_READY"
    };
    for (const char* marker : markers) {
        const auto n = std::strlen(marker);
        if (std::strncmp(format, marker, n) == 0 &&
            (format[n] == '\0' || format[n] == ' ')) return true;
    }
    return false;
}
}
