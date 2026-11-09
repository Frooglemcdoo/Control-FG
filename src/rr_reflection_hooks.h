#pragma once
// This exact internal helper consumes RCX only. Its callsite ignores RAX, but
// preserve the returned bits anyway. No common-helper or GI callsite patch.
using RRReflectionBindFn=std::uintptr_t (*)(void*);
static RRReflectionBindFn rrReflectionOriginalBind=nullptr;
static RRAlbedoCallPatch rrReflectionCallPatch{};
static bool rrReflectionCaptureEnabled=false;
#include "rr_reflection_handoff_windows.h"
static std::uintptr_t RRReflectionHookBind(void* owner) {
 const auto result=rrReflectionOriginalBind(owner); // exactly once; native exceptions propagate
 const DWORD saved=GetLastError();
 if(rrReflectionCaptureEnabled&&control_rr::RRUserRuntimeWorkRequested()&&control_rr::RRUserPresetValue()==control_rr::RRPresetF)RRReflectionAfterBind(owner);
 SetLastError(saved);return result;
}
static bool RRReflectionInitializeAccessOnly(HMODULE d3d) noexcept {
 if(!d3d||d3d!=verifiedD3d)return false;
 rrReflectionCaptureEnabled=false;
 const bool ready=rrReflectionAccess.Initialize(d3d);
 if(ready) Log("RR_TEMPORAL_ACCESS_READY native_texture_getters=1 reflection_geometry_capture=0 primary_queue_hook=0 hit_distance_runtime=0 specular_mvec_runtime=0");
 return ready;
}
static bool RRReflectionInstall(HMODULE renderer,HMODULE d3d) noexcept {
 if(!renderer||renderer!=verifiedRenderer||d3d!=verifiedD3d||!rrReflectionAccess.Initialize(d3d))return false;
 auto* base=reinterpret_cast<unsigned char*>(renderer);
 // Also verify the owner selection preceding the exact E8 instruction.
 const unsigned char ownerBytes[]{0x49,0x8b,0x0f,0x48,0x8b,0x89,0x38,0x02,0x00,0x00};
 if(memcmp(base+0x129658,ownerBytes,sizeof(ownerBytes)))return false;
 rrReflectionOriginalBind=reinterpret_cast<RRReflectionBindFn>(base+control_rr_reflection::BindFunctionRva);
 if(!RRAlbedoPrepareCallPatch(base+control_rr_reflection::BindCallRva,reinterpret_cast<void*>(rrReflectionOriginalBind),
   reinterpret_cast<void*>(&RRReflectionHookBind),&rrReflectionCallPatch))return false;
 if(!RRAlbedoExchangeCall(&rrReflectionCallPatch,true))return false;
 if(!rrReflectionCallPatch.writeHealthy){
  if(!RRAlbedoExchangeCall(&rrReflectionCallPatch,false))Log("RR_REFLECTION_HOOK_ROLLBACK_FAILED");
  return false;
 }
 if(!RRReflectionInstallPrimaryAppend(d3d)){
  if(!RRAlbedoExchangeCall(&rrReflectionCallPatch,false))Log("RR_REFLECTION_HOOK_ROLLBACK_FAILED");
  return false;
 }
 rrReflectionCaptureEnabled=true;
 Log("RR_REFLECTION_HOOK_INSTALLED callsite=0x129662 helper=0xA3940 copy_only=1 rr_eval=disabled");return true;
}
