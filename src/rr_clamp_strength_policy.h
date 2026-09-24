#pragma once
#include <cstdint>
#include <atomic>
namespace control_rr_clamp {
inline std::atomic<unsigned> requested{60},effective{100};
inline std::atomic<bool> applied{true};
inline unsigned Requested() noexcept {return requested.load(std::memory_order_acquire);}
inline constexpr unsigned VariantCount=51;
inline bool Valid(unsigned v) noexcept {return v>=25&&v<=75;}
inline unsigned Normalize(unsigned v) noexcept {return Valid(v)?v:60;}
inline int Variant(unsigned v) noexcept {return Valid(v)?static_cast<int>(v-25):-1;}
inline unsigned FromX(int x) noexcept {if(x<=30)return 25;if(x>=710)return 75;return 25+static_cast<unsigned>((x-30)*50+340)/680;}
inline int ToX(unsigned v) noexcept {return 30+static_cast<int>(Normalize(v)-25)*680/50;}
inline bool Admit(bool scoped,bool listMatches,bool originalObserved,bool variantReady,unsigned dispatchCount) noexcept {
 return scoped&&listMatches&&originalObserved&&variantReady&&dispatchCount==1;
}
struct History {
 unsigned value=100;bool initialized=false;
 bool Update(unsigned next) noexcept {const bool changed=!initialized||value!=next;value=next;initialized=true;return changed;}
};
}

