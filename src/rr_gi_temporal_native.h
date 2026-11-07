#pragma once
#include "rr_gi_stage_policy.h"
#include "rt_surface_source_capture.h"
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
static RRIndirectCallPatch rrNativeGITemporalDispatchPatch{},rrNativeGISpatialDispatchPatch{};
static thread_local bool rrNativeGISingleHistory=false;
static thread_local unsigned rrNativeGISpatialApplied=0;
static thread_local unsigned rrNativeGISpatialObserved=0;
static void RRNativeHookGISpatialDispatch(void* state,const void* groups){
 if(rrNativeGIHistoryScope)++rrNativeGISpatialObserved;
 const DWORD saved=GetLastError();control_rr_reflection::Context context{};
 const bool eligible=rrNativeGIHistoryScope&&rrNativeGISingleHistory&&rrNativeGIHistoryDispatches==1&&
  rrNativeCopy.StateMatches(reinterpret_cast<std::uintptr_t>(state))&&RRReflectionCurrent(nullptr,context)&&
  context.list&&context.frame==rrNativeFrame.Key().frame;
 SetLastError(saved);
 if(!eligible){rrNativeDispatchOriginal(state,groups);return;}
 const bool previous=control_rr_gi::spatialDispatch;
 __try {
  control_rr_gi::spatialDispatch=true;
  if(control_rr_clamp::InvokeGI(rrNativeDispatchOriginal,state,groups,reinterpret_cast<ID3D12GraphicsCommandList*>(context.list))){++rrNativeGISpatialApplied;rt_surface_source::Record(rt_surface_source::giScope,rt_surface_source::Event::GISpatial);}
 }__finally {control_rr_gi::spatialDispatch=previous;}
}
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
#ifdef CONTROL_RR_GI_SEQUENCE
 if(used&&(control_rr_gi::rawBypass||gi_path_capture::enabled.load())){
  const DWORD captureSaved=GetLastError();
  RRInputCaptureAtGI(context.frame,reinterpret_cast<ID3D12GraphicsCommandList*>(context.list));
  SetLastError(captureSaved);
 }
