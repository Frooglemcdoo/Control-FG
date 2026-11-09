#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
namespace control_rr {
using Jump5=std::array<std::uint8_t,5>;
using Tail14=std::array<std::uint8_t,14>;
inline bool RelativeJump(std::uint64_t site,std::uint64_t target,Jump5& out) noexcept {
    out={};if(site>UINT64_MAX-5) return false;
    const auto next=site+5;std::int64_t distance;
    if(target>=next) {
        if(target-next>INT32_MAX) return false;
        distance=static_cast<std::int64_t>(target-next);
    } else {
        if(next-target>UINT64_C(2147483648)) return false;
        distance=-static_cast<std::int64_t>(next-target);
    }
    out[0]=0xe9;const auto rel=static_cast<std::int32_t>(distance);
    std::memcpy(out.data()+1,&rel,4);return true;
}
inline bool CheckedTailPatch(const Jump5& original,std::uint64_t site,
    std::uint64_t expectedTarget,std::uint64_t relay,Jump5& out) noexcept {
    out={};Jump5 expected{};
    if(!RelativeJump(site,expectedTarget,expected) || original!=expected) return false;
    return RelativeJump(site,relay,out);
}
inline bool AbsoluteTail(std::uint64_t target,Tail14& out) noexcept {
    out={};if(!target) return false;
    // JMP [RIP+0] followed by the address. Preserves RCX/RDX/R8/R9, stack,
    // return address and all general/vector registers; no trampoline prologue.
    out[0]=0xff;out[1]=0x25;std::memcpy(out.data()+6,&target,8);return true;
}
inline constexpr std::uint32_t EvaluationTailSites[]={0x1d30c,0x1e156};
inline constexpr std::uint32_t EvaluationCExport=0x52a20;
// This header builds checked bytes only. It does not patch a process or enable RR.
}
