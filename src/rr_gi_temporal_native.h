#pragma once
#include "rr_gi_stage_policy.h"
#ifdef CONTROL_GI_BOUNDARY_CAPTURE
#include "rr_gi_boundary.h"
static std::uintptr_t RRNativeGIColorIdentity(void* color) noexcept {
 using Get=void*(*)(void*);
 auto getter=reinterpret_cast<Get>(GetProcAddress(verifiedD3d,"?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ"));
 if(!getter||!color)return 0;
 __try {
  void* texture=getter(color);
  return texture?*reinterpret_cast<std::uintptr_t*>(static_cast<unsigned char*>(texture)+0x88):0;
 }__except(EXCEPTION_EXECUTE_HANDLER){return 0;}
}
#endif
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
 const auto mode=control_rr_gi::stageMode.load(std::memory_order_acquire);
 const unsigned effectivePasses=control_rr_gi::SpatialPasses(mode,passes);
 const auto view=control_rr_gi::EffectiveView(mode,control_rr_gi::viewRequested.load(std::memory_order_acquire));
 const auto previousView=control_rr_gi::activeView;control_rr_gi::activeView=view;
 const bool outlier=control_rr_gi::UseOutlier(mode,control_rr_gi::outlierRequested.load(std::memory_order_acquire))||view==control_rr_gi::DiagnosticView::Mask;
 const bool previousOutlier=control_rr_gi::outlierEnabled;
 control_rr_gi::outlierEnabled=outlier;
 const bool previousHistoryDisabled=control_rr_gi::historyDisabled;
 control_rr_gi::historyDisabled=!control_rr_gi::Temporal(mode);
 rrNativeGIHistoryScope=true;rrNativeGIMotionBound=false;rrNativeGIHistoryDispatches=0;rrNativeGIHistoryReason="no_temporal_bind";
 __try {
  rrNativeDiffuseFilterOriginal(color,history,effectivePasses);
#ifdef CONTROL_GI_BOUNDARY_CAPTURE
  if(control_rr_gi::boundaryRequested.load()) {
   const DWORD saved=GetLastError();
   control_rr_gi::Boundary observation{};
   observation.frame=rrNativeFrame.Key().frame;observation.color=RRNativeGIColorIdentity(color);
   observation.stage=mode;observation.view=view;observation.spatialPasses=effectivePasses;
   observation.cleanup=outlier;observation.applied=rrNativeGIHistoryDispatches==1;
   observation.valid=observation.color!=0;
   control_rr_gi::PublishBoundary(observation);SetLastError(saved);
  }
#endif
 }
 __finally {
  const DWORD saved=GetLastError();
  const bool okay=rrNativeGIHistoryDispatches==1;
  control_rr_gi::historyDisabled=previousHistoryDisabled;
  control_rr_gi::outlierEnabled=previousOutlier;
  control_rr_gi::activeView=previousView;
  rrNativeGIHistoryScope=false;rrNativeGIMotionBound=false;
  static unsigned long long calls=0; ++calls;
  static auto lastMode=control_rr_gi::StageMode::Both;
  static bool lastOutlier=false;
  static auto lastView=control_rr_gi::DiagnosticView::Normal;
  if(calls<=4||calls%240==0||mode!=lastMode||outlier!=lastOutlier||view!=lastView)
   native_lighting::Trace("gi_temporal_adaptive build=R216 stage=%s view=%s outlier=%u temporal=%u spatial_requested=%u applied=%u calls=%llu dispatches=%u spatial_passes=%u pipeline_ready=%u engine_record_copy=0 provider_override=0 reason=%s rr_selected=%u",control_rr_gi::StageName(mode),control_rr_gi::ViewName(view),unsigned(outlier),unsigned(control_rr_gi::Temporal(mode)),passes,unsigned(okay),calls,rrNativeGIHistoryDispatches,effectivePasses,control_rr_gi::ready.load(),rrNativeGIHistoryReason,unsigned(rrNativeFrame.Selected()));
  lastMode=mode;lastOutlier=outlier;lastView=view;
  SetLastError(saved);
 }
}
