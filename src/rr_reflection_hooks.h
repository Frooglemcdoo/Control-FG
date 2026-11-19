#pragma once
// This exact internal helper consumes RCX only. Its callsite ignores RAX, but
// preserve the returned bits anyway. No common-helper or GI callsite patch.
using RRReflectionBindFn=std::uintptr_t (*)(void*);
static RRReflectionBindFn rrReflectionOriginalBind=nullptr;
static RRAlbedoCallPatch rrReflectionCallPatch{};
static bool rrReflectionCaptureEnabled=false;
#include "rr_reflection_handoff_windows.h"
#ifdef CONTROL_REBLUR_ONLY
#include "nrd_specular_handoff.h"
static thread_local control_nrd::SpecularHandoff rrReblurHandoff;
static void (*rrReblurLightBindOriginal)(void*)=nullptr;
static RRAlbedoCallPatch rrReblurLightBindPatch{};
static void RRReblurLightBind(void* shader) {
 rrReblurLightBindOriginal(shader);
 const DWORD saved=GetLastError();
 control_rr_reflection::Context current{};
 const bool valid=RRReflectionCurrent(nullptr,current);
 const auto owner=rrReblurHandoff.Take(valid?current:control_rr_reflection::Context{});
 if(owner)if(const auto callbacks=control_nrd_bridge::specular.load())callbacks->produce(reinterpret_cast<void*>(owner));
 SetLastError(saved);
}
#endif
static std::uintptr_t RRReflectionHookBind(void* owner) {
 const auto result=rrReflectionOriginalBind(owner); // exactly once; native exceptions propagate
 const DWORD saved=GetLastError();
#ifdef CONTROL_REBLUR_ONLY
 // This helper binds providers before the reflection lighting pipeline.
 // Inject only after its later lighting bind has established native GPU state.
 control_rr_reflection::Context context{};
 rrReblurHandoff.Arm(reinterpret_cast<std::uintptr_t>(owner),RRReflectionCurrent(nullptr,context)?context:control_rr_reflection::Context{});
#else
 if(rrReflectionCaptureEnabled&&control_rr::RRUserRuntimeWorkRequested()&&control_rr::RRUserPresetValue()==control_rr::RRPresetF)RRReflectionAfterBind(owner);
#endif
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
#ifdef CONTROL_REBLUR_ONLY
 rrReblurLightBindOriginal=reinterpret_cast<void(*)(void*)>(base+0x1db750);
 if(!RRAlbedoPrepareCallPatch(base+0x1296f0,reinterpret_cast<void*>(rrReblurLightBindOriginal),reinterpret_cast<void*>(&RRReblurLightBind),&rrReblurLightBindPatch))return false;
#endif
 if(!RRAlbedoExchangeCall(&rrReflectionCallPatch,true))return false;
 if(!rrReflectionCallPatch.writeHealthy){
  if(!RRAlbedoExchangeCall(&rrReflectionCallPatch,false))Log("RR_REFLECTION_HOOK_ROLLBACK_FAILED");
  return false;
 }
#ifdef CONTROL_REBLUR_ONLY
 if(!RRAlbedoExchangeCall(&rrReblurLightBindPatch,true)||!rrReblurLightBindPatch.writeHealthy){
  RRAlbedoExchangeCall(&rrReblurLightBindPatch,false);
  RRAlbedoExchangeCall(&rrReflectionCallPatch,false);
  return false;
 }
 Log("REBLUR_BUILD reflection_guides=lighting_bind producer=0x129662 consumer=0x1296f0");
#else
 if(!RRReflectionInstallPrimaryAppend(d3d)){
  if(!RRAlbedoExchangeCall(&rrReflectionCallPatch,false))Log("RR_REFLECTION_HOOK_ROLLBACK_FAILED");
  return false;
 }
 rrReflectionCaptureEnabled=true;
#endif
 Log("RR_REFLECTION_HOOK_INSTALLED callsite=0x129662 helper=0xA3940 copy_only=1 rr_eval=disabled");return true;
}
