#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
#include <cstdint>
#include <array>
#include <algorithm>
// Native descriptors cannot be copied out of a shader-visible heap. Mirror their
// CPU-source writes as they happen; snapshots copy only from this CPU-only heap.
// This preserves descriptor VALUES, not resource ownership or image contents.
namespace PTTableMirror {
using Microsoft::WRL::ComPtr;
inline std::recursive_mutex gate;
inline thread_local unsigned privateWrites=0;
struct Mute {Mute(){++privateWrites;}~Mute(){--privateWrites;}};
struct Scope {
    std::unique_lock<std::recursive_mutex> lock;
    bool capture;
    explicit Scope(bool enabled):lock(gate,std::defer_lock),capture(enabled&&!privateWrites){if(capture)lock.lock();}
};
struct Range {D3D12_CPU_DESCRIPTOR_HANDLE first{};UINT count=0;};
class Bank {
    static constexpr UINT Prefix=244000,MaximumSlots=2097152;
    struct Heap {
        ComPtr<ID3D12DescriptorHeap> native,shadow;
        ComPtr<IUnknown> identity;
        SIZE_T begin=0;
        UINT count=0,step=0;
        std::vector<bool> seen;
        std::vector<std::uint64_t> versions;
    };
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12DescriptorHeap> defaults;
    std::map<SIZE_T,std::shared_ptr<Heap>> heaps;
    UINT slots=0,step=0;
    std::uint64_t version=1;
    std::shared_ptr<Heap> Find(SIZE_T address){
        auto it=heaps.upper_bound(address);if(it==heaps.begin())return {};--it;auto h=it->second;
        if(address<h->begin||(address-h->begin)%h->step||(address-h->begin)/h->step>=h->count)return {};return h;
    }
    static D3D12_CPU_DESCRIPTOR_HANDLE At(const Heap& h,UINT index){
        auto p=h.shadow->GetCPUDescriptorHandleForHeapStart();p.ptr+=SIZE_T(index)*h.step;return p;
    }
    bool Initialize(ID3D12Device* d){
        if(device){ComPtr<IUnknown> a,b;return SUCCEEDED(device.As(&a))&&SUCCEEDED(d->QueryInterface(IID_PPV_ARGS(&b)))&&a.Get()==b.Get();}
        if(!d)return false;Mute mute;ComPtr<ID3D12DescriptorHeap> empty;
        D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=49;
        if(FAILED(d->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&empty))))return false;
        const UINT stride=d->GetDescriptorHandleIncrementSize(hd.Type);auto at=empty->GetCPUDescriptorHandleForHeapStart();
        for(UINT n=0;n<5;++n){d->CreateConstantBufferView(nullptr,at);at.ptr+=stride;}
        D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=DXGI_FORMAT_R32_FLOAT;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
        srv.Texture2D.MipLevels=1;srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        for(UINT n=5;n<41;++n){d->CreateShaderResourceView(nullptr,&srv,at);at.ptr+=stride;}
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};uav.Format=DXGI_FORMAT_R32_FLOAT;uav.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
        for(UINT n=41;n<49;++n){d->CreateUnorderedAccessView(nullptr,nullptr,&uav,at);at.ptr+=stride;}
        device=d;defaults=std::move(empty);step=stride;return true;
    }
    void Mark(Heap& h,UINT first,UINT count){
        if(version==UINT64_MAX){for(auto& entry:heaps)std::fill(entry.second->seen.begin(),entry.second->seen.end(),false);return;}
        const auto stamp=version++;
        for(UINT n=0;n<count;++n){h.seen[first+n]=true;h.versions[first+n]=stamp;}
    }
