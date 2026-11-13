#pragma once
#include <atomic>
#include <cstdint>
#include "rr_gi_stage_policy.h"
namespace gi_path_capture {
// Written by the UI before enabled is published. Jobs snapshot all settings.
inline std::atomic<bool> enabled{false};
inline std::atomic<bool> timedSequence{false};
inline std::atomic<unsigned> sequencePhase{0}; // 0 stationary, 1 moving
inline wchar_t directory[260]{};
inline std::atomic<unsigned> condition{0}; // 1 raw, 2 Stable, 3 unchanged raw motion pair, 4 unchanged BOTH motion pair
inline std::atomic<unsigned> validFrames[5]{},invalidFrames[5]{};
inline constexpr unsigned stride=4;
inline unsigned ExportStride(unsigned c) noexcept {return (c==3||c==4)?1:stride;}
inline bool PairAligned(unsigned c,unsigned index,std::uint64_t previous,std::uint64_t frame) noexcept {return (c!=3&&c!=4)||index==0||(previous&&frame==previous+1);}
// Admit the exact requested path; never relax RAW into accepting filtered GI.
inline bool BoundaryMatches(unsigned captureCondition,bool valid,bool applied,std::uint64_t observedFrame,
 std::uint64_t frame,control_rr_gi::StageMode stage,unsigned requested,unsigned observed) noexcept {
 if(!valid||!applied||observedFrame!=frame)return false;
 if(captureCondition==4)return stage==control_rr_gi::StageMode::Both&&requested>0&&observed==requested;
 if(captureCondition==2)return stage==control_rr_gi::StageMode::BothSteadySpatial&&requested>0&&observed==requested;
 if(captureCondition==0||captureCondition==1||captureCondition==3)return stage==control_rr_gi::StageMode::RawBypass&&!requested&&!observed;
 return false;
}
inline unsigned Extent(unsigned n,unsigned step) noexcept {return (n+step-1)/step;}
}
