#pragma once
#include <cstdint>
namespace control_rr_part1 {
inline constexpr std::uint64_t MaxReadbackBytes=65536u*8u;
inline bool CopyLayoutAllowed(std::uint64_t bytes,std::uint64_t stride,
                              unsigned state,unsigned heap,unsigned mode) noexcept {
    return bytes && bytes<=MaxReadbackBytes && stride==8 && bytes%8==0 &&
        state==0xC0 && heap==1 && mode==2;
}
enum class CopyOp { ToCopySource, CopyBytes, Restore };
struct CopyProgress {
    bool started=false, initial=false, copy=false, restoreAttempted=false, restored=false;
};
// Each operation must contain its own platform exception boundary and return
// false on ambiguity. Never retry a failed initial/final barrier.
template<class Emit>
bool RecordCopy(Emit&& emit,CopyProgress& p) noexcept {
    p={};p.started=true;
    if(!emit(CopyOp::ToCopySource)) return false;
    p.initial=true;
    p.copy=emit(CopyOp::CopyBytes);
    p.restoreAttempted=true;
    p.restored=emit(CopyOp::Restore);
    return p.copy && p.restored;
}
inline bool CanMap(bool copied,std::uint64_t fence) noexcept {
    return copied && fence!=UINT64_MAX && fence>=1;
}
} // namespace control_rr_part1
