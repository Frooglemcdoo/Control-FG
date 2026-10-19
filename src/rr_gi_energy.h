#pragma once
// R26: GI-only native current-frame energy clamp; no spatial iterations.
// The pre-accumulator's camera-cut provider disables its history contribution.
static RRIndirectCallPatch rrGIEnergyDispatchPatch;
struct RRGIEnergyScope {
 control_rr_native_clamp::Lease lease{};
 unsigned dispatches=0;
 bool verified=true;
};
static thread_local RRGIEnergyScope* rrGIEnergyScope=nullptr;
static bool RRGIEnergyEnabled() {
 static const bool enabled=[](){
  auto path=ModulePath(selfModule);const auto slash=path.find_last_of(L"\\/");
  if(slash==std::wstring::npos)return false;
  path.resize(slash+1);path+=L"Control-RR-GI-Energy-Test.ini";
  const bool on=GetPrivateProfileIntW(L"GIEnergyTest",L"CurrentFrameClamp",0,path.c_str())==1;
  Log("RR_GI_ENERGY_R26_CONFIG enabled=%u restart_required=1",unsigned(on));return on;
 }();return enabled;
}
static void RRGIEnergyDispatch(void* state,const void* groups) {
 auto* scope=rrGIEnergyScope;
 if(scope){
  ++scope->dispatches;
  if(scope->dispatches!=1||!rrNativeClampApi.StillForced(scope->lease)){
   scope->verified=false;rrNativeFrame.Fail("gi_energy_camera_cut_contract");
   // Never execute an unverified history blend in this experimental scope.
   return;
  }
 }
 rrNativeDispatchOriginal(state,groups);
}
static void* RRGIEnergyColor() noexcept {
 __try {
  auto* base=reinterpret_cast<unsigned char*>(verifiedRenderer);
  // Same container/getter as the original GI filter call at +0x12CD96.
  using Get=void*(*)(void*);
  return reinterpret_cast<Get>(base+0x133a0)(base+0x90e9f8);
 }__except(EXCEPTION_EXECUTE_HANDLER){return nullptr;}
}
static void RRGIEnergyBeforeRelease(void* history) {
 if(!rrNativeFrameEnabled||!rrNativeFrame.Selected()||RRNativePartialActive()||
    !(rrNativeRTFrame.effects&control_rr::RTDiffuseGI)||!RRGIEnergyEnabled()||
    rrGIEnergyScope||history!=reinterpret_cast<unsigned char*>(verifiedRenderer)+0x90eac8)return;
 const DWORD saved=GetLastError();
 void* color=RRGIEnergyColor();RRGIEnergyScope scope{};
 if(GetModuleHandleW(L"renodx-control-rr.addon64")||!color||!rrNativeDiffuseFilterOriginal||!rrNativeClampApi.Arm(scope.lease)){
  if(rrNativeRTFrame.frame%240==0)Log("RR_GI_ENERGY_R26 applied=0 reason=unavailable fallback=r21_raw");
  SetLastError(saved);return;
 }
 bool restored=false;
 __try {
  rrGIEnergyScope=&scope;
  rrNativeDiffuseFilterOriginal(color,history,0);
 }__finally {
  const DWORD error=GetLastError();rrGIEnergyScope=nullptr;
  restored=rrNativeClampApi.Restore(scope.lease);
  if(!restored||!scope.verified||scope.dispatches!=1)rrNativeFrame.Fail("gi_energy_dispatch_or_restore");
  if(rrNativeFrame.Reset()||rrNativeRTFrame.frame%240==0||!restored||!scope.verified||scope.dispatches!=1)
   Log("RR_GI_ENERGY_R26 frame=%llu applied=%u dispatches=%u camera_cut_verified=%u restored=%u spatial_passes=0 history_weight=0 native_history_released_after=1",
    static_cast<unsigned long long>(rrNativeRTFrame.frame),unsigned(restored&&scope.verified&&scope.dispatches==1),scope.dispatches,unsigned(scope.verified),unsigned(restored));
  SetLastError(error);
 }
 SetLastError(saved);
}
