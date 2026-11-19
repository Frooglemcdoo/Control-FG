#pragma once
#include <cstdint>
#include <algorithm>
namespace gi_reuse {
inline std::uint64_t GIAutoPairBytes(unsigned width,unsigned height) noexcept {return std::uint64_t(width)*height*32+std::uint64_t((std::min)(width,512u))*(std::min)(height,512u)*192;}
inline std::uint64_t GIAutoWeightBytes(unsigned width,unsigned height,unsigned rays) noexcept {
 return (std::uint64_t(width)*height*rays*4+31)&~std::uint64_t(31);
}
}