#endif
 if(used){rt_surface_source::Record(rt_surface_source::giScope,rt_surface_source::Event::GITemporal);++rrNativeGIHistoryDispatches;rrNativeGIHistoryReason="applied";}
 else rrNativeGIHistoryReason="native_pipeline_fallback";
}
static void RRNativeRunGIFilter(void* color,void* history,unsigned passes){
 const unsigned capture=rt_surface_source::EffectiveRequest();
 rt_surface_source::Record(capture,rt_surface_source::Event::GIEntry);
 const bool eligible=rrNativeFrameEnabled&&rrNativeGIKeepThisFrame&&rrNativeGIHistoryZeroThisFrame&&
  rrNativeFrame.Selected()&&rrNativeFrame.Stage()==control_rr::RRFrameStage::Lighting&&color&&passes&&
  reinterpret_cast<std::uintptr_t>(history)==reinterpret_cast<std::uintptr_t>(verifiedRenderer)+0x90eac8;
 if(!eligible||rrNativeGIHistoryScope){rrNativeDiffuseFilterOriginal(color,history,passes);return;}
 const auto mode=rt_surface_source::Stage(capture,control_rr_gi::stageMode.load(std::memory_order_acquire));
 const unsigned previousCapture=rt_surface_source::giScope;rt_surface_source::giScope=capture;
 rt_surface_source::Record(capture,rt_surface_source::Event::GIEligible);
 const unsigned effectivePasses=rt_surface_source::SpatialPasses(capture,mode,passes);
 const auto view=rt_surface_source::View(capture,mode,control_rr_gi::viewRequested.load(std::memory_order_acquire));
 const bool previousOwned=control_rr_gi::ownedDenoise;
 control_rr_gi::ownedDenoise=capture==7;
 const bool previousRaw=control_rr_gi::rawBypass;
 control_rr_gi::rawBypass=mode==control_rr_gi::StageMode::RawBypass;
 rrNativeGISpatialObserved=0;
 const auto previousView=control_rr_gi::activeView;control_rr_gi::activeView=view;
 const bool outlier=control_rr_gi::UseOutlier(mode,control_rr_gi::outlierRequested.load(std::memory_order_acquire))||view==control_rr_gi::DiagnosticView::Mask;
 const bool previousOutlier=control_rr_gi::outlierEnabled;
 control_rr_gi::outlierEnabled=outlier;
 const bool previousHistoryDisabled=control_rr_gi::historyDisabled;
 control_rr_gi::historyDisabled=!control_rr_gi::Temporal(mode);
 const bool previousSteady=control_rr_gi::spatialSteady;
 control_rr_gi::spatialSteady=mode==control_rr_gi::StageMode::BothSteadySpatial;
 const bool previousBounded=control_rr_gi::spatialHistoryBounded;
 control_rr_gi::spatialHistoryBounded=mode==control_rr_gi::StageMode::BothBoundedHistory;
 rrNativeGISingleHistory=capture==7||mode==control_rr_gi::StageMode::BothSingleHistory||control_rr_gi::spatialHistoryBounded||control_rr_gi::spatialSteady;rrNativeGISpatialApplied=0;
 rrNativeGIHistoryScope=true;rrNativeGIMotionBound=false;rrNativeGIHistoryDispatches=0;rrNativeGIHistoryReason="no_temporal_bind";
 __try {
  rrNativeDiffuseFilterOriginal(color,history,effectivePasses);
#ifdef CONTROL_GI_BOUNDARY_CAPTURE
  if(control_rr_gi::boundaryRequested.load()) {
   const DWORD saved=GetLastError();
   control_rr_gi::Boundary observation{};
   observation.frame=rrNativeFrame.Key().frame;observation.color=RRNativeGIColorIdentity(color);
   observation.stage=mode;observation.view=view;observation.spatialPasses=effectivePasses;observation.spatialObserved=rrNativeGISpatialObserved;
   observation.cleanup=outlier;observation.applied=rrNativeGIHistoryDispatches==1;
   observation.valid=observation.color!=0;
   control_rr_gi::PublishBoundary(observation);SetLastError(saved);
  }
#endif
 }
 __finally {
  const DWORD saved=GetLastError();
  const bool okay=rrNativeGIHistoryDispatches==1;
  rt_surface_source::giScope=previousCapture;
  control_rr_gi::historyDisabled=previousHistoryDisabled;
  control_rr_gi::outlierEnabled=previousOutlier;
  control_rr_gi::activeView=previousView;
  if(rrNativeGISingleHistory){
   static unsigned long long samples=0;++samples;
   if(samples<=4||samples%240==0||rrNativeGISpatialApplied!=effectivePasses)
    Log("gi_spatial_history build=R261 steady=%u bounded=%u frame=%llu temporal_applied=%u spatial_expected=%u spatial_applied=%u complete=%u",unsigned(control_rr_gi::spatialSteady),unsigned(control_rr_gi::spatialHistoryBounded),rrNativeFrame.Key().frame,rrNativeGIHistoryDispatches,effectivePasses,rrNativeGISpatialApplied,unsigned(okay&&rrNativeGISpatialApplied==effectivePasses));
  }
  if(control_rr_gi::rawBypass){
   static unsigned long long bypassCalls=0;++bypassCalls;
   if(bypassCalls<=4||bypassCalls%240==0||!okay||rrNativeGISpatialObserved)
    Log("GI_BYPASS_R262 frame=%llu mode=raw_bypass copy_applied=%u spatial_requested=%u spatial_observed=%u complete=%u calls=%llu",rrNativeFrame.Key().frame,rrNativeGIHistoryDispatches,effectivePasses,rrNativeGISpatialObserved,unsigned(okay&&rrNativeGISpatialObserved==0),bypassCalls);
  }
  control_rr_gi::rawBypass=previousRaw;
  control_rr_gi::ownedDenoise=previousOwned;
  control_rr_gi::spatialSteady=previousSteady;
  control_rr_gi::spatialHistoryBounded=previousBounded;
  rrNativeGISingleHistory=false;
  rrNativeGIHistoryScope=false;rrNativeGIMotionBound=false;
  static unsigned long long calls=0; ++calls;
  static auto lastMode=control_rr_gi::StageMode::Both;
  static bool lastOutlier=false;
  static auto lastView=control_rr_gi::DiagnosticView::Normal;
  if(calls<=4||calls%240==0||mode!=lastMode||outlier!=lastOutlier||view!=lastView)
   Log("gi_temporal_adaptive build=R217 stage=%s view=%s smooth=%u temporal=%u spatial_requested=%u applied=%u calls=%llu dispatches=%u spatial_passes=%u pipeline_ready=%u engine_record_copy=0 provider_override=0 reason=%s rr_selected=%u",control_rr_gi::StageName(mode),control_rr_gi::ViewName(view),unsigned(outlier),unsigned(control_rr_gi::Temporal(mode)),passes,unsigned(okay),calls,rrNativeGIHistoryDispatches,effectivePasses,control_rr_gi::ready.load(),rrNativeGIHistoryReason,unsigned(rrNativeFrame.Selected()));
  lastMode=mode;lastOutlier=outlier;lastView=view;
  SetLastError(saved);
 }
}

