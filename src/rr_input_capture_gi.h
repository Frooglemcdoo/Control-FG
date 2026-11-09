#pragma once
// Read the audited native raw-DGI ShaderTexture at the RR evaluation boundary.
// Called only while an overlay capture is pending. Never guess a writable state.
static bool RRInputCaptureReadShaderTexture(size_t offset,RRGuideInputTexture* out) noexcept {
 if(!out||!verifiedRenderer||!verifiedD3d)return false;
 using Get=void*(*)(void*);
 auto getter=reinterpret_cast<Get>(GetProcAddress(verifiedD3d,kRRGetNativeTextureSymbol));
 if(!getter)return false;
 __try {
  void* texture=getter(reinterpret_cast<unsigned char*>(verifiedRenderer)+offset);
  if(!texture)return false;
  auto* resource=*reinterpret_cast<ID3D12Resource**>(static_cast<unsigned char*>(texture)+0x88);
  return RRInputCaptureReadNative(texture,resource,out);
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}

static bool RRInputCaptureReadGI(RRGuideInputTexture* out) noexcept {
 return RRInputCaptureReadShaderTexture(kRRShaderDiffuseGIColorRva,out);
}
static bool RRInputCaptureReadLighting(RRGuideInputTexture* diffuse,RRGuideInputTexture* specular) noexcept {
 return RRInputCaptureReadShaderTexture(kRRShaderLightBufferDiffuseRva,diffuse)&&
        RRInputCaptureReadShaderTexture(kRRShaderLightBufferSpecularRva,specular);
}
