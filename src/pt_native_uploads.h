#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <map>
#include <mutex>
#include <cstring>
// The native 64-KiB constant upload pool bypasses NativeBuffer creation.
// Retain actual committed UPLOAD resources at creation; never dereference a VA.
// Session ownership prevents address reuse. A bounded pool rejects new entries
// rather than evicting storage that an already-recorded list may still reference.
namespace PTNativeView {
class Uploads {
    using Resource=Microsoft::WRL::ComPtr<ID3D12Resource>;
    std::mutex mutex;
    std::map<UINT64,Resource> buffers;
public:
    bool Created(ID3D12Resource* resource){
        if(!resource)return false;
        const auto desc=resource->GetDesc();D3D12_HEAP_PROPERTIES heap{};D3D12_HEAP_FLAGS flags{};
        if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER||desc.Width!=65536||
           FAILED(resource->GetHeapProperties(&heap,&flags))||heap.Type!=D3D12_HEAP_TYPE_UPLOAD)return false;
        const auto address=resource->GetGPUVirtualAddress();if(!address||address>UINT64_MAX-65536)return false;
        std::lock_guard<std::mutex> guard(mutex);
        const auto old=buffers.find(address);if(old!=buffers.end())return old->second.Get()==resource;
        if(buffers.size()>=2048)return false; // 128 MiB maximum retained pool
        buffers.emplace(address,resource);return true;
    }
    bool Read(UINT64 address,void* destination,SIZE_T bytes){
        if(!destination||!bytes||bytes>4096)return false;
        std::lock_guard<std::mutex> guard(mutex);
        auto found=buffers.upper_bound(address);if(found==buffers.begin())return false;--found;
        const auto offset=address-found->first;if(offset>=65536||bytes>65536-offset)return false;
        void* data=nullptr;const D3D12_RANGE range{static_cast<SIZE_T>(offset),static_cast<SIZE_T>(offset)+bytes};
        if(FAILED(found->second->Map(0,&range,&data))||!data)return false;
        std::memcpy(destination,static_cast<const unsigned char*>(data)+offset,bytes);
        const D3D12_RANGE noWrite{};found->second->Unmap(0,&noWrite);return true;
    }
};
inline Uploads uploads;
}
