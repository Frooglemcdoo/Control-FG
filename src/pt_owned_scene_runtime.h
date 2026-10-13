#pragma once
#include <set>
#include "pt_owned_scene.h"
#include "pt_scene_dispatch_plan.h"
#include "pt_scene_leases.h"
#include "pt_geometry_snapshot.h"
#include "pt_command_bindings.h"
#include "pt_scene_observer.h"
#include "pt_native_shader_table.h"
#include "pt_native_buffer_observer.h"
#include <wrl/client.h>
#include <mutex>
#include <condition_variable>
// Retain at the verified native creation boundary, while its owner is present.
// GPU geometry preservation is explicit at the native build boundary; ordinary
// ownership/lookup calls do not record commands or infer immutable contents.
namespace PTOwnedScene {
class Runtime {
    using Resource=Microsoft::WRL::ComPtr<ID3D12Resource>;
    struct Entry {Resource resource;U64 token,address,bytes;};
    std::mutex mutex;std::condition_variable completed;U64 pending=0;Index index;std::map<U64,Entry> owners;std::map<U64,U64> wrappers;
    std::shared_ptr<PTGeometrySnapshot::Pool> geometryPool=std::make_shared<PTGeometrySnapshot::Pool>();
    U64 nextToken=0,held=0;bool active=true;
public:
    struct Stats {U64 retained=0,rejected=0,budget=0,observed=0,unownedGeometry=0,validatedRelease=0,mismatchedRelease=0,peakBytes=0;U64 resolved[6]{};};
private:
    Stats stats{};const char* geometryFailure="none";
    std::shared_ptr<LeaseBudget> leaseBudget;
public:
    explicit Runtime(std::shared_ptr<LeaseBudget> budget=std::make_shared<LeaseBudget>())
        :leaseBudget(std::move(budget)){}
    static constexpr U64 MaximumBytes=1024ull*1024*1024,MaximumResource=64ull*1024*1024;
    // Own references are movable so release validation happens without holding
    // the registry mutex across the native call (which can recurse).
    struct Release {Resource resource;U64 address=0,bytes=0;};
    unsigned Created(U64 wrapper,IUnknown* native,U64 address,U64 bytes){
        std::lock_guard<std::mutex> guard(mutex);if(!active)return 0;
        auto old=owners.find(wrapper);if(old!=owners.end()){index.Remove(old->second.token);wrappers.erase(old->second.token);held-=old->second.bytes;owners.erase(old);}
        if(!native||!address||!bytes||bytes>MaximumResource){++stats.rejected;return 2;}
        if(owners.size()>=8192||bytes>MaximumBytes-held){++stats.budget;return 3;}
        Resource r;if(FAILED(native->QueryInterface(IID_PPV_ARGS(&r)))){++stats.rejected;return 2;}
        const auto d=r->GetDesc();
        if(d.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER||d.Width<bytes||d.Width>MaximumResource||r->GetGPUVirtualAddress()!=address){++stats.rejected;return 2;}
        if(d.Width>MaximumBytes-held){++stats.budget;return 3;}
        const auto token=++nextToken;held+=d.Width;stats.peakBytes=(std::max)(stats.peakBytes,held);++stats.retained;
        index.Insert({token,address,d.Width});owners.emplace(wrapper,Entry{std::move(r),token,address,d.Width});wrappers.emplace(token,wrapper);return 1;
    }
    Release BeforeRelease(U64 wrapper){
        std::lock_guard<std::mutex> guard(mutex);Release out{};
        if(!active)return out;auto it=owners.find(wrapper);if(it==owners.end())return out;
        auto& e=it->second;index.Remove(e.token);wrappers.erase(e.token);++pending;
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
    std::set<U64> LiveTokens(){std::lock_guard<std::mutex> guard(mutex);std::set<U64> tokens;for(const auto& item:owners)tokens.insert(item.second.token);return tokens;}
    bool Identify(U64 address,U64 bytes,Allocation& allocation){
        allocation={};std::lock_guard<std::mutex> guard(mutex);if(!active)return false;
        const auto token=index.Find(address,bytes);if(!token)return false;
        const auto found=wrappers.find(token);if(found!=wrappers.end()){
            const auto& e=owners.at(found->second);allocation={token,address,e.bytes-(address-e.address)};return true;
        }
        return false;
    }
    // Native root CBVs use different upload slices in each pass. Read only an
    // owned UPLOAD allocation at the recording/submission boundary; never map
    // arbitrary GPU addresses or assume equal addresses mean equal camera data.
    bool ReadUpload(U64 address,void* destination,std::size_t bytes){
        if(!destination||!address||!bytes||bytes>4096)return false;
        std::lock_guard<std::mutex> guard(mutex);if(!active)return false;
        const auto token=index.Find(address,bytes);if(!token)return false;
        const auto found=wrappers.find(token);if(found!=wrappers.end()){const auto& e=owners.at(found->second);
            D3D12_HEAP_PROPERTIES properties{};D3D12_HEAP_FLAGS flags{};
            if(FAILED(e.resource->GetHeapProperties(&properties,&flags))||properties.Type!=D3D12_HEAP_TYPE_UPLOAD)return false;
            const SIZE_T offset=static_cast<SIZE_T>(address-e.address);const D3D12_RANGE range{offset,offset+bytes};void* mapped=nullptr;
            if(FAILED(e.resource->Map(0,&range,&mapped))||!mapped)return false;
            std::memcpy(destination,static_cast<const unsigned char*>(mapped)+offset,bytes);
            const D3D12_RANGE noWrite{};e.resource->Unmap(0,&noWrite);return true;
        }
        return false;
    }
    // The caller is at the native AS build boundary and holds its recording
    // context. The owner mutex prevents release/reuse during metadata capture.
    bool FreezeGeometry(ID3D12GraphicsCommandList* list,std::vector<PTSceneTopology::Geometry>& geometry,
                        const std::shared_ptr<LeaseBudget>& budget,std::shared_ptr<PTGeometrySnapshot::Transfer>& transfer){
        transfer.reset();std::lock_guard<std::mutex> guard(mutex);geometryFailure="geometry_input";if(!active||geometry.empty())return false;
        std::map<U64,PTGeometrySnapshot::Source> sources;
        for(const auto& g:geometry)for(const U64 token:{g.indexToken,g.positionToken}){
            if(sources.count(token))continue;
            const Entry* entry=nullptr;U64 wrapper=0;
            const auto found=wrappers.find(token);
            if(found!=wrappers.end()){wrapper=found->second;entry=&owners.at(wrapper);}
            geometryFailure="geometry_native_owner";
            PTNativeBufferObserver::Snapshot native{};
            if(!entry||!PTNativeBufferObserver::Read(reinterpret_cast<const void*>(wrapper),native)||
               native.resource!=reinterpret_cast<UINT_PTR>(entry->resource.Get())||native.address!=entry->address||native.bytes>entry->bytes)return false;
            D3D12_RESOURCE_STATES state{};geometryFailure="geometry_recorded_state";
            if(!PTCommandBindings::tracker.BufferState(list,entry->resource.Get(),static_cast<D3D12_RESOURCE_STATES>(native.state),state))return false;
            sources.emplace(token,PTGeometrySnapshot::Source{{token,entry->address,entry->bytes},entry->resource,state});
        }
        // Allocate metadata before recording; no failure may drop resources
        // once a copy references them in the command list.
        std::vector<std::shared_ptr<PTSceneTopology::FrozenGeometry>> frozen;
        for(const auto& g:geometry){
            auto f=std::make_shared<PTSceneTopology::FrozenGeometry>();
            const auto& i=sources.at(g.indexToken).allocation;const auto& p=sources.at(g.positionToken).allocation;
            f->indexBase=i.address;f->indexBytes=i.bytes;f->positionBase=p.address;f->positionBytes=p.bytes;frozen.push_back(std::move(f));
        }
        std::vector<PTGeometrySnapshot::Source> ordered;for(const auto& item:sources)ordered.push_back(item.second);
        PTCommandBindings::Mute privateCopies;
        if(!PTGeometrySnapshot::Record(list,ordered,budget,transfer,geometryPool,&geometryFailure))return false;
        for(std::size_t n=0;n<geometry.size();++n){
            auto& f=*frozen[n];f.owner=transfer->storage;
            f.privateIndex=transfer->storage->copies.at(geometry[n].indexToken)->GetGPUVirtualAddress();
            f.privatePosition=transfer->storage->copies.at(geometry[n].positionToken)->GetGPUVirtualAddress();
            geometry[n].frozen=std::move(frozen[n]);
        }
        return true;
    }
    // Called inside the matching writerReady callback, before native waitJob
    // can reclaim its CPU rows. Freeze identifiers, roots and allocation tokens
    // together; a later preparation cannot bind replacement allocations.
    bool SnapshotWriter(const PTNativeJobRoute::Ticket& ticket,PTNativeShaderTable::Identifiers& identifiers,PTSceneDispatch::Table& table,
                        const char** failure=nullptr,std::size_t* failedRow=nullptr){
        identifiers={};table={};
        if(failure)*failure="writer_input";if(failedRow)*failedRow=0;
        if(!ticket.serial||ticket.writer.rays!=2||ticket.writer.count<=0||ticket.writer.count>65536)return false;
        std::lock_guard<std::mutex> guard(mutex);if(failure)*failure="writer_owner_inactive";if(!active)return false;
        PTNativeBufferObserver::Snapshot buffer{};PTNativeShaderTable::Identifiers ids{};
        if(failure)*failure="writer_buffer_identity";
        if(!PTNativeBufferObserver::Read(reinterpret_cast<void*>(ticket.writer.buffer),buffer)||
           buffer.address!=ticket.layout.buffer||buffer.bytes!=ticket.layout.bytes)return false;
        if(failure)*failure="writer_pipeline_identifiers";
        if(!PTNativeShaderTable::ReadIdentifiers(reinterpret_cast<const void*>(ticket.writer.state),ids)||ids.pipeline!=ticket.pipeline)return false;
        std::vector<PTNativeBinding::Binding> bindings;bindings.reserve(static_cast<std::size_t>(ticket.writer.count));
        for(int n=0;n<ticket.writer.count;++n){PTNativeBinding::Binding b{};
            if(failure)*failure="writer_binding_read";if(failedRow)*failedRow=static_cast<std::size_t>(n);
            if(!PTNativeBinding::Read(reinterpret_cast<const void*>(ticket.writer.bindings),static_cast<unsigned>(n),b))return false;
            bindings.push_back(b);
        }
        PTSceneDispatch::Table snapshot;
        if(!PTNativeShaderTable::Build(ticket.layout,ids,ticket.serial,2,bindings,index,snapshot,failure,failedRow))return false;
        if(failure)*failure="writer_hit_range";
        if(snapshot.address!=ticket.hitTable||snapshot.bytes.size()!=ticket.hitBytes)return false;
        identifiers=ids;table=std::move(snapshot);if(failure)*failure="none";return true;
    }
    // Resolve and AddRef in the same owner transaction. A release/reuse cannot
    // occur between validating an allocation token and retaining its resource.
    bool PrepareLease(const PTSceneDispatch::Selection& selected,const PTSceneDispatch::Scene& scene,
                      const PTSceneDispatch::Table& table,const PTSceneDispatch::ExportSet& exports,
                      PTSceneDispatch::Plan& plan,LeaseBatch& batch){
        plan={};if(batch.Count()||!leaseBudget)return false;
        std::lock_guard<std::mutex> guard(mutex);if(!active)return false;
        PTSceneDispatch::Plan candidate;
        if(PTSceneDispatch::Prepare(selected,scene,table,index,exports,candidate).status!=PTSceneDispatch::Status::prepared)return false;
        std::map<U64,const Entry*> tokens;for(const auto& owner:owners)tokens.emplace(owner.second.token,&owner.second);
        auto payload=std::make_unique<LeaseBatch::Payload>();payload->budget=leaseBudget;
        U64 bytes=0;
        for(const auto token:candidate.allocationTokens){
            const auto found=tokens.find(token);if(found==tokens.end())return false;
            const auto& entry=*found->second;
            if(entry.bytes>UINT64_MAX-bytes)return false;
            bytes+=entry.bytes;payload->resources.push_back(entry.resource);
        }
        if(!leaseBudget->Acquire(bytes))return false;
        payload->bytes=bytes;payload->keepAlive=candidate.keepAlive;batch.payload=std::move(payload);plan=std::move(candidate);return true;
    }
    // Recheck under the allocation lock without acquiring a second GPU lease.
    // The caller must keep the original lease through this submission's fence.
    bool Revalidate(const PTSceneDispatch::Scene& scene,const PTSceneDispatch::Table& table,
                    const PTSceneDispatch::ExportSet& exports,const PTSceneDispatch::Plan& prepared){
        std::lock_guard<std::mutex> guard(mutex);if(!active)return false;
        const PTSceneDispatch::Selection selected{scene.address,scene.version,table.address,table.version,table.pipeline};
        PTSceneDispatch::Plan current;
        return PTSceneDispatch::Prepare(selected,scene,table,index,exports,current).status==PTSceneDispatch::Status::prepared&&
            PTSceneDispatch::SameDispatch(prepared,current);
    }
    const char* GeometryFailure(){std::lock_guard<std::mutex> guard(mutex);return geometryFailure;}
    PTGeometrySnapshot::Pool::Stats GeometryStatistics(){return geometryPool->Statistics();}
    Stats Stop(){std::unique_lock<std::mutex> guard(mutex);active=false;completed.wait(guard,[this]{return pending==0;});index.Clear();wrappers.clear();owners.clear();held=0;return stats;}
};
}
