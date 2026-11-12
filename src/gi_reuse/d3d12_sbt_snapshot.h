#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <utility>
#include "native_gi_table.h"
namespace gi_reuse {
// One immutable GPU copy of the pinned native one-ray GI SBT. Record the copy
// in the original ray recording, before its native dispatch. The engine must
// finish/schedule its SBT writer and upload BEFORE that native use. Native
// Control tables are DEFAULT buffers; the UPLOAD path is a fixture/adapter case.
// Retain this object through the LAST consumer's fence, not merely ray-list Reset.
// This owns table bytes only, not TLAS/BLAS/local-root referenced resources.
class D3D12GISbtSnapshot {
 Microsoft::WRL::ComPtr<ID3D12Resource> source_,copy_,hitUpload_;
 D3D12_DISPATCH_RAYS_DESC dispatch_{};
 UINT64 bytes_=0;bool recorded_=false;
 bool transitionSource_=false;
 bool copyCommon_=false;
 bool overrideHits_=false;UINT64 hitBytes_=0,hitAllocationBytes_=0;
public:
 D3D12GISbtSnapshot()=default;
 D3D12GISbtSnapshot(const D3D12GISbtSnapshot&)=delete;
 D3D12GISbtSnapshot& operator=(const D3D12GISbtSnapshot&)=delete;
 HRESULT Initialize(ID3D12Device* device,ID3D12Resource* source,const D3D12_DISPATCH_RAYS_DESC& native,
                    D3D12_RESOURCE_STATES sourceState=D3D12_RESOURCE_STATE_GENERIC_READ) noexcept {
  if(source_||recorded_||!device||!source||!native.Width||!native.Height||native.Depth!=1||
     native.RayGenerationShaderRecord.SizeInBytes!=32||native.MissShaderTable.SizeInBytes!=32||
     native.MissShaderTable.StrideInBytes!=32||native.HitGroupTable.StrideInBytes!=96||
     !native.HitGroupTable.SizeInBytes||native.HitGroupTable.SizeInBytes%96||native.CallableShaderTable.SizeInBytes)return E_INVALIDARG;
  const auto d=source->GetDesc();const auto base=source->GetGPUVirtualAddress();
  if(d.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER||!base||native.RayGenerationShaderRecord.StartAddress!=base)return E_INVALIDARG;
  Microsoft::WRL::ComPtr<ID3D12Device> owner;auto hr=source->GetDevice(IID_PPV_ARGS(&owner));if(FAILED(hr)||owner.Get()!=device)return E_INVALIDARG;
  D3D12_HEAP_PROPERTIES properties{};D3D12_HEAP_FLAGS flags{};hr=source->GetHeapProperties(&properties,&flags);
  if(FAILED(hr))return hr;
  const bool upload=properties.Type==D3D12_HEAP_TYPE_UPLOAD&&sourceState==D3D12_RESOURCE_STATE_GENERIC_READ;
  const bool local=properties.Type==D3D12_HEAP_TYPE_DEFAULT&&sourceState==D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
  if(!upload&&!local)return E_INVALIDARG;
  UINT64 end=32;
  for(const auto table:{native.MissShaderTable,native.HitGroupTable}){
   if(table.StartAddress<base||table.StartAddress%64)return E_INVALIDARG;
   const auto offset=table.StartAddress-base;
   if(offset>d.Width||table.SizeInBytes>d.Width-offset)return E_INVALIDARG;
   if(offset+table.SizeInBytes>end)end=offset+table.SizeInBytes;
  }
  if(end>16ull*1024*1024||end>UINT64_MAX-63)return E_INVALIDARG;
  D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;heap.CreationNodeMask=heap.VisibleNodeMask=1;
  D3D12_RESOURCE_DESC target{};target.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;target.Width=(end+63)&~UINT64(63);
  target.Height=1;target.DepthOrArraySize=target.MipLevels=1;target.SampleDesc.Count=1;target.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  if(copy_){
   Microsoft::WRL::ComPtr<ID3D12Device> copyDevice;hr=copy_->GetDevice(IID_PPV_ARGS(&copyDevice));
   if(FAILED(hr)||copyDevice.Get()!=device)return E_INVALIDARG;
  }
  if(!copy_||copy_->GetDesc().Width<target.Width){
   Microsoft::WRL::ComPtr<ID3D12Resource> next;
   hr=device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&target,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&next));if(FAILED(hr))return hr;
   copy_=std::move(next);copyCommon_=false;
  }
  source_=source;bytes_=end;dispatch_=native;transitionSource_=local;const auto address=copy_->GetGPUVirtualAddress();
  dispatch_.RayGenerationShaderRecord.StartAddress=address;
  dispatch_.MissShaderTable.StartAddress=address+(native.MissShaderTable.StartAddress-base);
  dispatch_.HitGroupTable.StartAddress=address+(native.HitGroupTable.StartAddress-base);
  return S_OK;
 }
 // A matched original writer snapshot supplies these IDs/constants; only its
 // source addresses have been rebased into privately copied hit-source bytes.
 // Native TraceRay retains two rows per geometry. Reconstruct active even rows
 // using the already validated native ABI; reserved odd rows remain zero.
 // This operation never writes the native table and records no GPU commands.
 HRESULT ReplaceHitTable(ID3D12Device* device,const NativeGITable& table,
                         UINT64 expectedPipeline,UINT64 expectedVersion,UINT64 budget)noexcept {
  if(!device||!source_||recorded_||overrideHits_||!expectedPipeline||!expectedVersion||
     table.pipeline!=expectedPipeline||table.version!=expectedVersion||
     table.bytes.size()!=dispatch_.HitGroupTable.SizeInBytes||table.bytes.size()%192||
     table.active.size()!=table.bytes.size()/96)return E_INVALIDARG;
  Microsoft::WRL::ComPtr<ID3D12Device> owner;
  if(FAILED(copy_->GetDevice(IID_PPV_ARGS(&owner)))||owner.Get()!=device)return E_INVALIDARG;
  const std::array<unsigned char,96> zero{};
  for(std::size_t row=0;row<table.active.size();row+=2){const auto* bytes=table.bytes.data()+row*96;
   if(table.active[row]!=1||table.active[row+1]!=0||!std::memcmp(bytes,zero.data(),32)||
      std::memcmp(bytes+88,zero.data(),8)||std::memcmp(bytes+96,zero.data(),96))return E_INVALIDARG;
  }
  D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;
  desc.Width=table.bytes.size();desc.Height=1;desc.DepthOrArraySize=desc.MipLevels=1;
  desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  const bool reuse=hitUpload_&&hitUpload_->GetDesc().Width>=desc.Width;
  if(reuse)desc=hitUpload_->GetDesc();
  const auto allocation=device->GetResourceAllocationInfo(0,1,&desc).SizeInBytes;
  if(!allocation||allocation==UINT64_MAX||allocation>budget||
     (!reuse&&hitUpload_&&hitAllocationBytes_>budget-allocation))return E_OUTOFMEMORY;
  auto upload=hitUpload_;
  if(!reuse){D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_UPLOAD;heap.CreationNodeMask=heap.VisibleNodeMask=1;
   auto hr=device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&upload));
   if(FAILED(hr))return hr;
  }
  void* mapped=nullptr;const D3D12_RANGE empty{0,0};auto hr=upload->Map(0,&empty,&mapped);if(FAILED(hr))return hr;
  std::memcpy(mapped,table.bytes.data(),table.bytes.size());
  const D3D12_RANGE written{0,table.bytes.size()};upload->Unmap(0,&written);
  hitUpload_=std::move(upload);hitBytes_=table.bytes.size();hitAllocationBytes_=allocation;overrideHits_=true;return S_OK;
 }
 ID3D12Resource* HitUploadStorage()const noexcept{return hitUpload_.Get();}
 UINT64 HitUploadAllocationBytes()const noexcept{return hitAllocationBytes_;}
 bool CanRecord(ID3D12GraphicsCommandList* list)const noexcept {
  return list&&copy_&&source_&&!recorded_&&
   (list->GetType()==D3D12_COMMAND_LIST_TYPE_DIRECT||list->GetType()==D3D12_COMMAND_LIST_TYPE_COMPUTE);
 }
 bool Record(ID3D12GraphicsCommandList* list) noexcept {
  if(!CanRecord(list))return false;
  if(copyCommon_){D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
   b.Transition={copy_.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_DEST};
   list->ResourceBarrier(1,&b);copyCommon_=false;}
  D3D12_RESOURCE_BARRIER sourceBarrier{};sourceBarrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  sourceBarrier.Transition={source_.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
   D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE};
  if(transitionSource_)list->ResourceBarrier(1,&sourceBarrier);
  list->CopyBufferRegion(copy_.Get(),0,source_.Get(),0,bytes_);
  if(overrideHits_)list->CopyBufferRegion(copy_.Get(),
    dispatch_.HitGroupTable.StartAddress-copy_->GetGPUVirtualAddress(),hitUpload_.Get(),0,hitBytes_);
  if(transitionSource_){sourceBarrier.Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_SOURCE;
   sourceBarrier.Transition.StateAfter=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;list->ResourceBarrier(1,&sourceBarrier);}
  D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  b.Transition={copy_.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE};
  list->ResourceBarrier(1,&b);recorded_=true;return true;
 }
 // Allocation accounting only; this accessor does not imply recorded data.
 ID3D12Resource* Storage()const noexcept{return copy_.Get();}
 ID3D12Resource* Resource()const noexcept{return recorded_?copy_.Get():nullptr;}
 bool MatchesExtent(UINT width,UINT height)const noexcept {
  return copy_&&dispatch_.Width==width&&dispatch_.Height==height&&dispatch_.Depth==1;
 }
 const D3D12_DISPATCH_RAYS_DESC* Dispatch()const noexcept{return recorded_?&dispatch_:nullptr;}
 UINT64 Bytes()const noexcept{return bytes_;}
 // Only on a whole invocation returned by the retirement pool's Take: BOTH
 // recording resets and final consumer fence must have completed. Not a fence
 // check itself. Buffers decay to COMMON at the completed Execute boundary.
 bool RearmAfterRetirement()noexcept {
  if(!recorded_)return false;
  source_.Reset();dispatch_={};bytes_=hitBytes_=0;overrideHits_=recorded_=false;copyCommon_=true;return true;
 }
};
}
