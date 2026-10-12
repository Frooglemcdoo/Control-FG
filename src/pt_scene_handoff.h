#pragma once
#include "pt_scene_live_owner.h"
// Joins the native writer's CPU snapshot with the exact submitted scene cut.
// A prepared result remains CPU preflight: the renderer still owns the private
// GPU gate, geometry content/state proof, output resources and fence retirement.
namespace PTSceneHandoff {
using Ticket=PTNativeJobRoute::Ticket;
enum class Status {pending,prepared,rejected,missing};
class Runtime {
    struct Entry {
        Ticket ticket;
        PTSceneDescriptors::Acceleration acceleration;
        Microsoft::WRL::ComPtr<ID3D12StateObject> pipeline;
        Microsoft::WRL::ComPtr<IUnknown> list;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
        UINT tableFirst=0,srvSlot=0;
        PTNativeShaderTable::Identifiers identifiers;
        PTSceneDispatch::Table table;
        bool written=false,submitted=false;
    };
    PTSceneLive::Owner& owner;
    std::mutex mutex;
    std::map<std::uint64_t,Entry> pending;
    static bool Same(const Ticket& a,const Ticket& b){
        return a.serial==b.serial&&a.list==b.list&&a.pipeline==b.pipeline&&a.counter==b.counter&&a.hitTable==b.hitTable&&
            a.hitBytes==b.hitBytes&&a.width==b.width&&a.height==b.height&&a.writer==b.writer&&
            a.layout.buffer==b.layout.buffer&&a.layout.bytes==b.layout.bytes&&a.layout.raygenBytes==b.layout.raygenBytes&&
            a.layout.missOffset==b.layout.missOffset&&a.layout.missStride==b.layout.missStride&&a.layout.hitRelative==b.layout.hitRelative&&
            a.layout.hitStride==b.layout.hitStride&&a.layout.hitRecords==b.layout.hitRecords;
    }
public:
    explicit Runtime(PTSceneLive::Owner& session):owner(session){}
    bool Recorded(const Ticket& ticket,ID3D12DescriptorHeap* heap,UINT tableFirst,UINT srvSlot){
        std::lock_guard<std::mutex> guard(mutex);
        if(!owner.Active()||!ticket.serial||!ticket.list||!ticket.pipeline||!ticket.width||!ticket.height||
           !heap||srvSlot>=36||pending.size()>=16||pending.count(ticket.serial))return false;
        const auto description=heap->GetDesc();
        if(description.Type!=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV||!(description.Flags&D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)||
           std::uint64_t(tableFirst)+49>description.NumDescriptors)return false;
        std::uint64_t address=0,bytes=0;
        if(ticket.writer.rays!=2||ticket.writer.count<=0||
           !PTNativeShaderTable::HitRange(ticket.layout,static_cast<std::size_t>(ticket.writer.count),address,bytes)||
           ticket.hitTable!=address||ticket.hitBytes!=bytes)return false;
        Entry entry;entry.ticket=ticket;entry.heap=heap;entry.tableFirst=tableFirst;entry.srvSlot=srvSlot;
        if(FAILED(reinterpret_cast<IUnknown*>(ticket.list)->QueryInterface(IID_PPV_ARGS(&entry.list))))return false;
        auto* pipeline=reinterpret_cast<ID3D12StateObject*>(ticket.pipeline);
        if(FAILED(pipeline->QueryInterface(IID_PPV_ARGS(&entry.pipeline))))return false;
        if(!owner.Scenes().Mark(reinterpret_cast<IUnknown*>(ticket.list),ticket.serial))return false;
        pending.emplace(ticket.serial,std::move(entry));return true;
    }
    // Called at the intercepted submission, before forwarding ExecuteCommandLists.
    // Descriptor writes AFTER recording are included. The retained heap identity
    // and exact shader slot choose the AS; unused stale slots are never scanned.
    void Submitting(UINT count,ID3D12CommandList*const* lists){
        std::lock_guard<std::mutex> guard(mutex);
        if(!count||count>256||!lists){pending.clear();return;}
        for(UINT n=0;n<count;++n){
            Microsoft::WRL::ComPtr<IUnknown> identity;
            if(!lists[n]||FAILED(lists[n]->QueryInterface(IID_PPV_ARGS(&identity)))){pending.clear();return;}
            for(auto it=pending.begin();it!=pending.end();){
                auto& e=it->second;
                if(e.list.Get()!=identity.Get()){++it;continue;}
                if(e.submitted||!owner.Select(e.heap.Get(),e.tableFirst,e.srvSlot,e.acceleration)){it=pending.erase(it);continue;}
                e.submitted=true;++it;
            }
        }
    }
    bool WriterReady(const Ticket& ticket){
        std::lock_guard<std::mutex> guard(mutex);const auto found=pending.find(ticket.serial);
        if(found==pending.end())return false;
        auto& entry=found->second;
        if(!Same(entry.ticket,ticket)||entry.written||!owner.Resources().SnapshotWriter(ticket,entry.identifiers,entry.table)){
            pending.erase(found);return false;
        }
        entry.written=true;return true;
    }
    void Cancelled(std::uint64_t serial){std::lock_guard<std::mutex> guard(mutex);pending.erase(serial);}
    Status Prepare(std::uint64_t serial,const PTSceneDispatch::ExportSet& exports,PTSceneDispatch::Plan& plan,PTOwnedScene::LeaseBatch& lease){
        plan={};std::lock_guard<std::mutex> guard(mutex);const auto found=pending.find(serial);
        if(found==pending.end())return Status::missing;
        auto& entry=found->second;if(!entry.written||!entry.submitted)return Status::pending;
        if(lease.Count())return Status::rejected;
        if(exports.nativePipeline!=entry.ticket.pipeline||exports.rays.size()!=2){pending.erase(found);return Status::rejected;}
        for(unsigned ray=0;ray<2;++ray)if(exports.rays[ray].nativeOpaque!=entry.identifiers.hit[ray]||exports.rays[ray].nativeAlpha!=entry.identifiers.hit[2+ray]){
            pending.erase(found);return Status::rejected;
        }
        PTSceneDispatch::Scene scene;const auto selected=owner.Scenes().Scene(serial,entry.acceleration,scene);
        if(selected==PTSceneOrder::Status::pending)return Status::pending;
        if(selected!=PTSceneOrder::Status::ready){pending.erase(found);return Status::rejected;}
        const PTSceneDispatch::Selection selection{scene.address,scene.version,entry.table.address,entry.table.version,entry.table.pipeline};
        const bool prepared=owner.Resources().PrepareLease(selection,scene,entry.table,exports,plan,lease);
        pending.erase(found);return prepared?Status::prepared:Status::rejected;
    }
};
}
