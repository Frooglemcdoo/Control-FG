#pragma once
#include <atomic>
#include "gi_auto_capture_control.h"
#include <d3d12.h>
namespace gi_auto_dispatch {
inline bool Enabled()noexcept {
#ifdef CONTROL_GI_AUTO_TARGETS
 static const bool enabled=[] {char b[4]{};return GetEnvironmentVariableA("CONTROLTOH_GI_AUTO",b,4)==1&&b[0]=='1';}();return enabled;
#else
 return false;
#endif
}
inline std::atomic<bool(*)(ID3D12RootSignature*)noexcept> rootReady{nullptr};
inline std::atomic<bool(*)(ID3D12GraphicsCommandList4*,const D3D12_DISPATCH_RAYS_DESC*,ID3D12StateObject*)noexcept> ray{nullptr};
inline std::atomic<bool(*)(ID3D12GraphicsCommandList*,UINT,UINT,UINT)noexcept> shade{nullptr};
}
