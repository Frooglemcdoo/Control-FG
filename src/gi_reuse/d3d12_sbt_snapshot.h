#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
namespace gi_reuse {
// One immutable GPU copy of the pinned native one-ray GI SBT. Record the copy
// in the original ray recording, before its native dispatch. The engine must
// finish its CPU SBT writer BEFORE submitting that recording, as for native use.
// Retain this object through the LAST consumer's fence, not merely ray-list Reset.
// This owns table bytes only, not TLAS/BLAS/local-root referenced resources.
class D3D12GISbtSnapshot {
 Microsoft::WRL::ComPtr<ID3D12Resource> source_,copy_;
 D3D12_DISPATCH_RAYS_DESC dispatch_{};
 UINT64 bytes_=0;bool recorded_=false;
public:
 D3D12GISbtSnapshot()=default;
 D3D12GISbtSnapshot(const D3D12GISbtSnapshot&)=delete;
 D3D12GISbtSnapshot& operator=(const D3D12GISbtSnapshot&)=delete;
 HRESULT Initialize(ID3D12Device* device,ID3D12Resource* source,const D3D12_DISPATCH_RAYS_DESC& native) noexcept {
  if(copy_||!device||!source||!native.Width||!native.Height||native.Depth!=1||
     native.RayGenerationShaderRecord.SizeInBytes!=32||native.MissShaderTable.SizeInBytes!=32||
     native.MissShaderTable.StrideInBytes!=32||native.HitGroupTable.StrideInBytes!=96||
     !native.HitGroupTable.SizeInBytes||native.HitGroupTable.SizeInBytes%96||native.CallableShaderTable.SizeInBytes)return E_INVALIDARG;
  const auto d=source->GetDesc();const auto base=source->GetGPUVirtualAddress();
  if(d.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER||!base||native.RayGenerationShaderRecord.StartAddress!=base)return E_INVALIDARG;
  Microsoft::WRL::ComPtr<ID3D12Device> owner;auto hr=source->GetDevice(IID_PPV_ARGS(&owner));if(FAILED(hr)||owner.Get()!=device)return E_INVALIDARG;
  D3D12_HEAP_PROPERTIES properties{};D3D12_HEAP_FLAGS flags{};hr=source->GetHeapProperties(&properties,&flags);
  if(FAILED(hr)||properties.Type!=D3D12_HEAP_TYPE_UPLOAD)return E_INVALIDARG;
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
  hr=device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&target,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&copy_));if(FAILED(hr))return hr;
  source_=source;bytes_=end;dispatch_=native;const auto address=copy_->GetGPUVirtualAddress();
  dispatch_.RayGenerationShaderRecord.StartAddress=address;
  dispatch_.MissShaderTable.StartAddress=address+(native.MissShaderTable.StartAddress-base);
  dispatch_.HitGroupTable.StartAddress=address+(native.HitGroupTable.StartAddress-base);
  return S_OK;
 }
 bool Record(ID3D12GraphicsCommandList* list) noexcept {
  if(!list||!copy_||!source_||recorded_)return false;
  list->CopyBufferRegion(copy_.Get(),0,source_.Get(),0,bytes_);
  D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  b.Transition={copy_.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE};
  list->ResourceBarrier(1,&b);recorded_=true;return true;
 }
 ID3D12Resource* Resource()const noexcept{return recorded_?copy_.Get():nullptr;}
 const D3D12_DISPATCH_RAYS_DESC* Dispatch()const noexcept{return recorded_?&dispatch_:nullptr;}
 UINT64 Bytes()const noexcept{return bytes_;}
};
}
