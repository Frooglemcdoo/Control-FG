#pragma once
#include "pt_scene_live_owner.h"
// Joins the native writer's CPU snapshot with the exact submitted scene cut.
// A prepared result remains CPU preflight: the renderer still owns submission
// admission, descriptor/state proof, output resources and fence retirement.
namespace PTSceneHandoff {
using Ticket=PTNativeJobRoute::Ticket;
enum class Status {pending,prepared,rejected,missing};
class Prepared {
    friend class Runtime;
    PTSceneLive::Owner* owner=nullptr;
    std::uint64_t engineFrame=0;
    UINT width=0,height=0;
    PTSceneDescriptors::Acceleration acceleration;
    Microsoft::WRL::ComPtr<ID3D12StateObject> nativePipeline;
    PTSceneDispatch::Table table;
    PTSceneDispatch::ExportSet exports;
    PTSceneDispatch::Plan plan;
};
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
    struct Failure {const char* reason;std::size_t row;};
    std::map<std::uint64_t,Failure> failures;
    auto Erase(std::map<std::uint64_t,Entry>::iterator it,const char* reason="handoff_retired",std::size_t row=0){
        failures[it->first]={reason,row};while(failures.size()>64)failures.erase(failures.begin());
        owner.Scenes().Forget(it->first);return pending.erase(it);
    }
    void Clear(){for(const auto& entry:pending)owner.Scenes().Forget(entry.first);pending.clear();}
    static bool Same(const Ticket& a,const Ticket& b){
        return a.serial==b.serial&&a.engineFrame==b.engineFrame&&a.list==b.list&&a.pipeline==b.pipeline&&a.counter==b.counter&&a.hitTable==b.hitTable&&
            a.hitBytes==b.hitBytes&&a.width==b.width&&a.height==b.height&&a.writer==b.writer&&
            a.layout.buffer==b.layout.buffer&&a.layout.bytes==b.layout.bytes&&a.layout.raygenBytes==b.layout.raygenBytes&&
            a.layout.missOffset==b.layout.missOffset&&a.layout.missStride==b.layout.missStride&&a.layout.hitRelative==b.layout.hitRelative&&
            a.layout.hitStride==b.layout.hitStride&&a.layout.hitRecords==b.layout.hitRecords;
    }
public:
    explicit Runtime(PTSceneLive::Owner& session):owner(session){}
    bool Recorded(const Ticket& ticket,ID3D12DescriptorHeap* heap,UINT tableFirst,UINT srvSlot){
        std::lock_guard<std::mutex> guard(mutex);
        if(!owner.Active()||!ticket.serial||!ticket.engineFrame||!ticket.list||!ticket.pipeline||!ticket.width||!ticket.height||
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
        if(!count||count>256||!lists){Clear();return;}
        for(UINT n=0;n<count;++n){
            Microsoft::WRL::ComPtr<IUnknown> identity;
            if(!lists[n]||FAILED(lists[n]->QueryInterface(IID_PPV_ARGS(&identity)))){Clear();return;}
            for(auto it=pending.begin();it!=pending.end();){
                auto& e=it->second;
                if(e.list.Get()!=identity.Get()){++it;continue;}
                if(e.submitted){it=Erase(it,"handoff_replayed");continue;}
                if(!owner.Select(e.heap.Get(),e.tableFirst,e.srvSlot,e.acceleration)){it=Erase(it,"handoff_scene_descriptor");continue;}
                e.submitted=true;++it;
            }
        }
    }
    bool WriterReady(const Ticket& ticket){
        std::lock_guard<std::mutex> guard(mutex);const auto found=pending.find(ticket.serial);
        if(found==pending.end())return false;
        auto& entry=found->second;
        if(!Same(entry.ticket,ticket)||entry.written){Erase(found,"handoff_writer_identity");return false;}
        const char* failure="writer_unknown";std::size_t row=0;
        if(!owner.Resources().SnapshotWriter(ticket,entry.identifiers,entry.table,&failure,&row)){
            Erase(found,failure,row);return false;
        }
        entry.written=true;return true;
    }
    // Non-consuming inspection for private-pipeline export mapping. Preparation
    // repeats all identity checks; cancellation cannot revive a stale snapshot.
    bool ReadyIdentifiers(std::uint64_t serial,PTNativeShaderTable::Identifiers& output){
        output={};std::lock_guard<std::mutex> guard(mutex);const auto found=pending.find(serial);
        if(found==pending.end()||!found->second.written||!found->second.submitted)return false;
        output=found->second.identifiers;return true;
    }
    // Discovery only: the pass adapter must also match the native view/root
    // bindings. Multiple candidates are rejected, never resolved by newest ID.
    bool FindReady(std::uint64_t engineFrame,UINT width,UINT height,std::uint64_t nativePipeline,std::uint64_t& serial){
        serial=0;if(!engineFrame||!width||!height||!nativePipeline)return false;
        std::lock_guard<std::mutex> guard(mutex);const Entry* candidate=nullptr;
        for(const auto& item:pending){const auto& entry=item.second;const auto& ticket=entry.ticket;
            if(ticket.engineFrame!=engineFrame||ticket.width!=width||ticket.height!=height||ticket.pipeline!=nativePipeline)continue;
            if(candidate)return false;candidate=&entry;
        }
        if(!candidate||!candidate->written||!candidate->submitted)return false;
        serial=candidate->ticket.serial;return true;
    }
    const char* NotReady(std::uint64_t serial,std::size_t& row){
        row=0;std::lock_guard<std::mutex> guard(mutex);
        const auto lost=failures.find(serial);if(lost!=failures.end()){row=lost->second.row;return lost->second.reason;}
        const auto entry=pending.find(serial);if(entry==pending.end())return "handoff_missing";
        if(!entry->second.written&&!entry->second.submitted)return "handoff_writer_and_submission_pending";
        if(!entry->second.written)return "handoff_writer_pending";
        if(!entry->second.submitted)return "handoff_submission_pending";
        return "handoff_ambiguous_or_mismatch";
    }
    void Cancelled(std::uint64_t serial){std::lock_guard<std::mutex> guard(mutex);const auto found=pending.find(serial);if(found!=pending.end())Erase(found);}
    void Reset(IUnknown* list){
        Microsoft::WRL::ComPtr<IUnknown> identity;
        std::lock_guard<std::mutex> guard(mutex);
        if(!list||FAILED(list->QueryInterface(IID_PPV_ARGS(&identity)))){Clear();return;}
        for(auto it=pending.begin();it!=pending.end();){
            // A submitted writer snapshot belongs to the old immutable cut and
            // can still be consumed after the command list starts its next use.
            if(it->second.list.Get()==identity.Get()&&!it->second.submitted)it=Erase(it,"handoff_reset_before_submission");else ++it;
        }
    }
    // Optional immutable proof permits checking the same writer snapshot at a
    // later lighting submission. The original ray ticket can then be retired.
    Status Prepare(std::uint64_t serial,const PTSceneDispatch::ExportSet& exports,PTSceneDispatch::Plan& plan,PTOwnedScene::LeaseBatch& lease,
                   std::shared_ptr<const Prepared>* proof=nullptr){
        if(proof)proof->reset();
        plan={};std::lock_guard<std::mutex> guard(mutex);const auto found=pending.find(serial);
        if(found==pending.end())return Status::missing;
        auto& entry=found->second;if(!entry.written||!entry.submitted)return Status::pending;
        if(lease.Count())return Status::rejected;
        if(exports.nativePipeline!=entry.ticket.pipeline||exports.rays.size()!=2){Erase(found);return Status::rejected;}
        for(unsigned ray=0;ray<2;++ray)if(exports.rays[ray].nativeOpaque!=entry.identifiers.hit[ray]||exports.rays[ray].nativeAlpha!=entry.identifiers.hit[2+ray]){
            Erase(found);return Status::rejected;
        }
        PTSceneDispatch::Scene scene;const auto selected=owner.Scenes().Scene(serial,entry.acceleration,scene);
        if(selected==PTSceneOrder::Status::pending)return Status::pending;
        if(selected!=PTSceneOrder::Status::ready){Erase(found);return Status::rejected;}
        const PTSceneDispatch::Selection selection{scene.address,scene.version,entry.table.address,entry.table.version,entry.table.pipeline};
        const bool prepared=owner.Resources().PrepareLease(selection,scene,entry.table,exports,plan,lease);
        if(prepared&&proof){
            auto snapshot=std::make_shared<Prepared>();snapshot->owner=&owner;snapshot->acceleration=entry.acceleration;
            snapshot->engineFrame=entry.ticket.engineFrame;snapshot->width=entry.ticket.width;snapshot->height=entry.ticket.height;
            snapshot->nativePipeline=entry.pipeline;snapshot->table=std::move(entry.table);snapshot->exports=exports;snapshot->plan=plan;
            *proof=std::move(snapshot);
        }
        Erase(found);return prepared?Status::prepared:Status::rejected;
    }
    // The renderer marks a distinct ticket at the actual private DispatchRays
    // position and calls this AFTER catalog Submit, BEFORE native Execute.
    // Pending is disabled for this submission; this method never waits for a
    // native writer or queues a GPU wait. Caller must Forget the later ticket.
    Status Revalidate(std::uint64_t submissionTicket,std::uint64_t engineFrame,UINT width,UINT height,const std::shared_ptr<const Prepared>& proof){
        if(!submissionTicket||!proof||proof->owner!=&owner||!owner.Active()||!engineFrame||proof->engineFrame!=engineFrame||
           proof->width!=width||proof->height!=height)return Status::rejected;
        PTSceneDispatch::Scene scene;const auto selected=owner.Scenes().Scene(submissionTicket,proof->acceleration,scene);
        if(selected==PTSceneOrder::Status::pending)return Status::pending;
        if(selected!=PTSceneOrder::Status::ready)return Status::rejected;
        return owner.Resources().Revalidate(scene,proof->table,proof->exports,proof->plan)?Status::prepared:Status::rejected;
    }
};
}
