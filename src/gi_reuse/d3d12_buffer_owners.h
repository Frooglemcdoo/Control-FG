#pragma once
#include <windows.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <array>
namespace gi_reuse {
// Development inventory only. Strong references prevent destruction of observed
// resource objects; they do not freeze contents, own placed heaps, or prove states.
template<std::size_t Capacity=2048,UINT64 ByteBudget=512ull*1024*1024> class BasicBufferOwners {
 struct Entry {Microsoft::WRL::ComPtr<ID3D12Resource> owner;UINT64 address=0,bytes=0,touched=0;bool geometry=false;};
 std::array<Entry,Capacity> entries_{};
 SRWLOCK lock_=SRWLOCK_INIT;
 UINT64 bytes_=0,clock_=0,evictions_=0;unsigned count_=0;bool limited_=false;
public:
 static constexpr UINT64 Budget=ByteBudget;
 void Observe(ID3D12Resource* resource) noexcept {
  if(!resource)return;const auto desc=resource->GetDesc();
  if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER)return;
  const auto address=resource->GetGPUVirtualAddress();
  if(!address||!desc.Width||desc.Width>UINT64_MAX-address)return;
  // Release evicted objects after unlocking: COM destruction can reenter hooks.
  std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,Capacity> retired{};std::size_t retireCount=0;
  AcquireSRWLockExclusive(&lock_);
  bool known=false;for(auto& e:entries_)if(e.owner.Get()==resource){known=true;e.touched=++clock_;break;}
  if(!known&&desc.Width<=Budget){
   while(count_==Capacity||desc.Width>Budget-bytes_){
    Entry* victim=nullptr;for(auto& e:entries_)if(e.owner&&!e.geometry&&(!victim||e.touched<victim->touched))victim=&e;
    if(!victim){limited_=true;break;}
    bytes_-=victim->bytes;--count_;++evictions_;retired[retireCount++]=std::move(victim->owner);*victim={};
   }
   if(count_<Capacity&&desc.Width<=Budget-bytes_){for(auto& e:entries_)if(!e.owner){e={resource,address,desc.Width,++clock_,false};++count_;bytes_+=desc.Width;break;}}
  }else if(!known)limited_=true;
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
 void RetentionStats(unsigned& geometry,UINT64& evictions) noexcept {
  AcquireSRWLockShared(&lock_);geometry=0;for(const auto& e:entries_)if(e.owner&&e.geometry)++geometry;evictions=evictions_;ReleaseSRWLockShared(&lock_);
 }
};
using BufferOwners=BasicBufferOwners<>;
} // namespace gi_reuse
