#pragma once
#include "frozen_guide_training.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <array>
namespace gi_reuse {
// Reusable scratch only. A split-recording lease must be retired before reuse.
// No scene data/history is cached. All buffers start COMMON, and decay to COMMON
// at completed Execute boundaries. Callers establish access states per phase.
class D3D12FrozenWorkingSet {
 std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,4> resources_;
 UINT width_=0,height_=0,slots_=0;
 UINT64 allocationBytes_=0;
public:
 enum Buffer : unsigned {Training,Weights,Cells,Guides};
 struct Layout {
  UINT width=0,height=0,slots=0,count=0;
  std::array<UINT64,4> bytes{};
  explicit operator bool()const noexcept{return count!=0;}
 };
 static Layout Describe(UINT width,UINT height,UINT slots)noexcept {
  if(width<2||(width&1)||width>16384||!height||height>16384||!slots||slots>65536)return {};
  const UINT64 count=UINT64(width/2)*height;
  if(count>16*1024*1024)return {};
  return {width,height,slots,static_cast<UINT>(count),
   {count*sizeof(GITrainingRecord),count*sizeof(float),UINT64(slots)*sizeof(GITrainingCell),UINT64(slots)*sizeof(GIKeyedGuide)}};
 }
 // Budget is for real committed allocations, not logical buffer lengths.
 // Failure is transactional; a partially allocated set is never published.
 HRESULT Initialize(ID3D12Device* device,const Layout& layout,UINT64 budget)noexcept {
  if(resources_[0]||!device||!budget)return E_INVALIDARG;
  const auto expected=Describe(layout.width,layout.height,layout.slots);
  if(!expected||layout.count!=expected.count||layout.bytes!=expected.bytes)return E_INVALIDARG;
  std::array<D3D12_RESOURCE_DESC,4> descriptions{};UINT64 bytes=0;
  for(unsigned i=0;i<4;++i){auto& d=descriptions[i];
   d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=layout.bytes[i];d.Height=1;
   d.DepthOrArraySize=d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
   d.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
   const auto allocation=device->GetResourceAllocationInfo(0,1,&d);
   if(!allocation.SizeInBytes||allocation.SizeInBytes==UINT64_MAX||allocation.SizeInBytes>budget-bytes)return E_OUTOFMEMORY;
   bytes+=allocation.SizeInBytes;
  }
  D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;heap.CreationNodeMask=heap.VisibleNodeMask=1;
  std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,4> buffers;
  for(unsigned i=0;i<4;++i){const auto hr=device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,
   &descriptions[i],D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&buffers[i]));if(FAILED(hr))return hr;}
  resources_=std::move(buffers);width_=layout.width;height_=layout.height;slots_=layout.slots;allocationBytes_=bytes;return S_OK;
 }
 bool Matches(UINT width,UINT height,UINT slots)const noexcept {
  return resources_[0]&&width==width_&&height==height_&&slots==slots_;
 }
 ID3D12Resource* Get(Buffer buffer)const noexcept{return static_cast<unsigned>(buffer)<4?resources_[buffer].Get():nullptr;}
 UINT64 AllocationBytes()const noexcept{return allocationBytes_;}
 UINT Count()const noexcept{return width_/2*height_;}
};
}
