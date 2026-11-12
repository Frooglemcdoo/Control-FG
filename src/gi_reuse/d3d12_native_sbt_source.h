#pragma once
#include <d3d12.h>
#include <cstring>
#include <cstdint>
namespace gi_reuse {
// Exact cceb99cb... D3D wrapper, at the original GI ray call. The caller must
// establish the pinned engine build and current native writer/dispatch identity.
// Never probe arbitrary pointers or retain the wrapper beyond this call.
// Acquires only SBT resource storage, not the scene referenced by SBT records.
inline bool AcquireNativeGISbtSource(const void* wrapper,UINT64 expectedBase,
                                    UINT64 expectedCapacity,ID3D12Resource** output) noexcept {
 if(!output)return false;*output=nullptr;
 if(!wrapper||!expectedBase||expectedBase%64||!expectedCapacity)return false;
 __try {
  const auto* data=static_cast<const unsigned char*>(wrapper);
  ID3D12Resource* resource=nullptr;UINT64 capacity=0,base=0;
  std::memcpy(&resource,data,8);std::memcpy(&capacity,data+0x30,8);std::memcpy(&base,data+0x48,8);
  if(!resource||base!=expectedBase||capacity!=expectedCapacity)return false;
  const auto desc=resource->GetDesc();
  if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER||desc.Width<capacity||resource->GetGPUVirtualAddress()!=base)return false;
  D3D12_HEAP_PROPERTIES heap{};D3D12_HEAP_FLAGS flags{};
  if(FAILED(resource->GetHeapProperties(&heap,&flags))||heap.Type!=D3D12_HEAP_TYPE_DEFAULT)return false;
  resource->AddRef();*output=resource;return true;
 }__except(EXCEPTION_EXECUTE_HANDLER){*output=nullptr;return false;}
}
// Resolve the CURRENT ray invocation's alternating native slot. The native
// raytrace routine increments context+a8's counter before selecting this slot.
// Call synchronously inside its DispatchRays hook, before native raytrace can
// advance/reuse either slot. No global resource registry or later TLS lookup.
// A host must establish exact native build/scope before supplying context.
inline bool AcquireNativeGICurrentSbt(const void* context,UINT64 expectedBase,
                                     ID3D12Resource** output,UINT64* capacity)noexcept {
 if(!output||!capacity)return false;*output=nullptr;*capacity=0;
 if(!context||!expectedBase)return false;
 __try {
  const auto* base=static_cast<const unsigned char*>(context);
  const auto* timeline=*reinterpret_cast<const unsigned char* const*>(base+0xa8);
  if(!timeline)return false;
  const auto serial=*reinterpret_cast<const UINT64*>(timeline);
  if(!serial||serial==UINT64_MAX)return false;
  const auto* wrapper=base+(serial&1)*0x50;
  UINT64 size=0;std::memcpy(&size,wrapper+0x30,8);
  if(!AcquireNativeGISbtSource(wrapper,expectedBase,size,output))return false;
  // Reject a raced slot selection. This check detects a changed observation;
  // the caller's native scope, not this comparison, excludes the actual race.
  if(*reinterpret_cast<const UINT64*>(timeline)!=serial){(*output)->Release();*output=nullptr;return false;}
  *capacity=size;return true;
 }__except(EXCEPTION_EXECUTE_HANDLER){
  // AcquireNativeGISbtSource has its own fault boundary; no native memory is
  // touched after successful acquisition except the counter reread above.
  if(*output){(*output)->Release();*output=nullptr;}return false;
 }
}
}
