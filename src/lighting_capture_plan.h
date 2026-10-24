#pragma once
#include "native_lighting_live_state.h"
namespace native_lighting::capture_plan {
struct Step { Mode mode; const wchar_t* name; bool gpu; };
inline constexpr Step steps[]={
    {Mode::Native,L"native",false},
    {Mode::RoomBoth,L"both",true},
    {Mode::RoomAuthoredClip,L"default-clip",false},
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
inline bool Settled(uint64_t elapsed,uint64_t frames,bool observed) noexcept {
    return elapsed>=settleMs && frames>=settleFrames && observed;
}
}
