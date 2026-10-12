#pragma once
#include "pt_owned_scene.h"
#include "pt_scene_observer.h"
#include <wrl/client.h>
#include <mutex>
#include <condition_variable>
// Retain at the verified native creation boundary, while its owner is present.
// Bounded CPU-only integration: no GPU work, resource-state changes or waits.
namespace PTOwnedScene {
class Runtime {
    using Resource=Microsoft::WRL::ComPtr<ID3D12Resource>;
    struct Entry {Resource resource;U64 token,address,bytes;};
    std::mutex mutex;std::condition_variable completed;U64 pending=0;Index index;std::map<U64,Entry> owners;
    U64 nextToken=0,held=0;bool active=true;
public:
    struct Stats {U64 retained=0,rejected=0,budget=0,observed=0,unownedGeometry=0,validatedRelease=0,mismatchedRelease=0,peakBytes=0;U64 resolved[6]{};};
private:
    Stats stats{};
public:
    static constexpr U64 MaximumBytes=1024ull*1024*1024,MaximumResource=64ull*1024*1024;
    // Own references are movable so release validation happens without holding
    // the registry mutex across the native call (which can recurse).
    struct Release {Resource resource;U64 address=0,bytes=0;};
    unsigned Created(U64 wrapper,IUnknown* native,U64 address,U64 bytes){
        std::lock_guard<std::mutex> guard(mutex);if(!active)return 0;
        auto old=owners.find(wrapper);if(old!=owners.end()){index.Remove(old->second.token);held-=old->second.bytes;owners.erase(old);}
        if(!native||!address||!bytes||bytes>MaximumResource){++stats.rejected;return 2;}
        if(owners.size()>=8192||bytes>MaximumBytes-held){++stats.budget;return 3;}
        Resource r;if(FAILED(native->QueryInterface(IID_PPV_ARGS(&r)))){++stats.rejected;return 2;}
        const auto d=r->GetDesc();
        if(d.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER||d.Width<bytes||d.Width>MaximumResource||r->GetGPUVirtualAddress()!=address){++stats.rejected;return 2;}
        if(d.Width>MaximumBytes-held){++stats.budget;return 3;}
        const auto token=++nextToken;held+=d.Width;stats.peakBytes=(std::max)(stats.peakBytes,held);++stats.retained;
        index.Insert({token,address,d.Width});owners.emplace(wrapper,Entry{std::move(r),token,address,d.Width});return 1;
    }
    Release BeforeRelease(U64 wrapper){
        std::lock_guard<std::mutex> guard(mutex);Release out{};
        if(!active)return out;auto it=owners.find(wrapper);if(it==owners.end())return out;
        auto& e=it->second;index.Remove(e.token);++pending;
        out={std::move(e.resource),e.address,e.bytes};owners.erase(it);return out;
    }
    unsigned AfterRelease(Release& r){
        if(!r.resource)return 0;
        const auto d=r.resource->GetDesc();
        const bool valid=d.Dimension==D3D12_RESOURCE_DIMENSION_BUFFER&&d.Width==r.bytes&&r.resource->GetGPUVirtualAddress()==r.address;
        r.resource.Reset();std::lock_guard<std::mutex> guard(mutex);
        held-=r.bytes;--pending;completed.notify_all();
        if(valid)++stats.validatedRelease;else ++stats.mismatchedRelease;
        return valid?5u:6u;
    }
    void Geometry(const PTSceneObserver::Event& e){
        std::lock_guard<std::mutex> guard(mutex);if(!active)return;
        if(e.geometryType||e.vertexFormat!=6||e.transform||(e.indexFormat!=57&&e.indexFormat!=42)){++stats.unownedGeometry;return;}
        if(index.Observe({e.index,e.position,e.stride,0,0,e.indexFormat==57?2u:4u,e.indexCount,e.vertexCount}))++stats.observed;
        else ++stats.unownedGeometry;
    }
    Status Resolve(const PTNativeBinding::Binding& binding,PTSceneAdmission::Record& record){
        std::lock_guard<std::mutex> guard(mutex);if(!active){record={};return Status::missingAllocation;}
        const auto status=index.Resolve(binding,record);++stats.resolved[static_cast<unsigned>(status)];return status;
    }
    Stats Stop(){std::unique_lock<std::mutex> guard(mutex);active=false;completed.wait(guard,[this]{return pending==0;});index.Clear();owners.clear();held=0;return stats;}
};
}
