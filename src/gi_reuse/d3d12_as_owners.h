#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <array>
#include <unordered_map>
#include <vector>
#include "buffer_range_index.h"
namespace gi_reuse {
// Diagnostic AS resource identities, deliberately separate from geometry inputs.
// Retains both placed resources and their explicit heaps. Reserved/tiled storage
// is unsupported. Identity does not prove active alias, residency or AS contents.
template<std::size_t Capacity=8192,UINT64 Budget=512ull*1024*1024> class ASResourceOwners {
 using Resource=Microsoft::WRL::ComPtr<ID3D12Resource>;
 struct Entry {Resource resource;Microsoft::WRL::ComPtr<ID3D12Heap> heap;UINT64 address=0,bytes=0,token=0;};
 std::array<Entry,Capacity> entries_{};
 std::unordered_map<ID3D12Resource*,std::size_t> pointers_;
 BufferRangeIndex<Capacity> ranges_;
 SRWLOCK lock_=SRWLOCK_INIT;
 UINT64 bytes_=0,next_=0,rejected_=0;std::size_t count_=0;
public:
 struct Lease {Resource resource;Microsoft::WRL::ComPtr<ID3D12Heap> heap;UINT64 token=0,offset=0;};
 // Call only at successful committed/placed resource creation with initial AS
 // state, while the native owner is live. Null heap denotes committed storage.
 bool Created(ID3D12Resource* resource,ID3D12Heap* heap=nullptr) noexcept {
  if(!resource)return false;
  const auto d=resource->GetDesc();const auto address=resource->GetGPUVirtualAddress();
  if(d.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER||!address||!d.Width||d.Width>UINT64_MAX-address)return false;
  Resource held=resource;Microsoft::WRL::ComPtr<ID3D12Heap> heldHeap=heap;
  AcquireSRWLockExclusive(&lock_);
  const auto found=pointers_.find(resource);
  if(found!=pointers_.end()){const auto& old=entries_[found->second];const bool same=old.address==address&&old.bytes==d.Width&&old.heap.Get()==heap;ReleaseSRWLockExclusive(&lock_);return same;}
  if(count_==Capacity||d.Width>Budget-bytes_||next_==UINT64_MAX){++rejected_;ReleaseSRWLockExclusive(&lock_);return false;}
  std::size_t slot=0;while(slot<Capacity&&entries_[slot].resource)++slot;
  try{pointers_.emplace(resource,slot);}catch(...){++rejected_;ReleaseSRWLockExclusive(&lock_);return false;}
  if(!ranges_.Insert(address,d.Width,slot)){pointers_.erase(resource);++rejected_;ReleaseSRWLockExclusive(&lock_);return false;}
  entries_[slot]={std::move(held),std::move(heldHeap),address,d.Width,++next_};++count_;bytes_+=d.Width;
  ReleaseSRWLockExclusive(&lock_);return true;
 }
 bool Resolve(UINT64 address,Lease& out,RangeMatch* reason=nullptr) noexcept {
  out={};std::size_t slot=0;AcquireSRWLockShared(&lock_);
  const auto match=ranges_.Find(address,1,slot);if(reason)*reason=match;
  if(match==RangeMatch::Unique){const auto& e=entries_[slot];out={e.resource,e.heap,e.token,address-e.address};}
  ReleaseSRWLockShared(&lock_);return match==RangeMatch::Unique;
 }
 struct Reference {UINT64 address=0,token=0;};
 // Validate all identities and AddRef under one owner transaction, following
 // the recovered PT PrepareLease contract. No partial output on failure.
 // This proves retained resource identity, not alias state or GPU contents.
 bool ResolveBatch(const std::vector<Reference>& references,std::vector<Lease>& out) noexcept {
  out.clear();if(references.empty()||references.size()>8193)return false;
  std::vector<Lease> candidate;
  try{candidate.resize(references.size());}catch(...){return false;}
  bool valid=true;AcquireSRWLockShared(&lock_);
  for(std::size_t i=0;i<references.size();++i){const auto& r=references[i];std::size_t slot=0;
   if(!r.token||ranges_.Find(r.address,1,slot)!=RangeMatch::Unique||entries_[slot].token!=r.token){valid=false;break;}
   const auto& e=entries_[slot];candidate[i]={e.resource,e.heap,e.token,r.address-e.address};
  }
  ReleaseSRWLockShared(&lock_);
  if(valid)out=std::move(candidate);
  return valid; // Failed candidates release only after leaving the lock.
 }
 // Remove only the exact observed generation. Caller must hold a valid native
 // resource reference and obtain the token before forwarding native release.
 // Existing leases remain valid; this is NOT GPU completion or alias proof.
 // Budget/Stats cover indexed owners only, not independent outstanding leases.
 // No game hook calls this until lifecycle coverage and submission leases exist.
 bool Retire(ID3D12Resource* resource,UINT64 token,Lease& retired) noexcept {
  retired={};if(!resource||!token)return false;
  Entry held;AcquireSRWLockExclusive(&lock_);
  const auto found=pointers_.find(resource);
  if(found==pointers_.end()||entries_[found->second].token!=token){ReleaseSRWLockExclusive(&lock_);return false;}
  const auto slot=found->second;
  if(!ranges_.Remove(slot)){ReleaseSRWLockExclusive(&lock_);return false;}
  held=std::move(entries_[slot]);entries_[slot]={};
  pointers_.erase(found);--count_;bytes_-=held.bytes;
  ReleaseSRWLockExclusive(&lock_);
  // Move COM references outside the lock; destruction may enter native code.
  retired={std::move(held.resource),std::move(held.heap),held.token,0};return true;
 }
 void Stats(UINT64& count,UINT64& bytes,UINT64& rejected) noexcept {
  AcquireSRWLockShared(&lock_);count=static_cast<UINT64>(count_);bytes=bytes_;rejected=rejected_;ReleaseSRWLockShared(&lock_);
 }
};
}
