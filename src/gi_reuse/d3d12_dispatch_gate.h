#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstring>
namespace gi_reuse {
// One recording owns one flag. Arm is recorded in a SEPARATE prefix list only
// on first submission. Close belongs to the game list and runs on every replay.
// Both boundaries leave the flag in COMMON; shader reads implicitly promote it. Native
// command-list ordering must still serialize writes to the list's own resources.
class D3D12DispatchGate {
 Microsoft::WRL::ComPtr<ID3D12Resource> flag_,zero_;
 static void Transition(ID3D12GraphicsCommandList* list,ID3D12Resource* resource,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after) noexcept {
  D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  b.Transition={resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};list->ResourceBarrier(1,&b);
 }
public:
 HRESULT Initialize(ID3D12Device* device) noexcept {
  if(!device||flag_)return E_INVALIDARG;
  D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=4;d.Height=1;
  d.DepthOrArraySize=d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  D3D12_HEAP_PROPERTIES h{};h.Type=D3D12_HEAP_TYPE_DEFAULT;h.CreationNodeMask=h.VisibleNodeMask=1;
  auto hr=device->CreateCommittedResource(&h,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&flag_));if(FAILED(hr))return hr;
  h.Type=D3D12_HEAP_TYPE_UPLOAD;hr=device->CreateCommittedResource(&h,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&zero_));if(FAILED(hr))return hr;
  void* mapped=nullptr;const D3D12_RANGE empty{};hr=zero_->Map(0,&empty,&mapped);if(FAILED(hr))return hr;
  if(!mapped){zero_->Unmap(0,&empty);return E_UNEXPECTED;}
  std::memset(mapped,0,4);const D3D12_RANGE written{0,4};zero_->Unmap(0,&written);return S_OK;
 }
 ID3D12Resource* Resource()const noexcept{return flag_.Get();}
 bool Ready()const noexcept{return flag_&&zero_;}
 void Arm(ID3D12GraphicsCommandList* prefix,ID3D12Resource* controls,UINT64 offset) const noexcept {
  Transition(prefix,flag_.Get(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_DEST);
  prefix->CopyBufferRegion(flag_.Get(),0,controls,offset,4);
  Transition(prefix,flag_.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_COMMON);
 }
 void Close(ID3D12GraphicsCommandList* list) const noexcept {Arm(list,zero_.Get(),0);}
};
}