public:
    void Invalidate(){std::lock_guard<std::recursive_mutex> lock(gate);for(auto& entry:heaps)std::fill(entry.second->seen.begin(),entry.second->seen.end(),false);}
    struct Snapshot {
        ComPtr<ID3D12DescriptorHeap> native,descriptors;
        UINT first=0,step=0;
        std::uint64_t required=0;
        std::array<std::uint64_t,49> versions{};
        D3D12_CPU_DESCRIPTOR_HANDLE At(UINT slot)const {
            if(!descriptors||slot>=49)return {};auto at=descriptors->GetCPUDescriptorHandleForHeapStart();at.ptr+=SIZE_T(slot)*step;return at;
        }
    };
    bool HeapCreated(ID3D12Device* d,ID3D12DescriptorHeap* native){
        std::lock_guard<std::recursive_mutex> lock(gate);if(!native||privateWrites)return false;
        const auto desc=native->GetDesc();
        if(desc.Type!=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV||!(desc.Flags&D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)||
           desc.NumDescriptors<Prefix+49||desc.NumDescriptors-Prefix>MaximumSlots-slots||heaps.size()>=16||!Initialize(d))return false;
        const SIZE_T base=native->GetCPUDescriptorHandleForHeapStart().ptr+SIZE_T(Prefix)*step;
        if(heaps.count(base))return false;auto h=std::make_shared<Heap>();h->native=native;h->begin=base;h->count=desc.NumDescriptors-Prefix;h->step=step;
        if(FAILED(native->QueryInterface(IID_PPV_ARGS(&h->identity))))return false;
        D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=desc.Type;hd.NumDescriptors=h->count;Mute mute;
        if(FAILED(d->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&h->shadow))))return false;
        h->seen.resize(h->count,false);h->versions.resize(h->count,0);slots+=h->count;heaps.emplace(base,std::move(h));return true;
    }
    void Copy(UINT count,D3D12_CPU_DESCRIPTOR_HANDLE destination,D3D12_CPU_DESCRIPTOR_HANDLE source){
        std::lock_guard<std::recursive_mutex> lock(gate);if(privateWrites||!count||!device||!source.ptr)return;
        // A bulk native copy can begin in the material prefix and cross into the
        // fixed 49-slot tables. Clip only the mirrored destination portion.
        const SIZE_T end=destination.ptr+SIZE_T(count)*step;if(end<destination.ptr)return;
        for(auto& entry:heaps){auto& h=*entry.second;const SIZE_T heapEnd=h.begin+SIZE_T(h.count)*step;
            const SIZE_T first=std::max(destination.ptr,h.begin),last=std::min(end,heapEnd);
            if(first>=last||(first-h.begin)%step||(first-destination.ptr)%step||(last-first)%step)continue;
            const UINT index=static_cast<UINT>((first-h.begin)/step),n=static_cast<UINT>((last-first)/step);
            auto src=source;src.ptr+=first-destination.ptr;Mute mute;
            device->CopyDescriptorsSimple(n,At(h,index),src,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);Mark(h,index,n);
        }
    }
    void Copy(const std::vector<Range>& destinations,const std::vector<Range>& sources){
        std::lock_guard<std::recursive_mutex> lock(gate);if(privateWrites||!device)return;
        std::uint64_t a=0,b=0;for(const auto& r:destinations)a+=r.count;for(const auto& r:sources)b+=r.count;
        if(a!=b||a>1048576){Invalidate();return;}
        std::size_t di=0,si=0;UINT dn=0,sn=0;
        while(di<destinations.size()&&si<sources.size()){
            const auto& d=destinations[di];const auto& s=sources[si];const UINT n=std::min(d.count-dn,s.count-sn);
            auto dst=d.first,src=s.first;dst.ptr+=SIZE_T(dn)*step;src.ptr+=SIZE_T(sn)*step;if(n)Copy(n,dst,src);
            dn+=n;sn+=n;if(dn==d.count){++di;dn=0;}if(sn==s.count){++si;sn=0;}
        }
    }
    void Constant(const D3D12_CONSTANT_BUFFER_VIEW_DESC* desc,D3D12_CPU_DESCRIPTOR_HANDLE destination){
        std::lock_guard<std::recursive_mutex> lock(gate);if(privateWrites)return;auto h=Find(destination.ptr);if(!h)return;
        const UINT index=static_cast<UINT>((destination.ptr-h->begin)/h->step);Mute mute;device->CreateConstantBufferView(desc,At(*h,index));Mark(*h,index,1);
    }
    void ShaderResource(ID3D12Resource* resource,const D3D12_SHADER_RESOURCE_VIEW_DESC* desc,D3D12_CPU_DESCRIPTOR_HANDLE destination){
        std::lock_guard<std::recursive_mutex> lock(gate);if(privateWrites)return;auto h=Find(destination.ptr);if(!h)return;
        const UINT index=static_cast<UINT>((destination.ptr-h->begin)/h->step);Mute mute;device->CreateShaderResourceView(resource,desc,At(*h,index));Mark(*h,index,1);
    }
    void UnorderedAccess(ID3D12Resource* resource,ID3D12Resource* counter,const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc,D3D12_CPU_DESCRIPTOR_HANDLE destination){
        std::lock_guard<std::recursive_mutex> lock(gate);if(privateWrites)return;auto h=Find(destination.ptr);if(!h)return;
        const UINT index=static_cast<UINT>((destination.ptr-h->begin)/h->step);Mute mute;device->CreateUnorderedAccessView(resource,counter,desc,At(*h,index));Mark(*h,index,1);
    }
    bool Read(ID3D12DescriptorHeap* native,UINT first,std::uint64_t required,Snapshot& output){
        output={};std::lock_guard<std::recursive_mutex> lock(gate);
        if(!device||!native||first<Prefix||!required||(required>>49))return false;
        auto h=Find(native->GetCPUDescriptorHandleForHeapStart().ptr+SIZE_T(first)*step);if(!h)return false;
        ComPtr<IUnknown> identity;if(FAILED(native->QueryInterface(IID_PPV_ARGS(&identity)))||identity.Get()!=h->identity.Get())return false;
        const UINT index=first-Prefix;if(index>h->count||49>h->count-index)return false;
        for(UINT n=0;n<49;++n)if((required&(1ull<<n))&&!h->seen[index+n])return false;
        Snapshot result;result.native=native;result.first=first;result.step=step;result.required=required;
        D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=49;Mute mute;
        if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&result.descriptors))))return false;
        device->CopyDescriptorsSimple(49,result.At(0),defaults->GetCPUDescriptorHandleForHeapStart(),hd.Type);
        for(UINT n=0;n<49;){if(!h->seen[index+n]){++n;continue;}const UINT start=n;
            while(n<49&&h->seen[index+n]){result.versions[n]=h->versions[index+n];++n;}
            device->CopyDescriptorsSimple(n-start,result.At(start),At(*h,index+start),hd.Type);
        }
        output=std::move(result);return true;
    }
    // Refresh an already-owned CPU snapshot without allocation. A cloned native
    // draw table must follow native descriptor edits even when PT is rejected;
    // clearing PT alpha alone would leave its fallback using stale inputs.
    bool Refresh(Snapshot& snapshot){
        std::lock_guard<std::recursive_mutex> lock(gate);
        if(!device||!snapshot.native||!snapshot.descriptors||snapshot.first<Prefix||snapshot.step!=step)return false;
        auto h=Find(snapshot.native->GetCPUDescriptorHandleForHeapStart().ptr+SIZE_T(snapshot.first)*step);if(!h)return false;
        ComPtr<IUnknown> identity;if(FAILED(snapshot.native.As(&identity))||identity.Get()!=h->identity.Get())return false;
        const UINT index=snapshot.first-Prefix;if(index>h->count||49>h->count-index)return false;
        for(UINT n=0;n<49;++n)if((snapshot.required&(1ull<<n))&&!h->seen[index+n])return false;
        Mute mute;device->CopyDescriptorsSimple(49,snapshot.At(0),defaults->GetCPUDescriptorHandleForHeapStart(),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        snapshot.versions={};
        for(UINT n=0;n<49;){if(!h->seen[index+n]){++n;continue;}const UINT first=n;
            while(n<49&&h->seen[index+n]){snapshot.versions[n]=h->versions[index+n];++n;}
            device->CopyDescriptorsSimple(n-first,snapshot.At(first),At(*h,index+first),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }
        return true;
    }
    bool Unchanged(const Snapshot& snapshot){
        std::lock_guard<std::recursive_mutex> lock(gate);if(!snapshot.native||!snapshot.descriptors||snapshot.first<Prefix)return false;
        auto h=Find(snapshot.native->GetCPUDescriptorHandleForHeapStart().ptr+SIZE_T(snapshot.first)*step);if(!h)return false;
        const UINT index=snapshot.first-Prefix;if(index>h->count||49>h->count-index)return false;
        for(UINT n=0;n<49;++n)if(snapshot.required&(1ull<<n))if(!h->seen[index+n]||h->versions[index+n]!=snapshot.versions[n])return false;
        return true;
    }
};
inline Bank bank;
}
