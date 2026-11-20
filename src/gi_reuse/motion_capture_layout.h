#pragma once
#include <cstdint>
namespace gi_motion {
inline constexpr unsigned Frames=16,StopCue=4,RayStride=224,HeaderBytes=2048;
inline constexpr unsigned ControlBit=33554432u;
inline std::uint64_t Bytes(unsigned w,unsigned h,unsigned rays) noexcept {
 return std::uint64_t(w)*h*rays*RayStride+HeaderBytes;
}
// Four overlapping GPU recordings, with readbacks retained until safe export.
// Normal gameplay keeps its original 16 GiB admission ceiling.
inline constexpr std::uint64_t NormalOwnerBudget=16ull*1024*1024*1024;
inline constexpr std::uint64_t MaxOwnerBudget=40ull*1024*1024*1024;
inline constexpr unsigned InFlight=4;
inline std::uint64_t OwnerAllowance(unsigned w,unsigned h,unsigned rays) noexcept {
 return 2*Bytes(w,h,rays)+512ull*1024*1024;
}
inline std::uint64_t OwnerBudget(unsigned w,unsigned h,unsigned rays) noexcept {
 return InFlight*OwnerAllowance(w,h,rays)+512ull*1024*1024;
}
inline bool Addressable(unsigned w,unsigned h,unsigned rays,std::uint64_t base) noexcept {
 return w&&h&&(rays==4||rays==10)&&base+Bytes(w,h,rays)<=0xffffffe0ull;
}
// No crops, subsampling, frame skipping, or precision reduction to meet a budget.
inline std::uint64_t Reservation(unsigned w,unsigned h,unsigned rays,std::uint64_t images) noexcept {
 const auto hit=std::uint64_t((w*16+255)&~255u)*h*3*rays+std::uint64_t((w*8+255)&~255u)*h+2048;
 const auto trace=Bytes(w,h,rays);
 return Frames*(images+hit+trace+128ull*1024*1024)+InFlight*(trace+256ull*1024*1024)+4ull*1024*1024*1024;
}
}
