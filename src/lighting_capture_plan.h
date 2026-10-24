#pragma once
#include "native_lighting_live_state.h"
namespace native_lighting::capture_plan {
struct Step { Mode mode; const wchar_t* name; bool gpu; };
inline constexpr Step steps[]={
    {Mode::Native,L"native-user-rr",false},
    {Mode::RoomNativeNoRR,L"native-rr-off",false},
    {Mode::RoomBoth,L"both",true},
    {Mode::RoomAuthoredClip,L"default-clip-user-rr",false},
    {Mode::RoomClipNoRR,L"default-clip-rr-off",false},
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
