#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <mutex>
#include <vector>
namespace gi_reuse {
// Only fully retired owners return buffers. DEFAULT buffers must be COMMON;
// UPLOAD buffers remain GENERIC_READ. No content or scene identity is cached.
class GIScratchPool {
 using Ptr=Microsoft::WRL::ComPtr<ID3D12Resource>;
 struct Idle {Ptr resource;Microsoft::WRL::ComPtr<IUnknown> device;UINT64 bytes;D3D12_HEAP_TYPE type;};
 std::mutex mutex_;std::vector<Idle> idle_;UINT64 bytes_=0,reuses_=0;
public:
 struct Stats {UINT64 bytes,reuses;};
 Stats Statistics(){std::lock_guard<std::mutex> guard(mutex_);return {bytes_,reuses_};}
 static constexpr UINT64 Limit=128ull*1024*1024;
 Ptr Take(ID3D12Device* device,UINT64 bytes,D3D12_HEAP_TYPE type){
  Microsoft::WRL::ComPtr<IUnknown> identity;if(!device||FAILED(device->QueryInterface(IID_PPV_ARGS(&identity))))return {};
  std::lock_guard<std::mutex> guard(mutex_);
  for(auto it=idle_.begin();it!=idle_.end();++it)if(it->device.Get()==identity.Get()&&it->bytes==bytes&&it->type==type){
   auto result=std::move(it->resource);bytes_-=bytes;idle_.erase(it);++reuses_;return result;
  }
  return {};
 }
 void Return(Ptr resource,D3D12_HEAP_TYPE type) noexcept {
  try {
   if(!resource)return;const auto bytes=resource->GetDesc().Width;if(bytes>Limit)return;
   Microsoft::WRL::ComPtr<IUnknown> device;if(FAILED(resource->GetDevice(IID_PPV_ARGS(&device))))return;
   std::lock_guard<std::mutex> guard(mutex_);
   while(!idle_.empty()&&(bytes>Limit-bytes_||idle_.size()>=128)){bytes_-=idle_.front().bytes;idle_.erase(idle_.begin());}
   idle_.push_back({std::move(resource),std::move(device),bytes,type});bytes_+=bytes;
  }catch(...){}
 }
};
inline GIScratchPool& SharedGIScratchPool(){static GIScratchPool pool;return pool;}
}
