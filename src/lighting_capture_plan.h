#pragma once
#include "native_lighting_live_state.h"
namespace native_lighting::capture_plan {
struct Step { Mode mode; const wchar_t* name; bool gpu; };
inline constexpr Step steps[]={
    {Mode::Native,L"native-user-rr",false},
    {Mode::RoomNativeNoRR,L"native-rr-off",false},
    {Mode::RoomBoth,L"both",true},
    {kDefaultMode,L"default-fixed-user-rr",false},
    {Mode::RoomCorrectedNoRR,L"default-fixed-rr-off",false},
    {Mode::RoomCorrectedNoBakedGi,L"default-baked-gi-off",false},
    {Mode::RoomCorrectedNoBakedGiNoRR,L"default-baked-gi-rr-off",false},
    {Mode::RoomCorrectedVisibility,L"default-spot-signal",false},
    {Mode::RoomCorrectedRawVisibility,L"default-raw-spot-signal",false},
    {Mode::RoomCorrectedLightIds,L"default-spot-light-ids",false},
    {Mode::RoomCorrectedGuarded,L"default-history-guard",false},
    {Mode::RoomCorrectedNoAO,L"default-screen-ao-off",false},
    {Mode::RoomBentNormalOff,L"previous-history-on",false},
    {Mode::RoomCorrectedRaw,L"default-raw-shadows",false},
    {Mode::RoomAuthoredClip,L"previous-clip-user-rr",false},
    {Mode::RoomClipNoRR,L"previous-clip-rr-off",false},
    {Mode::RoomLocalMapsOff,L"clip-local-maps-off",false},
    {Mode::RoomContactOff,L"clip-contact-off",false},
    {Mode::RoomScreenOcclusionOff,L"clip-screen-ao-off",false},
    {Mode::RoomSelectedOff,L"selected-light-off",false},
    {Mode::RoomAuthoredSpatial,L"clip-no-history",false},
    {Mode::RoomAuthoredRaw,L"clip-raw",false},
    {Mode::RoomGi,L"gi-only",false},
    {Mode::RoomOpen,L"selected-light-open",false},
    {Mode::RtSpotVisibilityRaw,L"unclipped-visibility-raw",false},
    {Mode::RtSpotVisibilityFiltered,L"unclipped-visibility-filtered",false},
    {Mode::RtSpotLightIds,L"light-ids",false},
    {Mode::RoomHitX,L"hit-x",false},
    {Mode::RoomHitY,L"hit-y",false},
    {Mode::RoomHitZ,L"hit-z",false}
};
inline constexpr unsigned count=sizeof(steps)/sizeof(steps[0]);
inline constexpr unsigned imagesPerStep=3;
// One stable RR-off pose for blocker identity and current filtered visibility.
inline constexpr Step blockerStep{Mode::RoomCorrectedNoRR,L"shadow-blocker",true};
inline constexpr Mode BlockerMode(Mode selected) noexcept {
    return selected==Mode::RoomCorrectedMapPriority?selected:blockerStep.mode;
}
inline constexpr uint64_t blockerSettleMs=5000;
inline bool BlockerSettled(uint64_t elapsed,uint64_t frames,bool observed) noexcept {
    return elapsed>=blockerSettleMs && frames>=90 && observed;
}
// Shift+F9 collects a bounded motion trace in the accepted default. Never
// reselect between samples: that would reset history during the measured walk.
inline constexpr Step transitionStep{kDefaultMode,L"transition-default",true};
inline constexpr unsigned transitionSamples=6;
inline constexpr uint64_t transitionSpacingMs=4000,transitionTimeoutMs=60000;
inline bool TransitionSettled(uint64_t elapsed,uint64_t frames,bool observed,
                              uint64_t sinceLastRequest,bool first) noexcept {
    return elapsed>=1000 && frames>=30 && observed && (first||sinceLastRequest>=transitionSpacingMs);
}
inline constexpr uint64_t settleMs=2500,settleFrames=90,timeoutMs=20000,burstIntervalMs=200;
// GI-only changes frame policy, not a direct-light PSO. Do not wait for the
// RT replacement's binding marker when that replacement is intentionally off.
inline bool Applied(Mode mode,unsigned observed,bool rrSelected) noexcept {
    if(BypassesRR(mode) && rrSelected)return false;
    if(mode==Mode::Native || mode==Mode::RoomNative || mode==Mode::RoomNativeNoRR || mode==Mode::RoomGi)return true;
    return (observed&RequiredBinding(mode))==RequiredBinding(mode);
}
inline bool Settled(uint64_t elapsed,uint64_t frames,bool observed) noexcept {
    return elapsed>=settleMs && frames>=settleFrames && observed;
}
}
