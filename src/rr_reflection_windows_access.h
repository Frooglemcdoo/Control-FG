#pragma once
#include <cstring>
#include "rr_reflection_d3d12.h"
namespace control_rr_reflection {
// Resolve from the exact hash-verified d3d module only. These getter exports
// are distinct native interfaces even though both take one pointer on x64.
struct WindowsAccess {
 using Getter=void* (*)(void*);
 Getter container=nullptr,shader=nullptr;
 void* user=nullptr;
 bool (*current)(void*,Context&)=nullptr;
 bool Initialize(HMODULE d3dModule) noexcept {
  container=nullptr;shader=nullptr;
  if(!d3dModule)return false;
  container=reinterpret_cast<Getter>(GetProcAddress(d3dModule,
   "?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ"));
  shader=reinterpret_cast<Getter>(GetProcAddress(d3dModule,
   "?getNativeTexture@ShaderTexture@d3d@@QEAAPEAVNativeTexture@2@XZ"));
  return container&&shader;
 }
 static bool Read(void*,Address address,void* out,std::size_t size) noexcept {
  if(!address||!out||!size||size-1>UINTPTR_MAX-address)return false;
  __try {std::memcpy(out,reinterpret_cast<const void*>(address),size);return true;}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}
 }
 static Address Container(void* self,Address object) noexcept {
  __try {auto* a=static_cast<WindowsAccess*>(self);return a->container&&object?
   reinterpret_cast<Address>(a->container(reinterpret_cast<void*>(object))):0;}
  __except(EXCEPTION_EXECUTE_HANDLER){return 0;}
 }
 static Address Shader(void* self,Address object) noexcept {
  __try {auto* a=static_cast<WindowsAccess*>(self);return a->shader&&object?
   reinterpret_cast<Address>(a->shader(reinterpret_cast<void*>(object))):0;}
  __except(EXCEPTION_EXECUTE_HANDLER){return 0;}
 }
 static bool Describe(void*,Address resource,Shape& out,Address& device) noexcept {
  out={};device=0;ID3D12Device* held=nullptr;bool ok=false;
  __try {
   if(resource){auto* r=reinterpret_cast<ID3D12Resource*>(resource);auto d=r->GetDesc();
    if(SUCCEEDED(r->GetDevice(IID_PPV_ARGS(&held)))&&held){
     out={d.Width,d.Height,static_cast<std::uint32_t>(d.Format),d.DepthOrArraySize,d.MipLevels,
          d.SampleDesc.Count,d.SampleDesc.Quality,static_cast<std::uint32_t>(d.Flags),static_cast<std::uint32_t>(d.Dimension)};
     device=reinterpret_cast<Address>(held);ok=true;
    }
   }
  } __except(EXCEPTION_EXECUTE_HANDLER){ok=false;}
  // Device identity is borrowed from the live resource. Release the temporary
  // QueryInterface reference on both successful and rejected reads.
  if(held){__try{held->Release();}__except(EXCEPTION_EXECUTE_HANDLER){ok=false;}}
  if(!ok){out={};device=0;}return ok;
 }
 static bool Current(void* self,Context& out) noexcept {
  out={};
  __try {auto* a=static_cast<WindowsAccess*>(self);return a->current&&a->current(a->user,out);}
  __except(EXCEPTION_EXECUTE_HANDLER){out={};return false;}
 }
 Access Callbacks() noexcept {return {this,&Read,&Container,&Shader,&Describe,&Current};}
};
}
