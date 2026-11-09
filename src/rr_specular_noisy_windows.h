#pragma once
#include "rr_specular_noisy.h"
// Native adapters used by the r20 frame coordinator.
// Caller must pass the already hash-verified d3d module and committed frame scope.
namespace control_rr_specular {
struct NativeCopyApi {
 struct Int3 {int x,y,z;};
 using StateFn=void* (*)();
 using CopyFn=void (*)(void*,int,int,const Int3&,void*,int,int,const Int3&,const Int3*);
 StateFn state=nullptr;CopyFn copy=nullptr;
 bool Initialize(HMODULE verifiedModule) noexcept {
  state=nullptr;copy=nullptr;if(!verifiedModule)return false;
  state=reinterpret_cast<StateFn>(GetProcAddress(verifiedModule,"?retrieveForThread@DeviceState@d3d@@SAAEAV12@XZ"));
  copy=reinterpret_cast<CopyFn>(GetProcAddress(verifiedModule,"?copyRegion@NativeTextureUtil@d3d@@SAXPEAVNativeTexture@2@HHAEBV?$Vector3Template@H@m@@0HH1PEBV45@@Z"));
  return state&&copy;
 }
 bool StateMatches(Address expected) noexcept {
  __try {return expected&&state&&reinterpret_cast<Address>(state())==expected;}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}
 }
 bool CopyNative(Address target,Address source) noexcept {
  if(!copy||!target||!source||target==source)return false;
  const Int3 zero{0,0,0};
  __try {copy(reinterpret_cast<void*>(target),0,0,zero,reinterpret_cast<void*>(source),0,0,zero,nullptr);return true;}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}
 }
};
// Native resize uses this same routine to detach both histories. The following
// filter invocation allocates/clears missing histories. Only call at the native
// pre-filter boundary on the recording thread, never inside an active filter.
struct NativeHistoryReset {
 using ResetFn=void (*)();
 ResetFn reset=nullptr;
 bool failed=false;
 bool Initialize(HMODULE verifiedRendererModule) noexcept {
  reset=nullptr;failed=false;if(!verifiedRendererModule)return false;
  reset=reinterpret_cast<ResetFn>(reinterpret_cast<unsigned char*>(verifiedRendererModule)+0x13e20);
  return true;
 }
 bool ResetBeforeFilter() noexcept {
  if(!reset||failed)return false;
  // A fault can follow a partial detach/release. Do not retry it or manually
  // write native pool pointers to make the reset appear successful.
  __try {reset();return true;}
  __except(EXCEPTION_EXECUTE_HANDLER){failed=true;return false;}
 }
};
}
