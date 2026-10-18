#pragma once
// Borrow AA wrappers only during the synchronous originalAA invocation.
static thread_local void* rrInputCaptureNativeTextures[9]{};
static thread_local unsigned long long rrInputCaptureNativeFrame=0;
static void RRInputCaptureSetNativeTextures(void* const* textures,unsigned count,unsigned long long frame) noexcept {
 for(unsigned i=0;i<9;++i)rrInputCaptureNativeTextures[i]=textures&&i<count?textures[i]:nullptr;
 rrInputCaptureNativeFrame=frame;
}
static bool RRInputCaptureReadNative(void* texture,ID3D12Resource* expected,RRGuideInputTexture* out) noexcept {
 __try {
  if(!texture||!expected)return false;
  auto* p=static_cast<unsigned char*>(texture);*out={};out->nativeTexture=texture;
  out->nativeFlags=*reinterpret_cast<unsigned*>(p+0x38);out->manualStateDisabled=p[0x68];
  if((out->nativeFlags&0x30u)||out->manualStateDisabled)return false;
  out->resource=*reinterpret_cast<ID3D12Resource**>(p+0x88);
  if(out->resource!=expected)return false;
  out->stateTracker=*reinterpret_cast<void**>(p+0x50);if(!out->stateTracker)out->stateTracker=p+0x40;
  out->trackedState=*reinterpret_cast<unsigned*>(static_cast<unsigned char*>(out->stateTracker)+0x20);
  // Only established read states. No split, writable, copy or guessed states.
  if(!(out->trackedState&0x40u)||(out->trackedState&~0xe0u))return false;
  out->desc=expected->GetDesc();
  return !(out->desc.Flags&D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS);
 } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
static bool RRInputCaptureFindNative(unsigned long long frame,ID3D12Resource* const* resources,RRGuideInputTexture* out) noexcept {
 if(frame!=rrInputCaptureNativeFrame)return false;
 for(unsigned i=0;i<3;++i){bool found=false;
  for(void* texture:rrInputCaptureNativeTextures)if(RRInputCaptureReadNative(texture,resources[i],&out[i])){found=true;break;}
  if(!found)return false;
 }
 return true;
}
