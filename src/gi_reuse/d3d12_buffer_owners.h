#pragma once
#include <windows.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <array>
namespace gi_reuse {
inline const GUID asStorageKey={0x289a1134,0x12ab,0x41b7,{0x9a,0x11,0x43,0x12,0x28,0x91,0x00,0x01}};
// Development inventory only. Strong references prevent destruction of observed
// resource objects; they do not freeze contents, own placed heaps, or prove states.
template<std::size_t Capacity=2048,UINT64 ByteBudget=512ull*1024*1024> class BasicBufferOwners {
 struct Entry {Microsoft::WRL::ComPtr<ID3D12Resource> owner;UINT64 address=0,bytes=0,touched=0;bool geometry=false,scene=false,candidate=false;};
 std::array<Entry,Capacity> entries_{};
 SRWLOCK lock_=SRWLOCK_INIT;
 UINT64 bytes_=0,clock_=0,evictions_=0,rejected_=0,excluded_=0;unsigned count_=0;bool limited_=false;
public:
 static constexpr UINT64 Budget=ByteBudget;
 void Observe(ID3D12Resource* resource,D3D12_RESOURCE_STATES state=D3D12_RESOURCE_STATE_COMMON) noexcept {
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
  const bool candidate=scene&&(state&(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_INDEX_BUFFER))!=0;
  excluded=excluded||(heapKnown&&heap.Type==D3D12_HEAP_TYPE_READBACK);
  // Release evicted objects after unlocking: COM destruction can reenter hooks.
  std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,Capacity> retired{};std::size_t retireCount=0;
  AcquireSRWLockExclusive(&lock_);
  if(excluded){
   ++excluded_;for(auto& e:entries_)if(e.owner.Get()==resource){bytes_-=e.bytes;--count_;retired[retireCount++]=std::move(e.owner);e={};break;}
   ReleaseSRWLockExclusive(&lock_);return;
  }
  bool known=false;for(auto& e:entries_)if(e.owner.Get()==resource){known=true;e.touched=++clock_;e.candidate=e.candidate||candidate;break;}
  const unsigned rank=candidate?2u:(scene?1u:0u);
  auto eligible=[rank](const Entry& e){return e.owner&&!e.geometry&&(e.candidate?2u:(e.scene?1u:0u))<=rank;};
  if(!known&&desc.Width<=Budget){
   // Decide admission before evicting anything. A refused large buffer must not
   // empty the usable inventory and then fail against protected owners.
   UINT64 available=Budget-bytes_;std::size_t slots=Capacity-count_;
   for(const auto& e:entries_)if(eligible(e)){available+=e.bytes;++slots;}
   if(!slots||desc.Width>available){limited_=true;++rejected_;ReleaseSRWLockExclusive(&lock_);return;}
   while(count_==Capacity||desc.Width>Budget-bytes_){
    Entry* victim=nullptr;for(auto& e:entries_)if(eligible(e)){
     const unsigned er=e.candidate?2u:(e.scene?1u:0u);const unsigned vr=victim?(victim->candidate?2u:(victim->scene?1u:0u)):3u;
     if(!victim||er<vr||(er==vr&&e.touched<victim->touched))victim=&e;
    }
    if(!victim){limited_=true;break;}
    bytes_-=victim->bytes;--count_;++evictions_;retired[retireCount++]=std::move(victim->owner);*victim={};
   }
   if(count_<Capacity&&desc.Width<=Budget-bytes_){for(auto& e:entries_)if(!e.owner){e={resource,address,desc.Width,++clock_,false,scene,candidate};++count_;bytes_+=desc.Width;break;}}
  }else if(!known){limited_=true;++rejected_;}
  ReleaseSRWLockExclusive(&lock_);
 }
 bool Resolve(UINT64 address,UINT64 bytes,Microsoft::WRL::ComPtr<ID3D12Resource>& owner,UINT64& offset) noexcept {
  owner.Reset();offset=0;if(!address||!bytes||bytes>UINT64_MAX-address)return false;
  AcquireSRWLockExclusive(&lock_);unsigned matches=0;Entry* match=nullptr;
  for(auto& e:entries_){if(!e.owner)continue;
   if(address>=e.address&&address-e.address<=e.bytes&&bytes<=e.bytes-(address-e.address)){
    ++matches;match=&e;
   }}
  if(matches==1){match->geometry=true;match->touched=++clock_;owner=match->owner;offset=address-match->address;}
  ReleaseSRWLockExclusive(&lock_);return matches==1;
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
using BufferOwners=BasicBufferOwners<>;
} // namespace gi_reuse