#ifdef CONTROL_RR_GI_SEQUENCE
static RRIndirectCallPatch rrNativeGIProducerDispatchPatch{};
static int rrGIProducerProviderIds[3]{-1,-1,-1};
static bool RRNativeGIProducerProviders() noexcept {
 const char* names[3]{"g_uDGIPass","g_uDGIPassCount","g_uRTDiffuseRayCount"};
 __try {
  for(unsigned i=0;i<3;++i){
   struct Slot {int id,tag;} slot{-1,0};
   rrNativeClampApi.resolve(&slot,names[i]);
   control_rr_native_clamp::Entry metadata{};
   if(!rrNativeClampApi.EntryFor(slot.id,metadata))return false;
   rrGIProducerProviderIds[i]=slot.id;
  }
  return true;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
static void RRNativeHookGIProducerDispatch(void* state,const void* groups){
 rrNativeDispatchOriginal(state,groups);
#ifdef CONTROL_GI_BOUNDARY_CAPTURE
 if(!GIBatchCaptureEnabled()||!control_rr_gi::boundaryRequested.load())return;
 const DWORD saved=GetLastError();
 control_rr_reflection::Context context{};
 if(rrNativeFrameEnabled&&rrNativeFrame.Selected()&&rrNativeCopy.StateMatches(reinterpret_cast<std::uintptr_t>(state))&&
    RRReflectionCurrent(nullptr,context)&&context.list&&context.frame==rrNativeFrame.Key().frame){
  // GI providers are registered lazily by the engine; resolve after the native dispatch.
  unsigned values[3]{};bool valid=RRNativeGIProducerProviders();
  for(unsigned i=0;i<3;++i)valid=rrNativeClampApi.ReadCurrent(rrGIProducerProviderIds[i],values[i])&&valid;
  RRInputCaptureGIBatch(context.frame,reinterpret_cast<ID3D12GraphicsCommandList*>(context.list),values[0],values[1],values[2],valid);
 }
 SetLastError(saved);
#endif
}
static void RRNativeInstallGIProducerCapture(unsigned char* base) noexcept {
 if(!GIBatchCaptureEnabled())return;
 bool ready=rrNativeGIProducerDispatchPatch.Prepare(base+0x12cc16,reinterpret_cast<void**>(base+0x5dfcc0),
   reinterpret_cast<void*>(rrNativeDispatchOriginal),reinterpret_cast<void*>(&RRNativeHookGIProducerDispatch));
 if(ready)ready=rrNativeGIProducerDispatchPatch.Exchange(true)&&rrNativeGIProducerDispatchPatch.healthy;
 if(!ready&&rrNativeGIProducerDispatchPatch.changed)rrNativeGIProducerDispatchPatch.Exchange(false);
 Log("GI_HITS_R271_HOOK ready=%u callsite=0x12cc16 native_lighting_unchanged=1",unsigned(ready));
}
#endif
