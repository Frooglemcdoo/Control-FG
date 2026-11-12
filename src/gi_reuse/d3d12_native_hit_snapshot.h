#pragma once
#include "native_hit_snapshot_plan.h"
#include "d3d12_storage_lease.h"
namespace gi_reuse {
// Preparation allocates but records no commands. Recording must occur at the
// ORIGINAL ray use, under its native ordering/state contract. This preserves
// hit-shader index/attribute bytes only; TLAS, materials, textures and descriptor
// contents still need their existing separate protection.
class D3D12NativeHitSnapshot {
 using Ptr=Microsoft::WRL::ComPtr<ID3D12Resource>;
public:
 struct Source {std::uint64_t token=0;BufferStorageLease storage;D3D12_RESOURCE_STATES state{};};
private:
 NativeHitSnapshotPlan plan_;
 std::vector<Source> sources_; // Ordered exactly like plan.Copies().
 std::vector<D3D12_RESOURCE_BARRIER> beforeCopies_,afterCopies_;
 std::vector<PTNativeBinding::Binding> bindings_;
 Microsoft::WRL::ComPtr<ID3D12Device> device_;
 Ptr packed_;
 UINT64 allocationBytes_=0;
 bool prepared_=false,recorded_=false,common_=false;
public:
 HRESULT Prepare(ID3D12Device* device,const std::vector<PTNativeBinding::Binding>& bindings,
                 const std::vector<Source>& sources,UINT64 budget)noexcept {
  if(!device||prepared_||recorded_||!budget||sources.empty()||sources.size()>8192||
     (device_&&device_.Get()!=device))return E_INVALIDARG;
  try {
   std::vector<NativeHitSnapshotPlan::Allocation> allocations;allocations.reserve(sources.size());
   for(const auto& s:sources){const auto& lease=s.storage;
    if(!s.token||!lease.resource||(lease.kind!=BufferStorageKind::Committed&&lease.kind!=BufferStorageKind::Placed)||
       (lease.kind==BufferStorageKind::Placed&&!lease.heap)||
       (s.state!=D3D12_RESOURCE_STATE_COMMON&&!(s.state&D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE))||
       (unsigned(s.state)&~unsigned(D3D12_RESOURCE_STATE_GENERIC_READ)))return E_INVALIDARG;
    Microsoft::WRL::ComPtr<ID3D12Device> owner;
    if(FAILED(lease.resource->GetDevice(IID_PPV_ARGS(&owner)))||owner.Get()!=device)return E_INVALIDARG;
    const auto desc=lease.resource->GetDesc();D3D12_HEAP_PROPERTIES heap{};D3D12_HEAP_FLAGS flags{};
    if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER||desc.Width!=lease.bytes||
       lease.resource->GetGPUVirtualAddress()!=lease.address||
       FAILED(lease.resource->GetHeapProperties(&heap,&flags))||heap.Type!=D3D12_HEAP_TYPE_DEFAULT)return E_INVALIDARG;
    allocations.push_back({s.token,lease.address,lease.bytes});
   }
   NativeHitSnapshotPlan plan;if(!plan.Build(bindings,std::move(allocations),budget))return E_INVALIDARG;
   std::vector<Source> retained;retained.reserve(plan.Copies().size());
   std::vector<const Source*> byToken;byToken.reserve(sources.size());
   for(const auto& source:sources)byToken.push_back(&source);
   std::sort(byToken.begin(),byToken.end(),[](const Source* a,const Source* b){return a->token<b->token;});
   for(const auto& copy:plan.Copies()){
    const auto found=std::lower_bound(byToken.begin(),byToken.end(),copy.source.token,
      [](const Source* source,std::uint64_t token){return source->token<token;});
    if(found==byToken.end()||(*found)->token!=copy.source.token)return E_INVALIDARG;
    retained.push_back(**found);
   }
   Ptr packed=packed_;bool common=common_;
   D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=plan.Bytes();desc.Height=1;
   desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
   if(packed&&packed->GetDesc().Width>=desc.Width)desc=packed->GetDesc();
   const auto size=device->GetResourceAllocationInfo(0,1,&desc).SizeInBytes;
   if(!size||size==UINT64_MAX||size>budget)return E_OUTOFMEMORY;
   if(!packed||packed->GetDesc().Width<desc.Width){
    // A larger transactional replacement temporarily retains the retired old
    // allocation too. Include both in the supplied owner budget.
    if(packed_&&allocationBytes_>budget-size)return E_OUTOFMEMORY;
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;heap.CreationNodeMask=heap.VisibleNodeMask=1;
    auto hr=device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&packed));
    if(FAILED(hr))return hr;common=false;
   }
   std::vector<PTNativeBinding::Binding> rebased;
   if(!plan.Rebase(packed->GetGPUVirtualAddress(),packed->GetDesc().Width,rebased))return E_INVALIDARG;
   // Build the barriers during CPU preparation. Hundreds of source views must
   // not turn into thousands of separate ResourceBarrier calls at recording.
   std::vector<D3D12_RESOURCE_BARRIER> before,after;before.reserve(retained.size()+1);after.reserve(retained.size()+1);
   D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
   if(common){barrier.Transition={packed.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
      D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_DEST};before.push_back(barrier);}
   for(const auto& source:retained)if(!(source.state&D3D12_RESOURCE_STATE_COPY_SOURCE)){
    barrier.Transition={source.storage.resource.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
     source.state,D3D12_RESOURCE_STATE_COPY_SOURCE};before.push_back(barrier);
    std::swap(barrier.Transition.StateBefore,barrier.Transition.StateAfter);after.push_back(barrier);
   }
   barrier.Transition={packed.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
    D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE};after.push_back(barrier);
   plan_=std::move(plan);sources_=std::move(retained);beforeCopies_=std::move(before);afterCopies_=std::move(after);
   bindings_=std::move(rebased);packed_=std::move(packed);
   device_=device;allocationBytes_=size;common_=common;prepared_=true;return S_OK;
  }catch(...){return E_OUTOFMEMORY;}
 }
 bool CanRecord(ID3D12GraphicsCommandList* list)const noexcept {
  if(!prepared_||recorded_||!list||list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT)return false;
  Microsoft::WRL::ComPtr<ID3D12Device> owner;
  return SUCCEEDED(list->GetDevice(IID_PPV_ARGS(&owner)))&&owner.Get()==device_.Get();
 }
 bool Record(ID3D12GraphicsCommandList* list)noexcept {
  if(!CanRecord(list))return false;
  if(!beforeCopies_.empty())list->ResourceBarrier(static_cast<UINT>(beforeCopies_.size()),beforeCopies_.data());
  for(std::size_t i=0;i<sources_.size();++i){const auto& source=sources_[i];const auto& copy=plan_.Copies()[i];
   list->CopyBufferRegion(packed_.Get(),copy.destinationOffset,source.storage.resource.Get(),copy.offset,copy.bytes);
  }
  list->ResourceBarrier(static_cast<UINT>(afterCopies_.size()),afterCopies_.data());
  recorded_=true;common_=false;return true;
 }
 const std::vector<PTNativeBinding::Binding>& Bindings()const noexcept{return bindings_;}
 ID3D12Resource* Storage()const noexcept{return packed_.Get();}
 ID3D12Resource* Resource()const noexcept{return recorded_?packed_.Get():nullptr;}
 UINT64 AllocationBytes()const noexcept{return allocationBytes_;}
 // Host may call ONLY after whole-owner retirement: final fence and BOTH seals.
 // Retained allocation bytes remain visible for active+idle budget accounting.
 bool RearmAfterRetirement()noexcept {
  if(!recorded_)return false;
  plan_={};sources_.clear();beforeCopies_.clear();afterCopies_.clear();bindings_.clear();prepared_=recorded_=false;common_=true;return true;
 }
};
}
