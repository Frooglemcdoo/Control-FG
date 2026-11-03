#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <map>
#include <algorithm>
#include <atomic>
#include "geometry_metadata.h"

namespace gi_reuse {
// Metadata only: caller must retain resources and capture buffer bytes separately.
// Feed in ExecuteCommandLists order, never CPU recording order. One queue only.
struct ASRecord {
    std::uint64_t address=0, revision=0, instances=0;
    std::uint32_t count=0;
    bool topLevel=false;
    std::shared_ptr<const GeometryMetadata> geometry{};
    bool allowUpdate=false;
};
enum class ASOperation { Build, Copy, Invalidate, Update };
// Descriptor metadata at one event in submission order, not a resource/byte lease.
struct ASMetadataCut {
    std::vector<ASRecord> records;
    std::uint64_t queue=0,fence=0,generation=0,event=0,destination=0;
    const ASRecord* Find(std::uint64_t address) const noexcept {
        const auto it=std::lower_bound(records.begin(),records.end(),address,
            [](const ASRecord& r,std::uint64_t a){return r.address<a;});
        return it!=records.end()&&it->address==address?&*it:nullptr;
    }
};
struct ASCutReceipt {
    std::shared_ptr<const ASMetadataCut> data;
    void Store(std::shared_ptr<const ASMetadataCut> value) noexcept {std::atomic_store(&data,std::move(value));}
    std::shared_ptr<const ASMetadataCut> Load() const noexcept {return std::atomic_load(&data);}
};
struct ASEvent { ASOperation operation{}; ASRecord record{}; std::uint64_t source=0; std::shared_ptr<ASCutReceipt> cut{}; };
template<std::size_t Capacity> struct ASJournal {
    static constexpr std::size_t Limit=Capacity;
    std::array<ASEvent,Capacity> events{};
    std::size_t size=0;
    std::uint64_t generation=0;
    bool complete=true;
    void Reset() noexcept {++generation;size=0;complete=true;}
    bool Append(ASEvent event) noexcept {
        if(size==Capacity){complete=false;return false;}
        events[size++]=event;return true;
    }
};
// Grow on the heap, never create a large temporary on the render-thread stack.
// A hard limit still fails closed; events and referenced geometry are not dropped.
template<std::size_t Capacity> struct GrowingASJournal {
    static constexpr std::size_t Limit=Capacity;
    std::vector<ASEvent> events;
    std::size_t size=0;
    std::uint64_t generation=0;
    bool complete=true;
    void Reset() noexcept {++generation;events.clear();size=0;complete=true;}
    bool Append(ASEvent event) {
        if(size==Capacity){complete=false;return false;}
        events.push_back(std::move(event));size=events.size();return true;
    }
};
template<std::size_t Capacity> class ASLedger {
    // Address-indexed heap storage: bounded history without a render-stack array
    // or an O(history) scan for every submitted build. No age-based eviction.
    std::map<std::uint64_t,ASRecord> records_;
    std::uint64_t queue_=0, revision_=0, fence_=0;
    bool complete_=true;const char* failure_=nullptr;
public:
    static constexpr std::size_t Limit=Capacity;
    std::size_t Size() const noexcept {return records_.size();}
    std::uint64_t SubmissionFence() const noexcept {return fence_;}
    bool Complete() const noexcept {return complete_;}
    // Any unobserved mutation/order invalidates the entire claim. No silent eviction.
    void Invalidate(const char* reason="ledger_invalidated") noexcept {complete_=false;if(!failure_)failure_=reason;}
    const char* FailureReason() const noexcept {return failure_?failure_:"none";}
    const ASRecord* Find(std::uint64_t address) const noexcept {
        if(!complete_||!address)return nullptr;
        const auto found=records_.find(address);
        return found==records_.end()?nullptr:&found->second;
    }
    // Includes retained historical AS records, not a claim of active TLAS membership.
    template<class Visitor> bool Visit(Visitor&& visitor) const {
        if(!complete_)return false;
        for(const auto& entry:records_)if(!visitor(entry.second))return false;
        return true;
    }
    // Plans only the format-sized three-index read. Vertex IDs must be decoded from
    // same-generation bytes and bounded against vertexCount before GPU loads.
    bool PlanTriangle(std::uint64_t address,std::uint64_t expectedRevision,
                      std::uint32_t geometryIndex,std::uint32_t primitive,
                      TriangleReadPlan& output) const noexcept {
        output={};
        const auto* r=Find(address);
        if(!r||!expectedRevision||r->revision!=expectedRevision||r->topLevel||!r->geometry)return false;
        const auto* t=r->geometry->At(geometryIndex);
        if(!t||primitive>=t->indexCount/3)return false;
        output={*t,r->revision,t->indices+std::uint64_t(primitive)*3*GeometryMetadata::IndexSize(*t),primitive};
        return true;
    }
    template<class Journal> bool Submit(std::uint64_t queue,std::uint64_t fence,
                                      const Journal& journal,std::uint64_t expectedGeneration) noexcept {
        // Replays replace the receipt. Never leave an earlier successful cut
        // visible after a failed replay. Consumers separately prove GPU completion.
        for(std::size_t i=0;i<journal.size&&i<Journal::Limit&&i<journal.events.size();++i)if(journal.events[i].cut)journal.events[i].cut->Store({});
        std::vector<std::pair<std::shared_ptr<ASCutReceipt>,std::shared_ptr<const ASMetadataCut>>> cuts;
        if(!complete_||!queue||!fence||!journal.complete||journal.size>Journal::Limit||journal.size>journal.events.size()||journal.generation!=expectedGeneration||
           (queue_&&queue_!=queue)||fence<=fence_){Invalidate("ledger_submission_contract");return false;}
        // Caller supplies a fresh increasing completion fence for each submission.
        queue_=queue;fence_=fence;
        for(std::size_t i=0;i<journal.size;++i){
            const auto& e=journal.events[i];const auto address=e.record.address;
            if(!address){Invalidate("ledger_zero_address");return false;}
            if(e.operation==ASOperation::Invalidate){
                records_.erase(address);
                continue;
            }
            ASRecord next=e.record;
            if(e.operation==ASOperation::Copy){
                const auto* source=Find(e.source);
                if(!source){Invalidate("ledger_copy_source_missing");return false;}
                next=*source;next.address=address;
            }else if(e.operation==ASOperation::Update){
                const auto* source=Find(e.source);
                if(!source||!source->allowUpdate||next.topLevel!=source->topLevel||next.count!=source->count||
                   (!next.topLevel&&(!next.geometry||!source->geometry||!next.geometry->CanUpdateFrom(*source->geometry)))){
                    Invalidate("ledger_update_contract");return false;
                }
                next.allowUpdate=true;
            }else if(e.operation!=ASOperation::Build){Invalidate("ledger_unknown_operation");return false;}
            if(next.geometry&&(next.topLevel||next.count!=next.geometry->Size())){
                Invalidate("ledger_geometry_count");return false;
            }
            const auto found=records_.find(address);
            if(found==records_.end()&&records_.size()==Capacity){Invalidate("ledger_capacity");return false;}
            next.revision=++revision_;
            try {if(found!=records_.end())found->second=next;else records_.emplace(address,std::move(next));}
            catch(...){Invalidate("ledger_allocation");return false;}
            if(e.cut)try {
                auto cut=std::make_shared<ASMetadataCut>();
                cut->queue=queue;cut->fence=fence;cut->generation=expectedGeneration;
                cut->event=i;cut->destination=address;cut->records.reserve(records_.size());
                for(const auto& entry:records_)cut->records.push_back(entry.second);
                cuts.emplace_back(e.cut,std::move(cut));
            }catch(...){Invalidate("ledger_cut_allocation");return false;}
        }
        for(auto& cut:cuts)cut.first->Store(std::move(cut.second));
        return true;
    }
    // Copies metadata only after the caller-observed fence reaches this submission.
    // UINT64_MAX is D3D12's device-removed sentinel, never successful completion.
    bool Snapshot(std::uint64_t queue,std::uint64_t completedFence,ASLedger& output) const noexcept {
        if(!complete_||!queue_||queue!=queue_||completedFence==UINT64_MAX||completedFence<fence_)return false;
        // Build the copy before touching output. Allocation failure cannot publish
        // a partial snapshot or corrupt an earlier immutable snapshot.
        try {ASLedger copy=*this;output=std::move(copy);return true;}
        catch(...){return false;}
    }
};
} // namespace gi_reuse
