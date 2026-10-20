#pragma once
#include <atomic>
#include <cstdint>

namespace native_lighting {
enum class Mode : unsigned { Native, DisableBakedGi, DisableDirect, DisableDirectDiffuse, DisableDirectSpecular, BypassShadows, DisableLightPattern, BypassScreenShadows, BypassSpotShadows, BypassPointShadows };
inline constexpr unsigned kGiBit = 1, kDirectBit = 2, kAllBits = 3;
inline constexpr unsigned kDiffuseReadyBit = 4, kSpecularReadyBit = 8, kShadowReadyBit = 16, kPatternReadyBit = 32;
inline constexpr unsigned kScreenReadyBit = 64, kSpotReadyBit = 128, kPointReadyBit = 256;
struct ModeChoice { Mode mode; const wchar_t* label; };
inline constexpr ModeChoice kModeChoices[] = {
    {Mode::Native, L"NATIVE"}, {Mode::BypassShadows, L"ALL SHADOWS"},
    {Mode::BypassScreenShadows, L"SCREEN SHADOWS"},
    {Mode::BypassSpotShadows, L"SPOT SHADOWS"},
    {Mode::BypassPointShadows, L"POINT SHADOWS"}
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
    case Mode::BypassScreenShadows: return kScreenReadyBit;
    case Mode::BypassSpotShadows: return kSpotReadyBit;
    case Mode::BypassPointShadows: return kPointReadyBit;
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
    case Mode::BypassScreenShadows: return "screen_shadows_bypass";
    case Mode::BypassSpotShadows: return "spot_shadows_bypass";
    case Mode::BypassPointShadows: return "point_shadows_bypass";
    default: return "native";
    }
}
inline bool Disables(Mode mode, unsigned kind) noexcept {
    if (mode == Mode::Native) return false;
    return kind == RequiredBinding(mode);
}

// A selection and its observation masks share one atomic word. Late worker
// recordings cannot mark a newer selection as confirmed.
class LiveSelection {
    static constexpr unsigned kModeBits = 4, kObservedShift = kModeBits;
    static constexpr unsigned kSequenceShift = kModeBits + 2;
    static constexpr uint64_t kModeMask = (uint64_t(1) << kModeBits) - 1;
    static constexpr uint64_t kObservedMask = uint64_t(kAllBits) << kObservedShift;
    std::atomic<uint64_t> state_{0};
public:
    uint64_t Read() const noexcept { return state_.load(std::memory_order_acquire); }
    static Mode GetMode(uint64_t state) noexcept { return static_cast<Mode>(state & kModeMask); }
    static uint64_t Sequence(uint64_t state) noexcept { return state >> kSequenceShift; }
    static unsigned Observed(uint64_t state) noexcept { return unsigned((state >> kObservedShift) & kAllBits); }
    uint64_t Select(Mode mode) noexcept {
        auto old = Read();
        uint64_t next;
        do { next = ((Sequence(old) + 1) << kSequenceShift) | static_cast<unsigned>(mode); }
        while (!state_.compare_exchange_weak(old, next, std::memory_order_acq_rel));
        return next;
    }
    bool Observe(uint64_t selected, unsigned kind) noexcept {
        auto current = Read();
        while ((current & ~kObservedMask) == (selected & ~kObservedMask)) {
            const auto next = current | (uint64_t(kind & kAllBits) << kObservedShift);
            if (next == current) return false;
            if (state_.compare_exchange_weak(current, next, std::memory_order_acq_rel)) return true;
        }
        return false;
    }
};
} // namespace native_lighting
