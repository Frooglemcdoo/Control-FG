#pragma once
#include "snapshot_budget.h"
#include "d3d12_storage_lease.h"
#include "pt_owned_scene.h"
#include <map>
#include <mutex>
// Copies are recorded at the BLAS build boundary. Source states are supplied
// from the verified native buffer owner; no shader/root bindings are modified.
namespace GIGeometrySnapshot {
using U64=std::uint64_t;
using Microsoft::WRL::ComPtr;
struct Source {PTOwnedScene::Allocation allocation{};ComPtr<ID3D12Resource> resource;D3D12_RESOURCE_STATES state{};U64 offset=0,bytes=0;gi_reuse::BufferStorageLease lease;};
// Idle resources are reusable only after every recording, flight and scene
// owner has released Storage. Contents are ALWAYS freshly copied on checkout.
// 64 MiB idle cache is separate from the 512 MiB active snapshot budget.
class Pool {
    struct Idle {ComPtr<ID3D12Resource> resource;ComPtr<IUnknown> device;U64 bytes;};
    std::mutex mutex;std::vector<Idle> idle;U64 held=0,created=0,reused=0,evicted=0;
public:
    static constexpr U64 MaximumIdleBytes=64ull*1024*1024;
    struct Stats {U64 bytes,allocations,reuses,evictions;};
    Stats Statistics(){std::lock_guard<std::mutex> lock(mutex);return {held,created,reused,evicted};}
    ComPtr<ID3D12Resource> Take(ID3D12Device* device,U64 bytes){
        ComPtr<IUnknown> identity;if(!device||FAILED(device->QueryInterface(IID_PPV_ARGS(&identity))))return {};
        std::lock_guard<std::mutex> lock(mutex);
        for(auto it=idle.begin();it!=idle.end();++it)if(it->device.Get()==identity.Get()&&it->bytes==bytes){
            auto out=std::move(it->resource);held-=bytes;idle.erase(it);++reused;return out;
        }
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=bytes;desc.Height=1;
        desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ComPtr<ID3D12Resource> out;
        if(FAILED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&out))))return {};
        if(!gi_reuse::RetainCreatedStorage(out.Get()))return {};
        ++created;return out;
    }
    void Return(ComPtr<ID3D12Resource> resource) noexcept {
        // Destruction must stay noexcept, including allocation pressure.
        try {
            if(!resource)return;const auto bytes=resource->GetDesc().Width;
            ComPtr<IUnknown> device;if(FAILED(resource->GetDevice(IID_PPV_ARGS(&device))))return;
            std::lock_guard<std::mutex> lock(mutex);
            if(bytes>MaximumIdleBytes)return;
            // A full cache must adapt to the current scene. Previously old
            // sizes occupied it forever, so every new size was destroyed on
            // return and allocated again on the following frame.
            // Only idle, fully retired storage is eligible for eviction.
            while(!idle.empty()&&(bytes>MaximumIdleBytes-held||idle.size()>=1024)){
                held-=idle.front().bytes;idle.erase(idle.begin());++evicted;
            }
            idle.push_back({std::move(resource),std::move(device),bytes});held+=bytes;
        }catch(...){}
    }
};
struct Storage {
    std::shared_ptr<PTOwnedScene::LeaseBudget> budget;
    std::shared_ptr<Pool> pool;
    U64 bytes=0;
    std::map<U64,ComPtr<ID3D12Resource>> copies;
    ~Storage(){if(pool)for(auto& entry:copies)pool->Return(std::move(entry.second));copies.clear();if(bytes)budget->Release(bytes);}
};
struct Transfer {
    std::shared_ptr<Storage> storage;
    std::vector<Source> sources;
};
inline bool Record(ID3D12GraphicsCommandList* list,const std::vector<Source>& sources,
                   const std::shared_ptr<PTOwnedScene::LeaseBudget>& budget,std::shared_ptr<Transfer>& out,
                   const std::shared_ptr<Pool>& pool={},const char** failure=nullptr){
    if(failure)*failure="copy_input";
    out.reset();if(!list||!budget||sources.empty()||sources.size()>8192)return false;
    if(list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT&&list->GetType()!=D3D12_COMMAND_LIST_TYPE_COMPUTE)return false;
    auto transfer=std::make_shared<Transfer>();transfer->storage=std::make_shared<Storage>();
    auto& storage=*transfer->storage;storage.budget=budget;storage.pool=pool;
    std::map<U64,Source> unique;U64 total=0;
    ComPtr<ID3D12Device> device;if(FAILED(list->GetDevice(IID_PPV_ARGS(&device))))return false;
    for(const auto& s:sources){
        if(failure)*failure="copy_source_identity";
        if(!s.resource||!s.allocation.token||!s.allocation.address||!s.allocation.bytes)return false;
        const auto desc=s.resource->GetDesc();D3D12_HEAP_PROPERTIES heap{};D3D12_HEAP_FLAGS flags{};
        if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER||desc.Width!=s.allocation.bytes||
           s.resource->GetGPUVirtualAddress()!=s.allocation.address)return false;
        const U64 bytes=s.bytes?s.bytes:s.allocation.bytes;
        if(failure)*failure="copy_source_range";
        if(s.offset>s.allocation.bytes||bytes>s.allocation.bytes-s.offset||s.offset%4)return false;
        if(failure)*failure="copy_source_heap";
        if(FAILED(s.resource->GetHeapProperties(&heap,&flags))||heap.Type!=D3D12_HEAP_TYPE_DEFAULT)return false;
        if(failure)*failure="copy_source_read_state";
        if((s.state!=D3D12_RESOURCE_STATE_COMMON&&!(s.state&D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE))||
           (unsigned(s.state)&~unsigned(D3D12_RESOURCE_STATE_GENERIC_READ)))return false;
        // No transition is needed when COPY_SOURCE is already present. Queue
        // restrictions apply to explicit barriers, not extra idle read flags.
        if(failure)*failure="copy_source_compute_transition";
        if(list->GetType()==D3D12_COMMAND_LIST_TYPE_COMPUTE&&!(s.state&D3D12_RESOURCE_STATE_COPY_SOURCE)&&
           (s.state&(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_INDEX_BUFFER|
                     D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)))return false;
        auto old=unique.find(s.allocation.token);
        if(old!=unique.end()){
            if(old->second.resource.Get()!=s.resource.Get()||old->second.state!=s.state||old->second.offset!=s.offset||old->second.bytes!=bytes)return false;
            continue;
        }
        if(bytes>UINT64_MAX-3)return false;
        const U64 capacity=(bytes+3)&~U64(3);
        if(capacity>UINT64_MAX-total)return false;
        total+=capacity;auto ranged=s;ranged.bytes=bytes;
        if(!gi_reuse::AcquireStorageLease(s.resource.Get(),ranged.lease))return false;
        unique.emplace(s.allocation.token,std::move(ranged));
    }
    if(failure)*failure="copy_active_budget";
    if(!budget->Acquire(total))return false;storage.bytes=total;
    if(failure)*failure="copy_allocation";
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
    for(const auto& entry:unique){
        auto desc=entry.second.resource->GetDesc();desc.Flags=D3D12_RESOURCE_FLAG_NONE;
        // Copy only valid source bytes; the last raw DWORD may need private padding.
        desc.Width=(entry.second.bytes+3)&~U64(3);
        ComPtr<ID3D12Resource> copy;
        if(pool)copy=pool->Take(device.Get(),desc.Width);
        else if(FAILED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&copy))))return false;
        if(!copy||!gi_reuse::RetainCreatedStorage(copy.Get()))return false;
        storage.copies.emplace(entry.first,std::move(copy));transfer->sources.push_back(entry.second);
    }
    // All allocation/validation work completes before the first GPU command.
    for(const auto& s:transfer->sources){
        auto* dst=storage.copies.at(s.allocation.token).Get();
        D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition={s.resource.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,s.state,D3D12_RESOURCE_STATE_COPY_SOURCE};
        // COMMON buffers promote to COPY_SOURCE, then may promote to additional
        // read states for the native BLAS input. Do not insert a false explicit
        // COMMON barrier after earlier implicit reads in the same list.
        const bool transition=s.state!=D3D12_RESOURCE_STATE_COMMON&&!(s.state&D3D12_RESOURCE_STATE_COPY_SOURCE);
        if(transition)list->ResourceBarrier(1,&barrier);
        D3D12_RESOURCE_BARRIER destination{};destination.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        destination.Transition={dst,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_DEST};
        list->ResourceBarrier(1,&destination);
        list->CopyBufferRegion(dst,0,s.resource.Get(),s.offset,s.bytes);
        if(transition){std::swap(barrier.Transition.StateBefore,barrier.Transition.StateAfter);list->ResourceBarrier(1,&barrier);}
        barrier.Transition={dst,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_COMMON};
        list->ResourceBarrier(1,&barrier);
    }
    if(failure)*failure="none";
    out=std::move(transfer);return true;
}
}
