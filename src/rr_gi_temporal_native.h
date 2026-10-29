#pragma once
// Exact native bind/dispatch callsites; engine shader records are always forwarded intact.
static bool rrNativeGIHistoryZeroThisFrame=false; // existing frame-latched Native Blend eligibility
static thread_local bool rrNativeGIHistoryScope=false;
static thread_local unsigned rrNativeGIHistoryDispatches=0;
static thread_local const char* rrNativeGIHistoryReason="idle";
static thread_local bool rrNativeGIMotionBound=false;
static RRAlbedoCallPatch rrNativeGITemporalBindPatch{};
static RRIndirectCallPatch rrNativeGITemporalDispatchPatch{};
static void RRNativeHookGITemporalBind(void* record){
 rrNativeTemporalBindOriginal(record);
 if(rrNativeGIHistoryScope){rrNativeGIMotionBound=true;rrNativeGIHistoryReason="native_bind_returned";}
}
static void RRNativeHookGITemporalDispatch(void* state,const void* groups){
 const DWORD saved=GetLastError();
 control_rr_reflection::Context context{};
 const bool eligible=rrNativeGIHistoryScope&&rrNativeGIMotionBound&&
  rrNativeCopy.StateMatches(reinterpret_cast<std::uintptr_t>(state))&&RRReflectionCurrent(nullptr,context)&&
  context.list&&context.frame==rrNativeFrame.Key().frame;
 SetLastError(saved);
 if(!eligible){rrNativeDispatchOriginal(state,groups);return;}
 const bool used=control_rr_clamp::InvokeGI(rrNativeDispatchOriginal,state,groups,reinterpret_cast<ID3D12GraphicsCommandList*>(context.list));
 if(used){++rrNativeGIHistoryDispatches;rrNativeGIHistoryReason="applied";}
 else rrNativeGIHistoryReason="native_pipeline_fallback";
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
  rrNativeGIHistoryScope=false;rrNativeGIMotionBound=false;
  static unsigned long long calls=0; ++calls;
  if(calls<=4||calls%240==0)
   native_lighting::Trace("gi_temporal_adaptive build=R212 applied=%u calls=%llu dispatches=%u spatial_passes=%u pipeline_ready=%u engine_record_copy=0 provider_override=0 reason=%s rr_selected=%u",unsigned(okay),calls,rrNativeGIHistoryDispatches,passes,control_rr_gi::ready.load(),rrNativeGIHistoryReason,unsigned(rrNativeFrame.Selected()));
  SetLastError(saved);
 }
}
