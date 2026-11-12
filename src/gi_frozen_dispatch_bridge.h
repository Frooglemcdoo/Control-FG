#pragma once
#include <atomic>
#include <d3d12.h>
namespace gi_frozen_dispatch {
using Ray=bool(*)(ID3D12GraphicsCommandList4*,const D3D12_DISPATCH_RAYS_DESC*,ID3D12StateObject*)noexcept;
using Shade=bool(*)(ID3D12GraphicsCommandList*,UINT,UINT,UINT)noexcept;
// Return true only when the corresponding native call must be suppressed.
// No bridge is installed until the coordinator has a native scene contract.
inline std::atomic<Ray> ray{nullptr};
inline std::atomic<Shade> shade{nullptr};
}
