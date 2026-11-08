#pragma once
#include <cstdint>
#include <atomic>
#include "settings_check_state.h"
#include "rr_suite_policy.h"
namespace control_rr_clamp {
inline constexpr unsigned kReflectionDetailPercent = 60;
inline std::atomic<unsigned> effective{100};
inline std::atomic<bool> applied{true};
enum class Mode : unsigned { Native, Override60, Bypass };
inline std::atomic<Mode> requestedMode{Mode::Override60};
inline Mode Next(Mode mode) noexcept {
 return mode==Mode::Native?Mode::Override60:mode==Mode::Override60?Mode::Bypass:Mode::Native;
}
inline constexpr unsigned Requested() noexcept {return kReflectionDetailPercent;}
inline unsigned RuntimeRequested() noexcept {
#ifdef CONTROL_RR_F9_SUITE
 const auto* step=rr_suite::Current();if(step)return step->raw?0:Requested();
#endif
 const auto mode=requestedMode.load(std::memory_order_acquire);
 return mode==Mode::Native?100u:mode==Mode::Bypass?0u:Requested();
}
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

