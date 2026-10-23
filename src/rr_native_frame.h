#pragma once
#include "rr_frame_coordinator.h"
#include "rr_lighting_diagnostic.h"
#include "rr_native_preset.h"
#include "rr_user_control.h"
#include "rr_guide_parameters.h"
#include "rr_indirect_call_windows.h"
#include "rr_native_option_windows.h"
#include "rr_specular_noisy_windows.h"
#include "rr_specular_native_clamp.h"
#include "rr_specular_clamp_policy.h"
#include "rr_no_clamp_test.h"
#include "rr_rt_stack.h"
#include "rr_contact_shadow_policy.h"
#include "rr_broad_diffuse_policy.h"
#include "rr_gi_sample_control.h"
#include "rr_sample_diagnostic.h"
#include "rr_specular_sample_windows.h"
static control_rr::RRFrameCoordinator rrNativeFrame;
static control_rr::RRRTFrame rrNativeRTFrame;
static control_rr_specular::NativeCopyApi rrNativeCopy;
static control_rr_specular::NativeHistoryReset rrNativeHistory;
static control_rr_native_clamp::Api rrNativeClampApi;
static thread_local control_rr_native_clamp::Lease rrNativeClampLease{};
static thread_local unsigned rrClampFrameStrength=100;
static thread_local control_rr_specular::Scope* rrNativeSpecularScope=nullptr;
static bool rrNativeFrameEnabled=false,rrNativeLightingComplete=false;
static std::atomic<bool> rrNativePartialFrame{false};
static bool RRNativePartialActive() noexcept {return rrNativePartialFrame.load(std::memory_order_acquire);}
static unsigned long long rrNativeResetCalls=0;
using RRNativeResetFn=bool (*)(unsigned,unsigned,unsigned,unsigned,bool,bool,bool,bool,bool&);
using RRNativeFilterFn=void (*)(void*,void*,unsigned,bool);
using RRNativeDispatchFn=void (*)(void*,const void*);
using RRNativeTemporalBindFn=void (*)(void*);
using RRNativeContactShadowFilterFn=void (*)(void*,void*,void*,bool,int,int);
using RRNativeDiffuseFilterFn=void (*)(void*,void*,unsigned);
static RRNativeResetFn rrNativeResetOriginal=nullptr;
static RRNativeFilterFn rrNativeFilterOriginal=nullptr;
static RRNativeDispatchFn rrNativeDispatchOriginal=nullptr;
static RRNativeTemporalBindFn rrNativeTemporalBindOriginal=nullptr;
static RRNativeContactShadowFilterFn rrNativeContactShadowFilterOriginal=nullptr;
static RRNativeDiffuseFilterFn rrNativeDiffuseFilterOriginal=nullptr;
static RRIndirectCallPatch rrNativeResetPatch{},rrNativeDispatchPatch{};
static RRAlbedoCallPatch rrNativeFilterPatch{},rrNativeTemporalBindPatch{};
using RRNativeOptionFn=bool (*)(void*);
using RRNativeGIReleaseFn=void (*)(void*);
static RRNativeOptionFn rrNativeOptionOriginal=nullptr;
static RRNativeGIReleaseFn rrNativeGIReleaseOriginal=nullptr;
static RRAlbedoCallPatch rrNativeReflectionOptionPatch{},rrNativeGIRadiusOptionPatch{},rrNativeGIBypassOptionPatch{};
static RRAlbedoCallPatch rrNativeGIReleasePatch{};
using RRNativeGISampleFn=unsigned (*)(void*);
static RRNativeGISampleFn rrNativeGISampleOriginal=nullptr;
static RRAlbedoCallPatch rrNativeGISamplePatch{};
static control_rr::GISampleFrameSelection rrNativeGISampleSelection{};
static RRSpecularSamplePatch rrNativeSpecularSamplePatch{};
static unsigned rrNativeSpecularSampleRequest=0;
static bool rrNativeSpecularSamplesAvailable=false;
static RRAlbedoCallPatch rrNativeContactShadowFilterPatch{};
static RRAlbedoCallPatch rrNativeBroadDiffuseFilterPatchA{},rrNativeBroadDiffuseFilterPatchB{};
static RRAlbedoCallPatch rrNativeGIFilterPatch{};
static bool rrNativeGIKeepThisFrame=false;
static bool rrNativeSpecularKeepThisFrame=false,rrNativeSpecularHistoryPending=false;
static bool rrNativeSpecularTemporalThisFrame=false;
static RRNativeJitterOptionPatch rrNativeJitterOptionPatch{};
static std::atomic<unsigned long long> rrNativeRegisteredOptionRepairs{0};
static std::atomic<unsigned long long> rrNativeContactShadowBypasses{0};
static std::atomic<unsigned long long> rrNativeBroadDiffuseBypassesA{0},rrNativeBroadDiffuseBypassesB{0};
static std::atomic<unsigned long long> rrNativeClampDispatches{0},rrNativeClampFallbacks{0},rrNativeClampRestoreFailures{0};
// r20w: a resolution change is a hard RR epoch boundary.  Do not allocate or
// record any FULL-RR auxiliary work while Control is rebuilding its own render
// targets.  Require a run of identical extents before allowing a fresh epoch to
// prepare.  Present-side retirement continues while this gate is closed.
struct RRNativeResizeEpoch {
 unsigned outputWidth=0,outputHeight=0,width=0,height=0;
 unsigned stableFrames=0;
 unsigned long long epoch=0;
 bool initialized=false,paused=false;
};
static RRNativeResizeEpoch rrNativeResizeEpoch{};
static constexpr unsigned RRNativeResizeSettleFrames=8;
static bool RRNativeEpochOldWorkRetired() noexcept {
 // r21y: reflection hit-array capture is not installed, so epoch retirement only
 // waits on resources that can still be submitted to RR: live GBuffer guides and
 // the historical diffuse replay path (normally bypassed by RRGBufferGuideMode).
 bool liveIdle=true,diffuseIdle=true;
 if(TryAcquireSRWLockExclusive(&rrLiveLock)){
  __try {liveIdle=!rrLive||(!rrLive->preparing&&RRLiveAllFree(rrLive));}
  __except(EXCEPTION_EXECUTE_HANDLER){liveIdle=false;}
  ReleaseSRWLockExclusive(&rrLiveLock);
 } else liveIdle=false;
 diffuseIdle=(control_rr::RRGBufferGuideMode&&!control_rr::RRTexturedCharacterGuides) || rrDiffuseStage.load(std::memory_order_acquire)!=RRDiffuseStage::Recording;
 return liveIdle&&diffuseIdle;
}
// r20x: live E<->F changes are also hard RR epochs. The overlay arms the
// preset pause before publishing the new selection so Streamline sees eOff even
// if its options callback runs before this native reset hook. We then wait for
// old guide/reflection/diffuse leases (whose fences cover RR consumption) to
// retire before exposing the new feature handle.
struct RRNativePresetEpoch {
 unsigned preset=control_rr::RRPresetF;
 unsigned settleFrames=0;
 unsigned long long selectionGeneration=0;
 unsigned long long epoch=0;
 bool initialized=false,paused=false,releasePending=false;
};
static RRNativePresetEpoch rrNativePresetEpoch{};
static constexpr unsigned RRNativePresetSettleFrames=2;
static bool RRNativePresetGate(bool requested,unsigned long long frame) noexcept {
 auto& p=rrNativePresetEpoch;
 const unsigned desired=control_rr::RRUserPresetValue();
 const unsigned long long generation=control_rr::RRUserPresetSelectionGeneration();
 if(!requested){
  if(p.paused||p.releasePending)
   Log("RR_PRESET_EPOCH_CANCEL frame=%llu epoch=%llu reason=rr_not_full desired=%u",frame,p.epoch,desired);
  p.preset=desired;p.selectionGeneration=generation;p.settleFrames=0;p.initialized=true;p.paused=false;p.releasePending=false;
  control_rr::RRUserSetPresetOptionsWindow(false);
  control_rr::RRUserSetPresetTransitionPaused(false);
  return true;
 }
 if(!p.initialized){
  p.preset=desired;p.selectionGeneration=generation;p.settleFrames=0;p.epoch=1;p.initialized=true;p.paused=false;p.releasePending=false;
  control_rr::RRUserSetPresetOptionsWindow(false);
  control_rr::RRUserSetPresetTransitionPaused(false);
  Log("RR_PRESET_EPOCH_INITIAL frame=%llu epoch=%llu preset=%u selection_generation=%llu",frame,p.epoch,desired,generation);
  return true;
 }
 if(generation!=p.selectionGeneration){
  const unsigned previous=p.preset;
  p.preset=desired;p.selectionGeneration=generation;p.settleFrames=0;p.paused=true;p.releasePending=false;++p.epoch;
  control_rr::RRUserSetPresetOptionsWindow(false);
  control_rr::RRUserSetPresetTransitionPaused(true);
  Log("RR_PRESET_EPOCH_BEGIN frame=%llu epoch=%llu old_preset=%u new_preset=%u selection_generation=%llu settle_frames=%u aux_work_paused=1",
   frame,p.epoch,previous,desired,generation,RRNativePresetSettleFrames);
  return false;
 }
 if(p.releasePending){
  if(!RR20POptionsReadyForPreset(p.preset)){
   if(p.settleFrames==RRNativePresetSettleFrames||(p.settleFrames%120)==0)
    Log("RR_PRESET_EPOCH_OPTIONS_WAIT frame=%llu epoch=%llu preset=%u stable=%u streamline_options_ready=0 aux_work_paused=1",
     frame,p.epoch,p.preset,p.settleFrames);
   if(p.settleFrames<0xffffffffu)++p.settleFrames;
   return false;
  }
  p.releasePending=false;p.paused=false;
  control_rr::RRUserSetPresetOptionsWindow(false);
  control_rr::RRUserSetPresetTransitionPaused(false);
  Log("RR_PRESET_EPOCH_ACTIVATE frame=%llu epoch=%llu preset=%u selection_generation=%llu streamline_options_ready=1 fresh_history=1 aux_work_paused=0",
   frame,p.epoch,p.preset,p.selectionGeneration);
  return true;
 }
 if(!p.paused)return true;
 if(p.settleFrames<RRNativePresetSettleFrames)++p.settleFrames;
 if(p.settleFrames<RRNativePresetSettleFrames){
  Log("RR_PRESET_EPOCH_SETTLE frame=%llu epoch=%llu stable=%u required=%u preset=%u aux_work_paused=1",
   frame,p.epoch,p.settleFrames,RRNativePresetSettleFrames,p.preset);
  return false;
 }
 if(!RRNativeEpochOldWorkRetired()){
  if((p.settleFrames%8)==0)
   Log("RR_PRESET_EPOCH_DRAIN frame=%llu epoch=%llu stable=%u old_work_retired=0 preset=%u aux_work_paused=1",
    frame,p.epoch,p.settleFrames,p.preset);
  if(p.settleFrames<0xffffffffu)++p.settleFrames;
  return false;
 }
 // Old RR consumption is fenced out. R3 proved that releasing only the
 // mod-owned alternate E handle is not enough: the native Control RR cache keeps
 // the previous base feature resident and VRAM remains at the two-feature peak.
 // Treat every live E<->F change as a full native RR feature epoch. Release the
 // cached base feature plus all alternates, retain only the persistent NGX
 // parameters, then recreate exactly one feature for the newly selected preset.
#ifdef CONTROL_FG_RR_VRAM_RESIZE_R2
 if(!RRNativeVramReleaseCachedRRFeaturesForPresetSwitch(frame,p.preset)){
  if((p.settleFrames%8)==0)
   Log("RR_PRESET_EPOCH_FREE_WAIT frame=%llu epoch=%llu preset=%u old_work_retired=1 native_rr_cache_released=0 aux_work_paused=1",
    frame,p.epoch,p.preset);
  if(p.settleFrames<0xffffffffu)++p.settleFrames;
  return false;
 }
#endif
 if(!RR20PFreeStreamlineResources("preset_switch_retired",frame)){
  if((p.settleFrames%8)==0)
   Log("RR_PRESET_EPOCH_FREE_WAIT frame=%llu epoch=%llu preset=%u old_work_retired=1 streamline_resources_freed=0 aux_work_paused=1",
    frame,p.epoch,p.preset);
  if(p.settleFrames<0xffffffffu)++p.settleFrames;
  return false;
 }
 // Keep auxiliary work paused, but open a narrow Streamline-options window so
 // the requested preset can be configured while RR evaluation remains disabled.
 p.releasePending=true;
 control_rr::RRUserSetPresetOptionsWindow(true);
 Log("RR_PRESET_EPOCH_OPTIONS_WINDOW frame=%llu epoch=%llu preset=%u stable=%u old_work_retired=1 action=streamline_options_rebuild_while_aux_paused",
  frame,p.epoch,p.preset,p.settleFrames);
 return false;
}
static bool RRNativeResizeGate(bool requested,unsigned long long frame,unsigned outputWidth,unsigned outputHeight,unsigned width,unsigned height) noexcept {
 auto& r=rrNativeResizeEpoch;
 if(!requested){
  if(r.paused)Log("RR_RESIZE_EPOCH_CANCEL frame=%llu epoch=%llu reason=rr_not_full",frame,r.epoch);
  r={outputWidth,outputHeight,width,height,0,r.epoch,true,false};
  control_rr::RRUserSetRuntimePaused(false);return true;
 }
 if(!r.initialized){
  r={outputWidth,outputHeight,width,height,RRNativeResizeSettleFrames,1,true,false};
  control_rr::RRUserSetRuntimePaused(false);
  Log("RR_RESIZE_EPOCH_INITIAL frame=%llu epoch=%llu output=%ux%u render=%ux%u",frame,r.epoch,outputWidth,outputHeight,width,height);
  return true;
 }
 const bool changed=r.outputWidth!=outputWidth||r.outputHeight!=outputHeight||r.width!=width||r.height!=height;
 if(changed){
  const unsigned oldOutputWidth=r.outputWidth,oldOutputHeight=r.outputHeight,oldWidth=r.width,oldHeight=r.height;
  r.outputWidth=outputWidth;r.outputHeight=outputHeight;r.width=width;r.height=height;r.stableFrames=0;r.paused=true;++r.epoch;
  control_rr::RRUserSetRuntimePaused(true);
  Log("RR_RESIZE_EPOCH_BEGIN frame=%llu epoch=%llu old_output=%ux%u new_output=%ux%u old_render=%ux%u new_render=%ux%u settle_frames=%u aux_work_paused=1",
   frame,r.epoch,oldOutputWidth,oldOutputHeight,outputWidth,outputHeight,oldWidth,oldHeight,width,height,RRNativeResizeSettleFrames);
  return false;
 }
 if(!r.paused)return true;
 if(r.stableFrames<RRNativeResizeSettleFrames)++r.stableFrames;
 if(r.stableFrames<RRNativeResizeSettleFrames){
  if(r.stableFrames<=2||r.stableFrames==RRNativeResizeSettleFrames-1)
   Log("RR_RESIZE_EPOCH_SETTLE frame=%llu epoch=%llu stable=%u required=%u output=%ux%u render=%ux%u aux_work_paused=1",
    frame,r.epoch,r.stableFrames,RRNativeResizeSettleFrames,outputWidth,outputHeight,width,height);
  return false;
 }
 if(!RRNativeEpochOldWorkRetired()){
  if((r.stableFrames%8)==0)Log("RR_RESIZE_EPOCH_DRAIN frame=%llu epoch=%llu stable=%u old_work_retired=0 aux_work_paused=1",frame,r.epoch,r.stableFrames);
  if(r.stableFrames<0xffffffffu)++r.stableFrames;
  return false;
 }
#ifdef CONTROL_FG_RR_VRAM_RESIZE_R2
 if(!RRNativeVramReleaseCachedRRFeaturesForResize(frame,r.outputWidth,r.outputHeight,r.width,r.height)){
  if((r.stableFrames%8)==0)Log("RR_RESIZE_EPOCH_DRAIN frame=%llu epoch=%llu stable=%u old_work_retired=1 native_rr_cache_released=0 aux_work_paused=1",frame,r.epoch,r.stableFrames);
  if(r.stableFrames<0xffffffffu)++r.stableFrames;
  return false;
 }
#endif
 r.paused=false;control_rr::RRUserSetRuntimePaused(false);
 Log("RR_RESIZE_EPOCH_RELEASE frame=%llu epoch=%llu stable=%u output=%ux%u render=%ux%u action=rebuild_aux_then_rr_history_reset",
  frame,r.epoch,r.stableFrames,outputWidth,outputHeight,width,height);
 return true;
}
static bool RRNativeReadMode(bool* support,bool* active) noexcept {
 *support=false;*active=false;
 __try {
  auto* state=*reinterpret_cast<unsigned char**>(reinterpret_cast<unsigned char*>(verifiedD3d)+0x111be0);
  if(!state)return false;*support=state[9]!=0;*active=state[0x1e]!=0;return true;
 } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
static bool RRNativeKeepRegisteredOptionOff() noexcept {
 // r20m wrote this hidden registered option TRUE while RR was selected. User
 // testing showed that exiting in that state could affect the next unmodded
 // render. r20n therefore never writes TRUE and normalizes any old leaked TRUE
 // value to the native OFF baseline. Reviewed RR decisions use mod-owned state.
 __try {
  auto* value=reinterpret_cast<unsigned char*>(verifiedRenderer)+0x914620;
  const unsigned before=*value;
  if(before){
   *value=0;MemoryBarrier();
   if(*value!=0)return false;
   const auto repair=rrNativeRegisteredOptionRepairs.fetch_add(1,std::memory_order_relaxed)+1;
   if(repair<=4||(repair&(repair-1))==0)
    Log("RR_NATIVE_OPTION_REPAIRED count=%llu previous=%u forced=0 persistent_true_never_written=1",repair,before);
  }
  return *value==0;
 } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
static bool RRNativeSelectedOption(void*) noexcept {
 const bool active=rrNativeFrame.Selected()||RRNativePartialActive();
 return control_rr::NativeRenderOption(rrNativeFrameEnabled,active,rrNativeFrame.Stopped());
}
// Only the reflection pass-selection callsite uses this override. GI radius
// and partial-RR behavior retain their existing policies.
static bool RRNativeReflectionOption(void* option) noexcept {
 return RRNativeSelectedOption(option)&&!rrNativeSpecularKeepThisFrame;
}
// Preserve the existing bypass unless this frame explicitly requests native GI filtering.
static bool RRNativeGIBypassOption(void* option) noexcept {
 control_rr_diagnostic::Record(control_rr_diagnostic::Event::GIOption);
 const bool bypass=RRNativeSelectedOption(option)&&!rrNativeGIKeepThisFrame;
 if(bypass)control_rr_diagnostic::Record(control_rr_diagnostic::Event::GIBypassSelected);
 return bypass;
}
// Override the sole producer-local getter, before allocation and provider binding.
// Never mutate the registered option or the shader constant independently.
static unsigned RRNativeGISampleCount(void* option) {
 const unsigned nativeCount=rrNativeGISampleOriginal(option);
 const DWORD saved=GetLastError();
 const bool full=rrNativeFrameEnabled&&rrNativeFrame.Selected()&&!rrNativeFrame.Stopped()&&!RRNativePartialActive();
 const unsigned effective=control_rr::GISampleComparison(nativeCount,rrNativeGISampleSelection.requested,full);
 static thread_local unsigned lastNative=~0u,lastEffective=~0u;
 if(nativeCount!=lastNative||effective!=lastEffective||rrNativeFrame.Key().frame%240==0){
  Log("RR_GI_SAMPLES_R33 frame=%llu native=%u effective=%u requested=%u full_rr=%u allocation_and_shader=shared_native_local temporal_changes=0",
   static_cast<unsigned long long>(rrNativeFrame.Key().frame),nativeCount,effective,rrNativeGISampleSelection.requested,unsigned(full));
  native_lighting::Trace("gi_sample_producer frame=%llu native=%u effective=%u requested=%u full_rr=%u allocation_and_shader=shared_local",
   static_cast<unsigned long long>(rrNativeFrame.Key().frame),nativeCount,effective,rrNativeGISampleSelection.requested,unsigned(full));
  lastNative=nativeCount;lastEffective=effective;
 }
 SetLastError(saved);return effective;
}
static bool RRNativePrepareAA(void* color,void*& normal,void*& diffuse,void*& specular) noexcept {
 if(!rrNativeFrameEnabled)return true;
 bool support=false,active=false;
 if(!RRNativeReadMode(&support,&active)){rrNativeFrame.Fail();return false;}
 if(!active&&!rrNativeFrame.Selected())return true;
 std::uintptr_t resource=0;
 const bool readable=color&&control_rr_reflection::WindowsAccess::Read(nullptr,reinterpret_cast<std::uintptr_t>(color)+0x88,&resource,sizeof(resource))&&resource;
 if(!control_rr::PrepareNativeAAArguments(active,rrNativeFrame.Selected(),rrNativeFrame.Stopped(),readable,color,normal,diffuse,specular)){
  rrNativeFrame.Fail();Log("RR_FRAME_STOP reason=native_aa_argument_contract");return false;
 }
 // Both renderer callers pass null t7/t8/t9. The native RR wrapper nevertheless
 // dereferences them for transitions and descriptor population before its tail.
 // Use the already engine-owned input-color NativeTexture for that setup only;
 // repeated shader-read transitions are idempotent. Do not synthesize a native
 // object or claim these are guides. The mandatory RR tail replaces ALL three
 // resources and verifies the setters before NGX is allowed to evaluate.
 return true;
}
static bool RRNativeReadRTSettings(unsigned* enabledBits) noexcept {
 *enabledBits=0;
 __try {
  const auto* base=reinterpret_cast<const unsigned char*>(verifiedRenderer);
  // Native graphics-setting updates at +0x137BB1..+0x137CC2 write these
  // option value bytes. The constructor/getter contract uses object+0xB0.
  for(unsigned i=0;i<7;++i)if(base[control_rr::RTOptionValues[i]])*enabledBits|=1u<<i;
  return true;
 } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
static void RRNativeHookGIRelease(void* history) {
 control_rr_diagnostic::Record(control_rr_diagnostic::Event::GIRelease);
 // P2 rough-surface energy belongs in diffuse GI, not the specular reflection
 // target. This reviewed callsite is after the native DGI producer and before
 // the later filter/history branch, so inject before the ownership release.
 const DWORD bridgeSaved=GetLastError();
 if(rrNativeFrameEnabled&&rrNativeFrame.Selected()&&
    (rrNativeRTFrame.effects&control_rr::RTDiffuseGI))
  PTReflectionBridgeDiffuseGI();
 SetLastError(bridgeSaved);

 // Preserve the native ownership operation exactly once, including exceptions.
 rrNativeGIReleaseOriginal(history);
 const DWORD saved=GetLastError();
 if(rrNativeFrameEnabled){
  unsigned long long frame=0;DWORD fault=0;std::uintptr_t retained=1;
  const auto expected=reinterpret_cast<std::uintptr_t>(verifiedRenderer)+0x90eac8;
  const bool correct=reinterpret_cast<std::uintptr_t>(history)==expected;
  const bool empty=correct&&control_rr_reflection::WindowsAccess::Read(nullptr,expected,&retained,sizeof(retained))&&retained==0;
  if(RRNativePartialActive()) {
   const bool frameKnown=ReadEngineFrameSafe(&frame,&fault);
   if(frameKnown&&(frame%240==0))
    Log("RR_PARTIAL_GI frame=%llu history_empty=%u correct_history=%u feature_rr=0 native_gi_bypass=1 partial=1",frame,unsigned(empty),unsigned(correct));
  } else {
   const bool okay=ReadEngineFrameSafe(&frame,&fault)&&rrNativeRTFrame.ObserveGI(frame,
    rrNativeFrame.Selected()&&rrNativeFrame.Stage()==control_rr::RRFrameStage::Lighting,correct,empty);
   if(okay)control_rr_diagnostic::Record(control_rr_diagnostic::Event::GIValidated);
   if(!okay)rrNativeFrame.Fail("diffuse_gi_bypass_contract");
   if(!okay||rrNativeFrame.Reset()||frame%240==0)
    Log("RR_NOISY_GI frame=%llu success=%u history_empty=%u native_filter_skipped=1 rt_effects=0x%X",frame,unsigned(okay),unsigned(empty),rrNativeRTFrame.effects);
  }
 }
 SetLastError(saved);
}
// Verified native GI filter call at renderer+0x12cd96. Never synthesize its arguments.
static void RRNativeHookGIFilter(void* color,void* history,unsigned passes) {
 control_rr_diagnostic::Record(control_rr_diagnostic::Event::GIFilterEntry);
 rrNativeDiffuseFilterOriginal(color,history,passes);
 const DWORD saved=GetLastError();
 if(rrNativeFrameEnabled&&rrNativeGIKeepThisFrame){
  unsigned long long frame=0;DWORD fault=0;std::uintptr_t retained=0;
  const auto expected=reinterpret_cast<std::uintptr_t>(verifiedRenderer)+0x90eac8;
  const bool correct=reinterpret_cast<std::uintptr_t>(history)==expected;
  const bool readable=correct&&control_rr_reflection::WindowsAccess::Read(nullptr,expected,&retained,sizeof(retained));
  const bool okay=ReadEngineFrameSafe(&frame,&fault)&&rrNativeRTFrame.ObserveFilteredGI(frame,
   rrNativeFrame.Selected()&&rrNativeFrame.Stage()==control_rr::RRFrameStage::Lighting,
   correct&&color!=nullptr,readable&&retained!=0,passes);
  if(okay)control_rr_diagnostic::Record(control_rr_diagnostic::Event::GIFilterValidated);
  else rrNativeFrame.Fail("diffuse_gi_filter_contract");
  if(!okay||rrNativeFrame.Reset())native_lighting::Trace("rr_gi_filter frame=%llu native_completed=1 validated=%u passes=%u history_retained=%u rr_selected=%u",
   frame,unsigned(okay),passes,unsigned(retained!=0),unsigned(rrNativeFrame.Selected()));
 }
 SetLastError(saved);
}
static bool RRNativeLightingReady(unsigned long long frame) noexcept {
 unsigned effects=0;
 const bool readable=RRNativeReadRTSettings(&effects);
 const bool ready=readable&&rrNativeRTFrame.Complete(frame,effects,rrNativeLightingComplete);
 if(!ready)Log("RR_RT_REJECT frame=%llu expected_frame=%llu expected_effects=0x%X current_effects=0x%X settings_read=%u reflections_complete=%u gi_required=%u gi_observed=%u",frame,
  static_cast<unsigned long long>(rrNativeRTFrame.frame),rrNativeRTFrame.effects,effects,unsigned(readable),unsigned(rrNativeLightingComplete),
  unsigned((rrNativeRTFrame.effects&control_rr::RTDiffuseGI)!=0),unsigned(rrNativeRTFrame.giObserved));
 return ready;
}
static bool RRNativeReady(unsigned width,unsigned height) noexcept {
 // r21y production: RR consumes only the owned GBuffer reflectance guides.
 // The retired reflection hit-array copies and unbound hit-distance pass are
 // no longer readiness prerequisites. This removes their GPU work and memory.
 if(!rrNativeFrameEnabled||!TryAcquireSRWLockExclusive(&rrLiveLock))return false;
 bool ready=false;
 __try {
  if(rrLive&&rrLive->prepared&&!rrLive->preparing&&!rrLive->stopped&&rrLive->guideFrames>=64&&
   rrLive->width==width&&rrLive->height==height){
   for(const auto& slot:rrLive->policy.Inspect())if(slot.state==control_rr::SlotState::Free){ready=true;break;}
  }
 } __except(EXCEPTION_EXECUTE_HANDLER){ready=false;}
 ReleaseSRWLockExclusive(&rrLiveLock);return ready;
}
static bool RRNativeHookReset(unsigned outputWidth,unsigned outputHeight,unsigned width,unsigned height,
 bool flag0,bool flag1,bool nativeRequest,bool flag3,bool& reset) {
 rrNativeSpecularSamplePatch.BeginFrame(0);
 if(!rrNativeFrameEnabled)return rrNativeResetOriginal(outputWidth,outputHeight,width,height,flag0,flag1,nativeRequest,flag3,reset);
 const DWORD saved=GetLastError();unsigned long long frame=0;DWORD fault=0;bool supported=false,active=false;
 const bool known=ReadEngineFrameSafe(&frame,&fault)&&RRNativeReadMode(&supported,&active);
 const auto diagnosticMode=native_lighting::LiveSelection::GetMode(native_lighting::selection.Read());
 const bool diagnosticBypass=native_lighting::IsSignalDiagnostic(diagnosticMode);
 control_rr::RRSetDiagnosticBypass(diagnosticBypass);
 static bool previousDiagnosticBypass=false;
 if(previousDiagnosticBypass!=diagnosticBypass){
  native_lighting::Trace("rr_diagnostic_bypass active=%u mode=%s saved_request=%u",unsigned(diagnosticBypass),native_lighting::ModeName(diagnosticMode),unsigned(control_rr::RRUserRequested()));
  previousDiagnosticBypass=diagnosticBypass;
 }
 const bool requested=control_rr::RRUserRequested()&&!diagnosticBypass;
 const bool partialRequested=control_rr::RRUserPartialRequested()&&!diagnosticBypass;
 const bool resizeGate=known?RRNativeResizeGate(requested,frame,outputWidth,outputHeight,width,height):!control_rr::RRUserRuntimePaused();
 const bool presetGate=known?RRNativePresetGate(requested,frame):!control_rr::RRUserRuntimePaused();
 const bool runtimeRequested=requested&&resizeGate&&presetGate&&!control_rr::RRUserRuntimePaused();
 if(known&&runtimeRequested&&(!control_rr::RRGBufferGuideMode||control_rr::RRTexturedCharacterGuides))RRDiffuseRequestExtent(frame,width,height);
 unsigned effects=0;const bool configuration=RRNativeReadRTSettings(&effects)&&control_rr::RRSupportsRTSettings(effects);
 // r20t: avoid guide-owner locks and warmup readiness checks while FULL RR is off.
 const bool ready=runtimeRequested&&known&&supported&&configuration&&RRNativeReady(width,height);
 const auto previousKey=rrNativeFrame.Key();const auto previousStage=rrNativeFrame.Stage();
 const bool wasRecovering=rrNativeFrame.Recovering(),wasStopped=rrNativeFrame.Stopped();
 const bool selected=rrNativeFrame.Begin({frame,outputWidth,outputHeight,width,height,effects},runtimeRequested,ready);
 // Snapshot diagnostic selection before either producer allocates hit arrays.
 const auto lightingMode=native_lighting::LiveSelection::GetMode(native_lighting::selection.Read());
 const unsigned specularRequest=selected&&rrNativeSpecularSamplesAvailable?native_lighting::SpecularSampleRequest(lightingMode):0;
 if(specularRequest!=rrNativeSpecularSampleRequest){
  rrNativeSpecularSampleRequest=specularRequest;
  rrNativeSpecularHistoryPending=true;
  rrNativeFrame.ResetForSignalChange();
 }
 const unsigned giRequest=native_lighting::GiSampleRequest(lightingMode,control_rr::GISampleRequested());
 if(rrNativeGISampleSelection.Latch(giRequest,selected)){
  rrNativeFrame.ResetForSignalChange();
  Log("RR_GI_SAMPLES_LIVE frame=%llu requested=%u rr_history_reset=1 apply=before_native_gi_allocation",frame,rrNativeGISampleSelection.requested);
 }
 const bool partial=partialRequested&&known&&supported&&configuration&&!rrNativeFrame.Stopped()&&!selected;
 rrNativePartialFrame.store(partial,std::memory_order_release);
 // Both filter policies use the same selection as the sample-count latch.
 if(known&&configuration)RRReportSampleOptions(verifiedRenderer,frame,selected,rrNativeGISampleSelection.requested);
 const bool giRequested=selected&&(effects&control_rr::RTDiffuseGI)&&
  native_lighting::PreservesGiFiltering(lightingMode);
 bool giFiltered=false;
 if(giRequested){
  const int passes=static_cast<int>(rrNativeGISampleOriginal(reinterpret_cast<unsigned char*>(verifiedRenderer)+0x913400));
  giFiltered=passes>0;
  if(!giFiltered)control_rr_diagnostic::Record(control_rr_diagnostic::Event::GIFilterUnavailable);
 }
 if(rrNativeGIKeepThisFrame!=giFiltered)rrNativeFrame.ResetForSignalChange();
 rrNativeGIKeepThisFrame=giFiltered;
 const bool temporalOnly=selected&&native_lighting::UsesSpecularTemporalOnly(lightingMode)&&control_rr_temporal::ready.load(std::memory_order_acquire);
 const bool specularFiltered=selected&&(native_lighting::PreservesSpecularFiltering(lightingMode)||temporalOnly);
 if(rrNativeSpecularKeepThisFrame!=specularFiltered||rrNativeSpecularTemporalThisFrame!=temporalOnly){
  rrNativeFrame.ResetForSignalChange();
  if(specularFiltered)rrNativeSpecularHistoryPending=true;
 }
 rrNativeSpecularKeepThisFrame=specularFiltered;rrNativeSpecularTemporalThisFrame=temporalOnly;
 static unsigned lastPolicy=~0u;
 const unsigned policy=unsigned(native_lighting::IsRtSpotMode(lightingMode))|(unsigned(giFiltered)<<1)|(unsigned(temporalOnly)<<2);
 if(policy!=lastPolicy||frame%240==0){
  native_lighting::Trace("lighting_policy frame=%llu spot_replacement=%u gi_filter=%u specular_temporal=%u specular_request=%u gi_request=%u rr_selected=%u",
   frame,unsigned(native_lighting::IsRtSpotMode(lightingMode)),unsigned(giFiltered),unsigned(temporalOnly),specularRequest,giRequest,unsigned(selected));lastPolicy=policy;
 }
 rrNativeRTFrame.Begin(frame,effects,selected,giFiltered);
 control_rr_diagnostic::lastEffects.store(effects,std::memory_order_relaxed);
 control_rr_diagnostic::lastSelected.store(selected,std::memory_order_relaxed);
 if(previousKey.frame&&previousKey.rtEffects!=effects)
  Log("RR_RT_SETTINGS frame=%llu old_effects=0x%X new_effects=0x%X rr_selected=%u reset=%u",frame,previousKey.rtEffects,effects,unsigned(selected),unsigned(rrNativeFrame.Reset()));
 if(previousKey.frame&&!control_rr::SameExtent(previousKey,rrNativeFrame.Key()))
  Log("RR_FRAME_RESIZE frame=%llu old_output=%ux%u new_output=%ux%u old_render=%ux%u new_render=%ux%u stopped=%u recovery=sr_rebuild_warmup_reset",frame,
   previousKey.outputWidth,previousKey.outputHeight,outputWidth,outputHeight,previousKey.width,previousKey.height,width,height,unsigned(rrNativeFrame.Stopped()));
 if(!wasRecovering&&rrNativeFrame.Recovering())
  Log("RR_FRAME_PAUSED frame=%llu previous_frame=%llu previous_stage=%u reason=%s recovery=complete_sr_then_rr_reset",frame,static_cast<unsigned long long>(previousKey.frame),unsigned(previousStage),rrNativeFrame.Resizing()?"resolution_changed":"frame_without_lighting");
 if(!wasStopped&&rrNativeFrame.Stopped())
  Log("RR_FRAME_STOP frame=%llu previous_frame=%llu previous_stage=%u reason=%s",frame,static_cast<unsigned long long>(previousKey.frame),unsigned(previousStage),rrNativeFrame.StopReason());
 rrNativeLightingComplete=false;
 rrLiveCaptureRRFrame=false;
 // Fail closed to native SR if we cannot keep Control's registered RR option at
 // OFF. The private jitter mirror is cleared before calling into native reset so
 // an exception/failure cannot inherit the previous frame's RR jitter state.
 if(!RRNativeKeepRegisteredOptionOff()||!rrNativeJitterOptionPatch.Set(false)){
  rrNativeFrame.Fail("native_registered_option_guard");rrLiveCaptureRRFrame=false;rrNativePartialFrame.store(false,std::memory_order_release);
  rrNativeJitterOptionPatch.Set(false);
  SetLastError(saved);
  const bool srResult=rrNativeResetOriginal(outputWidth,outputHeight,width,height,flag0,flag1,false,flag3,reset);
  const DWORD srError=GetLastError();
  bool srSupport=false,srActive=false;RRNativeReadMode(&srSupport,&srActive);
  RRPerfFrameMode(frame,outputWidth,outputHeight,width,height,requested,false,ready,effects,rrNativeGISampleSelection.requested);
  control_rr::RRUserWaitingStatus(known&&srSupport,configuration,effects);
  control_rr::RRUserFrameStatus(requested,false,true,false,rrNativeFrame.Resizing()||control_rr::RRUserRuntimePaused());
  Log("RR_NATIVE_OPTION_GUARD_FAILED frame=%llu sr_reset_success=%u sr_active=%u rr_stopped=1 registered_option_true_never_written=1",frame,unsigned(srResult),unsigned(srActive));
  SetLastError(srError);return srResult;
 }
 SetLastError(saved);
 bool result=rrNativeResetOriginal(outputWidth,outputHeight,width,height,flag0,flag1,selected,flag3,reset);
#ifdef CONTROL_FG_RR_VRAM_RESIZE_R2
 if(!result&&selected&&runtimeRequested){
  const DWORD firstError=GetLastError();
  if(RRNativeVramRecoverCachedRRFeature(frame,outputWidth,outputHeight,width,height)){
   SetLastError(saved);
   result=rrNativeResetOriginal(outputWidth,outputHeight,width,height,flag0,flag1,selected,flag3,reset);
   Log("RR_VRAM_NATIVE_CACHE_RETRY frame=%llu success=%u first_error=%lu retry_error=%lu output=%ux%u render=%ux%u",
    frame,unsigned(result),static_cast<unsigned long>(firstError),static_cast<unsigned long>(GetLastError()),
    outputWidth,outputHeight,width,height);
  }else SetLastError(firstError);
 }
#endif
 const DWORD nativeError=GetLastError();
 const bool observed=RRNativeReadMode(&supported,&active);
 rrNativeFrame.FeatureResult(result&&observed,active);
 const bool mirrorRR=control_rr::NativeRenderOption(rrNativeFrameEnabled,rrNativeFrame.Selected()||partial,rrNativeFrame.Stopped());
 if(!rrNativeJitterOptionPatch.Set(mirrorRR)){rrNativeFrame.Fail("native_jitter_option_mirror");rrNativePartialFrame.store(false,std::memory_order_release);}
 rrNativeSpecularSamplePatch.BeginFrame(rrNativeFrame.Selected()&&!rrNativeFrame.Stopped()&&!RRNativePartialActive()?specularRequest:0);
 RRPerfFrameMode(frame,outputWidth,outputHeight,width,height,requested,selected,ready,effects,rrNativeGISampleSelection.requested);
 control_rr::RRUserWaitingStatus(known&&supported,configuration,effects);
 control_rr::RRUserFrameStatus(requested,selected,rrNativeFrame.Stopped(),rrNativeFrame.Recovering(),rrNativeFrame.Resizing()||control_rr::RRUserRuntimePaused());
 const auto count=++rrNativeResetCalls;
 if(count<=4||(selected&&reset)||(partial&&(count%120)==0)||(count%240)==0||!result)
  Log("RR_FRAME_MODE frame=%llu mode=%s selected=%u partial=%u ready=%u supported=%u active=%u reset_success=%u stopped=%u width=%u height=%u reset=%u rt_effects=0x%X registered_option=off runtime_paused=%u runtime_pause_reasons=0x%X resize_epoch=%llu preset_epoch=%llu",frame,partial?"partial":(selected?"full":"off"),unsigned(selected),unsigned(partial),unsigned(ready),unsigned(supported),unsigned(active),unsigned(result),unsigned(rrNativeFrame.Stopped()),width,height,unsigned(reset),effects,unsigned(control_rr::RRUserRuntimePaused()),control_rr::RRUserRuntimePauseReasons(),rrNativeResizeEpoch.epoch,rrNativePresetEpoch.epoch);
 SetLastError(nativeError);return result;
}
static void RRNativeHookTemporalBind(void* technique) {
 if(!rrNativeTemporalBindOriginal)return;
 auto* scope=rrNativeSpecularScope;
 if(!rrNativeFrameEnabled||!scope){rrNativeTemporalBindOriginal(technique);return;}
 if(scope->preserveNativeFiltering){
  rrNativeTemporalBindOriginal(technique);
  scope->nativeFilterBound=true;
  rrClampFrameStrength=control_rr_clamp::Requested();
  return;
 }
 const DWORD saved=GetLastError();
 rrClampFrameStrength=control_rr_clamp::Requested();
 rrNativeClampLease={};scope->nativeClampPrepared=false;scope->nativeClampRestoreFailed=false;
 const HMODULE fullRenoDX=GetModuleHandleW(L"renodx-control-rr.addon64");
 // r21y production owns one signal path. If the full RenoDX Control RR add-on is
 // present it may replace the same temporal shader and hook NGX, so do not claim
 // native-clamp parity for that frame; use the proven raw-copy emergency fallback.
 scope->knownReplacementContamination=(fullRenoDX!=nullptr);
 if(scope->knownReplacementContamination){
  rrNativeClampLease.reason="full_renodx_addon_loaded";
  const auto falls=rrNativeClampFallbacks.fetch_add(1,std::memory_order_relaxed)+1;
  if(falls<=4||(scope->frame.frame%240)==0)
   Log("RR_NATIVE_SPECULAR_CLAMP_CONTAMINATION frame=%llu reason=%s fallback=raw_copy",
    static_cast<unsigned long long>(scope->frame.frame),rrNativeClampLease.reason);
 }
 // Every clean frame uses Control's untouched 0x600347E7 PSO and forces only
 // the current-thread g_uCameraCut provider across this exact bind -> dispatch span.
 if(control_rr::UseNativeSignalClamp(scope->knownReplacementContamination,true,rrClampFrameStrength)){
  bool armed=false;
  __try {armed=rrNativeClampApi.Arm(rrNativeClampLease);}
  __except(EXCEPTION_EXECUTE_HANDLER){rrNativeClampLease.reason="camera_cut_arm_exception";armed=false;}
  scope->nativeClampPrepared=armed;
  if(!armed){
   const auto falls=rrNativeClampFallbacks.fetch_add(1,std::memory_order_relaxed)+1;
   if(falls<=4||(scope->frame.frame%240)==0)
    Log("RR_NATIVE_SPECULAR_CLAMP_FALLBACK frame=%llu stage=temporal_bind reason=%s fallback=raw_copy",static_cast<unsigned long long>(scope->frame.frame),rrNativeClampLease.reason);
  }
 }
 SetLastError(saved);
 // Native exceptions deliberately propagate to the surrounding filter. Its
 // __finally restores any armed provider before unwinding.
 rrNativeTemporalBindOriginal(technique);
 const DWORD nativeError=GetLastError();
 if(scope->nativeClampPrepared&&rrNativeClampLease.armed)rrNativeClampLease.bindReturned=true;
 SetLastError(nativeError);
}
static void RRNativeHookDispatch(void* state,const void* groups) {
 auto* scope=rrNativeSpecularScope;
 if(!rrNativeFrameEnabled||!scope){rrNativeDispatchOriginal(state,groups);return;}
 const DWORD saved=GetLastError();
 auto access=rrReflectionAccess;access.current=&RRReflectionCurrent;access.user=nullptr;
 if(scope->preserveNativeFiltering){
  bool variantUsed=!scope->temporalOnly;
  control_rr_specular::ObserveFilteredDispatch(*scope,access.Callbacks(),rrNativeCopy,
   reinterpret_cast<std::uintptr_t>(verifiedRenderer),control_rr_specular::TemporalDispatchRva,
   reinterpret_cast<std::uintptr_t>(state),[&]{
    SetLastError(saved);
    if(scope->temporalOnly && scope->nativeFilterValidated) {
     (void)control_rr_clamp::Invoke(rrNativeDispatchOriginal,state,groups,
      reinterpret_cast<ID3D12GraphicsCommandList*>(scope->dispatch.list),rrClampFrameStrength,true,&variantUsed);
    }else rrNativeDispatchOriginal(state,groups);
   });
  const DWORD temporalError=GetLastError();
  if(scope->temporalOnly&&!variantUsed){scope->failed=true;scope->recorded=false;scope->reason="temporal_variant_unavailable";}
  if(scope->temporalOnly&&(rrNativeFrame.Reset()||scope->frame.frame%240==0||scope->failed))
   native_lighting::Trace("specular_temporal frame=%llu recorded=%u spatial_passes=%u history_percent=60 clamp_percent=60 variant_used=%u reason=%s",
    static_cast<unsigned long long>(scope->frame.frame),unsigned(scope->recorded&&!scope->failed),scope->spatialPasses,unsigned(variantUsed),scope->reason);
  SetLastError(temporalError);return;
 }
 const auto signalPath=control_rr::UseNativeSignalClamp(scope->knownReplacementContamination,scope->nativeClampPrepared,rrClampFrameStrength)
  ?control_rr_specular_policy::Path::NativeClamp
  :control_rr_specular_policy::Path::RawCopy;

 if(signalPath==control_rr_specular_policy::Path::NativeClamp){
  bool admitted=false,forced=false;
  __try {
   admitted=control_rr_specular::ValidateReferenceDispatch(*scope,access.Callbacks(),rrNativeCopy,
    reinterpret_cast<std::uintptr_t>(verifiedRenderer),control_rr_specular::TemporalDispatchRva,reinterpret_cast<std::uintptr_t>(state));
   if(admitted){
    forced=rrNativeClampLease.bindReturned&&rrNativeClampApi.StillForced(rrNativeClampLease);
    if(!forced)scope->reason=rrNativeClampLease.reason;
   }
  } __except(EXCEPTION_EXECUTE_HANDLER){scope->reason="native_clamp_validation_exception";}
  if(admitted&&forced){
   scope->attempted=true;scope->failed=true;scope->reason="native_control_energy_clamp_inflight";
   bool nativeReturned=false,restored=false;DWORD dispatchError=saved;
   SetLastError(saved);
   __try {
    if(!control_rr_clamp::Invoke(rrNativeDispatchOriginal,state,groups,reinterpret_cast<ID3D12GraphicsCommandList*>(scope->dispatch.list),rrClampFrameStrength))
     rrNativeFrame.Fail("clamp_strength_dispatch_count");
    dispatchError=GetLastError();
    static thread_local unsigned lastStrength=~0u,lastEffective=~0u;
    const unsigned actual=control_rr_clamp::effective.load(std::memory_order_acquire);
    if(lastStrength!=rrClampFrameStrength||lastEffective!=actual||(scope->frame.frame%240)==0){
     Log("RR_CLAMP_STRENGTH_CS3 frame=%llu requested=%u effective=%u applied=%u native_pso_restored=1",static_cast<unsigned long long>(scope->frame.frame),rrClampFrameStrength,actual,unsigned(control_rr_clamp::applied.load()));
     lastStrength=rrClampFrameStrength;lastEffective=actual;
    }
    nativeReturned=true;rrNativeClampLease.dispatched=true;
   } __finally {
    restored=rrNativeClampApi.Restore(rrNativeClampLease);
    if(!restored){
     scope->nativeClampRestoreFailed=true;
     rrNativeClampRestoreFailures.fetch_add(1,std::memory_order_relaxed);
    }
   }
   if(nativeReturned){
    control_rr_diagnostic::Record(control_rr_diagnostic::Event::ReflectionClamp);
    scope->recorded=true;scope->nativeClampDispatched=true;scope->failed=!restored;
    scope->reason=restored?"native_control_energy_clamp":"native_control_energy_clamp_restore_failed";
    const auto count=rrNativeClampDispatches.fetch_add(1,std::memory_order_relaxed)+1;
    if(count<=4||(scope->frame.frame%240)==0||!restored)
     Log("RR_SPECULAR_SIGNAL frame=%llu mode=native_control_energy_clamp count=%llu camera_cut_provider=%d original=%u forced=1 native_dispatch=1 raw_copy=0 restore=%u",
      static_cast<unsigned long long>(scope->frame.frame),count,rrNativeClampLease.provider,rrNativeClampLease.original,unsigned(restored));
    if(!restored)rrNativeFrame.Fail("native_camera_cut_restore");
    SetLastError(dispatchError);return;
   }
  }
  // We have not issued any replacement/copy command yet. Restore the provider
  // before falling back to the proven raw-copy path. If restoration itself is
  // unverified, reject RR for this frame and let Control execute its native
  // temporal dispatch rather than carrying uncertain provider state into more work.
  const char* failReason=scope->reason;
  bool restored=true;
  if(rrNativeClampLease.armed)restored=rrNativeClampApi.Restore(rrNativeClampLease);
  scope->nativeClampPrepared=false;
  const auto falls=rrNativeClampFallbacks.fetch_add(1,std::memory_order_relaxed)+1;
  if(falls<=4||(scope->frame.frame%240)==0||!restored)
   Log("RR_NATIVE_SPECULAR_CLAMP_FALLBACK frame=%llu stage=temporal_dispatch reason=%s restore=%u fallback=%s",
    static_cast<unsigned long long>(scope->frame.frame),failReason?failReason:"unknown",unsigned(restored),restored?"raw_copy":"native_game_dispatch_rr_rejected");
  if(!restored){
   scope->nativeClampRestoreFailed=true;scope->failed=true;rrNativeFrame.Fail("native_camera_cut_restore_before_fallback");
   SetLastError(saved);rrNativeDispatchOriginal(state,groups);return;
  }
 }

 bool substituted=false;
 __try {
  substituted=control_rr_specular::Substitute(*scope,access.Callbacks(),rrNativeCopy,reinterpret_cast<std::uintptr_t>(verifiedRenderer),
   control_rr_specular::TemporalDispatchRva,reinterpret_cast<std::uintptr_t>(state));
 } __except(EXCEPTION_EXECUTE_HANDLER){scope->failed=true;scope->reason="substitution_exception";}
 if(!substituted){
  rrNativeFrame.Fail();Log("RR_FRAME_STOP frame=%llu reason=temporal_substitution detail=%s attempted=%u",static_cast<unsigned long long>(scope->frame.frame),scope->reason,unsigned(scope->attempted));
  const auto& pair=scope->pair;
  Log("RR_TEMPORAL_PAIR source=%p expected_source=%p target=%p source_resource=%p target_resource=%p source_mips=%u target_mips=%u source_extent=%llux%u target_extent=%llux%u source_format=%u target_format=%u source_flags=0x%X target_flags=0x%X source_manual=%u target_manual=%u entry_list=%p dispatch_list=%p",
   reinterpret_cast<void*>(pair.source),reinterpret_cast<void*>(scope->sourceNative),reinterpret_cast<void*>(pair.target),reinterpret_cast<void*>(pair.sourceResource),reinterpret_cast<void*>(pair.targetResource),
   pair.shape.mips,pair.targetShape.mips,static_cast<unsigned long long>(pair.shape.width),pair.shape.height,static_cast<unsigned long long>(pair.targetShape.width),pair.targetShape.height,pair.shape.format,pair.targetShape.format,
   pair.sourceFlags,pair.targetFlags,unsigned(pair.sourceManual),unsigned(pair.targetManual),reinterpret_cast<void*>(scope->frame.list),reinterpret_cast<void*>(scope->dispatch.list));
  if(!scope->attempted){SetLastError(saved);rrNativeDispatchOriginal(state,groups);return;}
  RaiseException(0xE0425252u,EXCEPTION_NONCONTINUABLE,0,nullptr);
 }
 control_rr_diagnostic::Record(control_rr_diagnostic::Event::ReflectionRaw);
 control_rr_clamp::effective.store(0,std::memory_order_release);
 control_rr_clamp::applied.store(rrClampFrameStrength==0,std::memory_order_release);
 const auto count=scope->frame.frame;
 if(count<=4||(count%240)==0)
  Log("RR_CLAMP_STRENGTH_CS3_RAW frame=%llu raw_reflection=1 distance_value_cap=65504",static_cast<unsigned long long>(scope->frame.frame));
 if(count<=4||(count%240)==0)
  Log("RR_SPECULAR_SIGNAL frame=%llu mode=raw_copy_fallback native_dispatch=0 raw_copy=1 native_clamp_requested=runtime_selection",
   static_cast<unsigned long long>(scope->frame.frame));
 SetLastError(saved);
}
static void RRNativeHookContactShadowFilter(void* color,void* history,void* auxiliary,
 bool temporal,int spatialSize,int spatialStep) {
 if(!rrNativeFrameEnabled||!rrNativeContactShadowFilterOriginal){
  if(rrNativeContactShadowFilterOriginal)rrNativeContactShadowFilterOriginal(color,history,auxiliary,temporal,spatialSize,spatialStep);
  return;
 }
 const bool full=rrNativeFrame.Selected()&&!rrNativeFrame.Stopped();
 const bool contact=(rrNativeRTFrame.effects&control_rr::RTContactShadow)!=0;
 const bool fullSpot=native_lighting::rt_spot_runtime::NeedsNativeShadowFiltering();
 const DWORD filterError=GetLastError();
 native_lighting::rt_spot_runtime::ReportShadowFiltering(full,contact,temporal,spatialSize,spatialStep);
 SetLastError(filterError);
 const auto policy=control_rr::ContactShadowFilterPolicy(full,contact,temporal,spatialSize,spatialStep,fullSpot);
 if(!policy.neutralized){
  rrNativeContactShadowFilterOriginal(color,history,auxiliary,temporal,spatialSize,spatialStep);return;
 }
 const DWORD saved=GetLastError();
 SetLastError(saved);
 rrNativeContactShadowFilterOriginal(color,history,auxiliary,policy.temporal,policy.spatialSize,policy.spatialStep);
 const DWORD nativeError=GetLastError();
 const auto count=rrNativeContactShadowBypasses.fetch_add(1,std::memory_order_relaxed)+1;
 const auto frame=rrNativeFrame.Key().frame;
 if(count<=2||(frame&&frame%600==0))
  Log("RR_NOISY_CONTACT_SHADOW frame=%llu count=%llu temporal_in=%u temporal_out=0 spatial_size_in=%d spatial_size_out=0 spatial_step_in=%d spatial_step_out=1 native_call_preserved=1 feature_rr=1",
   static_cast<unsigned long long>(frame),count,unsigned(temporal),spatialSize,spatialStep);
 SetLastError(nativeError);
}
static void RRNativeHookBroadDiffuseCommon(unsigned site, void* color, void* history, unsigned passes) {
 control_rr_diagnostic::Record(site==0?control_rr_diagnostic::Event::BroadA:control_rr_diagnostic::Event::BroadB);
 if(!rrNativeDiffuseFilterOriginal)return;
 // These two DLF calls include temporal feedback even with zero spatial passes.
 // R98 can preserve the complete native call without changing RR inputs or shadow shaders.
 const DWORD entryError=GetLastError();
 const auto selected=native_lighting::selection.Read();
 const auto mode=native_lighting::LiveSelection::GetMode(selected);
 const bool full=rrNativeFrameEnabled&&rrNativeFrame.Selected()&&!rrNativeFrame.Stopped();
 const bool partial=RRNativePartialActive();
 const bool preserve=native_lighting::PreservesBroadDiffuseFiltering(mode);
 const auto policy=control_rr::BroadDiffusePolicy(full,partial,rrNativeRTFrame.effects,preserve);
 SetLastError(entryError);
 if(!policy.bypass){
  rrNativeDiffuseFilterOriginal(color,history,passes);
  control_rr_diagnostic::Record(control_rr_diagnostic::Event::BroadNative);
 }else control_rr_diagnostic::Record(control_rr_diagnostic::Event::BroadBypass);
 const DWORD resultError=GetLastError();
 // One bounded queue entry per site/policy transition. The UI thread writes the log.
 // RR selection is CPU state, not a claim that GPU reconstruction completed.
 struct Observation {
  bool valid=false, full=false, partial=false, bypass=false;
  uint64_t sequence=0;
  unsigned effects=0, passes=0;
 };
 static thread_local Observation observations[2];
 auto& last=observations[site];
 const auto sequence=native_lighting::LiveSelection::Sequence(selected);
 if(!last.valid||last.sequence!=sequence||last.full!=full||last.partial!=partial||
    last.bypass!=policy.bypass||last.effects!=rrNativeRTFrame.effects||last.passes!=passes){
  native_lighting::Trace("rr_broad_diffuse seq=%llu site=%u mode=%s rr_selected=%u partial=%u rt_effects=0x%X passes=%u preserve_requested=%u native_call_completed=%u bypass=%u gpu_confirmed=0",
   sequence,site,native_lighting::ModeName(mode),unsigned(full),unsigned(partial),rrNativeRTFrame.effects,passes,
   unsigned(preserve),unsigned(!policy.bypass),unsigned(policy.bypass));
  last={true,full,partial,policy.bypass,sequence,rrNativeRTFrame.effects,passes};
 }
 SetLastError(resultError);
 if(!policy.bypass)return;
 const DWORD saved=GetLastError();
 auto& counter=(site==0)?rrNativeBroadDiffuseBypassesA:rrNativeBroadDiffuseBypassesB;
 const auto count=counter.fetch_add(1,std::memory_order_relaxed)+1;
 const auto frame=rrNativeFrame.Key().frame;
 // Preserve the parent renderer function and all of its non-DLF work.  The raw
 // producer texture remains in place; only the temporal+spatial DLF transform is
 // omitted. This is intentionally FULL-only and reversible at the callsite.
 if(count<=2||(frame&&frame%600==0))
  Log("RR_NOISY_BROAD_DIFFUSE frame=%llu site=%u count=%llu passes_in=%u temporal_feedback_skipped=1 spatial_filter_skipped=1 parent_248940_preserved=1 rt_effects=0x%X feature_rr=1",
   static_cast<unsigned long long>(frame),site,count,passes,rrNativeRTFrame.effects);
 SetLastError(saved);
}
static void RRNativeHookBroadDiffuseA(void* color,void* history,unsigned passes) {RRNativeHookBroadDiffuseCommon(0,color,history,passes);}
static void RRNativeHookBroadDiffuseB(void* color,void* history,unsigned passes) {RRNativeHookBroadDiffuseCommon(1,color,history,passes);}
static void RRReportSpecularProducer() noexcept {
 const auto* observation=rrNativeSpecularSamplePatch.observation;
 if(!observation||!observation->seen)return;
 const auto now=*observation;
 static control_rr::SpecularSampleObservation previous{};
 const auto frame=rrNativeFrame.Key().frame;
 if(std::memcmp(&now,&previous,sizeof(now))||frame%240==0){
  native_lighting::Trace("specular_sample_producer frame=%llu native=%u effective=%u requested=%u rr_selected=%u allocation_and_shader=shared_local cpu_observation=1",
   static_cast<unsigned long long>(frame),now.nativeCount,now.effectiveCount,now.requested,unsigned(rrNativeFrame.Selected()));
  previous=now;
 }
}
static void RRNativeHookFilter(void* color,void* history,unsigned passes,bool evaluateColor) {
 control_rr_diagnostic::Record(control_rr_diagnostic::Event::ReflectionEntry);
 if(!rrNativeFrameEnabled){
  rrNativeFilterOriginal(color,history,passes,evaluateColor);
  control_rr_diagnostic::Record(control_rr_diagnostic::Event::ReflectionNative);return;
 }
 const DWORD saved=GetLastError();
 RRReportSpecularProducer();
 const auto perfStart=RRPerfClock();
 if(RRNativePartialActive()){
  if(rrNativeFrame.NeedsHistoryReset()){
   const bool historyReset=rrNativeHistory.ResetBeforeFilter();rrNativeFrame.HistoryResetResult(historyReset);
   Log("RR_NATIVE_HISTORY_RESET success=%u reason=enter_partial",unsigned(historyReset));
  }
  SetLastError(saved);rrNativeFilterOriginal(color,history,passes,evaluateColor);
  control_rr_diagnostic::Record(control_rr_diagnostic::Event::ReflectionNative);
  const DWORD nativeError=GetLastError();RRPerfFilter(rrNativeFrame.Key().frame,RRPerfElapsed(perfStart));
  if(rrNativeFrame.Key().frame%240==0)
   Log("RR_PARTIAL_FILTER frame=%llu spatial_passes=%u brdf=%u feature_rr=0 temporal_substitution=0 partial=1",static_cast<unsigned long long>(rrNativeFrame.Key().frame),passes,unsigned(evaluateColor));
  SetLastError(nativeError);return;
 }
 if(!rrNativeFrame.Selected()){
  if(rrNativeFrame.NeedsHistoryReset()){
   const bool reset=rrNativeHistory.ResetBeforeFilter();rrNativeFrame.HistoryResetResult(reset);
   Log("RR_NATIVE_HISTORY_RESET success=%u",unsigned(reset));
  }
  SetLastError(saved);rrNativeFilterOriginal(color,history,passes,evaluateColor);
  control_rr_diagnostic::Record(control_rr_diagnostic::Event::ReflectionNative);
  const DWORD nativeError=GetLastError();RRPerfFilter(rrNativeFrame.Key().frame,RRPerfElapsed(perfStart));SetLastError(nativeError);return;
 }
 control_rr_specular::Scope scope{};
 auto access=rrReflectionAccess;access.current=&RRReflectionCurrent;access.user=nullptr;
 bool ready=RRReflectionCurrent(nullptr,scope.frame)&&scope.frame.frame==rrNativeFrame.Key().frame&&!rrNativeSpecularScope;
 if(ready){scope.sourceNative=access.Callbacks().containerNative(&access,reinterpret_cast<std::uintptr_t>(color));ready=scope.sourceNative!=0;}
 if(!ready||!rrNativeFrame.BeginLighting(scope.frame.frame)){
  rrNativeFrame.Fail();Log("RR_FRAME_STOP reason=filter_frame_context");SetLastError(saved);rrNativeFilterOriginal(color,history,passes,evaluateColor);
  control_rr_diagnostic::Record(control_rr_diagnostic::Event::ReflectionNative);return;
 }
 scope.preserveNativeFiltering=rrNativeSpecularKeepThisFrame;
 scope.temporalOnly=rrNativeSpecularTemporalThisFrame;
 if(scope.preserveNativeFiltering){
  // Reset once at the pre-filter boundary on entry, not once per frame. Native
  // setup owns missing-history allocation and the ordinary camera-cut provider.
  const bool clean=GetModuleHandleW(L"renodx-control-rr.addon64")==nullptr;
  const bool historyReady=clean&&(!rrNativeSpecularHistoryPending||rrNativeHistory.ResetBeforeFilter());
  if(!historyReady){
   rrNativeFrame.Fail("specular_filter_history_or_replacement");
   control_rr_diagnostic::Record(control_rr_diagnostic::Event::SpecularFilterRejected);
   SetLastError(saved);rrNativeFilterOriginal(color,history,passes,evaluateColor);return;
  }
  rrNativeSpecularHistoryPending=false;
 }
 scope.rrCommitted=true;scope.spatialPasses=scope.preserveNativeFiltering&&!scope.temporalOnly?passes:0;
 rrNativeClampLease={};rrNativeSpecularScope=&scope;
 SetLastError(saved);
 // Let native exceptions propagate. The finally block is also the last-resort
 // provider restoration boundary if the bind or dispatch unwinds unexpectedly.
 __try {rrNativeFilterOriginal(color,history,scope.spatialPasses,evaluateColor);}
 __finally {
  if(rrNativeClampLease.armed){
   const bool restored=rrNativeClampApi.Restore(rrNativeClampLease);
   if(!restored){
    scope.nativeClampRestoreFailed=true;scope.failed=true;
    rrNativeClampRestoreFailures.fetch_add(1,std::memory_order_relaxed);
   }
  }
  rrNativeSpecularScope=nullptr;
 }
 const DWORD nativeError=GetLastError();rrNativeLightingComplete=scope.recorded&&!scope.failed;
 if(scope.preserveNativeFiltering)control_rr_diagnostic::Record(rrNativeLightingComplete?
  control_rr_diagnostic::Event::SpecularFilterValidated:control_rr_diagnostic::Event::SpecularFilterRejected);
 RRPerfFilter(scope.frame.frame,RRPerfElapsed(perfStart));
 if(!rrNativeLightingComplete)rrNativeFrame.Fail();
 if(!rrNativeLightingComplete||rrNativeFrame.Reset()||scope.frame.frame%240==0)
  Log("RR_NOISY_REFLECTION frame=%llu produced=%u native_clamp=%u raw_copy_fallback=%u reference_dispatch=%u restore_failed=%u failed=%u spatial_passes=%u brdf=%u signal=%s reason=%s",
   static_cast<unsigned long long>(scope.frame.frame),unsigned(scope.recorded),unsigned(scope.nativeClampDispatched),
   unsigned(scope.recorded&&!scope.preserveNativeFiltering&&!scope.nativeClampDispatched&&!scope.referenceDispatched),unsigned(scope.referenceDispatched),
   unsigned(scope.nativeClampRestoreFailed),unsigned(scope.failed),scope.spatialPasses,unsigned(evaluateColor),
   scope.preserveNativeFiltering?"native_specular_filtered":scope.referenceDispatched?"renodx_reference":(scope.nativeClampDispatched?"native_control_clamp":"raw_copy_fallback"),scope.reason);
 SetLastError(nativeError);
}
static bool RRNativeInstallFrame(HMODULE renderer,HMODULE d3d) noexcept {
 if(!renderer||renderer!=verifiedRenderer||!d3d||d3d!=verifiedD3d||!RRNativeInstallPreset(d3d)||!rrNativeCopy.Initialize(d3d)||!rrNativeHistory.Initialize(renderer)||!rrNativeClampApi.Initialize(renderer,d3d))return false;
 auto* base=reinterpret_cast<unsigned char*>(renderer);auto* native=reinterpret_cast<unsigned char*>(d3d);
 rrNativeResetOriginal=reinterpret_cast<RRNativeResetFn>(native+0x1f3f0);
 rrNativeFilterOriginal=reinterpret_cast<RRNativeFilterFn>(base+0x15bd0);
 rrNativeDispatchOriginal=reinterpret_cast<RRNativeDispatchFn>(native+0x30690);
 rrNativeTemporalBindOriginal=reinterpret_cast<RRNativeTemporalBindFn>(base+0x1db750);
 rrNativeContactShadowFilterOriginal=reinterpret_cast<RRNativeContactShadowFilterFn>(base+0x17050);
 rrNativeDiffuseFilterOriginal=reinterpret_cast<RRNativeDiffuseFilterFn>(base+0x14c90);
 rrNativeOptionOriginal=reinterpret_cast<RRNativeOptionFn>(base+0x13d20);
 rrNativeGIReleaseOriginal=reinterpret_cast<RRNativeGIReleaseFn>(base+0x13df0);
 rrNativeGISampleOriginal=reinterpret_cast<RRNativeGISampleFn>(base+0x2dac0);
 if(reinterpret_cast<void*>(GetProcAddress(d3d,"?dispatch@DeviceUtil@d3d@@SAXAEAVDeviceState@2@V?$Vector3Template@H@m@@@Z"))!=reinterpret_cast<void*>(rrNativeDispatchOriginal)||
  reinterpret_cast<void*>(rrNativeCopy.copy)!=native+0x3d190)return false;

 // The three option getter sites are the only reviewed render decisions which
 // need the native RR semantic: reflection spatial-pass selection, diffuse-GI
 // radius selection and diffuse-GI filter bypass. They now consult the current
 // mod-owned frame selection instead of Control's registered option object.
 // Optional diagnostic: preparation failure must never disable baseline RR.
 const bool specularPrepared=rrNativeSpecularSamplePatch.Prepare(base+0x128ff4,base+0x9137e0);
 native_lighting::Trace("specular_sample_hook stage=prepare ready=%u reason=%s baseline_rr_independent=1",
  unsigned(specularPrepared),rrNativeSpecularSamplePatch.failure);
 if(!RRAlbedoPrepareCallPatch(base+0x12ba17,reinterpret_cast<void*>(rrNativeGISampleOriginal),reinterpret_cast<void*>(&RRNativeGISampleCount),&rrNativeGISamplePatch)||
  !rrNativeResetPatch.Prepare(base+0x11b697,reinterpret_cast<void**>(base+0x5df758),reinterpret_cast<void*>(rrNativeResetOriginal),reinterpret_cast<void*>(&RRNativeHookReset))||
  !rrNativeDispatchPatch.Prepare(base+0x16786,reinterpret_cast<void**>(base+0x5dfcc0),reinterpret_cast<void*>(rrNativeDispatchOriginal),reinterpret_cast<void*>(&RRNativeHookDispatch))||
  !RRAlbedoPrepareCallPatch(base+0x16744,reinterpret_cast<void*>(rrNativeTemporalBindOriginal),reinterpret_cast<void*>(&RRNativeHookTemporalBind),&rrNativeTemporalBindPatch)||
  !RRAlbedoPrepareCallPatch(base+0x12b8ad,reinterpret_cast<void*>(rrNativeOptionOriginal),reinterpret_cast<void*>(&RRNativeReflectionOption),&rrNativeReflectionOptionPatch)||
  !RRAlbedoPrepareCallPatch(base+0x12bda4,reinterpret_cast<void*>(rrNativeOptionOriginal),reinterpret_cast<void*>(&RRNativeSelectedOption),&rrNativeGIRadiusOptionPatch)||
  !RRAlbedoPrepareCallPatch(base+0x12cd45,reinterpret_cast<void*>(rrNativeOptionOriginal),reinterpret_cast<void*>(&RRNativeGIBypassOption),&rrNativeGIBypassOptionPatch)||
  !RRAlbedoPrepareCallPatch(base+0x12b8d7,reinterpret_cast<void*>(rrNativeFilterOriginal),reinterpret_cast<void*>(&RRNativeHookFilter),&rrNativeFilterPatch)||
  !RRAlbedoPrepareCallPatch(base+0x12cd96,reinterpret_cast<void*>(rrNativeDiffuseFilterOriginal),reinterpret_cast<void*>(&RRNativeHookGIFilter),&rrNativeGIFilterPatch)||
  !RRAlbedoPrepareCallPatch(base+0x12cd55,reinterpret_cast<void*>(rrNativeGIReleaseOriginal),reinterpret_cast<void*>(&RRNativeHookGIRelease),&rrNativeGIReleasePatch)||
  !RRAlbedoPrepareCallPatch(base+0x23fb55,reinterpret_cast<void*>(rrNativeContactShadowFilterOriginal),reinterpret_cast<void*>(&RRNativeHookContactShadowFilter),&rrNativeContactShadowFilterPatch)||
  !RRAlbedoPrepareCallPatch(base+0x2489a4,reinterpret_cast<void*>(rrNativeDiffuseFilterOriginal),reinterpret_cast<void*>(&RRNativeHookBroadDiffuseA),&rrNativeBroadDiffuseFilterPatchA)||
  !RRAlbedoPrepareCallPatch(base+0x24a3f2,reinterpret_cast<void*>(rrNativeDiffuseFilterOriginal),reinterpret_cast<void*>(&RRNativeHookBroadDiffuseB),&rrNativeBroadDiffuseFilterPatchB)||
  !rrNativeJitterOptionPatch.Prepare(base+0x11de2e,base+0x914620))return false;

 // Clear any r20m-era leaked TRUE value only to the native OFF baseline. From
 // this point forward the registered option is never written TRUE by the mod.
 if(!RRNativeKeepRegisteredOptionOff()||!rrNativeJitterOptionPatch.Set(false))return false;

 bool temporalBind=false,reflectionOption=false,giRadiusOption=false,giBypassOption=false,filter=false,gi=false,contactShadow=false,broadA=false,broadB=false,jitter=false,giSamples=false,giFilter=false;
 giFilter=RRAlbedoExchangeCall(&rrNativeGIFilterPatch,true);
 if(giFilter&&rrNativeGIFilterPatch.writeHealthy&&rrNativeDispatchPatch.Exchange(true)&&rrNativeDispatchPatch.healthy){
  temporalBind=RRAlbedoExchangeCall(&rrNativeTemporalBindPatch,true);
  if(temporalBind&&rrNativeTemporalBindPatch.writeHealthy){
  reflectionOption=RRAlbedoExchangeCall(&rrNativeReflectionOptionPatch,true);
  if(reflectionOption&&rrNativeReflectionOptionPatch.writeHealthy){
   giRadiusOption=RRAlbedoExchangeCall(&rrNativeGIRadiusOptionPatch,true);
   if(giRadiusOption&&rrNativeGIRadiusOptionPatch.writeHealthy){
    giBypassOption=RRAlbedoExchangeCall(&rrNativeGIBypassOptionPatch,true);
    if(giBypassOption&&rrNativeGIBypassOptionPatch.writeHealthy){
     filter=RRAlbedoExchangeCall(&rrNativeFilterPatch,true);
     if(filter&&rrNativeFilterPatch.writeHealthy){
      gi=RRAlbedoExchangeCall(&rrNativeGIReleasePatch,true);
      if(gi&&rrNativeGIReleasePatch.writeHealthy){
       contactShadow=RRAlbedoExchangeCall(&rrNativeContactShadowFilterPatch,true);
       if(contactShadow&&rrNativeContactShadowFilterPatch.writeHealthy){
        broadA=RRAlbedoExchangeCall(&rrNativeBroadDiffuseFilterPatchA,true);
        if(broadA&&rrNativeBroadDiffuseFilterPatchA.writeHealthy){
         broadB=RRAlbedoExchangeCall(&rrNativeBroadDiffuseFilterPatchB,true);
         if(broadB&&rrNativeBroadDiffuseFilterPatchB.writeHealthy){
          jitter=rrNativeJitterOptionPatch.Exchange(true);
        if(jitter&&rrNativeJitterOptionPatch.healthy&&(giSamples=RRAlbedoExchangeCall(&rrNativeGISamplePatch,true))&&rrNativeGISamplePatch.writeHealthy&&rrNativeResetPatch.Exchange(true)&&rrNativeResetPatch.healthy){
         if(specularPrepared){
          const bool installed=rrNativeSpecularSamplePatch.Exchange(true);
          rrNativeSpecularSamplesAvailable=installed&&rrNativeSpecularSamplePatch.healthy;
          if(!rrNativeSpecularSamplesAvailable&&rrNativeSpecularSamplePatch.changed)
           rrNativeSpecularSamplePatch.Exchange(false);
         }
         native_lighting::specularSamplesReady.store(rrNativeSpecularSamplesAvailable,std::memory_order_release);
         native_lighting::Trace("specular_sample_hook stage=install ready=%u baseline_rr_independent=1",unsigned(rrNativeSpecularSamplesAvailable));
         rrNativeFrameEnabled=true;
         control_rr_diagnostic::Record(control_rr_diagnostic::Event::HooksInstalled);
         Log("RR_NATIVE_OPTION_ISOLATION ready=1 registered_option=forced_off_never_true reflection=mod_state gi_radius=mod_state gi_bypass=mod_state jitter=private_mirror partial_mode=mod_owned_option_reads_no_feature13");
         Log("RR_NATIVE_SPECULAR_CLAMP_READY ready=1 target_shader_crc=0x600347E7 camera_cut_provider=name_resolved_per_filter bind_site=0x16744 dispatch_site=0x16786 temporal_history=forced_zero current_frame_energy_clamp=CS3_default_60 restore=verified_each_dispatch reshade_required=0");
         Log("RR_FRAME_HOOKS_READY native_feature=13 early_mode=1 temporal_signal=CS3_integrated_adjustable raw_copy=off_or_fallback public_reference_ui=0 reflection_geometry_capture=D1_F_only hit_distance_runtime=D1_optional specular_mvec_runtime=0 guide_stats_readback=0 zero_spatial=1 preserve_brdf=1 native_gi_bypass=1 contact_shadow_denoiser=temporal_off_spatial_zero broad_diffuse_denoiser=temporal_and_spatial_bypassed full_rt_settings=1 option_isolation=1");
         return true;
        }
       }
      }
     }
    }
   }
  }
 }
}
}
}
 rrNativeFrameEnabled=false;rrNativePartialFrame.store(false,std::memory_order_release);rrNativeJitterOptionPatch.Set(false);RRNativeKeepRegisteredOptionOff();
 rrNativeSpecularSamplesAvailable=false;native_lighting::specularSamplesReady.store(false,std::memory_order_release);
 rrNativeSpecularSamplePatch.BeginFrame(0);
 if(rrNativeSpecularSamplePatch.changed)rrNativeSpecularSamplePatch.Exchange(false);
 if(rrNativeResetPatch.changed)rrNativeResetPatch.Exchange(false);
 if(giFilter)RRAlbedoExchangeCall(&rrNativeGIFilterPatch,false);
 if(giSamples)RRAlbedoExchangeCall(&rrNativeGISamplePatch,false);
 if(jitter)rrNativeJitterOptionPatch.Exchange(false);
 if(broadB)RRAlbedoExchangeCall(&rrNativeBroadDiffuseFilterPatchB,false);
 if(broadA)RRAlbedoExchangeCall(&rrNativeBroadDiffuseFilterPatchA,false);
 if(contactShadow)RRAlbedoExchangeCall(&rrNativeContactShadowFilterPatch,false);
 if(gi)RRAlbedoExchangeCall(&rrNativeGIReleasePatch,false);
 if(filter)RRAlbedoExchangeCall(&rrNativeFilterPatch,false);
 if(temporalBind)RRAlbedoExchangeCall(&rrNativeTemporalBindPatch,false);
 if(giBypassOption)RRAlbedoExchangeCall(&rrNativeGIBypassOptionPatch,false);
 if(giRadiusOption)RRAlbedoExchangeCall(&rrNativeGIRadiusOptionPatch,false);
 if(reflectionOption)RRAlbedoExchangeCall(&rrNativeReflectionOptionPatch,false);
 if(rrNativeDispatchPatch.changed)rrNativeDispatchPatch.Exchange(false);
 Log("RR_FRAME_HOOKS_FAILED mode_changes_disabled=1 relays_retained=1 option_isolation_rollback=1");return false;
}
