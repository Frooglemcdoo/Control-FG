#pragma once
// Audited diffuse-000 SHA d54d...: history weight = (1-CameraCut)*0.7
// times depth/motion confidence. Keep native energy clamp and spatial passes.
static bool rrNativeGIHistoryZeroThisFrame=false;
static thread_local bool rrNativeGIHistoryScope=false;
static thread_local unsigned rrNativeGIHistoryDispatches=0;
static thread_local control_rr_native_clamp::Lease rrNativeGIHistoryLease{};
static RRAlbedoCallPatch rrNativeGITemporalBindPatch{};
static RRIndirectCallPatch rrNativeGITemporalDispatchPatch{};
static bool RRNativeGITemporalIdentity(void* record) noexcept {
 static void* cached=nullptr;static RRAlbedoShader::NativeDescriptor cachedDescriptor{};
 const size_t slots[]{0x10,0x30,0x50,0x70,0x90};
 for(auto slot:slots){
  RRAlbedoShader::NativeDescriptor d{};
  if(!RRAlbedoShader::ReadDescriptor(reinterpret_cast<std::uintptr_t>(record),slot,&d)||d.size!=5076)continue;
  if(record==cached&&d.shaderBase==cachedDescriptor.shaderBase&&d.identifier==cachedDescriptor.identifier&&d.stage==cachedDescriptor.stage)return true;
  unsigned char bytes[5076]{};char hash[65]{};
  if(!RRAlbedoShader::CopyResident(&d,bytes,sizeof(bytes))||!PTReflectionSha256(bytes,sizeof(bytes),hash))return false;
  if(std::strcmp(hash,"d54d94fdbb783796cc33bc78ab20dde687d90d9cba6f24c8092a6b177eec5d15"))return false;
  cached=record;cachedDescriptor=d;return true;
 }
 return false;
}
static void RRNativeHookGITemporalBind(void* record){
 if(!rrNativeGIHistoryScope){rrNativeTemporalBindOriginal(record);return;}
 const DWORD saved=GetLastError();
 if(!RRNativeGITemporalIdentity(record)||!rrNativeClampApi.Arm(rrNativeGIHistoryLease)){
  rrNativeFrame.Fail("gi_temporal_identity_or_arm_failed");
  SetLastError(saved);rrNativeTemporalBindOriginal(record);return;
 }
 SetLastError(saved);
 rrNativeTemporalBindOriginal(record);
 rrNativeGIHistoryLease.bindReturned=true;
}
static void RRNativeHookGITemporalDispatch(void* state,const void* groups){
 if(!rrNativeGIHistoryScope){rrNativeDispatchOriginal(state,groups);return;}
 const DWORD saved=GetLastError();
 const bool forced=rrNativeGIHistoryLease.bindReturned&&rrNativeClampApi.StillForced(rrNativeGIHistoryLease);
 if(!forced)rrNativeFrame.Fail("gi_temporal_dispatch_not_forced");
 SetLastError(saved);
 __try {
  rrNativeDispatchOriginal(state,groups);
  if(forced)++rrNativeGIHistoryDispatches;
 } __finally {
  const DWORD nativeError=GetLastError();
  if(!rrNativeClampApi.Restore(rrNativeGIHistoryLease))rrNativeFrame.Fail("gi_temporal_restore_failed");
  SetLastError(nativeError);
 }
}
static void RRNativeRunGIFilter(void* color,void* history,unsigned passes){
 const bool eligible=rrNativeFrameEnabled&&rrNativeGIKeepThisFrame&&rrNativeGIHistoryZeroThisFrame&&
  rrNativeFrame.Selected()&&rrNativeFrame.Stage()==control_rr::RRFrameStage::Lighting&&color&&passes&&
  reinterpret_cast<std::uintptr_t>(history)==reinterpret_cast<std::uintptr_t>(verifiedRenderer)+0x90eac8;
 if(!eligible||rrNativeGIHistoryScope){rrNativeDiffuseFilterOriginal(color,history,passes);return;}
 rrNativeGIHistoryScope=true;rrNativeGIHistoryLease={};rrNativeGIHistoryDispatches=0;
 __try {rrNativeDiffuseFilterOriginal(color,history,passes);}
 __finally {
  const DWORD saved=GetLastError();
  const bool restored=rrNativeClampApi.Restore(rrNativeGIHistoryLease);
  const bool okay=restored&&rrNativeGIHistoryDispatches==1;
  rrNativeGIHistoryScope=false;
  if(!okay)rrNativeFrame.Fail("gi_temporal_scope_incomplete");
  static unsigned long long calls=0; ++calls;
  if(calls<=4||calls%240==0||!okay)
   native_lighting::Trace("gi_temporal_zero build=R208 applied=%u calls=%llu dispatches=%u restored=%u spatial_passes=%u history_clear=0",unsigned(okay),calls,rrNativeGIHistoryDispatches,unsigned(restored),passes);
  SetLastError(saved);
 }
}
