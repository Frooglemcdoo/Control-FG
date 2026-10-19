#pragma once
// R27 isolates the native GI producer radius branch at renderer+0x12BDA4.
// It never writes a radius constant or changes the later filtering branch.
static bool RRGIStockRadiusEnabled() {
 static const bool enabled=[](){
  auto path=ModulePath(selfModule);const auto slash=path.find_last_of(L"\\/");
  if(slash==std::wstring::npos)return false;
  path.resize(slash+1);path+=L"Control-RR-GI-Radius-Test.ini";
  const bool on=GetPrivateProfileIntW(L"GIRadiusTest",L"UseStockRadius",0,path.c_str())==1;
  Log("RR_GI_RADIUS_R27_CONFIG stock_radius=%u restart_required=1",unsigned(on));return on;
 }();return enabled;
}
static bool RRGIRadiusOption(void* option) {
 const DWORD saved=GetLastError();
 const bool selected=RRNativeSelectedOption(option);
 const bool stock=selected&&!RRNativePartialActive()&&
  (rrNativeRTFrame.effects&control_rr::RTDiffuseGI)&&RRGIStockRadiusEnabled();
 SetLastError(saved);return stock?false:selected;
}
static void RRGIRadiusReport(unsigned long long frame) noexcept {
 if(!rrNativeFrame.Selected()||RRNativePartialActive())return;
 const auto base=reinterpret_cast<std::uintptr_t>(verifiedRenderer);
 float radius=0,rrMultiplier=0,stockMultiplier=0;
 const bool read=control_rr_reflection::WindowsAccess::Read(nullptr,base+0x12972b8,&radius,sizeof(radius))&&
  control_rr_reflection::WindowsAccess::Read(nullptr,base+0x67f1b0,&rrMultiplier,sizeof(rrMultiplier))&&
  control_rr_reflection::WindowsAccess::Read(nullptr,base+0x67ef88,&stockMultiplier,sizeof(stockMultiplier));
 Log("RR_GI_RADIUS_R27 frame=%llu stock_radius=%u values_read=%u radius=%.9g rr_multiplier=%.9g stock_multiplier=%.9g native_filter_skipped=1",
  frame,unsigned(RRGIStockRadiusEnabled()),unsigned(read),double(radius),double(rrMultiplier),double(stockMultiplier));
}
