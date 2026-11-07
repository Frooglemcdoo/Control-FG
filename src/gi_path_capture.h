#pragma once
#include <atomic>
#include <cstdint>
#include "rr_gi_stage_policy.h"
namespace gi_path_capture {
// Written by the UI before enabled is published. Jobs snapshot all settings.
inline std::atomic<bool> enabled{false};
inline wchar_t directory[260]{};
inline std::atomic<unsigned> condition{0}; // 1 raw, 2 Stable
inline std::atomic<unsigned> validFrames[3]{},invalidFrames[3]{};
inline constexpr unsigned stride=4;
// Admit the exact requested path; never relax RAW into accepting filtered GI.
inline bool BoundaryMatches(unsigned condition,bool valid,bool applied,std::uint64_t observedFrame,
 std::uint64_t frame,control_rr_gi::StageMode stage,unsigned requested,unsigned observed) noexcept {
 if(!valid||!applied||observedFrame!=frame)return false;
 if(condition==2)return stage==control_rr_gi::StageMode::BothSteadySpatial&&requested>0&&observed==requested;
 if(condition==0||condition==1)return stage==control_rr_gi::StageMode::RawBypass&&!requested&&!observed;
 return false;
}
inline unsigned Extent(unsigned n,unsigned step) noexcept {return (n+step-1)/step;}
}
