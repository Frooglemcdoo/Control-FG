#pragma once
// Audited diffuse-000: history weight = (1-CameraCut)*0.7 times confidence.
// Preserve the current-frame energy clamp and all native spatial passes.
static bool rrNativeGIHistoryZeroThisFrame=false;
static thread_local bool rrNativeGIHistoryScope=false;
static thread_local unsigned rrNativeGIHistoryDispatches=0;
static thread_local const char* rrNativeGIHistoryReason="idle";
static thread_local control_rr_native_clamp::Lease rrNativeGIHistoryLease{};
static RRAlbedoCallPatch rrNativeGITemporalBindPatch{};
static RRIndirectCallPatch rrNativeGITemporalDispatchPatch{};
static bool RRNativeGITemporalIdentity(void* record) noexcept {
 // renderer+1db750 loads seven descriptor slots at 0x10 strides, not 0x20.
 // Compute is slot 0x60 / stage 5 (also used by the existing composite loader).
 static void* cached=nullptr;static RRAlbedoShader::NativeDescriptor cachedDescriptor{};
 RRAlbedoShader::NativeDescriptor d{},after{};
 rrNativeGIHistoryReason="compute_descriptor";
 if(!RRAlbedoShader::ReadDescriptor(reinterpret_cast<std::uintptr_t>(record),0x60,&d))return false;
 static unsigned reports=0;
 if(reports++<4)native_lighting::Trace("gi_temporal_identity build=R209 slot=0x60 stage=%u id=%d bytes=%llu",d.stage,d.identifier,static_cast<unsigned long long>(d.size));
 rrNativeGIHistoryReason="compute_contract";
 if(d.stage!=5||d.identifier<0||d.size!=5076)return false;
 if(record==cached&&d.shaderBase==cachedDescriptor.shaderBase&&d.identifier==cachedDescriptor.identifier&&d.stage==cachedDescriptor.stage)return true;
 unsigned char bytes[5076]{};char hash[65]{};
 rrNativeGIHistoryReason="resident_copy";
 if(!RRAlbedoShader::CopyResident(&d,bytes,sizeof(bytes)))return false;
 rrNativeGIHistoryReason="descriptor_changed";
 if(!RRAlbedoShader::ReadDescriptor(reinterpret_cast<std::uintptr_t>(record),0x60,&after)||
    d.shaderBase!=after.shaderBase||d.identifier!=after.identifier||d.stage!=after.stage||d.size!=after.size)return false;
 rrNativeGIHistoryReason="shader_hash";
 if(!PTReflectionSha256(bytes,sizeof(bytes),hash))return false;
 if(std::strcmp(hash,"d54d94fdbb783796cc33bc78ab20dde687d90d9cba6f24c8092a6b177eec5d15")){
  if(reports<=4)native_lighting::Trace("gi_temporal_identity build=R209 rejected_sha256=%s",hash);
  return false;
 }
 cached=record;cachedDescriptor=d;return true;
}
static bool RRNativeGIArm() noexcept {
 // Keep the snapshot live even when a write succeeds but verification fails.
 auto& lease=rrNativeGIHistoryLease;
 rrNativeGIHistoryReason="camera_cut_resolve";
 if(!rrNativeClampApi.ResolveCameraCut(lease.provider))return false;
 rrNativeGIHistoryReason="camera_cut_snapshot";
 if(!rrNativeClampApi.ReadCurrent(lease.provider,lease.original)||lease.original>1)return false;
 lease.armed=true;
 rrNativeGIHistoryReason="camera_cut_force";
 if(!rrNativeClampApi.SetVerified(lease.provider,1)){
  if(!rrNativeClampApi.Restore(lease))rrNativeFrame.Fail("gi_temporal_arm_restore_failed");
  return false;
 }
 return true;
}
static void RRNativeHookGITemporalBind(void* record){
 if(!rrNativeGIHistoryScope){rrNativeTemporalBindOriginal(record);return;}
 const DWORD saved=GetLastError();
 if(!RRNativeGITemporalIdentity(record)||!RRNativeGIArm()){
  // Optional experiment rejected: native GI filtering and RR remain active.
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
 if(rrNativeGIHistoryLease.bindReturned&&!forced)rrNativeGIHistoryReason="dispatch_not_forced";
 SetLastError(saved);
 __try {
  rrNativeDispatchOriginal(state,groups);
  if(forced){++rrNativeGIHistoryDispatches;rrNativeGIHistoryReason="applied";}
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
 rrNativeGIHistoryScope=true;rrNativeGIHistoryLease={};rrNativeGIHistoryDispatches=0;rrNativeGIHistoryReason="no_temporal_bind";
 __try {rrNativeDiffuseFilterOriginal(color,history,passes);}
 __finally {
  const DWORD saved=GetLastError();
  const bool restored=rrNativeClampApi.Restore(rrNativeGIHistoryLease);
  const bool okay=restored&&rrNativeGIHistoryDispatches==1;
  rrNativeGIHistoryScope=false;
  if(!restored)rrNativeFrame.Fail("gi_temporal_scope_restore_failed");
  static unsigned long long calls=0; ++calls;
  if(calls<=4||calls%240==0)
   native_lighting::Trace("gi_temporal_zero build=R209 applied=%u calls=%llu dispatches=%u restored=%u spatial_passes=%u history_clear=0 reason=%s rr_selected=%u",unsigned(okay),calls,rrNativeGIHistoryDispatches,unsigned(restored),passes,rrNativeGIHistoryReason,unsigned(rrNativeFrame.Selected()));
  SetLastError(saved);
 }
}
