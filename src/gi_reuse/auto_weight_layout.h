#pragma once
#include <cstdint>
namespace gi_reuse {
inline std::uint64_t GIAutoWeightBytes(unsigned width,unsigned height,unsigned rays) noexcept {
 return (std::uint64_t(width)*height*rays*4+31)&~std::uint64_t(31);
}
}
