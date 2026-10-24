#pragma once
#include <atomic>
#include <cstdint>
#include "rr_specular_temporal_policy.h"

namespace native_lighting {
enum class Mode : unsigned { Native, DisableBakedGi, DisableDirect, DisableDirectDiffuse, DisableDirectSpecular, BypassShadows, DisableLightPattern, BypassScreenShadows, BypassSpotShadows, BypassPointShadows, SpotVarianceLow, SpotVarianceHigh, SpotPcfCenter, SpotPcf32, SpotStableSoft, SpotStableTight, SpotBilinear, SpotResolution2x, RtSpot, RtSpotRaw, RtSpotMapGuard, RtSpotNativeCull, RtSpotVisibilityRaw, RtSpotVisibilityFiltered, RtSpotLightIds, RtSpotBaseline, RtSpotDiffuseFiltered, RtSpotGiFiltered, RtSpotWorldAlbedo, RtSpotSpecularFiltered, RtSpotViewAngle, RtSpotGi2, RtSpotGi4, RtSpotGi8, RtSpotGiMenu, RtSpotGi12, RtSpotGi16, RtSpotSpec1, RtSpotSpec2, RtSpotSpec4, RtSpotSpec8, RtSpotSpec12, RtSpotSpec16, RtSpotTemporal2, RtSpotTemporal4, RoomNative, RoomGi, RoomSpot, RoomBoth, RoomNativeSelection, RoomOpen, RoomSelectedOff, RoomRaw, RoomBlocker, RoomHitX, RoomHitY, RoomHitZ, RoomAuthoredClip, RoomAuthoredSpatial, RoomAuthoredRaw, RoomNativeNoRR, RoomClipNoRR, RoomContactOff, RoomScreenOcclusionOff };
inline bool IsGiSampleMode(Mode mode) noexcept {
    return mode==Mode::RtSpotGi2 || mode==Mode::RtSpotGi4 ||
           mode==Mode::RtSpotGi8 || mode==Mode::RtSpotGiMenu ||
           mode==Mode::RtSpotGi12 || mode==Mode::RtSpotGi16;
}
inline constexpr bool IsHitPositionMode(Mode mode) noexcept {
    return mode==Mode::RoomHitX || mode==Mode::RoomHitY || mode==Mode::RoomHitZ;
}
inline constexpr bool UsesAuthoredClip(Mode mode) noexcept {
    return mode==Mode::RoomContactOff || mode==Mode::RoomScreenOcclusionOff || mode==Mode::RoomAuthoredClip || mode==Mode::RoomAuthoredSpatial || mode==Mode::RoomAuthoredRaw || mode==Mode::RoomClipNoRR || mode==Mode::RoomSelectedOff;
}
inline bool BypassesSpotHistory(Mode mode) noexcept {
    return mode==Mode::RoomAuthoredSpatial || mode==Mode::RoomAuthoredRaw;
}
inline bool IsSpotIsolationMode(Mode mode) noexcept {
    return UsesAuthoredClip(mode)||IsHitPositionMode(mode)||mode==Mode::RoomRaw||mode==Mode::RoomBlocker||mode==Mode::RoomNativeSelection||mode==Mode::RoomOpen||mode==Mode::RoomSelectedOff;
}
inline bool IsSignalDiagnostic(Mode mode) noexcept {
    return IsHitPositionMode(mode) || mode==Mode::RoomBlocker || mode==Mode::RtSpotVisibilityRaw ||
        mode==Mode::RtSpotVisibilityFiltered || mode==Mode::RtSpotLightIds;
}
// Full-color comparisons use the existing per-frame RR bypass, without changing
// the saved RR preference or turning the output into a signal visualization.
inline bool BypassesRR(Mode mode) noexcept {
    return IsSignalDiagnostic(mode) || mode==Mode::RoomNativeNoRR || mode==Mode::RoomClipNoRR;
}
inline std::atomic<bool> spotBandColorsReady{false}, spotHitPositionReady{false}, spotAuthoredClipReady{false};
inline std::atomic<bool> contactOffReady{false}, screenOcclusionLightReady{false}, screenOcclusionSelectorReady{false};
inline std::atomic<bool> spotOpenReady{false}, spotBlockerReady{false};
inline bool IsRoomMode(Mode mode) noexcept {
    return IsSpotIsolationMode(mode)||mode==Mode::RoomNativeNoRR||mode==Mode::RoomNative||mode==Mode::RoomGi||mode==Mode::RoomSpot||mode==Mode::RoomBoth;
}
inline bool UsesSpecularTemporalOnly(Mode mode) noexcept {
    return mode==Mode::RtSpotTemporal2||mode==Mode::RtSpotTemporal4;
}
// Specular tests leave GI at its saved value and retain the working light policies.
inline unsigned SpecularSampleRequest(Mode mode) noexcept {
    if(IsSpotIsolationMode(mode))return 4;
    switch(mode){
    case Mode::RtSpotTemporal2:return 2;
    case Mode::RtSpotTemporal4:return 4;
    case Mode::RoomNative:case Mode::RoomGi:case Mode::RoomSpot:case Mode::RoomBoth:return 4;
    case Mode::RtSpotSpec1:return 1;
    case Mode::RtSpotSpec2:return 2;
    case Mode::RtSpotSpec4:return 4;
    case Mode::RtSpotSpec8:return 8;
    case Mode::RtSpotSpec12:return 12;
    case Mode::RtSpotSpec16:return 16;
    default:return 0;
    }
}
inline std::atomic<bool> specularSamplesReady{false};
inline bool IsSpecularSampleMode(Mode mode) noexcept { return SpecularSampleRequest(mode)!=0; }
// A per-frame diagnostic override; never writes the user's saved GI setting.
inline unsigned GiSampleRequest(Mode mode,unsigned saved) noexcept {
    switch(mode){
    case Mode::RtSpotGi2:return 2;
    case Mode::RtSpotGi4:return 4;
    case Mode::RtSpotGi8:return 8;
    case Mode::RtSpotGi12:return 12;
    case Mode::RtSpotGi16:return 16;
    case Mode::RtSpotGiMenu:return 0;
    default:return saved;
    }
}
inline bool IsRtSpotMode(Mode mode) noexcept {
    if(mode==Mode::RoomNative||mode==Mode::RoomNativeNoRR||mode==Mode::RoomGi)return false;
    return IsSpecularSampleMode(mode) || IsGiSampleMode(mode) || mode == Mode::RtSpot || mode == Mode::RtSpotRaw ||
        mode == Mode::RtSpotMapGuard || mode == Mode::RtSpotNativeCull ||
        mode == Mode::RtSpotVisibilityRaw || mode == Mode::RtSpotVisibilityFiltered || mode == Mode::RtSpotLightIds || mode == Mode::RtSpotBaseline || mode == Mode::RtSpotDiffuseFiltered || mode == Mode::RtSpotGiFiltered || mode == Mode::RtSpotWorldAlbedo || mode == Mode::RtSpotSpecularFiltered || mode == Mode::RtSpotViewAngle;
}
inline bool UsesWorldAlbedo(Mode mode) noexcept { return IsSpecularSampleMode(mode) || IsGiSampleMode(mode) || mode == Mode::RtSpotWorldAlbedo || mode == Mode::RtSpotSpecularFiltered || mode == Mode::RtSpotViewAngle; }
inline bool UsesPerspectiveGuide(Mode mode) noexcept { return mode == Mode::RtSpotViewAngle; }
inline bool PreservesSpecularFiltering(Mode mode) noexcept { return mode == Mode::RtSpotSpecularFiltered; }
// These denoising diagnostics retain the centered, filtered shadow routes.
inline bool PreservesGiFiltering(Mode mode) noexcept { if(IsRoomMode(mode))return IsSpotIsolationMode(mode)||mode==Mode::RoomGi||mode==Mode::RoomBoth; return IsSpecularSampleMode(mode) || IsGiSampleMode(mode) || mode == Mode::RtSpotGiFiltered || mode == Mode::RtSpotWorldAlbedo || mode == Mode::RtSpotSpecularFiltered || mode == Mode::RtSpotViewAngle; }
inline bool PreservesBroadDiffuseFiltering(Mode mode) noexcept {
    return mode == Mode::RtSpotDiffuseFiltered;
}
inline bool UsesUnshadowedSpotSelection(Mode mode) noexcept {
    return IsRtSpotMode(mode)&&mode!=Mode::RoomNativeSelection;
}
inline bool UsesRawSpotVisibility(Mode mode) noexcept {
    return mode==Mode::RoomAuthoredRaw || IsHitPositionMode(mode) || mode == Mode::RoomBlocker || mode == Mode::RoomRaw || mode == Mode::RtSpotRaw || mode == Mode::RtSpotVisibilityRaw;
}
inline bool ShowsSpotVisibility(Mode mode) noexcept {
    return IsHitPositionMode(mode) || mode == Mode::RoomBlocker || mode == Mode::RtSpotVisibilityRaw || mode == Mode::RtSpotVisibilityFiltered;
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
    {Mode::Native, L"NATIVE"}, {Mode::RoomBoth, L"BOTH"}, {Mode::RoomAuthoredClip, L"DEFAULT (CLIP)"},
    {Mode::RoomNativeNoRR, L"NATIVE / RR OFF"}, {Mode::RoomClipNoRR, L"CLIP / RR OFF"}, {Mode::RoomSelectedOff, L"LIGHT OFF"},
    {Mode::RoomContactOff, L"CONTACT OFF"}, {Mode::RoomScreenOcclusionOff, L"SCREEN AO OFF"}, {Mode::RtSpotLightIds, L"LIGHT IDS"}
};
inline constexpr unsigned kModeCount = sizeof(kModeChoices) / sizeof(kModeChoices[0]);
inline unsigned RequiredVariant(Mode mode) noexcept {
    if(mode==Mode::RoomNative||mode==Mode::RoomGi)return 0;
    if(IsSpecularSampleMode(mode)||IsGiSampleMode(mode))return kRtSpotReadyBit;
    switch (mode) {
    case Mode::RtSpot: case Mode::RtSpotRaw: case Mode::RtSpotMapGuard: case Mode::RtSpotNativeCull:
    case Mode::RtSpotVisibilityRaw: case Mode::RtSpotVisibilityFiltered: case Mode::RtSpotLightIds: case Mode::RtSpotBaseline: case Mode::RtSpotDiffuseFiltered: case Mode::RtSpotGiFiltered: case Mode::RtSpotWorldAlbedo: case Mode::RtSpotSpecularFiltered: case Mode::RtSpotViewAngle: return kRtSpotReadyBit;
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
    return mode == Mode::RoomScreenOcclusionOff ? kAllBits : mode == Mode::Native ? kAllBits : mode == Mode::DisableBakedGi ? kGiBit : kDirectBit;
}
inline bool ModeAvailable(Mode mode,unsigned ready) noexcept {
    if(mode==Mode::RoomContactOff&&!contactOffReady.load(std::memory_order_acquire))return false;
    if(mode==Mode::RoomScreenOcclusionOff&&(!screenOcclusionLightReady.load(std::memory_order_acquire)||!screenOcclusionSelectorReady.load(std::memory_order_acquire)))return false;
    if(UsesAuthoredClip(mode)&&!spotAuthoredClipReady.load(std::memory_order_acquire))return false;
    if(IsHitPositionMode(mode)&&(!spotHitPositionReady.load(std::memory_order_acquire)||!spotBandColorsReady.load(std::memory_order_acquire)))return false;
    if(mode==Mode::RoomBlocker&&(!spotBlockerReady.load(std::memory_order_acquire)||!spotBandColorsReady.load(std::memory_order_acquire)))return false;
    if(mode==Mode::RoomOpen&&!spotOpenReady.load(std::memory_order_acquire))return false;
    const unsigned required=RequiredVariant(mode);
    return (ready&required)==required &&
        (!IsSpecularSampleMode(mode)||specularSamplesReady.load(std::memory_order_acquire)) &&
        (!UsesSpecularTemporalOnly(mode)||control_rr_temporal::ready.load(std::memory_order_acquire));
}
inline const char* ModeName(Mode mode) noexcept {
    switch (mode) {
    case Mode::RtSpotTemporal2:return "rt_spot_temporal_2";
    case Mode::RtSpotTemporal4:return "rt_spot_temporal_4";
    case Mode::RoomNative:return "room_native";
    case Mode::RoomGi:return "room_gi_only";
    case Mode::RoomSpot:return "room_spot_only";
    case Mode::RoomBoth:return "room_both";
    case Mode::RoomRaw:return "room_raw";
    case Mode::RoomBlocker:return "room_blocker";
    case Mode::RoomHitX:return "room_hit_x";
    case Mode::RoomHitY:return "room_hit_y";
    case Mode::RoomHitZ:return "room_hit_z";
    case Mode::RoomAuthoredSpatial:return "room_authored_spatial";
    case Mode::RoomAuthoredRaw:return "room_authored_raw";
    case Mode::RoomNativeNoRR:return "native_rr_off";
    case Mode::RoomContactOff:return "clip_contact_off";
    case Mode::RoomScreenOcclusionOff:return "clip_screen_occlusion_off";
    case Mode::RoomClipNoRR:return "clip_rr_off";
    case Mode::RoomAuthoredClip:return "room_authored_clip";
    case Mode::RoomNativeSelection:return "room_native_selection";
    case Mode::RoomOpen:return "room_open";
    case Mode::RoomSelectedOff:return "room_selected_off";
    case Mode::RtSpotSpec1:return "rt_spot_spec_1";
    case Mode::RtSpotSpec2:return "rt_spot_spec_2";
    case Mode::RtSpotSpec4:return "rt_spot_spec_4";
    case Mode::RtSpotSpec8:return "rt_spot_spec_8";
    case Mode::RtSpotSpec12:return "rt_spot_spec_12";
    case Mode::RtSpotSpec16:return "rt_spot_spec_16";
    case Mode::RtSpotGi2:return "rt_spot_gi_2";
    case Mode::RtSpotGi4:return "rt_spot_gi_4";
    case Mode::RtSpotGi8:return "rt_spot_gi_8";
    case Mode::RtSpotGi12:return "rt_spot_gi_12";
    case Mode::RtSpotGi16:return "rt_spot_gi_16";
    case Mode::RtSpotGiMenu:return "rt_spot_gi_menu";
    case Mode::RtSpotViewAngle: return "rt_spot_view_angle";
    case Mode::RtSpotSpecularFiltered: return "rt_spot_specular_filtered";
    case Mode::RtSpotGiFiltered: return "rt_spot_gi_filtered";
    case Mode::RtSpotWorldAlbedo: return "rt_spot_world_albedo";
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
    if (mode == Mode::Native || mode == Mode::SpotResolution2x || IsRoomMode(mode) || IsRtSpotMode(mode)) return false;
    return kind == RequiredBinding(mode);
}

// A selection and its observation masks share one atomic word. Late worker
// recordings cannot mark a newer selection as confirmed.
class LiveSelection {
    static constexpr unsigned kModeBits = 6, kObservedShift = kModeBits;
    static constexpr unsigned kSequenceShift = kModeBits + 2;
    static constexpr uint64_t kModeMask = (uint64_t(1) << kModeBits) - 1;
    static constexpr uint64_t kObservedMask = uint64_t(kAllBits) << kObservedShift;
    static_assert(static_cast<unsigned>(Mode::RoomScreenOcclusionOff) <= kModeMask, "Mode storage exhausted");
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
