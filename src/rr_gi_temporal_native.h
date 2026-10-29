#pragma once
// R210: native stable history, tighter per-pixel motion/depth rejection.
// Preserve the current-frame energy clamp and all native spatial passes.
static bool rrNativeGIHistoryZeroThisFrame=false;
static thread_local bool rrNativeGIHistoryScope=false;
static thread_local unsigned rrNativeGIHistoryDispatches=0;
static thread_local const char* rrNativeGIHistoryReason="idle";
static thread_local bool rrNativeGIMotionBound=false;
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
 if(reports++<4)native_lighting::Trace("gi_temporal_identity build=R210 slot=0x60 stage=%u id=%d bytes=%llu",d.stage,d.identifier,static_cast<unsigned long long>(d.size));
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
  if(reports<=4)native_lighting::Trace("gi_temporal_identity build=R210 rejected_sha256=%s",hash);
  return false;
 }
 cached=record;cachedDescriptor=d;return true;
}
// One immutable clone for the admitted native record; never overwrite a live descriptor.
alignas(16) static unsigned char rrGIMotionRecord[0xA8]{},rrGIMotionDescriptor[0x100]{};
static void* rrGIMotionSource=nullptr;
static RRAlbedoShader::NativeDescriptor rrGIMotionSourceDescriptor{};
static bool RRNativeGIPrepareMotion(void* record) noexcept {
 rrNativeGIHistoryReason="storage_unavailable";
 if(!rrGIMotionStorageReady.load(std::memory_order_acquire))return false;
 if(!RRNativeGITemporalIdentity(record))return false;
 RRAlbedoShader::NativeDescriptor d{};
 if(!RRAlbedoShader::ReadDescriptor(reinterpret_cast<std::uintptr_t>(record),0x60,&d))return false;
 rrNativeGIHistoryReason="clone_identity_changed";
 if(rrGIMotionSource)return record==rrGIMotionSource&&d.shaderBase==rrGIMotionSourceDescriptor.shaderBase&&
  d.identifier==rrGIMotionSourceDescriptor.identifier&&d.stage==rrGIMotionSourceDescriptor.stage&&d.size==rrGIMotionSourceDescriptor.size;
 rrNativeGIHistoryReason="clone_failed";
 if(!PTReflectionCloneShaderRecord(record,0x60,5,kGIMotionHistoryStorageId,sizeof(kGIMotionHistoryShader),rrGIMotionRecord,rrGIMotionDescriptor))return false;
 rrGIMotionSource=record;rrGIMotionSourceDescriptor=d;return true;
}
static void RRNativeHookGITemporalBind(void* record){
 if(!rrNativeGIHistoryScope){rrNativeTemporalBindOriginal(record);return;}
 const DWORD saved=GetLastError();
 const bool apply=RRNativeGIPrepareMotion(record);
 SetLastError(saved);
 rrNativeTemporalBindOriginal(apply?rrGIMotionRecord:record);
 rrNativeGIMotionBound=apply;
 if(apply)rrNativeGIHistoryReason="motion_history_bound";
}
static void RRNativeHookGITemporalDispatch(void* state,const void* groups){
 rrNativeDispatchOriginal(state,groups);
 if(rrNativeGIHistoryScope&&rrNativeGIMotionBound){++rrNativeGIHistoryDispatches;rrNativeGIHistoryReason="applied";}
}
static void RRNativeRunGIFilter(void* color,void* history,unsigned passes){
 const bool eligible=rrNativeFrameEnabled&&rrNativeGIKeepThisFrame&&rrNativeGIHistoryZeroThisFrame&&
  rrNativeFrame.Selected()&&rrNativeFrame.Stage()==control_rr::RRFrameStage::Lighting&&color&&passes&&
  reinterpret_cast<std::uintptr_t>(history)==reinterpret_cast<std::uintptr_t>(verifiedRenderer)+0x90eac8;
 if(!eligible||rrNativeGIHistoryScope){rrNativeDiffuseFilterOriginal(color,history,passes);return;}
 rrNativeGIHistoryScope=true;rrNativeGIMotionBound=false;rrNativeGIHistoryDispatches=0;rrNativeGIHistoryReason="no_temporal_bind";
 __try {rrNativeDiffuseFilterOriginal(color,history,passes);}
 __finally {
  const DWORD saved=GetLastError();
  const bool okay=rrNativeGIHistoryDispatches==1;
  rrNativeGIHistoryScope=false;
  rrNativeGIMotionBound=false;
  static unsigned long long calls=0; ++calls;
  if(calls<=4||calls%240==0)
   native_lighting::Trace("gi_temporal_adaptive build=R210 applied=%u calls=%llu dispatches=%u spatial_passes=%u history_clear=0 provider_override=0 motion_scale=400 depth_threshold=0.01 reason=%s rr_selected=%u",unsigned(okay),calls,rrNativeGIHistoryDispatches,passes,rrNativeGIHistoryReason,unsigned(rrNativeFrame.Selected()));
  SetLastError(saved);
 }
}
