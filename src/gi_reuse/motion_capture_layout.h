#pragma once
#include <cstdint>
namespace gi_motion {
inline constexpr unsigned Frames=16,StopCue=4,RayStride=224,HeaderBytes=2048;
inline constexpr unsigned ControlBit=33554432u;
inline std::uint64_t Bytes(unsigned w,unsigned h,unsigned rays) noexcept {
 return std::uint64_t(w)*h*rays*RayStride+HeaderBytes;
}
inline bool Addressable(unsigned w,unsigned h,unsigned rays,std::uint64_t base) noexcept {
 return w&&h&&(rays==4||rays==10)&&base+Bytes(w,h,rays)<=0xffffffe0ull;
}
// No crops, subsampling, frame skipping, or precision reduction to meet a budget.
inline std::uint64_t Reservation(unsigned w,unsigned h,unsigned rays,std::uint64_t images) noexcept {
 const auto hit=std::uint64_t((w*16+255)&~255u)*h*3*rays+std::uint64_t((w*8+255)&~255u)*h+2048;
 const auto trace=Bytes(w,h,rays);
 return Frames*(images+hit+trace+128ull*1024*1024)+2*trace+4ull*1024*1024*1024;
}
}
