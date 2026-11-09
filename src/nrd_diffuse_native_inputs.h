#pragma once
#include "rr_provider_auth.h"
// Included after the audited native provider API, camera and Windows readers.
// These are synchronous, read-only snapshots at the temporal GI bind. No
// descriptor inventory or resource-creation observation is installed here.
namespace control_nrd_native {
struct ShaderTexture {std::uintptr_t descriptor=0,native=0;};
struct Vector2 {float values[2]{};};
struct Texture {
 ShaderTexture provider{};
 ID3D12Resource* resource=nullptr; // caller retains before leaving the boundary
 D3D12_RESOURCE_DESC desc{};
 D3D12_RESOURCE_STATES state=D3D12_RESOURCE_STATE_COMMON;
};
struct Inputs {
 Texture raw,motion,hit;
 CameraSnapshot camera{};
 double clipShift[2]{};
};
inline bool Resolve(const control_rr_native_clamp::Api& api,const char* name,int& id) noexcept {
 id=-1;if(!api.ready||!api.resolve)return false;
 __try {struct Slot {int id,tag;} slot{-1,0};api.resolve(&slot,name);id=slot.id;return id>=0&&id<4096;}
 __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
inline bool ReadTexture(const ShaderTexture& source,Texture& output) noexcept {
 output={};if(!source.native||!source.descriptor)return false;
 __try {
  const auto* native=reinterpret_cast<const unsigned char*>(source.native);
  if((*reinterpret_cast<const UINT*>(native+0x38)&0x30u)||native[0x68])return false;
  auto* resource=*reinterpret_cast<ID3D12Resource*const*>(native+0x88);if(!resource)return false;
  auto* tracker=*reinterpret_cast<const unsigned char*const*>(native+0x50);if(!tracker)tracker=native+0x40;
  const UINT state=*reinterpret_cast<const UINT*>(tracker+0x20);
  // Accept complete consuming read states, or the completed GI producer UAV.
  // Do not guess split barriers, copy states or per-subresource state modes.
  if(state!=D3D12_RESOURCE_STATE_UNORDERED_ACCESS&&(!(state&0xc0u)||(state&~0xe0u)))return false;
  const auto d=resource->GetDesc();
  if(d.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||!d.Width||!d.Height||d.Width>8192||d.Height>8192||
     d.DepthOrArraySize!=1||d.SampleDesc.Count!=1||d.SampleDesc.Quality||
     (d.Flags&(D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS|D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)))return false;
  output={source,resource,d,static_cast<D3D12_RESOURCE_STATES>(state)};return true;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
inline bool Read(const control_rr_native_clamp::Api& api,UINT64 frame,UINT64 view,Inputs& output,const char*& reason) noexcept {
 output={};reason="native_input_identity";
 if(!frame||!view||!api.ready)return false;
 __try {
  Inputs next{};DWORD fault=0;
  if(!ReadCamera(&next.camera,&fault,&reason))return false;
  if(next.camera.engineFrame!=frame){reason="native_camera_frame_changed";return false;}
  if(reinterpret_cast<UINT64>(next.camera.view)!=view){reason="native_camera_view_changed";return false;}
  const control_rr_provider::Access access{nullptr,&RRProviderEntry,&RRProviderCompare};
  const char* names[]={"g_DLF_rwtColorSource","g_tGeometryVelocityTexture","g_tHitInfo","g_vInvScreenRes","g_vInvOutputRes"};
  int ids[5]{};for(unsigned i=0;i<5;++i)if(!Resolve(api,names[i],ids[i])){reason="native_provider_name";return false;}
  ShaderTexture values[3]{};Vector2 inverseScreen{},inverseOutput{};
  reason="native_texture_provider";
  for(unsigned i=0;i<3;++i)if(!control_rr_provider::Snapshot(access,ids[i],&RRProviderReadBound,values[i]))return false;
  reason="native_viewport_provider";
  if(!control_rr_provider::Snapshot(access,ids[3],&RRProviderReadBound,inverseScreen)||
     !control_rr_provider::Snapshot(access,ids[4],&RRProviderReadBound,inverseOutput))return false;
  reason="native_texture_state";
  if(!ReadTexture(values[0],next.raw)||!ReadTexture(values[1],next.motion)||!ReadTexture(values[2],next.hit))return false;
  reason="native_viewport_mapping";
  const double extent[]={double(next.raw.desc.Width),double(next.raw.desc.Height)};
  for(unsigned i=0;i<2;++i)if(!std::isfinite(inverseScreen.values[i])||!std::isfinite(inverseOutput.values[i])||
     std::abs(inverseScreen.values[i]*extent[i]-1)>1e-4||std::abs(inverseOutput.values[i]*extent[i]-1)>1e-4)return false;
  if(next.motion.desc.Width!=next.raw.desc.Width||next.motion.desc.Height!=next.raw.desc.Height||
     next.hit.desc.Width!=next.raw.desc.Width||next.hit.desc.Height!=next.raw.desc.Height)return false;
  reason="native_jitter_read";
  if(!next.camera.renderer||!control_rr_reflection::WindowsAccess::Read(nullptr,
     reinterpret_cast<std::uintptr_t>(next.camera.renderer)+0x480,next.clipShift,sizeof(next.clipShift)))return false;
  for(double shift:next.clipShift)if(!std::isfinite(shift))return false;
  reason="native_inputs_changed";
  ShaderTexture check{};Vector2 vectorCheck{};
  for(unsigned i=0;i<3;++i)if(!control_rr_provider::Authenticate(access,ids[i],values[i],check))return false;
  if(!control_rr_provider::Authenticate(access,ids[3],inverseScreen,vectorCheck)||
     !control_rr_provider::Authenticate(access,ids[4],inverseOutput,vectorCheck))return false;
  UINT64 current=0;if(!ReadEngineFrameSafe(&current,&fault)){reason="native_frame_read_failed";return false;}
  if(current!=frame){reason="native_frame_changed_during_read";return false;}
  output=next;reason="ready";return true;
 }__except(EXCEPTION_EXECUTE_HANDLER){output={};reason="native_input_exception";return false;}
}
}
