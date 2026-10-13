#pragma once
#include "pt_scene_catalog.h"
#include "pt_scene_descriptors.h"
#include "pt_owned_scene_runtime.h"
#include <wrl/client.h>
// Captures native build inputs into the submitted-scene catalog. The host must
// serialize Submit/Signal/Wait with the corresponding native queue operations.
// No dispatch is enabled merely by constructing this observer.
namespace PTSceneLive {
class Capture {
    using U64=std::uint64_t;
    template<class T> using ComPtr=Microsoft::WRL::ComPtr<T>;
    struct Identity {ComPtr<IUnknown> owner;U64 id=0,generation=0;};
    struct Upload {U64 token=0;std::vector<PTSceneTopology::Instance> instances;};
    PTOwnedScene::Runtime& resources;
    PTSceneCatalog::Catalog catalog;
    std::mutex mutex;
    std::map<IUnknown*,Identity> identities;
    std::map<U64,Upload> uploads;
    struct Progress {ComPtr<ID3D12Fence> fence;U64 value=0,completedStamp=0;std::map<U64,U64> stamps;};
    std::map<U64,Progress> progress;
    struct WaitFence {ComPtr<ID3D12Fence> fence;U64 completed=0;};
    std::map<U64,WaitFence> waitFences;
    void RefreshWaitFences(){
        for(auto& item:waitFences){auto& f=item.second;const auto done=f.fence->GetCompletedValue();
            if(done==UINT64_MAX){catalog.Invalidate("wait_fence_device_removed");return;}
            if(done!=f.completed){f.completed=done;stats.completedWaits+=catalog.ObserveFenceCompletion(item.first,done);}
        }
    }
    U64 checkpointOperations=0;
    U64 lastCheckpointAttempt=0;
    U64 serial=0;
    bool preserveGeometry=false;
    std::atomic<bool> renderingRequested{true};
    std::shared_ptr<PTOwnedScene::LeaseBudget> copyBudget=std::make_shared<PTOwnedScene::LeaseBudget>(512ull*1024*1024);
    std::map<U64,std::vector<std::shared_ptr<PTGeometrySnapshot::Transfer>>> recordedCopies;
    std::map<U64,PTGeometrySnapshot::Retirement::Batch> submittingCopies;
    PTGeometrySnapshot::Retirement retirement;
    Identity* Identify(IUnknown* object){
        if(!object)return nullptr;
        ComPtr<IUnknown> canonical;if(FAILED(object->QueryInterface(IID_PPV_ARGS(&canonical))))return nullptr;
        auto old=identities.find(canonical.Get());if(old!=identities.end())return &old->second;
        if(identities.size()>=1024)return nullptr;
        auto* key=canonical.Get();return &identities.emplace(key,Identity{std::move(canonical),++serial,0}).first->second;
    }
    Identity* List(IUnknown* object){
        auto* id=Identify(object);if(id&&!id->generation)id->generation=catalog.Begin(id->id);return id;
    }
    PTSceneCatalog::Allocation Allocation(U64 address,U64 bytes=256){
        PTOwnedScene::Allocation a{};if(!resources.Identify(address,bytes,a))return {};
        return {{a.token,a.address},a.bytes};
    }
    static bool Copy(void* dst,const void* src,std::size_t bytes)noexcept{
        __try{if(!src)return false;std::memcpy(dst,src,bytes);return true;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}
    }
    void Record(IUnknown* list,PTSceneCatalog::Snapshot snapshot){
        auto* id=List(list);
        if(!id){catalog.Invalidate("build_list_identity");return;}
        const char* reason="build_record_failed";
        if(!catalog.Record(id->id,id->generation,std::move(snapshot),&reason))catalog.Invalidate(reason);
    }
    bool Destination(IUnknown* list,U64 address,PTSceneCatalog::Snapshot& snapshot){
        // A registry miss is local to this write, not loss of queue ordering.
        // Never guess an owner: invalidate ALL live aliases at this address.
        // With no live owner, no old token can pass dispatch lease validation;
        // a later registration receives a fresh token and requires a new build.
        const auto owners=resources.DestinationOwners(address);
        if(owners.size()==1&&owners.front().bytes>=256){const auto& a=owners.front();
            snapshot.destination={{a.token,a.address},a.bytes};return true;
        }
        ++stats.untrackedDestinations;stats.lastUntrackedDestination=address;
        for(const auto& a:owners){PTSceneCatalog::Snapshot unknown;unknown.destination={{a.token,a.address},a.bytes};
            Record(list,std::move(unknown));++stats.destinationTombstones;}
        return false;
    }
public:
    struct Timing {U64 calls=0,totalMs=0,maxMs=0;};
    struct Stats {Timing builds,checkpoints;U64 skippedInvalid=0,untrackedDestinations=0,destinationTombstones=0,lastUntrackedDestination=0,copyFailures=0,offUpdatesPreserved=0,completedWaits=0,historyVersions=0,historyRows=0,historyNodes=0,copyBytes=0;bool healthy=true;const char* failure="none";};
private:
    Stats stats;
    struct Measure {Timing& value;ULONGLONG start=GetTickCount64();~Measure(){const U64 ms=GetTickCount64()-start;++value.calls;value.totalMs+=ms;value.maxMs=(std::max)(value.maxMs,ms);}};
public:
    Stats Statistics(){std::lock_guard<std::mutex> guard(mutex);stats.healthy=catalog.Healthy();stats.failure=catalog.Failure();const auto history=catalog.StatisticsNow();stats.historyVersions=history.versions;stats.historyRows=history.rows;stats.historyNodes=history.nodes;stats.copyBytes=copyBudget->Held();return stats;}
    explicit Capture(PTOwnedScene::Runtime& bank):resources(bank){}
    void RenderingRequested(bool value) noexcept {renderingRequested.store(value,std::memory_order_relaxed);}
    bool PreserveGeometry(){std::lock_guard<std::mutex> guard(mutex);if(!identities.empty())return false;preserveGeometry=true;return true;}
    void UploadInstances(U64 address,const void* data,U64 bytes){
        std::lock_guard<std::mutex> guard(mutex);
        // Erase the preceding snapshot even when the new upload is rejected.
        uploads.erase(address);
        if(!address||!bytes||bytes%64||bytes>2ull*1024*1024||uploads.size()>=64)return;
        const auto a=Allocation(address,bytes);if(!a.key.token)return;
        Upload upload;upload.token=a.key.token;upload.instances.resize(static_cast<std::size_t>(bytes/64));
        if(Copy(upload.instances.data(),data,static_cast<std::size_t>(bytes)))uploads.emplace(address,std::move(upload));
    }
    void Build(IUnknown* list,const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* desc){
        std::lock_guard<std::mutex> guard(mutex);Measure timing{stats.builds};
        if(!catalog.Healthy()){++stats.skippedInvalid;return;}
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC d{};
        if(!PTSceneObserver::ReadBuild(desc,d)){catalog.Invalidate();return;}
        PTSceneCatalog::Snapshot s;s.unavailable="scene_build_input_unsupported";if(!Destination(list,d.DestAccelerationStructureData,s))return;
        if(d.Inputs.Type==D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL){
            const auto u=uploads.find(d.Inputs.InstanceDescs);
            const auto a=Allocation(d.Inputs.InstanceDescs,U64(d.Inputs.NumDescs)*64);
            if(d.Inputs.DescsLayout==D3D12_ELEMENTS_LAYOUT_ARRAY&&d.Inputs.NumDescs&&d.Inputs.NumDescs<=32768&&
               u!=uploads.end()&&u->second.token==a.key.token&&u->second.instances.size()==d.Inputs.NumDescs){
                s.kind=PTSceneCatalog::Snapshot::Kind::tlas;s.instances=u->second.instances;
                for(const auto& i:s.instances)if(i.idMask>>24)s.bottom.emplace(i.blas,Allocation(i.blas));
            }
        }else if(d.Inputs.Type==D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL&&d.Inputs.NumDescs&&d.Inputs.NumDescs<=4096){
            s.kind=PTSceneCatalog::Snapshot::Kind::blas;s.unavailable="scene_geometry_owner_missing";
            for(UINT n=0;n<d.Inputs.NumDescs;++n){
                D3D12_RAYTRACING_GEOMETRY_DESC g{};
                if(!PTSceneObserver::ReadGeometry(d.Inputs,n,g)||g.Type!=D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES||
                   g.Triangles.Transform3x4||g.Triangles.VertexFormat!=DXGI_FORMAT_R32G32B32_FLOAT||
                   (g.Triangles.IndexFormat!=DXGI_FORMAT_R16_UINT&&g.Triangles.IndexFormat!=DXGI_FORMAT_R32_UINT)){
                    s.kind=PTSceneCatalog::Snapshot::Kind::unknown;s.unavailable="scene_geometry_format_unsupported";break;
                }
                const auto& t=g.Triangles;const UINT stride=t.IndexFormat==DXGI_FORMAT_R16_UINT?2u:4u;
                const U64 indexBytes=U64(t.IndexCount)*stride;
                if(!t.VertexCount||t.VertexBuffer.StrideInBytes<12||
                   U64(t.VertexCount-1)>(UINT64_MAX-12)/t.VertexBuffer.StrideInBytes){s.kind=PTSceneCatalog::Snapshot::Kind::unknown;break;}
                const U64 positionBytes=U64(t.VertexCount-1)*t.VertexBuffer.StrideInBytes+12;
                const auto index=Allocation(t.IndexBuffer,indexBytes),position=Allocation(t.VertexBuffer.StartAddress,positionBytes);
                s.geometry.push_back({t.IndexBuffer,t.VertexBuffer.StartAddress,t.VertexBuffer.StrideInBytes,stride,t.IndexCount,t.VertexCount,index.key.token,position.key.token});
            }
        }
        // An off-mode update can be the LAST update of an object that then
        // sleeps. Preserve it now: toggling PT on cannot request a native
        // rebuild, and reusing its older deformation would be incorrect.
        const bool offUpdate=!renderingRequested.load(std::memory_order_relaxed)&&
            (d.Inputs.Flags&D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE);
        if(preserveGeometry&&s.kind==PTSceneCatalog::Snapshot::Kind::blas){
            auto* id=List(list);ComPtr<ID3D12GraphicsCommandList> commands;
            if(!id||FAILED(list->QueryInterface(IID_PPV_ARGS(&commands)))){catalog.Invalidate();return;}
            auto& copies=recordedCopies[id->id];copies.emplace_back();
            if(!resources.FreezeGeometry(commands.Get(),s.geometry,copyBudget,copies.back())){
                ++stats.copyFailures;copies.pop_back();s.kind=PTSceneCatalog::Snapshot::Kind::unknown;s.unavailable=resources.GeometryFailure();
            }else if(offUpdate)++stats.offUpdatesPreserved;
        }
        Record(list,std::move(s));
    }
    void Compact(IUnknown* list,U64 dst,U64 src,D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE mode){
        std::lock_guard<std::mutex> guard(mutex);if(!catalog.Healthy()){++stats.skippedInvalid;return;}
        PTSceneCatalog::Snapshot s;if(!Destination(list,dst,s))return;
        if(mode==D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT||mode==D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_CLONE){
            s.kind=PTSceneCatalog::Snapshot::Kind::copy;s.source=Allocation(src);
        }
        Record(list,std::move(s));
    }
    bool Healthy()const {return catalog.Healthy();}
    Identity* Existing(IUnknown* object){
        ComPtr<IUnknown> canonical;if(!object||FAILED(object->QueryInterface(IID_PPV_ARGS(&canonical))))return nullptr;
        const auto found=identities.find(canonical.Get());return found==identities.end()?nullptr:&found->second;
    }
    void Reset(IUnknown* list){
        std::lock_guard<std::mutex> guard(mutex);auto* id=Existing(list);
        // Ordinary graphics/copy lists have no AS metadata to reset. Creating
        // entries for them could exhaust the small scene-only journal during play.
        if(!id)return;
        retirement.Poll();recordedCopies.erase(id->id);id->generation=catalog.Begin(id->id);
    }
    bool Mark(IUnknown* list,U64 ticket){
        std::lock_guard<std::mutex> guard(mutex);auto* id=List(list);
        if(!id||!catalog.Mark(id->id,id->generation,ticket)){catalog.Invalidate();return false;}return true;
    }
    bool Submit(IUnknown* queue,UINT count,ID3D12CommandList*const* lists){
        std::lock_guard<std::mutex> guard(mutex);auto* q=Identify(queue);
        if(!q||!count||count>256||!lists){catalog.Invalidate();return false;}
        RefreshWaitFences();
        std::vector<std::pair<U64,U64>> ordered;ordered.reserve(count);
        for(UINT n=0;n<count;++n){
            if(!lists[n]){catalog.Invalidate("null_submission_list");return false;}
            auto* l=Existing(lists[n]);if(!l||!l->generation)continue;
            ordered.push_back({l->id,l->generation});
        }
        std::vector<std::shared_ptr<PTGeometrySnapshot::Transfer>> transfers;
        for(const auto& item:ordered){auto found=recordedCopies.find(item.first);if(found!=recordedCopies.end())transfers.insert(transfers.end(),found->second.begin(),found->second.end());}
        if(!transfers.empty()){
            ComPtr<ID3D12CommandQueue> native;if(FAILED(queue->QueryInterface(IID_PPV_ARGS(&native)))){catalog.Invalidate();return false;}
            if(!retirement.Before(native.Get(),transfers,submittingCopies[q->id])){
                // Native submission still forwards. No fence can prove these
                // references safe to release, including after list Reset.
                (void)new std::vector<std::shared_ptr<PTGeometrySnapshot::Transfer>>(std::move(transfers));
                catalog.Invalidate();return false;
            }
        }
        return catalog.Submit(q->id,ordered);
    }
    void Submitted(ID3D12CommandQueue* queue){
        std::lock_guard<std::mutex> guard(mutex);auto* q=Identify(queue);if(!q){catalog.Invalidate();return;}
        auto batch=submittingCopies.find(q->id);if(batch==submittingCopies.end())return;
        if(!retirement.After(queue,batch->second))catalog.Invalidate();submittingCopies.erase(batch);
    }
    // Called after each forwarded queue operation, under the interception
    // queue mutex. The nested private Signal is excluded from native ordering.
    void Tail(ID3D12CommandQueue* queue){
        std::lock_guard<std::mutex> guard(mutex);if(!catalog.Healthy()){retirement.Poll();return;}auto* id=Identify(queue);if(!id){catalog.Invalidate();return;}
        auto& p=progress[id->id];
        if(!p.fence){ComPtr<ID3D12Device> device;
            if(FAILED(queue->GetDevice(IID_PPV_ARGS(&device)))||FAILED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&p.fence)))){catalog.Invalidate();return;}
        }
        if(p.value>=UINT64_MAX-1||FAILED(queue->Signal(p.fence.Get(),++p.value))){catalog.Invalidate();return;}
        p.stamps.emplace(p.value,catalog.TailStamp(id->id));
        ++checkpointOperations;retirement.Poll();
    }
    void Checkpoint(){
        std::lock_guard<std::mutex> guard(mutex);
        if(!catalog.Healthy()||checkpointOperations-lastCheckpointAttempt<256)return;
        lastCheckpointAttempt=checkpointOperations;Measure timing{stats.checkpoints};
        std::map<U64,U64> completed;
        for(auto& item:progress){auto& p=item.second;const auto done=p.fence->GetCompletedValue();
            if(done==UINT64_MAX){catalog.Invalidate();return;}
            auto end=p.stamps.upper_bound(done);
            if(end!=p.stamps.begin())p.completedStamp=std::max(p.completedStamp,std::prev(end)->second);
            p.stamps.erase(p.stamps.begin(),end);completed.emplace(item.first,p.completedStamp);
        }
        // Metadata can retire while newer GPU work remains in flight. Geometry
        // storage still has independent recording/submission/dispatch leases.
        const auto live=resources.LiveTokens();catalog.RetireHistory(completed,&live);
    }
    bool Signal(IUnknown* queue,IUnknown* fence,U64 value){
        std::lock_guard<std::mutex> guard(mutex);auto* q=Identify(queue);auto* f=Identify(fence);
        if(!q||!f){catalog.Invalidate();return false;}return catalog.Signal(q->id,f->id,value);
    }
    bool Wait(IUnknown* queue,IUnknown* fence,U64 value){
        std::lock_guard<std::mutex> guard(mutex);auto* q=Identify(queue);auto* f=Identify(fence);
        if(!q||!f){catalog.Invalidate();return false;}
        auto& tracked=waitFences[f->id];
        if(!tracked.fence&&FAILED(fence->QueryInterface(IID_PPV_ARGS(&tracked.fence)))){catalog.Invalidate("wait_fence_identity");return false;}
        if(!catalog.Wait(q->id,f->id,value))return false;
        const auto done=tracked.fence->GetCompletedValue();
        if(done==UINT64_MAX){catalog.Invalidate("wait_fence_device_removed");return false;}
        tracked.completed=done;stats.completedWaits+=catalog.ObserveFenceCompletion(f->id,done);
        return true;
    }
    struct PendingFence {PTSceneOrder::Graph::PendingFence graph;U64 actual=0;bool polled=false;IUnknown* identity=nullptr;};
    PendingFence PendingWait(U64 ticket){
        std::lock_guard<std::mutex> guard(mutex);RefreshWaitFences();PendingFence out;out.graph=catalog.PendingWait(ticket);
        const auto f=waitFences.find(out.graph.fence);
        if(f!=waitFences.end()){out.actual=f->second.fence->GetCompletedValue();out.polled=true;const auto identity=identities.find(f->second.fence.Get());
            out.identity=identity==identities.end()?f->second.fence.Get():identity->second.owner.Get();}
        return out;
    }
    PTSceneOrder::Status Scene(U64 ticket,const PTSceneDescriptors::Acceleration& selected,PTSceneDispatch::Scene& scene,const char** pendingReason=nullptr,unsigned rayMask=0xff){
        std::lock_guard<std::mutex> guard(mutex);RefreshWaitFences();
        return catalog.Scene(ticket,{{selected.token,selected.address},selected.bytes},scene,pendingReason,PTSceneOrder::Purpose::submission,rayMask);
    }
    // Recording only: the output remains disarmed until Scene succeeds at
    // actual queue submission and its exact versions match the prepared plan.
    PTSceneOrder::Status PreviewScene(U64 ticket,const PTSceneDescriptors::Acceleration& selected,PTSceneDispatch::Scene& scene,const char** pendingReason=nullptr,unsigned rayMask=0xff){
        return catalog.Scene(ticket,{{selected.token,selected.address},selected.bytes},scene,pendingReason,PTSceneOrder::Purpose::recordingPreview,rayMask);
    }
    void Forget(U64 ticket){catalog.Forget(ticket);}
    void Invalidate(){catalog.Invalidate("capture_external");}
};
}
