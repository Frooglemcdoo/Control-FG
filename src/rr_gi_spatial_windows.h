#pragma once
#include "rr_gi_spatial_policy.h"
// Hash-verified renderer diffuse-filter.asm: 0x151d6 obtains the native
// history texture; 0x151e3 clears it to float4(0) on first allocation.
// Reuse that initialization operation before each selected Native Blend GI
// filter, retaining temporal dispatch initialization and all spatial passes.
namespace control_rr_gi_spatial {
struct WindowsApi {
 using TextureFn=void* (*)(void*);
 struct alignas(16) Float4 {float x,y,z,w;};
 using ClearFn=void (*)(void*,const Float4&);
 TextureFn texture=nullptr;ClearFn clear=nullptr;
 bool attempted=false,ready=false;
 bool Initialize() noexcept {
  if(attempted)return ready;
  attempted=true;
  if(!verifiedRenderer||!verifiedD3d)return false;
  texture=reinterpret_cast<TextureFn>(GetProcAddress(verifiedD3d,"?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ"));
  clear=reinterpret_cast<ClearFn>(GetProcAddress(verifiedD3d,"?clearUAV@DeviceUtil@d3d@@SAXPEAVNativeTexture@2@V?$Vector4Template@M@m@@@Z"));
  auto* base=reinterpret_cast<unsigned char*>(verifiedRenderer);
  __try {
   ready=texture&&clear&&*reinterpret_cast<TextureFn*>(base+0x5dfd30)==texture&&
    *reinterpret_cast<ClearFn*>(base+0x5dfd40)==clear;
  } __except(EXCEPTION_EXECUTE_HANDLER){ready=false;}
  return ready;
 }
 bool Read(std::uintptr_t slot,std::uintptr_t& value) noexcept {
  __try {value=*reinterpret_cast<std::uintptr_t*>(slot);return true;}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}
 }
 std::uintptr_t Texture(std::uintptr_t container) noexcept {
  __try {return reinterpret_cast<std::uintptr_t>(texture(reinterpret_cast<void*>(container)));}
  __except(EXCEPTION_EXECUTE_HANDLER){return 0;}
 }
 bool Clear(std::uintptr_t target) noexcept {
  const Float4 zero{};
  __try {clear(reinterpret_cast<void*>(target),zero);return true;}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}
 }
};
}
static bool RRNativePrepareGISpatial(void* history) noexcept {
 static control_rr_gi_spatial::WindowsApi api;
 static unsigned long long calls=0,cleared=0,initialized=0,failures=0;
 ++calls;
 const auto result=api.Initialize()?control_rr_gi_spatial::Prepare(api,
  reinterpret_cast<std::uintptr_t>(history),reinterpret_cast<std::uintptr_t>(verifiedRenderer)+0x90eac8):control_rr_gi_spatial::Result::Failed;
 const bool okay=result!=control_rr_gi_spatial::Result::Failed;
 if(result==control_rr_gi_spatial::Result::Cleared)++cleared;
 if(result==control_rr_gi_spatial::Result::NativeInitialization)++initialized;
 if(!okay)++failures;
 if(calls<=4||calls%240==0||!okay)
  native_lighting::Trace("gi_spatial_only build=R207 applied=%u calls=%llu cleared=%llu native_initialization=%llu failures=%llu spatial_passes=native history_reallocation=0",unsigned(okay),calls,cleared,initialized,failures);
 return okay;
}
