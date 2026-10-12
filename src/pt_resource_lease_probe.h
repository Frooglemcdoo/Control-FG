#pragma once
#include <d3d12.h>
#include <atomic>
#include <cstdint>
// CPU ownership probe only. No GPU submissions, fences or resource-state changes.
namespace PTResourceLeaseProbe {
struct Lease {ID3D12Resource* resource=nullptr;UINT64 address=0,bytes=0;unsigned status=0;};
struct Result {unsigned status=0;UINT64 address=0,bytes=0;};
class Probe {
    std::atomic<unsigned> attempts{0};
    std::atomic<UINT64> heldBytes{0};
public:
    static constexpr UINT64 MaximumResource=8ull*1024*1024,MaximumHeld=64ull*1024*1024;
    static constexpr unsigned MaximumAttempts=32;
    // Call while the native owner still holds its reference.
    Lease Before(IUnknown* native,UINT64 expectedAddress,UINT64 expectedBytes){
        Lease l{};if(!native||!expectedAddress||!expectedBytes||expectedBytes>MaximumResource)return l;
        unsigned n=attempts.load();
        do {if(n>=MaximumAttempts)return l;}while(!attempts.compare_exchange_weak(n,n+1));
        l.status=1;
        if(FAILED(native->QueryInterface(__uuidof(ID3D12Resource),reinterpret_cast<void**>(&l.resource)))||!l.resource){l.status=2;return l;}
        const auto d=l.resource->GetDesc();const auto address=l.resource->GetGPUVirtualAddress();
        if(d.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER||d.Width<expectedBytes||d.Width>MaximumResource||address!=expectedAddress){
            l.resource->Release();l.resource=nullptr;l.status=3;return l;
        }
        UINT64 held=heldBytes.load();
        do {if(d.Width>MaximumHeld-held){l.resource->Release();l.resource=nullptr;l.status=4;return l;}}
        while(!heldBytes.compare_exchange_weak(held,held+d.Width));
        l.address=address;l.bytes=d.Width;return l;
    }
    // Call only after the native release helper has returned. The queried
    // interface owns its own reference, even if the native owner dropped its last.
    Result After(Lease& l){
        Result r{l.status,l.address,l.bytes};
        if(l.resource){
            const auto d=l.resource->GetDesc();
            r.status=d.Dimension==D3D12_RESOURCE_DIMENSION_BUFFER&&d.Width==l.bytes&&
                l.resource->GetGPUVirtualAddress()==l.address?5u:6u;
            l.resource->Release();heldBytes.fetch_sub(l.bytes);l.resource=nullptr;
        }
        l={};return r;
    }
    UINT64 HeldBytes()const{return heldBytes.load();}
};
}
