#pragma once
#include <atomic>
#include "gi_auto_capture_control.h"
#include <d3d12.h>
namespace gi_auto_dispatch {
// True when the latest GI ray-count query came from a full-RR frame. Owned GI
// only pairs producer/evaluator on those frames; the startup frames run the
// native count with no matching evaluator and must be left alone.
inline std::atomic<bool> fullRRRays{false};
inline bool Enabled()noexcept {
#ifdef CONTROL_GI_AUTO_TARGETS
 static const bool enabled=[] {char b[4]{};const auto n=GetEnvironmentVariableA("CONTROLTOH_GI_AUTO",b,4);return n==0||(n==1&&b[0]=='1');}();return enabled;
#else
 return false;
#endif
}
inline UINT LaunchRays()noexcept {
 static const UINT rays=[] {char value[8]{};const auto n=GetEnvironmentVariableA("CONTROLTOH_GI_AUTO_RAYS",value,8);
  // CONTROL_GI_DEFAULT_RAYS selects the launch default (R495: 4). 2/4/10 may be forced.
#ifndef CONTROL_GI_DEFAULT_RAYS
#define CONTROL_GI_DEFAULT_RAYS 4u
#endif
  return !n?UINT(CONTROL_GI_DEFAULT_RAYS):(n==1&&value[0]=='2')?2u:(n==1&&value[0]=='4')?4u:(n==2&&value[0]=='1'&&value[1]=='0'?10u:0u);}();
 return Enabled()?rays:0;
}
inline std::atomic<void(*)(bool)noexcept> bypassed{nullptr};
inline void Bypassed(bool evaluator)noexcept {if(const auto callback=bypassed.load())callback(evaluator);}
inline std::atomic<bool(*)(ID3D12RootSignature*)noexcept> rootReady{nullptr};
inline std::atomic<bool(*)(ID3D12GraphicsCommandList4*,const D3D12_DISPATCH_RAYS_DESC*,ID3D12StateObject*)noexcept> ray{nullptr};
inline std::atomic<bool(*)(ID3D12GraphicsCommandList*,UINT,UINT,UINT)noexcept> shade{nullptr};
}
