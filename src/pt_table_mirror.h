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
#include "pt_hotpath_timing.h"
#include "pt_descriptor_interest.h"
#include "pt_descriptor_gate.h"
// Native descriptors cannot be copied out of a shader-visible heap. Mirror their
// CPU-source writes as they happen; snapshots copy only from this CPU-only heap.
// This preserves descriptor VALUES, not resource ownership or image contents.
namespace PTTableMirror {
using Microsoft::WRL::ComPtr;
inline DescriptorGate gate;
inline thread_local unsigned privateWrites=0;
struct Mute {Mute(){++privateWrites;}~Mute(){--privateWrites;}};
struct Scope {
    PTHotpathTiming::Sample timing,phase;
    std::unique_lock<DescriptorGate> lock;
    bool capture;
    explicit Scope(bool enabled,bool descriptorWrite=false):timing(PTHotpathTiming::Kind::descriptors,enabled&&!privateWrites),
        phase(descriptorWrite?PTHotpathTiming::Kind::descriptor_wait:PTHotpathTiming::Kind::snapshot_wait,enabled&&!privateWrites),
        lock(gate,std::defer_lock),capture(enabled&&!privateWrites){
        if(capture)lock.lock();
        phase.Next(descriptorWrite?PTHotpathTiming::Kind::descriptor_native:PTHotpathTiming::Kind::snapshot_work);
    }
    void Observing(){phase.Next(PTHotpathTiming::Kind::descriptor_observer);}
};
struct Range {D3D12_CPU_DESCRIPTOR_HANDLE first{};UINT count=0;};
class Bank {
    static constexpr UINT Prefix=244000,MaximumSlots=2097152;
    struct Heap {
        ComPtr<ID3D12DescriptorHeap> native,shadow;
        ComPtr<IUnknown> identity;
        SIZE_T begin=0;
        UINT count=0,step=0;
        // Adjacent slots can be written by independent recording threads.
        // vector<bool> would make disjoint slots race on the same packed word.
        std::vector<unsigned char> seen;
        std::vector<std::uint64_t> versions,sourceTokens;
    };
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12DescriptorHeap> defaults;
    std::map<SIZE_T,std::shared_ptr<Heap>> heaps;
    UINT slots=0,step=0;
    std::atomic<std::uint64_t> version{1};
    std::atomic<bool> exhausted{false};
    std::atomic<std::uint64_t> singleCopies{0},batchCopies{0},mirroredDescriptors{0},redundantCopies{0};
    std::array<std::atomic<Heap*>,16> published{};
    std::atomic<unsigned> publishedCount{0};
    std::uint64_t nextSourceToken=1;
    std::map<SIZE_T,std::uint64_t> sourceTokens;
    void SourceWritten(SIZE_T first,UINT count){
        if(!step||!count)return;
        if(count>(SIZE_MAX-first)/step){sourceTokens.clear();return;}
        sourceTokens.erase(sourceTokens.lower_bound(first),sourceTokens.lower_bound(first+SIZE_T(count)*step));
    }
    std::uint64_t SourceToken(SIZE_T address){
        const auto known=sourceTokens.find(address);if(known!=sourceTokens.end())return known->second;
        // Unknown sources are captured immediately before assigning a token.
        // Every later observed write erases that token. Capacity eviction uses
        // fresh monotonically increasing tokens, never recycled generations.
        if(nextSourceToken==UINT64_MAX)return 0;
        if(sourceTokens.size()>=65536)sourceTokens.clear();
        const auto token=nextSourceToken++;PTDescriptorInterest::filter.Mark(address);sourceTokens.emplace(address,token);return token;
    }
    std::map<SIZE_T,DXGI_FORMAT> uavFormats;
    std::map<DXGI_FORMAT,bool> readableFormats;
    void CopyFormats(UINT count,SIZE_T destination,SIZE_T source){
        const SIZE_T bytes=SIZE_T(count)*step;if(source+bytes<source||destination+bytes<destination)return;
        std::vector<std::pair<SIZE_T,DXGI_FORMAT>> copied;
        for(auto it=uavFormats.lower_bound(source);it!=uavFormats.end()&&it->first-source<bytes;++it)
            copied.emplace_back(destination+(it->first-source),it->second);
        auto first=uavFormats.lower_bound(destination),last=uavFormats.lower_bound(destination+bytes);uavFormats.erase(first,last);
        if(uavFormats.size()+copied.size()>65536){uavFormats.clear();return;}
        for(const auto& entry:copied){PTDescriptorInterest::filter.Mark(entry.first);uavFormats.insert(entry);}
    }
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
        if(exhausted||version==UINT64_MAX){exhausted=true;for(auto& entry:heaps)std::fill(entry.second->seen.begin(),entry.second->seen.end(),false);return;}
        const auto stamp=version++;
        SourceWritten(At(h,first).ptr,count);
        for(UINT n=0;n<count;++n){h.seen[first+n]=true;h.versions[first+n]=stamp;h.sourceTokens[first+n]=0;}
    }
public:
    Bank(){for(auto& heap:published)heap.store(nullptr,std::memory_order_relaxed);}
    // No source/destination metadata is touched here. Published Heap objects
    // are retained for the Bank lifetime; their bounds/device never change.
    // The native API requires callers to serialize conflicting descriptor
    // accesses. Shared ownership allows disjoint writes, while every snapshot,
    // bulk copy, metadata write and heap creation still takes exclusive access.
    template<class Forward>
    bool TryParallelCopy(D3D12_CPU_DESCRIPTOR_HANDLE destination,D3D12_CPU_DESCRIPTOR_HANDLE source,Forward&& forward){
        if(privateWrites||!destination.ptr||!source.ptr||
           PTDescriptorInterest::acceleration.Contains(destination.ptr)||PTDescriptorInterest::acceleration.Contains(source.ptr))return false;
        Heap* target=nullptr;UINT index=0;
        const auto count=publishedCount.load(std::memory_order_acquire);
        for(unsigned n=0;n<count;++n){auto* h=published[n].load(std::memory_order_relaxed);
            if(destination.ptr>=h->begin&&(destination.ptr-h->begin)%h->step==0&&(destination.ptr-h->begin)/h->step<h->count){
                target=h;index=static_cast<UINT>((destination.ptr-h->begin)/h->step);break;}}
        if(!target)return false;
        PTHotpathTiming::Sample phase(PTHotpathTiming::Kind::descriptor_parallel_wait);
        std::shared_lock<DescriptorGate> transaction(gate);
        // Inspect CURRENT metadata under shared ownership. Historical UAV tags
        // and a cached token for the SOURCE do not require any metadata write.
        // A token at the DESTINATION must be revoked by the exclusive path.
        if(PTDescriptorInterest::acceleration.Contains(destination.ptr)||PTDescriptorInterest::acceleration.Contains(source.ptr)||
           sourceTokens.find(destination.ptr)!=sourceTokens.end())return false;
        const auto destinationFormat=uavFormats.find(destination.ptr),sourceFormat=uavFormats.find(source.ptr);
        const bool hasDestination=destinationFormat!=uavFormats.end(),hasSource=sourceFormat!=uavFormats.end();
        if(hasDestination!=hasSource||(hasSource&&destinationFormat->second!=sourceFormat->second))return false;
        // Equal typed formats also require no metadata mutation. The actual
        // resource descriptor is still copied and receives a fresh version.
        phase.Next(PTHotpathTiming::Kind::descriptor_parallel_native);
        Mute privateCopy;forward();
        phase.Next(PTHotpathTiming::Kind::descriptor_parallel_mirror);
        device->CopyDescriptorsSimple(1,At(*target,index),source,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        ++singleCopies;++mirroredDescriptors;
        const auto stamp=version.fetch_add(1,std::memory_order_relaxed);
        if(stamp==UINT64_MAX)exhausted.store(true,std::memory_order_release);
        target->versions[index]=stamp;target->sourceTokens[index]=0;target->seen[index]=1;
        return true;
    }
    void Invalidate(){std::lock_guard<DescriptorGate> lock(gate);sourceTokens.clear();uavFormats.clear();for(auto& entry:heaps)std::fill(entry.second->seen.begin(),entry.second->seen.end(),false);}
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
        std::lock_guard<DescriptorGate> lock(gate);if(!native||privateWrites)return false;
        const auto desc=native->GetDesc();
        // CPU heap address reuse must invalidate cached source identities too.
        if(desc.Type==D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)SourceWritten(native->GetCPUDescriptorHandleForHeapStart().ptr,desc.NumDescriptors);
        if(desc.Type!=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV||!(desc.Flags&D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)||
           desc.NumDescriptors<Prefix+49||desc.NumDescriptors-Prefix>MaximumSlots-slots||heaps.size()>=16||!Initialize(d))return false;
        const SIZE_T base=native->GetCPUDescriptorHandleForHeapStart().ptr+SIZE_T(Prefix)*step;
        if(heaps.count(base))return false;auto h=std::make_shared<Heap>();h->native=native;h->begin=base;h->count=desc.NumDescriptors-Prefix;h->step=step;
        if(FAILED(native->QueryInterface(IID_PPV_ARGS(&h->identity))))return false;
        D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=desc.Type;hd.NumDescriptors=h->count;Mute mute;
        if(FAILED(d->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&h->shadow))))return false;
        h->seen.resize(h->count,false);h->versions.resize(h->count,0);h->sourceTokens.resize(h->count,0);slots+=h->count;
        PTDescriptorInterest::filter.IncludeMirror(base,base+SIZE_T(h->count)*step);
        auto* publishedHeap=h.get();heaps.emplace(base,std::move(h));
        const auto position=publishedCount.load(std::memory_order_relaxed);
        published[position].store(publishedHeap,std::memory_order_relaxed);
        publishedCount.store(position+1,std::memory_order_release);return true;
    }
    void Copy(UINT count,D3D12_CPU_DESCRIPTOR_HANDLE destination,D3D12_CPU_DESCRIPTOR_HANDLE source){
        std::lock_guard<DescriptorGate> lock(gate);if(privateWrites||!count||!device||!source.ptr)return;
        SourceWritten(destination.ptr,count);
        if(count==1){
            auto h=Find(destination.ptr);
            if(h){
                const UINT index=static_cast<UINT>((destination.ptr-h->begin)/step);
                const auto token=SourceToken(source.ptr);
                if(token&&h->seen[index]&&h->sourceTokens[index]==token){++redundantCopies;return;}
                CopyFormats(1,destination.ptr,source.ptr);Mute mute;
                device->CopyDescriptorsSimple(1,At(*h,index),source,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                ++singleCopies;++mirroredDescriptors;Mark(*h,index,1);h->sourceTokens[index]=token;return;
            }
        }
        CopyFormats(count,destination.ptr,source.ptr);
        // A bulk native copy can begin in the material prefix and cross into the
        // fixed 49-slot tables. Clip only the mirrored destination portion.
        const SIZE_T end=destination.ptr+SIZE_T(count)*step;if(end<destination.ptr)return;
        for(auto& entry:heaps){auto& h=*entry.second;const SIZE_T heapEnd=h.begin+SIZE_T(h.count)*step;
            const SIZE_T first=std::max(destination.ptr,h.begin),last=std::min(end,heapEnd);
            if(first>=last||(first-h.begin)%step||(first-destination.ptr)%step||(last-first)%step)continue;
            const UINT index=static_cast<UINT>((first-h.begin)/step),n=static_cast<UINT>((last-first)/step);
            auto src=source;src.ptr+=first-destination.ptr;Mute mute;
            device->CopyDescriptorsSimple(n,At(h,index),src,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            ++singleCopies;mirroredDescriptors+=n;Mark(h,index,n);
        }
    }
    struct CopyStats {std::uint64_t single,batch,descriptors,redundant;};
    CopyStats Statistics(){std::lock_guard<DescriptorGate> lock(gate);return {singleCopies,batchCopies,mirroredDescriptors,redundantCopies};}
    void Copy(const std::vector<Range>& destinations,const std::vector<Range>& sources){
        std::lock_guard<DescriptorGate> lock(gate);if(privateWrites||!device)return;
        std::uint64_t a=0,b=0;
        for(const auto& r:destinations){if(r.count>(SIZE_MAX-r.first.ptr)/step){Invalidate();return;}a+=r.count;}
        for(const auto& r:sources){if(r.count>(SIZE_MAX-r.first.ptr)/step){Invalidate();return;}b+=r.count;}
        if(a!=b||a>1048576){Invalidate();return;}
        for(const auto& r:destinations)SourceWritten(r.first.ptr,r.count);
        // Preserve sparse typed-UAV metadata as ONE transaction. The old path
        // allocated a temporary vector and repeatedly searched destination
        // heaps for every single source range, including the bindless prefix.
        std::vector<std::pair<std::uint64_t,DXGI_FORMAT>> formats;
        std::uint64_t offset=0;bool overflow=false;
        for(const auto& r:sources){
            const SIZE_T bytes=SIZE_T(r.count)*step;
            for(auto it=uavFormats.lower_bound(r.first.ptr);it!=uavFormats.end()&&it->first-r.first.ptr<bytes;++it){
                if(formats.size()>=65536){overflow=true;break;}
                formats.emplace_back(offset+(it->first-r.first.ptr)/step,it->second);
            }
            if(overflow)break;offset+=r.count;
        }
        if(overflow)uavFormats.clear();
        else {
            for(const auto& r:destinations)uavFormats.erase(uavFormats.lower_bound(r.first.ptr),uavFormats.lower_bound(r.first.ptr+SIZE_T(r.count)*step));
            std::size_t di=0;offset=0;
            for(const auto& f:formats){
                while(di<destinations.size()&&f.first-offset>=destinations[di].count){offset+=destinations[di].count;++di;}
                if(di==destinations.size()){uavFormats.clear();break;}
                const auto address=destinations[di].first.ptr+SIZE_T(f.first-offset)*step;
                if(uavFormats.size()>=65536&&!uavFormats.count(address)){uavFormats.clear();break;}
                PTDescriptorInterest::filter.Mark(address);uavFormats[address]=f.second;
            }
        }
        // Clip destination ranges BEFORE walking source fragments. Only the
        // fixed-table suffix is mirrored. One native batch per intersected
        // heap replaces potentially thousands of tiny native copy calls.
        for(auto& entry:heaps){auto& h=*entry.second;const SIZE_T heapEnd=h.begin+SIZE_T(h.count)*step;
            std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> to,from;
            std::vector<UINT> toSizes,fromSizes,indices;
            std::size_t si=0;std::uint64_t sourceOffset=0,destinationOffset=0;
            for(const auto& d:destinations){
                const SIZE_T end=d.first.ptr+SIZE_T(d.count)*step;
                const SIZE_T first=std::max(d.first.ptr,h.begin),last=std::min(end,heapEnd);
                if(first<last&&(first-h.begin)%step==0&&(first-d.first.ptr)%step==0&&(last-first)%step==0){
                    const UINT index=static_cast<UINT>((first-h.begin)/step),count=static_cast<UINT>((last-first)/step);
                    to.push_back(At(h,index));toSizes.push_back(count);indices.push_back(index);
                    std::uint64_t position=destinationOffset+(first-d.first.ptr)/step;UINT left=count;
                    while(left){
                        while(si<sources.size()&&sourceOffset+sources[si].count<=position){sourceOffset+=sources[si].count;++si;}
                        if(si==sources.size()){Invalidate();return;}
                        const auto& source=sources[si];const UINT skip=static_cast<UINT>(position-sourceOffset);
                        const UINT n=std::min(left,source.count-skip);auto handle=source.first;handle.ptr+=SIZE_T(skip)*step;
                        from.push_back(handle);fromSizes.push_back(n);position+=n;left-=n;
                    }
                }
                destinationOffset+=d.count;
            }
            if(to.empty())continue;Mute mute;
            device->CopyDescriptors(static_cast<UINT>(to.size()),to.data(),toSizes.data(),
                static_cast<UINT>(from.size()),from.data(),fromSizes.data(),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            ++batchCopies;
            for(std::size_t n=0;n<to.size();++n){mirroredDescriptors+=toSizes[n];Mark(h,indices[n],toSizes[n]);}
        }
    }
    void Constant(const D3D12_CONSTANT_BUFFER_VIEW_DESC* desc,D3D12_CPU_DESCRIPTOR_HANDLE destination){
        std::lock_guard<DescriptorGate> lock(gate);if(privateWrites)return;SourceWritten(destination.ptr,1);uavFormats.erase(destination.ptr);auto h=Find(destination.ptr);if(!h)return;
        const UINT index=static_cast<UINT>((destination.ptr-h->begin)/h->step);Mute mute;device->CreateConstantBufferView(desc,At(*h,index));Mark(*h,index,1);
    }
    void ShaderResource(ID3D12Resource* resource,const D3D12_SHADER_RESOURCE_VIEW_DESC* desc,D3D12_CPU_DESCRIPTOR_HANDLE destination){
        std::lock_guard<DescriptorGate> lock(gate);if(privateWrites)return;SourceWritten(destination.ptr,1);uavFormats.erase(destination.ptr);auto h=Find(destination.ptr);if(!h)return;
        const UINT index=static_cast<UINT>((destination.ptr-h->begin)/h->step);Mute mute;device->CreateShaderResourceView(resource,desc,At(*h,index));Mark(*h,index,1);
    }
    void UnorderedAccess(ID3D12Resource* resource,ID3D12Resource* counter,const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc,D3D12_CPU_DESCRIPTOR_HANDLE destination){
        std::lock_guard<DescriptorGate> lock(gate);if(privateWrites)return;
        SourceWritten(destination.ptr,1);uavFormats.erase(destination.ptr);
        if(resource&&!counter){const auto rd=resource->GetDesc();
            if(rd.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&rd.DepthOrArraySize==1&&
               (!desc||desc->ViewDimension==D3D12_UAV_DIMENSION_TEXTURE2D)){
                if(uavFormats.size()>=65536)uavFormats.clear();
                PTDescriptorInterest::filter.Mark(destination.ptr);uavFormats[destination.ptr]=desc?desc->Format:rd.Format;
            }
        }
        auto h=Find(destination.ptr);if(!h)return;
        const UINT index=static_cast<UINT>((destination.ptr-h->begin)/h->step);Mute mute;device->CreateUnorderedAccessView(resource,counter,desc,At(*h,index));Mark(*h,index,1);
    }
    bool TypedLoadSupported(const Snapshot& snapshot,UINT slot){
        std::lock_guard<DescriptorGate> lock(gate);
        if(exhausted||!device||!snapshot.native||slot>=49||!Unchanged(snapshot))return false;
        const SIZE_T address=snapshot.native->GetCPUDescriptorHandleForHeapStart().ptr+SIZE_T(snapshot.first+slot)*step;
        const auto format=uavFormats.find(address);if(format==uavFormats.end()||format->second==DXGI_FORMAT_UNKNOWN)return false;
        const auto known=readableFormats.find(format->second);if(known!=readableFormats.end())return known->second;
        D3D12_FEATURE_DATA_FORMAT_SUPPORT support{};support.Format=format->second;
        const bool supported=SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT,&support,sizeof(support)))&&
            (support.Support2&D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD)!=0;
        readableFormats.emplace(format->second,supported);return supported;
    }
    bool Read(ID3D12DescriptorHeap* native,UINT first,std::uint64_t required,Snapshot& output){
        output={};std::lock_guard<DescriptorGate> lock(gate);
        if(exhausted||!device||!native||first<Prefix||!required||(required>>49))return false;
        auto h=Find(native->GetCPUDescriptorHandleForHeapStart().ptr+SIZE_T(first)*step);if(!h)return false;
        ComPtr<IUnknown> identity;if(FAILED(native->QueryInterface(IID_PPV_ARGS(&identity)))||identity.Get()!=h->identity.Get())return false;
        const UINT index=first-Prefix;if(index>h->count||49>h->count-index)return false;
        for(UINT n=0;n<49;++n)if((required&(1ull<<n))&&!h->seen[index+n])return false;
        Snapshot result;result.native=native;result.first=first;result.step=step;result.required=required;
        D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=49;Mute mute;
        if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&result.descriptors))))return false;
        SourceWritten(result.At(0).ptr,49);
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
        std::lock_guard<DescriptorGate> lock(gate);
        if(exhausted||!device||!snapshot.native||!snapshot.descriptors||snapshot.first<Prefix||snapshot.step!=step)return false;
        auto h=Find(snapshot.native->GetCPUDescriptorHandleForHeapStart().ptr+SIZE_T(snapshot.first)*step);if(!h)return false;
        ComPtr<IUnknown> identity;if(FAILED(snapshot.native.As(&identity))||identity.Get()!=h->identity.Get())return false;
        const UINT index=snapshot.first-Prefix;if(index>h->count||49>h->count-index)return false;
        for(UINT n=0;n<49;++n)if((snapshot.required&(1ull<<n))&&!h->seen[index+n])return false;
        SourceWritten(snapshot.At(0).ptr,49);
        Mute mute;device->CopyDescriptorsSimple(49,snapshot.At(0),defaults->GetCPUDescriptorHandleForHeapStart(),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        snapshot.versions={};
        for(UINT n=0;n<49;){if(!h->seen[index+n]){++n;continue;}const UINT first=n;
            while(n<49&&h->seen[index+n]){snapshot.versions[n]=h->versions[index+n];++n;}
            device->CopyDescriptorsSimple(n-first,snapshot.At(first),At(*h,index+first),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }
        return true;
    }
    bool Unchanged(const Snapshot& snapshot){
        std::lock_guard<DescriptorGate> lock(gate);if(exhausted||!snapshot.native||!snapshot.descriptors||snapshot.first<Prefix)return false;
        auto h=Find(snapshot.native->GetCPUDescriptorHandleForHeapStart().ptr+SIZE_T(snapshot.first)*step);if(!h)return false;
        const UINT index=snapshot.first-Prefix;if(index>h->count||49>h->count-index)return false;
        for(UINT n=0;n<49;++n)if(snapshot.required&(1ull<<n))if(!h->seen[index+n]||h->versions[index+n]!=snapshot.versions[n])return false;
        return true;
    }
};
inline Bank bank;
}
