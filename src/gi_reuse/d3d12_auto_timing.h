#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <array>
#include <cstring>
#include <utility>
namespace gi_reuse {
// Per sampled invocation, retained through both submissions and recording seals.
// No waits: ReadRetired is called only after the existing retirement proof.
class D3D12AutoTiming {
 Microsoft::WRL::ComPtr<ID3D12QueryHeap> heap_;
 Microsoft::WRL::ComPtr<ID3D12Resource> read_;
public:
 bool Initialize(ID3D12Device* device)noexcept {
  if(!device||heap_)return false;
  D3D12_QUERY_HEAP_DESC q{};q.Type=D3D12_QUERY_HEAP_TYPE_TIMESTAMP;q.Count=6;
  Microsoft::WRL::ComPtr<ID3D12QueryHeap> heap;
  if(FAILED(device->CreateQueryHeap(&q,IID_PPV_ARGS(&heap))))return false;
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_READBACK;hp.CreationNodeMask=hp.VisibleNodeMask=1;
  D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=48;rd.Height=1;
  rd.DepthOrArraySize=rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&read_))))return false;
  heap_=std::move(heap);return true;
 }
 bool Ready()const noexcept{return heap_&&read_;}
 void Mark(ID3D12GraphicsCommandList* list,UINT index)const noexcept {if(heap_)list->EndQuery(heap_.Get(),D3D12_QUERY_TYPE_TIMESTAMP,index);}
 void Resolve(ID3D12GraphicsCommandList* list,UINT first,UINT count)const noexcept {if(heap_)list->ResolveQueryData(heap_.Get(),D3D12_QUERY_TYPE_TIMESTAMP,first,count,read_.Get(),UINT64(first)*8);}
 bool ReadRetired(UINT64 frequency,std::array<double,3>& ms)const noexcept {
  if(!Ready()||!frequency)return false;
  void* mapped=nullptr;D3D12_RANGE range{0,48};
  if(FAILED(read_->Map(0,&range,&mapped)))return false;
  if(!mapped){D3D12_RANGE empty{};read_->Unmap(0,&empty);return false;}
  std::array<UINT64,6> ticks{};std::memcpy(ticks.data(),mapped,48);D3D12_RANGE empty{};read_->Unmap(0,&empty);
  for(unsigned i=0;i<3;++i){if(ticks[2*i+1]<ticks[2*i])return false;ms[i]=double(ticks[2*i+1]-ticks[2*i])*1000.0/double(frequency);}
  return true;
 }
};
}
