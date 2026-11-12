#pragma once
#ifdef CONTROL_GI_LONG_SEQUENCE
#include "gi_native_scene_window.h"
#include "gi_reuse/d3d12_native_gi_recording.h"
// Exact-build borrowing contract. This adapter covers native scene preparation
// and frame-retired resources/descriptors; it does NOT make mesh source bytes
// immutable. The invocation must independently prepare private hit-source,
// SBT and constant copies and retain its original native heap/fence lease.
namespace gi_frozen_scene_contract {
inline bool supported=false;
inline unsigned char* module=nullptr;
inline bool Install(HMODULE renderer,HMODULE d3d)noexcept {
 if(supported)return renderer==verifiedRenderer&&d3d==verifiedD3d;
 if(renderer!=verifiedRenderer||d3d!=verifiedD3d||!renderer||!d3d||
    !HashMatches(renderer,kRendererHash)||!HashMatches(d3d,kD3dHash)||
    !gi_native_scene_window::ready.load()||!GINativeSceneOwners::ready.load())return false;
 const auto* r=reinterpret_cast<const unsigned char*>(renderer);
 // Both complete native invocations, not just a late writer call, must remain
 // routed through our entry/exit adapters. The join owns the exact native job.
 if(gi_native_render_scope::original!=reinterpret_cast<gi_native_render_scope::Native>(const_cast<unsigned char*>(r)+0x13e860)||
    gi_native_prepare_scope::original!=reinterpret_cast<gi_native_prepare_scope::Native>(const_cast<unsigned char*>(r)+0x13de60)||
    gi_native_scene_window::originalJoin!=reinterpret_cast<gi_native_scene_window::JoinFunction>(const_cast<unsigned char*>(r)+0xa8bf0)||
    *reinterpret_cast<void*const*>(r+0x631da0)!=reinterpret_cast<void*>(&gi_native_render_scope::Invoke)||
    *reinterpret_cast<void*const*>(r+0x632270)!=reinterpret_cast<void*>(&gi_native_prepare_scope::Invoke))return false;
 module=reinterpret_cast<unsigned char*>(d3d);supported=true;return true;
}
// POD-only native frame/retirement sample; no COM calls through borrowed data.
inline bool FrameCut(const gi_reuse::NativeGIRecording& recording)noexcept {
 if(!module||!recording.heaps.deviceFenceRecord||!recording.heaps.deviceFenceObject||!recording.heaps.deviceEpoch)return false;
 __try {
  if(*reinterpret_cast<const UINT64*>(module+0x136c68)!=recording.heaps.frame)return false;
  const auto* device=*reinterpret_cast<const unsigned char*const*>(module+0x136d28);
  if(!device||*reinterpret_cast<ID3D12Device*const*>(device+0x78)!=recording.device)return false;
  const auto* fence=*reinterpret_cast<const unsigned char*const*>(device+0x88);
  return reinterpret_cast<UINT64>(fence)==recording.heaps.deviceFenceRecord&&fence&&
   *reinterpret_cast<const UINT64*>(fence)==recording.heaps.deviceEpoch&&
   *reinterpret_cast<const UINT64*>(fence+8)==recording.heaps.deviceFenceObject;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
inline bool Validate(const gi_native_render_scope::Scope& scope,const gi_reuse::NativeGIRecording& recording)noexcept {
 if(!supported||!scope.token||!scope.frame||!scope.owner||!recording.list||!recording.queue||
    !recording.device||recording.heaps.frame!=scope.frame||!recording.heaps.deviceEpoch)return false;
 const auto& current=gi_native_render_scope::current;const auto& joined=gi_native_scene_window::joined;
 if(current.token!=scope.token||current.frame!=scope.frame||current.owner!=scope.owner||current.pass!=scope.pass||
    joined.token!=scope.token||joined.frame!=scope.frame||!joined.epoch)return false;
 void* scene=nullptr;if(!gi_native_scene_window::Scene(scope,&scene)||scene!=joined.scene)return false;
 const auto window=gi_native_scene_window::window.Inspect();
 // Waiting next-frame preparations are allowed: they are blocked BEFORE the
 // native graphics signal, material uploads and asynchronous scene job launch.
 return window.reader==scope.token&&window.epoch==joined.epoch&&!window.preparations&&FrameCut(recording);
}
}
#endif
