#pragma once
#include <atomic>
#include <cstdint>

namespace native_lighting {
enum class Mode : unsigned { Native, DisableBakedGi, DisableDirect, DisableDirectDiffuse, DisableDirectSpecular, BypassShadows, DisableLightPattern, BypassScreenShadows, BypassSpotShadows, BypassPointShadows, SpotVarianceLow, SpotVarianceHigh, SpotPcfCenter, SpotPcf32, SpotStableSoft, SpotStableTight, SpotBilinear, SpotResolution2x, RtSpot, RtSpotRaw, RtSpotMapGuard, RtSpotNativeCull, RtSpotVisibilityRaw, RtSpotVisibilityFiltered, RtSpotLightIds, RtSpotBaseline, RtSpotDiffuseFiltered, RtSpotGiFiltered };
inline bool IsRtSpotMode(Mode mode) noexcept {
    return mode == Mode::RtSpot || mode == Mode::RtSpotRaw ||
        mode == Mode::RtSpotMapGuard || mode == Mode::RtSpotNativeCull ||
        mode == Mode::RtSpotVisibilityRaw || mode == Mode::RtSpotVisibilityFiltered || mode == Mode::RtSpotLightIds || mode == Mode::RtSpotBaseline || mode == Mode::RtSpotDiffuseFiltered || mode == Mode::RtSpotGiFiltered;
}
// These denoising diagnostics retain the centered, filtered shadow routes.
inline bool PreservesGiFiltering(Mode mode) noexcept { return mode == Mode::RtSpotGiFiltered; }
inline bool PreservesBroadDiffuseFiltering(Mode mode) noexcept {
    return mode == Mode::RtSpotDiffuseFiltered;
}
inline bool UsesUnshadowedSpotSelection(Mode mode) noexcept {
    return IsRtSpotMode(mode);
}
inline bool UsesRawSpotVisibility(Mode mode) noexcept {
    return mode == Mode::RtSpotRaw || mode == Mode::RtSpotVisibilityRaw;
}
inline bool ShowsSpotVisibility(Mode mode) noexcept {
    return mode == Mode::RtSpotVisibilityRaw || mode == Mode::RtSpotVisibilityFiltered;
}
inline bool ExpandsSpotCasters(Mode mode) noexcept {
    // R78 uses R71 native caster coverage; expansion hooks are not installed.
    (void)mode;
    return false;
}
inline constexpr unsigned kGiBit = 1, kDirectBit = 2, kAllBits = 3;
inline constexpr unsigned kDiffuseReadyBit = 4, kSpecularReadyBit = 8, kShadowReadyBit = 16, kPatternReadyBit = 32;
inline constexpr unsigned kScreenReadyBit = 64, kSpotReadyBit = 128, kPointReadyBit = 256;
inline constexpr unsigned kSpotVarianceLowReadyBit = 1u << 9;
inline constexpr unsigned kSpotVarianceHighReadyBit = 1u << 10;
inline constexpr unsigned kSpotPcfCenterReadyBit = 1u << 11;
inline constexpr unsigned kSpotPcf32ReadyBit = 1u << 12;
inline constexpr unsigned kSpotStableSoftReadyBit = 1u << 13;
inline constexpr unsigned kSpotStableTightReadyBit = 1u << 14;
inline constexpr unsigned kSpotBilinearReadyBit = 1u << 15;
inline constexpr unsigned kSpotResolutionReadyBit = 1u << 16;
inline constexpr unsigned kRtSpotReadyBit = 1u << 17;
struct ModeChoice { Mode mode; const wchar_t* label; };
inline constexpr ModeChoice kModeChoices[] = {
    {Mode::Native, L"NATIVE SHADOWS"},
    {Mode::RtSpot, L"CENTER FILTERED"},
    {Mode::RtSpotRaw, L"CENTER RAW"},
    {Mode::RtSpotMapGuard, L"SELECTED LIGHT OFF"},
    {Mode::RtSpotVisibilityFiltered, L"FILTERED VISIBILITY"},
    {Mode::RtSpotLightIds, L"LIGHT ID COLORS"},
    {Mode::RtSpotBaseline, L"RT R94 BASELINE"},
    {Mode::RtSpotDiffuseFiltered, L"DIFFUSE FILTER"},
    {Mode::RtSpotGiFiltered, L"GI FILTER"}
};
inline constexpr unsigned kModeCount = sizeof(kModeChoices) / sizeof(kModeChoices[0]);
inline unsigned RequiredVariant(Mode mode) noexcept {
    switch (mode) {
    case Mode::RtSpot: case Mode::RtSpotRaw: case Mode::RtSpotMapGuard: case Mode::RtSpotNativeCull:
    case Mode::RtSpotVisibilityRaw: case Mode::RtSpotVisibilityFiltered: case Mode::RtSpotLightIds: case Mode::RtSpotBaseline: case Mode::RtSpotDiffuseFiltered: case Mode::RtSpotGiFiltered: return kRtSpotReadyBit;
    case Mode::DisableBakedGi: return kGiBit;
    case Mode::DisableDirect: return kDirectBit;
    case Mode::DisableDirectDiffuse: return kDiffuseReadyBit;
    case Mode::DisableDirectSpecular: return kSpecularReadyBit;
    case Mode::BypassShadows: return kShadowReadyBit;
    case Mode::DisableLightPattern: return kPatternReadyBit;
    case Mode::BypassScreenShadows: return kScreenReadyBit;
    case Mode::BypassSpotShadows: return kSpotReadyBit;
    case Mode::BypassPointShadows: return kPointReadyBit;
    case Mode::SpotVarianceLow: return kSpotVarianceLowReadyBit;
    case Mode::SpotVarianceHigh: return kSpotVarianceHighReadyBit;
    case Mode::SpotPcfCenter: return kSpotPcfCenterReadyBit;
    case Mode::SpotPcf32: return kSpotPcf32ReadyBit;
    case Mode::SpotStableSoft: return kSpotStableSoftReadyBit;
    case Mode::SpotStableTight: return kSpotStableTightReadyBit;
    case Mode::SpotBilinear: return kSpotBilinearReadyBit;
    case Mode::SpotResolution2x: return kSpotResolutionReadyBit;
    default: return 0;
    }
}
inline unsigned RequiredBinding(Mode mode) noexcept {
    return mode == Mode::Native ? kAllBits : mode == Mode::DisableBakedGi ? kGiBit : kDirectBit;
}
inline const char* ModeName(Mode mode) noexcept {
    switch (mode) {
    case Mode::RtSpotGiFiltered: return "rt_spot_gi_filtered";
    case Mode::RtSpotDiffuseFiltered: return "rt_spot_diffuse_filtered";
    case Mode::RtSpotBaseline: return "rt_spot_r94_baseline";
    case Mode::RtSpotVisibilityRaw: return "rt_spot_visibility_raw";
    case Mode::RtSpotVisibilityFiltered: return "rt_spot_visibility_filtered";
    case Mode::RtSpotLightIds: return "rt_spot_light_ids";
    case Mode::RtSpot: return "rt_spot_replacement";
    case Mode::RtSpotRaw: return "rt_spot_raw";
    case Mode::RtSpotMapGuard: return "rt_spot_selected_light_off";
    case Mode::RtSpotNativeCull: return "rt_spot_native_coverage";
    case Mode::DisableBakedGi: return "gi_off";
    case Mode::DisableDirect: return "direct_off";
    case Mode::DisableDirectDiffuse: return "diffuse_off";
    case Mode::DisableDirectSpecular: return "specular_off";
    case Mode::BypassShadows: return "shadows_bypass";
    case Mode::DisableLightPattern: return "pattern_off";
    case Mode::BypassScreenShadows: return "screen_shadows_bypass";
    case Mode::BypassSpotShadows: return "spot_shadows_bypass";
    case Mode::BypassPointShadows: return "point_shadows_bypass";
    case Mode::SpotVarianceLow: return "SpotVarianceLow";
    case Mode::SpotVarianceHigh: return "SpotVarianceHigh";
    case Mode::SpotPcfCenter: return "SpotPcfCenter";
    case Mode::SpotPcf32: return "spot_pcf_32";
    case Mode::SpotStableSoft: return "SpotStableSoft";
    case Mode::SpotStableTight: return "SpotStableTight";
    case Mode::SpotBilinear: return "spot_bilinear";
    case Mode::SpotResolution2x: return "spot_resolution_2x";
    default: return "native";
    }
}
inline bool CanReportVisual(Mode mode, unsigned observed) noexcept {
    const auto required = RequiredBinding(mode);
    return (observed & required) == required;
}
inline bool Disables(Mode mode, unsigned kind) noexcept {
    if (mode == Mode::Native || mode == Mode::SpotResolution2x || IsRtSpotMode(mode)) return false;
    return kind == RequiredBinding(mode);
}

// A selection and its observation masks share one atomic word. Late worker
// recordings cannot mark a newer selection as confirmed.
class LiveSelection {
    static constexpr unsigned kModeBits = 5, kObservedShift = kModeBits;
    static constexpr unsigned kSequenceShift = kModeBits + 2;
    static constexpr uint64_t kModeMask = (uint64_t(1) << kModeBits) - 1;
    static constexpr uint64_t kObservedMask = uint64_t(kAllBits) << kObservedShift;
    static_assert(static_cast<unsigned>(Mode::RtSpotGiFiltered) <= kModeMask, "Mode storage exhausted");
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
