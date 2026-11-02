#pragma once
#include <windows.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <array>
#include <unordered_map>
#include "buffer_range_index.h"
#include "buffer_observation_history.h"
namespace gi_reuse {
inline const GUID asStorageKey={0x289a1134,0x12ab,0x41b7,{0x9a,0x11,0x43,0x12,0x28,0x91,0x00,0x01}};
// Development inventory only. Strong references prevent destruction of observed
// resource objects; they do not freeze contents, own placed heaps, or prove states.
template<std::size_t Capacity=8192,UINT64 ByteBudget=512ull*1024*1024> class BasicBufferOwners {
 struct Entry {Microsoft::WRL::ComPtr<ID3D12Resource> owner;UINT64 address=0,bytes=0,touched=0;bool geometry=false,scene=false,candidate=false,index=false;};
 static unsigned Rank(const Entry& e) noexcept {return e.index?3u:(e.candidate?2u:(e.scene?1u:0u));}
 std::array<Entry,Capacity> entries_{};
 std::unordered_map<ID3D12Resource*,std::size_t> slots_;
 BufferRangeIndex<Capacity> ranges_;
 BufferObservationHistory<(Capacity<4096?Capacity*4:16384)> history_;
 SRWLOCK lock_=SRWLOCK_INIT;
 UINT64 bytes_=0,clock_=0,evictions_=0,rejected_=0,excluded_=0;unsigned count_=0;bool limited_=false;
public:
 static constexpr UINT64 Budget=ByteBudget;
 static constexpr std::size_t MaxOwners=Capacity;
 void Observe(ID3D12Resource* resource,D3D12_RESOURCE_STATES state=D3D12_RESOURCE_STATE_COMMON) noexcept {
  if(!resource)return;
  // Known objects need neither COM metadata calls nor a linear inventory scan.
  if(!(state&D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE)){
   AcquireSRWLockExclusive(&lock_);const auto it=slots_.find(resource);
   if(it!=slots_.end()){auto& e=entries_[it->second];e.touched=++clock_;e.index=e.index||(e.scene&&(state&D3D12_RESOURCE_STATE_INDEX_BUFFER)!=0);e.candidate=e.candidate||(e.scene&&(state&(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_INDEX_BUFFER))!=0);ReleaseSRWLockExclusive(&lock_);return;}
   ReleaseSRWLockExclusive(&lock_);
  }
  ObserveSlow(resource,state);
 }
private:
 __declspec(noinline) void ObserveSlow(ID3D12Resource* resource,D3D12_RESOURCE_STATES state) noexcept {
  if(!resource)return;const auto desc=resource->GetDesc();
  if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER)return;
  UINT excluded=0,tagBytes=sizeof(excluded);
  if(state&D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE){excluded=1;resource->SetPrivateData(asStorageKey,sizeof(excluded),&excluded);}
  else if(FAILED(resource->GetPrivateData(asStorageKey,&tagBytes,&excluded)))excluded=0;
  const auto address=resource->GetGPUVirtualAddress();
  if(!address||!desc.Width||desc.Width>UINT64_MAX-address)return;
  D3D12_HEAP_PROPERTIES heap{};D3D12_HEAP_FLAGS flags{};
  const bool heapKnown=SUCCEEDED(resource->GetHeapProperties(&heap,&flags));
  const bool scene=heapKnown&&heap.Type==D3D12_HEAP_TYPE_DEFAULT;
  const bool index=scene&&(state&D3D12_RESOURCE_STATE_INDEX_BUFFER)!=0;
  const bool candidate=scene&&(state&(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_INDEX_BUFFER))!=0;
  excluded=excluded||(heapKnown&&heap.Type==D3D12_HEAP_TYPE_READBACK);
  // Release evicted objects after unlocking: COM destruction can reenter hooks.
  std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,Capacity> retired{};std::size_t retireCount=0;
  AcquireSRWLockExclusive(&lock_);
  if(excluded){
   ++excluded_;history_.Record(address,desc.Width,BufferEvent::Excluded,0);for(auto& e:entries_)if(e.owner.Get()==resource){bytes_-=e.bytes;--count_;ranges_.Remove(static_cast<std::size_t>(&e-entries_.data()));slots_.erase(e.owner.Get());retired[retireCount++]=std::move(e.owner);e={};break;}
   ReleaseSRWLockExclusive(&lock_);return;
  }
  bool known=false;for(auto& e:entries_)if(e.owner.Get()==resource){known=true;e.touched=++clock_;e.index=e.index||(e.scene&&(state&D3D12_RESOURCE_STATE_INDEX_BUFFER)!=0);e.candidate=e.candidate||candidate;break;}
  const unsigned rank=index?3u:(candidate?2u:(scene?1u:0u));
  auto eligible=[rank](const Entry& e){return e.owner&&!e.geometry&&Rank(e)<=rank;};
  if(!known&&desc.Width<=Budget){
   // Decide admission before evicting anything. A refused large buffer must not
   // empty the usable inventory and then fail against protected owners.
   UINT64 available=Budget-bytes_;std::size_t slots=Capacity-count_;
   for(const auto& e:entries_)if(eligible(e)){available+=e.bytes;++slots;}
   if(!slots||desc.Width>available){limited_=true;++rejected_;history_.Record(address,desc.Width,BufferEvent::Rejected,rank);ReleaseSRWLockExclusive(&lock_);return;}
   // Reserve the pointer-index node before mutation. Allocation failure cannot
   // leave the owner and address indexes out of sync.
   try{slots_.emplace(resource,Capacity);}catch(...){limited_=true;++rejected_;history_.Record(address,desc.Width,BufferEvent::Rejected,rank);ReleaseSRWLockExclusive(&lock_);return;}
   while(count_==Capacity||desc.Width>Budget-bytes_){
    Entry* victim=nullptr;for(auto& e:entries_)if(eligible(e)){
     const unsigned er=Rank(e);const unsigned vr=victim?Rank(*victim):4u;
     if(!victim||er<vr||(er==vr&&e.touched<victim->touched))victim=&e;
    }
    if(!victim){limited_=true;++rejected_;history_.Record(address,desc.Width,BufferEvent::Rejected,rank);slots_.erase(resource);ReleaseSRWLockExclusive(&lock_);return;}
    history_.Record(victim->address,victim->bytes,BufferEvent::Evicted,Rank(*victim));bytes_-=victim->bytes;--count_;++evictions_;ranges_.Remove(static_cast<std::size_t>(victim-entries_.data()));slots_.erase(victim->owner.Get());retired[retireCount++]=std::move(victim->owner);*victim={};
   }
   if(count_<Capacity&&desc.Width<=Budget-bytes_){for(auto& e:entries_)if(!e.owner){e={resource,address,desc.Width,++clock_,false,scene,candidate,index};history_.Record(address,desc.Width,BufferEvent::Admitted,rank);++count_;bytes_+=desc.Width;const auto slot=static_cast<std::size_t>(&e-entries_.data());slots_.find(resource)->second=slot;ranges_.Insert(address,desc.Width,slot);break;}}
  }else if(!known){limited_=true;++rejected_;history_.Record(address,desc.Width,BufferEvent::Rejected,rank);}
  ReleaseSRWLockExclusive(&lock_);
 }
public:
 bool Resolve(UINT64 address,UINT64 bytes,Microsoft::WRL::ComPtr<ID3D12Resource>& owner,UINT64& offset,RangeMatch* result=nullptr,bool protect=true) noexcept {
  owner.Reset();offset=0;AcquireSRWLockExclusive(&lock_);std::size_t slot=0;
  const auto match=ranges_.Find(address,bytes,slot);if(result)*result=match;
  if(match==RangeMatch::Unique){auto& e=entries_[slot];if(protect){e.geometry=true;e.touched=++clock_;}owner=e.owner;offset=address-e.address;}
  ReleaseSRWLockExclusive(&lock_);return match==RangeMatch::Unique;
 }
 BufferEvidence HistoricalEvidence(UINT64 address,UINT64 bytes) noexcept {
  AcquireSRWLockShared(&lock_);const auto result=history_.Find(address,bytes);ReleaseSRWLockShared(&lock_);return result;
 }
 void HistoryStats(UINT64& overwritten,UINT64& dropped) noexcept {
  AcquireSRWLockShared(&lock_);history_.Stats(overwritten,dropped);ReleaseSRWLockShared(&lock_);
 }
 void Stats(unsigned& count,UINT64& bytes,bool& limited) noexcept {
  AcquireSRWLockShared(&lock_);count=count_;bytes=bytes_;limited=limited_;ReleaseSRWLockShared(&lock_);
 }
 void DetailStats(UINT64& geometryBytes,UINT64& candidateBytes,UINT64& rejected,UINT64& excluded) noexcept {
  AcquireSRWLockShared(&lock_);geometryBytes=0;candidateBytes=0;
  for(const auto& e:entries_)if(e.owner){if(e.geometry)geometryBytes+=e.bytes;else if(e.candidate)candidateBytes+=e.bytes;}
  rejected=rejected_;excluded=excluded_;ReleaseSRWLockShared(&lock_);
 }
 void RetentionStats(unsigned& geometry,UINT64& evictions) noexcept {
  AcquireSRWLockShared(&lock_);geometry=0;for(const auto& e:entries_)if(e.owner&&e.geometry)++geometry;evictions=evictions_;ReleaseSRWLockShared(&lock_);
 }
};
// Development observer budget; no allocation or retention when observer is off.
using BufferOwners=BasicBufferOwners<8192,1024ull*1024*1024>;
} // namespace gi_reuse
