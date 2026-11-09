#pragma once
#include <cstdint>
namespace rt_lights {
// D3D12_DISPATCH_RAYS_DESC occupies 104 bytes on the Windows x64 ABI.
// Leave an explicit 8-byte gap before the two compute dispatch arguments.
struct IndirectLayout {
    static constexpr std::uint32_t Rays=0, RayDimensions=88;
    static constexpr std::uint32_t Extract=112, Pack=124, Bytes=136;
};
}
