#pragma once
// Read the audited native raw-DGI ShaderTexture at the RR evaluation boundary.
// Called only while an F9 capture is pending. Never guess a writable state.
static bool RRInputCaptureReadGI(RRGuideInputTexture* out) noexcept {
 if(!out||!verifiedRenderer||!verifiedD3d)return false;
 using Get=void*(*)(void*);
 auto getter=reinterpret_cast<Get>(GetProcAddress(verifiedD3d,kRRGetNativeTextureSymbol));
 if(!getter)return false;
 __try {
  void* texture=getter(reinterpret_cast<unsigned char*>(verifiedRenderer)+kRRShaderDiffuseGIColorRva);
  if(!texture)return false;
  auto* resource=*reinterpret_cast<ID3D12Resource**>(static_cast<unsigned char*>(texture)+0x88);
  return RRInputCaptureReadNative(texture,resource,out);
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
