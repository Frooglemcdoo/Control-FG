#pragma once
#include <windows.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <array>
namespace gi_reuse {
// Development inventory only. Strong references prevent destruction of observed
// resource objects; they do not freeze contents, own placed heaps, or prove states.
class BufferOwners {
 struct Entry {Microsoft::WRL::ComPtr<ID3D12Resource> owner;UINT64 address=0,bytes=0;};
 std::array<Entry,2048> entries_{};
 SRWLOCK lock_=SRWLOCK_INIT;
 UINT64 bytes_=0;unsigned count_=0;bool limited_=false;
public:
 static constexpr UINT64 Budget=512ull*1024*1024;
 void Observe(ID3D12Resource* resource) noexcept {
  if(!resource)return;const auto desc=resource->GetDesc();
  if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER)return;
  const auto address=resource->GetGPUVirtualAddress();
  if(!address||!desc.Width||desc.Width>UINT64_MAX-address)return;
  AcquireSRWLockExclusive(&lock_);
  bool known=false;for(unsigned i=0;i<count_;++i)if(entries_[i].owner.Get()==resource){known=true;break;}
  if(!known){if(count_==entries_.size()||desc.Width>Budget-bytes_)limited_=true;
   else{entries_[count_++]={resource,address,desc.Width};bytes_+=desc.Width;}}
  ReleaseSRWLockExclusive(&lock_);
 }
 bool Resolve(UINT64 address,UINT64 bytes,Microsoft::WRL::ComPtr<ID3D12Resource>& owner,UINT64& offset) noexcept {
  owner.Reset();offset=0;if(!address||!bytes||bytes>UINT64_MAX-address)return false;
  AcquireSRWLockShared(&lock_);unsigned matches=0;
  for(unsigned i=0;i<count_;++i){const auto& e=entries_[i];
   if(address>=e.address&&address-e.address<=e.bytes&&bytes<=e.bytes-(address-e.address)){
    ++matches;owner=e.owner;offset=address-e.address;
   }}
  ReleaseSRWLockShared(&lock_);if(matches!=1){owner.Reset();offset=0;return false;}return true;
 }
 void Stats(unsigned& count,UINT64& bytes,bool& limited) noexcept {
  AcquireSRWLockShared(&lock_);count=count_;bytes=bytes_;limited=limited_;ReleaseSRWLockShared(&lock_);
 }
};
} // namespace gi_reuse
