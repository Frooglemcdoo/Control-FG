#pragma once
// R20 comparison setting. Read once so filter ownership cannot change mid-frame.
// Missing/unreadable configuration preserves the confirmed R19 route.
static bool RRNativeGIRawInputRequested() noexcept {
 static const bool raw=[]() noexcept {
  const DWORD saved=GetLastError();
  wchar_t path[32768]{};
  const DWORD length=GetModuleFileNameW(nullptr,path,32768);
  bool requested=false;
  if(length&&length<32768){
   wchar_t* slash=wcsrchr(path,L'\\');
   if(slash&&wcscpy_s(slash+1,32768-size_t(slash+1-path),L"Control-RR-GI-Test.ini")==0)
    requested=GetPrivateProfileIntW(L"GI",L"RawInput",0,path)==1;
  }
  Log("RR_GI_INPUT_R20 mode=%s restart_required=1",requested?"raw_to_rr":"r19_filtered");
  SetLastError(saved);return requested;
 }();
 return raw;
}
