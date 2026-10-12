#pragma once
#include "pt_scene_dispatch_plan.h"
#include "pt_scene_order.h"
#include <memory>
#include <mutex>
// Immutable recording snapshots joined to queue execution order. Allocation
// identities are captured by the host while observing the native build. This
// catalog never interprets CPU recording order as GPU execution order.
namespace PTSceneCatalog {
using U64=std::uint64_t;
using Key=PTSceneOrder::Key;
using Status=PTSceneOrder::Status;
struct Allocation {Key key{};U64 bytes=0;};
struct Snapshot {
    enum class Kind {unknown,blas,tlas,copy};
    Kind kind=Kind::unknown;
    Allocation destination{},source{};
    std::vector<PTSceneTopology::Geometry> geometry;
    std::vector<PTSceneTopology::Instance> instances;
    // Exact allocation identities for the addresses embedded in instance rows.
    std::map<U64,Allocation> bottom;
};
class Catalog {
    struct Operation {std::shared_ptr<const Snapshot> snapshot;U64 ticket=0;};
    struct Recording {U64 generation=0;bool submitted=false;std::vector<Operation> operations;};
    struct Version {std::shared_ptr<const Snapshot> snapshot;U64 before=0;};
    mutable std::mutex mutex;
    PTSceneOrder::Graph graph;
    std::map<U64,Recording> recordings;
    std::map<U64,Version> versions;
    std::map<U64,U64> cuts;
    U64 serial=0;
    std::size_t recordedOperations=0,rows=0;
    bool valid=true;
    static constexpr std::size_t MaximumLists=256,MaximumOperations=65536,MaximumRows=1048576;
    static bool Equal(Key a,Key b){return a.token==b.token&&a.address==b.address;}
    static bool Known(const Allocation& a){return a.key.token&&a.key.address&&a.bytes;}
    static std::size_t Rows(const Snapshot& s){return s.geometry.size()+s.instances.size()+s.bottom.size();}
    Status Resolve(U64 cut,Key key,const Snapshot*& out,std::set<U64>& visiting)const {
        out=nullptr;PTSceneOrder::Selection selected;
        const auto status=graph.Select(cut,key,selected);if(status!=Status::ready)return status;
        const auto version=versions.find(selected.version);
        if(version==versions.end()||visiting.size()>=256||!visiting.insert(selected.version).second)return Status::invalid;
        const auto& v=version->second;const auto& s=*v.snapshot;
        if(!Equal(s.destination.key,key))return Status::invalid;
        if(s.kind==Snapshot::Kind::copy){
            const auto result=Resolve(v.before,s.source.key,out,visiting);
            visiting.erase(selected.version);return result;
        }
        visiting.erase(selected.version);out=&s;return Status::ready;
    }
public:
    // A successful native Reset starts a new recording generation. First use of
    // a newly created list must also call Begin. IDs belong to object lifetimes.
    U64 Begin(U64 list){
        std::lock_guard<std::mutex> guard(mutex);
        if(!valid||!list||(!recordings.count(list)&&recordings.size()>=MaximumLists)){valid=false;return 0;}
        auto& r=recordings[list];
        recordedOperations-=r.operations.size();
        for(const auto& op:r.operations){
            if(op.snapshot&&!r.submitted)rows-=Rows(*op.snapshot);
            if(op.ticket&&!r.submitted)cuts.erase(op.ticket);
        }
        r={++serial,false,{}};return r.generation;
    }
    bool Record(U64 list,U64 generation,Snapshot snapshot){
        std::lock_guard<std::mutex> guard(mutex);auto r=recordings.find(list);
        if(!valid||r==recordings.end()||r->second.generation!=generation||r->second.submitted||!Known(snapshot.destination))return false;
        // An unsupported build is a tombstone, not permission to reuse metadata
        // from an older valid build at this destination.
        bool admitted=false;
        switch(snapshot.kind){
        case Snapshot::Kind::blas:
            admitted=!snapshot.geometry.empty()&&snapshot.geometry.size()<=4096;
            for(const auto& g:snapshot.geometry)admitted=admitted&&g.indices&&g.positions&&g.indexToken&&g.positionToken&&g.indexCount&&g.vertexCount&&g.stride>=12&&(g.indexStride==2||g.indexStride==4);
            break;
        case Snapshot::Kind::tlas:
            admitted=!snapshot.instances.empty()&&snapshot.instances.size()<=32768;
            for(const auto& i:snapshot.instances){
                if(!(i.idMask>>24))continue;
                const auto b=snapshot.bottom.find(i.blas);
                admitted=admitted&&b!=snapshot.bottom.end()&&Known(b->second)&&b->second.key.address==i.blas;
            }
            break;
        case Snapshot::Kind::copy:admitted=Known(snapshot.source);break;
        default:break;
        }
        if(!admitted){snapshot.kind=Snapshot::Kind::unknown;snapshot.geometry.clear();snapshot.instances.clear();snapshot.bottom.clear();}
        const auto count=Rows(snapshot);
        if(recordedOperations>=MaximumOperations||count>MaximumRows-rows){valid=false;return false;}
        rows+=count;++recordedOperations;
        r->second.operations.push_back({std::make_shared<const Snapshot>(std::move(snapshot)),0});return true;
    }
    bool Mark(U64 list,U64 generation,U64 ticket){
        std::lock_guard<std::mutex> guard(mutex);auto r=recordings.find(list);
        if(!valid||!ticket||r==recordings.end()||r->second.generation!=generation||r->second.submitted||recordedOperations>=MaximumOperations)return false;
        // Tickets are unique for the session, including unsubmitted recordings.
        if(cuts.count(ticket)||cuts.size()>=MaximumOperations)return false;
        cuts.emplace(ticket,0);++recordedOperations;r->second.operations.push_back({{},ticket});return true;
    }
    bool Submit(U64 queue,const std::vector<std::pair<U64,U64>>& lists){
        std::lock_guard<std::mutex> guard(mutex);if(!valid||!queue)return false;
        std::set<U64> unique;
        // Validate the entire submission before exposing any of its operations.
        for(const auto& item:lists){const auto r=recordings.find(item.first);
            if(r==recordings.end()||r->second.generation!=item.second||r->second.submitted||!unique.insert(item.first).second){valid=false;return false;}
        }
        for(const auto& item:lists){auto& r=recordings[item.first];r.submitted=true;
            for(const auto& op:r.operations){
                if(op.ticket){const auto cut=graph.Mark(queue);if(!cut){valid=false;return false;}auto selected=cuts.find(op.ticket);if(selected!=cuts.end())selected->second=cut;continue;}
                const auto before=graph.Mark(queue);if(!before){valid=false;return false;}
                const U64 version=op.snapshot->kind==Snapshot::Kind::unknown?0:++serial;
                if(version)versions.emplace(version,Version{op.snapshot,before});
                if(!graph.Write(queue,op.snapshot->destination.key,version)){valid=false;return false;}
                if(!version)rows-=Rows(*op.snapshot);
            }
            recordedOperations-=r.operations.size();r.operations.clear();
        }
        return true;
    }
    bool Signal(U64 queue,U64 fence,U64 value){std::lock_guard<std::mutex> guard(mutex);if(!valid)return false;return valid=graph.Signal(queue,fence,value)!=0;}
    bool Wait(U64 queue,U64 fence,U64 value){std::lock_guard<std::mutex> guard(mutex);if(!valid)return false;return valid=graph.Wait(queue,fence,value)!=0;}
    // Host supplies live allocation identities after proving all queue tails
    // complete. Old compaction sources are resolved BEFORE pruning versions.
    template<class Alive> bool CompletedCheckpoint(Alive alive){
        std::lock_guard<std::mutex> guard(mutex);if(!valid)return false;
        for(const auto& c:cuts)if(c.second)return false;
        std::map<Key,PTSceneOrder::Selection> latest;if(!graph.CompletedCheckpoint(latest))return false;
        std::map<U64,Version> retained;std::map<Key,U64> baseline;std::size_t retainedRows=0;
        for(const auto& item:latest){
            if(!alive(item.first.token))continue;
            if(!item.second.version){baseline.emplace(item.first,0);continue;}
            const auto current=versions.find(item.second.version);if(current==versions.end())return false;
            const Snapshot* resolved=current->second.snapshot.get();
            if(resolved->kind==Snapshot::Kind::copy){std::set<U64> visiting;
                if(Resolve(current->second.before,resolved->source.key,resolved,visiting)!=Status::ready)return false;
            }
            auto materialized=std::make_shared<Snapshot>(*resolved);
            materialized->destination=current->second.snapshot->destination;
            materialized->source={};
            retainedRows+=Rows(*materialized);
            retained.emplace(item.second.version,Version{std::move(materialized),0});baseline.emplace(item.first,item.second.version);
        }
        for(const auto& r:recordings)if(!r.second.submitted)for(const auto& op:r.second.operations)if(op.snapshot)retainedRows+=Rows(*op.snapshot);
        if(retainedRows>MaximumRows||!graph.RebaseCompleted(baseline))return false;
        versions=std::move(retained);rows=retainedRows;return true;
    }
    void Forget(U64 ticket){std::lock_guard<std::mutex> guard(mutex);cuts.erase(ticket);}
    void Invalidate(){std::lock_guard<std::mutex> guard(mutex);valid=false;}
    Status Scene(U64 ticket,Allocation selected,PTSceneDispatch::Scene& output)const {
        output={};std::lock_guard<std::mutex> guard(mutex);if(!valid)return Status::invalid;
        const auto c=cuts.find(ticket);if(c==cuts.end())return Status::missing;if(!c->second)return Status::pending;
        if(!Known(selected))return Status::missing;
        const Snapshot* top=nullptr;std::set<U64> visiting;
        auto status=Resolve(c->second,selected.key,top,visiting);if(status!=Status::ready)return status;
        if(top->kind!=Snapshot::Kind::tlas)return Status::missing;
        PTSceneOrder::Selection version;status=graph.Select(c->second,selected.key,version);if(status!=Status::ready)return status;
        PTSceneDispatch::Scene scene;scene.address=selected.key.address;scene.version=version.version;
        scene.tlas={selected.key.token,selected.bytes};scene.instances=top->instances;
        for(const auto& instance:top->instances){
            if(!(instance.idMask>>24)||scene.blas.count(instance.blas))continue;
            const auto a=top->bottom.find(instance.blas);if(a==top->bottom.end())return Status::missing;
            const Snapshot* bottom=nullptr;status=Resolve(c->second,a->second.key,bottom,visiting);if(status!=Status::ready)return status;
            if(bottom->kind!=Snapshot::Kind::blas||!scene.topology.Build(instance.blas,bottom->geometry))return Status::missing;
            scene.blas.emplace(instance.blas,PTSceneDispatch::Scene::Acceleration{a->second.key.token,a->second.bytes});
        }
        output=std::move(scene);return Status::ready;
    }
};
}
