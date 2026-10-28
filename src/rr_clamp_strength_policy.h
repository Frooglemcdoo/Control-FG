#pragma once
#include <cstdint>
#include <atomic>
#include "rr_suite_policy.h"
namespace control_rr_clamp {
inline constexpr unsigned kReflectionDetailPercent = 60;
inline std::atomic<unsigned> effective{100};
inline std::atomic<bool> applied{true};
#ifndef CONTROL_RR_F9_SUITE
inline constexpr unsigned Requested() noexcept {return kReflectionDetailPercent;}
#else
inline unsigned Requested() noexcept {
#ifdef CONTROL_RR_F9_SUITE
 const auto* step=rr_suite::Current();if(step&&step->raw)return 0;
#endif
 return kReflectionDetailPercent;
}
#endif
inline constexpr unsigned VariantCount=1;
inline int Variant(unsigned v) noexcept {return v==kReflectionDetailPercent?0:-1;}
inline bool Admit(bool scoped,bool listMatches,bool originalObserved,bool variantReady,unsigned dispatchCount) noexcept {
 return scoped&&listMatches&&originalObserved&&variantReady&&dispatchCount==1;
}
struct History {
 unsigned value=100;bool initialized=false;
 bool Update(unsigned next) noexcept {const bool changed=!initialized||value!=next;value=next;initialized=true;return changed;}
};
}

