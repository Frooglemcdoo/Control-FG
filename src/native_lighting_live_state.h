#pragma once
#include <atomic>
#include <cstdint>

namespace native_lighting {
enum class Mode : unsigned { Native, DisableBakedGi, DisableDirect, DisableDirectDiffuse, DisableDirectSpecular, BypassShadows, DisableLightPattern };
inline constexpr unsigned kGiBit = 1, kDirectBit = 2, kAllBits = 3;
inline constexpr unsigned kDiffuseReadyBit = 4, kSpecularReadyBit = 8, kShadowReadyBit = 16, kPatternReadyBit = 32;
struct ModeChoice { Mode mode; const wchar_t* label; };
inline constexpr ModeChoice kModeChoices[] = {
    {Mode::Native, L"NATIVE"}, {Mode::BypassShadows, L"SHADOWS BYPASS"},
    {Mode::DisableLightPattern, L"PATTERN OFF"}, {Mode::DisableDirectDiffuse, L"DIFFUSE OFF"},
    {Mode::DisableDirectSpecular, L"SPECULAR OFF"}, {Mode::DisableBakedGi, L"GI OFF"},
    {Mode::DisableDirect, L"DIRECT OFF"}
};
inline constexpr unsigned kModeCount = sizeof(kModeChoices) / sizeof(kModeChoices[0]);
inline unsigned RequiredVariant(Mode mode) noexcept {
    switch (mode) {
    case Mode::DisableBakedGi: return kGiBit;
    case Mode::DisableDirect: return kDirectBit;
    case Mode::DisableDirectDiffuse: return kDiffuseReadyBit;
    case Mode::DisableDirectSpecular: return kSpecularReadyBit;
    case Mode::BypassShadows: return kShadowReadyBit;
    case Mode::DisableLightPattern: return kPatternReadyBit;
    default: return 0;
    }
}
inline unsigned RequiredBinding(Mode mode) noexcept {
    return mode == Mode::Native ? kAllBits : mode == Mode::DisableBakedGi ? kGiBit : kDirectBit;
}
inline const char* ModeName(Mode mode) noexcept {
    switch (mode) {
    case Mode::DisableBakedGi: return "gi_off";
    case Mode::DisableDirect: return "direct_off";
    case Mode::DisableDirectDiffuse: return "diffuse_off";
    case Mode::DisableDirectSpecular: return "specular_off";
    case Mode::BypassShadows: return "shadows_bypass";
    case Mode::DisableLightPattern: return "pattern_off";
    default: return "native";
    }
}
inline bool Disables(Mode mode, unsigned kind) noexcept {
    return (mode == Mode::DisableBakedGi && kind == kGiBit) ||
           ((mode == Mode::DisableDirect || mode == Mode::DisableDirectDiffuse || mode == Mode::DisableDirectSpecular || mode == Mode::BypassShadows || mode == Mode::DisableLightPattern) && kind == kDirectBit);
}

// A selection and its observation masks share one atomic word. Late worker
// recordings cannot mark a newer selection as confirmed.
class LiveSelection {
    std::atomic<uint64_t> state_{0};
public:
    uint64_t Read() const noexcept { return state_.load(std::memory_order_acquire); }
    static Mode GetMode(uint64_t state) noexcept { return static_cast<Mode>(state & 7); }
    static uint64_t Sequence(uint64_t state) noexcept { return state >> 5; }
    static unsigned Observed(uint64_t state) noexcept { return unsigned((state >> 3) & 3); }
    uint64_t Select(Mode mode) noexcept {
        auto old = Read();
        uint64_t next;
        do { next = ((Sequence(old) + 1) << 5) | static_cast<unsigned>(mode); }
        while (!state_.compare_exchange_weak(old, next, std::memory_order_acq_rel));
        return next;
    }
    bool Observe(uint64_t selected, unsigned kind) noexcept {
        auto current = Read();
        while ((current & ~uint64_t(24)) == (selected & ~uint64_t(24))) {
            const auto next = current | (uint64_t(kind & kAllBits) << 3);
            if (next == current) return false;
            if (state_.compare_exchange_weak(current, next, std::memory_order_acq_rel)) return true;
        }
        return false;
    }
};
} // namespace native_lighting
