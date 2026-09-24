#pragma once
#include "rr_preset_f.h"
#include "rr_evaluation_tail.h"
using RRNativeCreateFn=unsigned (*)(void*,unsigned,void*,void**);
static RRNativeCreateFn rrNativeCreateOriginal=nullptr;
static RRAlbedoCallPatch rrNativeCreatePatch{};
struct RRPresetApi {
 void* parameters;
 void Set(const char* key,unsigned value) const {
  using Fn=void (*)(void*,const char*,unsigned);
  reinterpret_cast<Fn>(reinterpret_cast<unsigned char*>(verifiedD3d)+0x51f50)(parameters,key,value);
 }
 bool Equals(const char* key,unsigned expected) const {
  unsigned actual=0;const auto result=ngxGetUInt(parameters,key,&actual);
  if(result!=1||actual!=expected){Log("RR_PRESET_REJECT key=%s result=0x%08X expected=%u actual=%u",key,result,expected,actual);return false;}
  return true;
 }
};
static bool RRNativeApplyPreset(void* parameters) noexcept {
 __try {return parameters&&ngxGetUInt&&control_rr::SetRRPreset(RRPresetApi{parameters},control_rr::RRUserPresetValue());}
 __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
struct RRPresetDimensions { unsigned width=0,height=0,outWidth=0,outHeight=0; };
static bool RRNativeReadPresetDimensions(void* parameters,RRPresetDimensions* out) noexcept {
 if(!parameters||!out||!ngxGetUInt)return false;*out={};
 __try {
  return ngxGetUInt(parameters,"Width",&out->width)==1&&ngxGetUInt(parameters,"Height",&out->height)==1&&
   ngxGetUInt(parameters,"OutWidth",&out->outWidth)==1&&ngxGetUInt(parameters,"OutHeight",&out->outHeight)==1&&
   out->width>=64&&out->height>=64&&out->outWidth>=out->width&&out->outHeight>=out->height&&out->outWidth<=8192&&out->outHeight<=8192;
 } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
static bool RRNativeLiveCreateContract(void* parameters) noexcept {
 if(!parameters||!ngxGetInt||!ngxGetUInt)return false;
 __try {
  int denoise=0;unsigned roughness=0,hwDepth=0;
  return ngxGetInt(parameters,"DLSS.Denoise.Mode",&denoise)==1&&denoise==1&&
   ngxGetUInt(parameters,"DLSS.Roughness.Mode",&roughness)==1&&roughness==1&&
   ngxGetUInt(parameters,"DLSS.Use.HW.Depth",&hwDepth)==1&&hwDepth==1;
 } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
struct RRPresetFeatureRecord {
 void* baseFeature=nullptr;
 void* feature=nullptr;
 unsigned preset=0;
 RRPresetDimensions dims{};
 bool alternate=false;
};
static SRWLOCK rrPresetFeatureLock=SRWLOCK_INIT;
// Experimental E/F live switching retains created NGX feature handles until
// process exit. We never destroy a feature while submitted GPU work could still
// reference it. There are at most two useful presets per Control base feature.
static std::vector<RRPresetFeatureRecord> rrPresetFeatureRecords;
static std::atomic<unsigned int> rrPresetLastEvaluated{0};
static std::atomic<unsigned long long> rrPresetLiveCreates{0};
static std::atomic<unsigned long long> rrPresetLiveSwitches{0};

static void RRNativeRememberControlFeature(void* feature,unsigned preset,void* parameters) noexcept {
 if(!feature||!control_rr::RRUserPresetSupported(preset))return;
 RRPresetDimensions dims{};RRNativeReadPresetDimensions(parameters,&dims);
 AcquireSRWLockExclusive(&rrPresetFeatureLock);
 bool found=false;for(auto& r:rrPresetFeatureRecords)if(!r.alternate&&r.feature==feature){r.preset=preset;r.dims=dims;found=true;break;}
 if(!found)rrPresetFeatureRecords.push_back({feature,feature,preset,dims,false});
 ReleaseSRWLockExclusive(&rrPresetFeatureLock);
}
static bool RRNativeFindControlFeature(void* feature,RRPresetFeatureRecord* out) noexcept {
 if(!feature||!out)return false;bool found=false;
 AcquireSRWLockShared(&rrPresetFeatureLock);
 for(const auto& r:rrPresetFeatureRecords)if(!r.alternate&&r.feature==feature){*out=r;found=true;break;}
 ReleaseSRWLockShared(&rrPresetFeatureLock);return found;
}
static void* RRNativeFindAlternateFeature(void* baseFeature,unsigned preset,const RRPresetDimensions& dims) noexcept {
 void* found=nullptr;AcquireSRWLockShared(&rrPresetFeatureLock);
 for(const auto& r:rrPresetFeatureRecords)if(r.alternate&&r.baseFeature==baseFeature&&r.preset==preset&&
  r.dims.width==dims.width&&r.dims.height==dims.height&&r.dims.outWidth==dims.outWidth&&r.dims.outHeight==dims.outHeight){found=r.feature;break;}
 ReleaseSRWLockShared(&rrPresetFeatureLock);return found;
}

// Resolve the NGX feature used by this evaluation. E<->F can switch live: if
// Control's cached feature was created for the other preset, create one mod-owned
// alternate feature from the same persistent parameter object and retain it.
// K/L/M remain diagnostic restart presets because current RR runtimes may map
// them to default behavior and we do not multiply experimental feature states.
static bool RRNativeResolveEvaluationPreset(void* list,void* controlFeature,void* parameters,
 void** evaluationFeature,bool* forceReset,unsigned* activePreset) noexcept {
 if(!evaluationFeature||!forceReset||!activePreset)return false;
 *evaluationFeature=controlFeature;*forceReset=false;*activePreset=0;
 const unsigned desired=control_rr::RRUserPresetValue();
 RRPresetFeatureRecord base{};
 if(!RRNativeFindControlFeature(controlFeature,&base)){
  Log("RR_PRESET_LIVE_REJECT reason=control_feature_unknown feature=%p requested=%s requested_value=%u",controlFeature,control_rr::RRUserPresetLabel(),desired);return false;
 }
 // Control and the mod-owned alternate feature share the persistent NGX
 // parameter object. Creating E leaves the preset hint keys at E. Before
 // activating a different feature epoch, restore those keys to the desired
 // preset so the feature handle and its evaluation parameters cannot disagree.
 const unsigned previousEvaluated=rrPresetLastEvaluated.load(std::memory_order_acquire);
 if(previousEvaluated!=desired){
  if(!RRNativeApplyPreset(parameters)){
   Log("RR_PRESET_LIVE_REJECT reason=evaluation_parameter_sync feature=%p previous=%u requested=%u",controlFeature,previousEvaluated,desired);return false;
  }
  Log("RR_PRESET_PARAMETER_SYNC from=%u to=%u feature=%p",previousEvaluated,desired,controlFeature);
 }
 if(base.preset==desired){
  *activePreset=desired;
  if(control_rr::RRUserConfirmedPresetValue()!=desired)control_rr::RRUserConfirmPresetCreate(desired);
 } else {
  if(!control_rr::RRUserPresetLiveSwitchable(base.preset,desired)){
   Log("RR_PRESET_LIVE_REJECT reason=restart_only_transition feature=%p base_preset=%u requested=%u",controlFeature,base.preset,desired);return false;
  }
  RRPresetDimensions dims{};
  if(!RRNativeReadPresetDimensions(parameters,&dims)||!RRNativeLiveCreateContract(parameters)){
   Log("RR_PRESET_LIVE_REJECT reason=create_contract feature=%p base_preset=%u requested=%u",controlFeature,base.preset,desired);return false;
  }
  void* alternate=RRNativeFindAlternateFeature(controlFeature,desired,dims);
  if(!alternate){
   void* created=nullptr;const DWORD saved=GetLastError();
   const unsigned result=rrNativeCreateOriginal(list,13,parameters,&created);const DWORD error=GetLastError();SetLastError(saved);
   if((result&0xFFF00000u)==0xBAD00000u||!created){
    Log("RR_PRESET_LIVE_CREATE result=0x%08X success=0 base_feature=%p requested=%s requested_value=%u created=%p",result,controlFeature,control_rr::RRUserPresetLabel(),desired,created);SetLastError(error);return false;
   }
   AcquireSRWLockExclusive(&rrPresetFeatureLock);
   rrPresetFeatureRecords.push_back({controlFeature,created,desired,dims,true});
   ReleaseSRWLockExclusive(&rrPresetFeatureLock);
   alternate=created;control_rr::RRUserConfirmPresetCreate(desired);
   const auto n=rrPresetLiveCreates.fetch_add(1,std::memory_order_relaxed)+1;
   Log("RR_PRESET_LIVE_CREATE result=0x%08X success=1 count=%llu base_feature=%p alternate_feature=%p preset=%s preset_value=%u render=%ux%u output=%ux%u retained_until_exit=1",result,n,controlFeature,alternate,control_rr::RRUserPresetLabel(),desired,dims.width,dims.height,dims.outWidth,dims.outHeight);
   SetLastError(error);
  }
  *evaluationFeature=alternate;*activePreset=desired;
  if(control_rr::RRUserConfirmedPresetValue()!=desired)control_rr::RRUserConfirmPresetCreate(desired);
 }
 const unsigned previous=rrPresetLastEvaluated.exchange(*activePreset,std::memory_order_acq_rel);
 if(previous&&previous!=*activePreset){
  *forceReset=true;const auto n=rrPresetLiveSwitches.fetch_add(1,std::memory_order_relaxed)+1;
  Log("RR_PRESET_LIVE_SWITCH count=%llu from=%u to=%u control_feature=%p evaluation_feature=%p reset=1 frame_boundary=1",n,previous,*activePreset,controlFeature,*evaluationFeature);
 }
 return true;
}

static unsigned RRNativeCreateWithPreset(void* list,unsigned feature,void* parameters,void** output) {
 const DWORD saved=GetLastError();
 if(feature!=13||!RRNativeApplyPreset(parameters)){
  if(output)*output=nullptr;
  Log("RR_PRESET_CREATE_REJECT feature=%u requested_preset=%s requested_value=%u native_create_called=0",feature,control_rr::RRUserPresetLabel(),control_rr::RRUserPresetValue());
  SetLastError(saved);return 0xBAD00001u;
 }
 const unsigned requested=control_rr::RRUserPresetValue();
 Log("RR_PRESET_REQUEST feature=13 preset=%s preset_value=%u quality_modes=6 readback=verified",control_rr::RRUserPresetLabel(),requested);
 SetLastError(saved);const unsigned result=rrNativeCreateOriginal(list,feature,parameters,output);const DWORD error=GetLastError();
 if((result&0xFFF00000u)!=0xBAD00000u&&output&&*output){control_rr::RRUserConfirmPresetCreate(requested);RRNativeRememberControlFeature(*output,requested,parameters);}
 Log("RR_PRESET_CREATE_RESULT result=0x%08X preset_requested=%s preset_value=%u confirmed_value=%u generation=%llu feature=%p",result,control_rr::RRUserPresetLabel(),requested,control_rr::RRUserConfirmedPresetValue(),control_rr::RRUserPresetCreateGeneration(),output?*output:nullptr);
 SetLastError(error);return result;
}
static bool RRNativeInstallPreset(HMODULE d3d) noexcept {
 if(d3d!=verifiedD3d||!d3d)return false;
 auto* base=reinterpret_cast<unsigned char*>(d3d);auto& p=rrNativeCreatePatch;
 p.site=base+0x1d457;auto* target=base+0x528d0;
 if(reinterpret_cast<void*>(GetProcAddress(d3d,"NVSDK_NGX_D3D12_CreateFeature"))!=target)return false;
 control_rr::Jump5 old{},next{};memcpy(old.data(),p.site,5);
 control_rr::Jump5 expected{};
 if(!control_rr::RelativeJump(reinterpret_cast<std::uintptr_t>(p.site),reinterpret_cast<std::uintptr_t>(target),expected)||old!=expected)return false;
 p.relay=AllocateExecutableRelayNear(p.site);if(!p.relay)return false;
 control_rr::Tail14 stub{};
 if(!control_rr::AbsoluteTail(reinterpret_cast<std::uintptr_t>(&RRNativeCreateWithPreset),stub)||
  !control_rr::CheckedTailPatch(old,reinterpret_cast<std::uintptr_t>(p.site),reinterpret_cast<std::uintptr_t>(target),reinterpret_cast<std::uintptr_t>(p.relay),next))return false;
 memcpy(p.original,old.data(),5);memcpy(p.replacement,next.data(),5);memcpy(p.relay,stub.data(),stub.size());
 DWORD previous=0;if(!VirtualProtect(p.relay,4096,PAGE_EXECUTE_READ,&previous)||!FlushInstructionCache(GetCurrentProcess(),p.relay,stub.size()))return false;
 rrNativeCreateOriginal=reinterpret_cast<RRNativeCreateFn>(target);
 if(!RRAlbedoExchangeCall(&p,true))return false;
 if(!p.writeHealthy){RRAlbedoExchangeCall(&p,false);return false;}
 Log("RR_PRESET_HOOK_READY site=0x1D457 target=0x528D0 default_preset=F selectable=E,F,K,L,M live_switch=E<->F restart_only=K,L,M create_confirmation=1 alternate_features=retained_until_exit");return true;
}
