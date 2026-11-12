#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <array>
#include <cstring>
#include <utility>
namespace gi_reuse {
// Exact ebfb5b47 renderer / cceb99cb D3D layouts. Call synchronously in the
// admitted original ray/GI6 scopes while the host excludes scene mutation.
// Retaining a resource does NOT exclude writes or extend native wrapper life.
// This reads actual containers, never the later AA ShaderTexture globals.
class D3D12NativeGITextures {
 struct Texture {
  const void* container=nullptr;
  const void* native=nullptr;
  const void* tracker=nullptr;
  ID3D12Resource* resource=nullptr;
  UINT state=0;
 };
 std::array<Texture,3> hits_{};
 std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,3> retained_;
 const void* owner_=nullptr;
 const void* bundle_=nullptr;
 UINT width_=0,height_=0;
 static bool ReadContainer(const void* container,Texture* out)noexcept {
  if(!out)return false;*out={};if(!container)return false;
  __try {
   Texture next;next.container=container;
   const auto* native=*reinterpret_cast<const unsigned char*const*>(static_cast<const unsigned char*>(container)+8);
   if(!native)return false;next.native=native;
   UINT flags=0;std::memcpy(&flags,native+0x38,4);
   if((flags&0x30u)||native[0x68])return false;
   std::memcpy(&next.tracker,native+0x50,8);
   if(!next.tracker)next.tracker=native+0x40;
   std::memcpy(&next.state,static_cast<const unsigned char*>(next.tracker)+0x20,4);
   std::memcpy(&next.resource,native+0x88,8);
   if(!next.resource)return false;*out=next;return true;
  }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
 }
 static bool ReadHits(const void* owner,const void** bundle,std::array<Texture,3>* out)noexcept {
  if(!owner||!bundle||!out)return false;*bundle=nullptr;*out={};
  __try {
   const auto* data=*reinterpret_cast<const unsigned char*const*>(static_cast<const unsigned char*>(owner)+0x240);
   if(!data)return false;
   for(unsigned i=0;i<3;++i){
    const auto* container=*reinterpret_cast<const void*const*>(data+i*8);
    if(!ReadContainer(container,&(*out)[i])){*out={};return false;}
   }
   *bundle=data;return true;
  }__except(EXCEPTION_EXECUTE_HANDLER){*out={};return false;}
 }
 // Keep SEH and COM-only POD locals separate from C++ owning objects.
 static bool Retain(const Texture& texture,ID3D12Device* device,UINT width,UINT height,
                    ID3D12Resource** out)noexcept {
  if(!out)return false;*out=nullptr;
  __try {
   const auto d=texture.resource->GetDesc();
   if(d.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||d.Width!=width||d.Height!=height||
      d.DepthOrArraySize!=1||d.MipLevels!=1||d.SampleDesc.Count!=1||d.SampleDesc.Quality||
      !(d.Flags&D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)||
      (d.Flags&D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS))return false;
   ID3D12Device* actual=nullptr;
   if(FAILED(texture.resource->GetDevice(__uuidof(ID3D12Device),reinterpret_cast<void**>(&actual))))return false;
   const bool same=actual==device;if(actual)actual->Release();if(!same)return false;
   texture.resource->AddRef();*out=texture.resource;return true;
  }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
 }
 static bool Same(const Texture& a,const Texture& b)noexcept {
  return a.container==b.container&&a.native==b.native&&a.tracker==b.tracker&&a.resource==b.resource;
 }
public:
 D3D12NativeGITextures()=default;
 D3D12NativeGITextures(const D3D12NativeGITextures&)=delete;
 D3D12NativeGITextures& operator=(const D3D12NativeGITextures&)=delete;
 bool AcquireAtRay(const void* owner,ID3D12Device* device,UINT width,UINT height)noexcept {
  if(owner_||!device||!width||!height)return false;
  std::array<Texture,3> textures{};const void* bundle=nullptr;
  if(!ReadHits(owner,&bundle,&textures))return false;
  std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,3> retained;
  for(unsigned i=0;i<3;++i){
   if(textures[i].state!=D3D12_RESOURCE_STATE_UNORDERED_ACCESS)return false;
   for(unsigned j=0;j<i;++j)if(textures[i].resource==textures[j].resource)return false;
   if(!Retain(textures[i],device,width,height,retained[i].GetAddressOf()))return false;
  }
  std::array<Texture,3> again{};const void* current=nullptr;
  if(!ReadHits(owner,&current,&again)||current!=bundle)return false;
  for(unsigned i=0;i<3;++i)if(!Same(textures[i],again[i])||again[i].state!=textures[i].state)return false;
  hits_=textures;retained_=std::move(retained);owner_=owner;bundle_=bundle;width_=width;height_=height;return true;
 }
 // Native A3940 already transitioned these same three textures for GI6. Return
 // its tracked read states for the temporary UAV transitions and exact restore.
 bool ReadAtShade(const void* owner,std::array<D3D12_RESOURCE_STATES,3>& states)const noexcept {
  states={};if(!owner_||owner!=owner_)return false;
  std::array<Texture,3> textures{};const void* bundle=nullptr;
  if(!ReadHits(owner,&bundle,&textures)||bundle!=bundle_)return false;
  std::array<D3D12_RESOURCE_STATES,3> next{};
  for(unsigned i=0;i<3;++i){const UINT state=textures[i].state;
   if(!Same(hits_[i],textures[i])||!(state&D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)||
      (state&~UINT(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)))return false;
   next[i]=static_cast<D3D12_RESOURCE_STATES>(state);
  }
  states=next;return true;
 }
 // Pass the value read from renderer+90E9F8 at GI6, not a later ShaderTexture.
 bool AcquireOutputAtShade(const void* container,ID3D12Device* device,
                           Microsoft::WRL::ComPtr<ID3D12Resource>& output)const noexcept {
  if(!owner_||!device||output)return false;
  Texture texture;if(!ReadContainer(container,&texture)||texture.state!=D3D12_RESOURCE_STATE_UNORDERED_ACCESS)return false;
  for(const auto& hit:hits_)if(texture.resource==hit.resource)return false;
  Microsoft::WRL::ComPtr<ID3D12Resource> retained;
  if(!Retain(texture,device,width_,height_,retained.GetAddressOf()))return false;
  Texture again;if(!ReadContainer(container,&again)||!Same(texture,again)||again.state!=texture.state)return false;
  output=std::move(retained);return true;
 }
 std::array<ID3D12Resource*,3> Hits()const noexcept{return {retained_[0].Get(),retained_[1].Get(),retained_[2].Get()};}
};
}
