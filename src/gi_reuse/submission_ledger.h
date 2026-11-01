#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace gi_reuse {
// Metadata only: caller must retain resources and capture buffer bytes separately.
// Feed in ExecuteCommandLists order, never CPU recording order. One queue only.
struct ASRecord {
    std::uint64_t address=0, revision=0, instances=0;
    std::uint32_t count=0;
    bool topLevel=false;
};
enum class ASOperation { Build, Copy, Invalidate };
struct ASEvent { ASOperation operation{}; ASRecord record{}; std::uint64_t source=0; };
template<std::size_t Capacity> struct ASJournal {
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
template<std::size_t Capacity> class ASLedger {
    std::array<ASRecord,Capacity> records_{};
    std::uint64_t queue_=0, revision_=0, fence_=0;
    bool complete_=true;
    ASRecord* Slot(std::uint64_t address) noexcept {
        ASRecord* empty=nullptr;
        for(auto& r:records_){if(r.address==address)return &r;if(!r.address&&!empty)empty=&r;}
        return empty;
    }
public:
    bool Complete() const noexcept {return complete_;}
    // Any unobserved mutation/order invalidates the entire claim. No silent eviction.
    void Invalidate() noexcept {complete_=false;}
    const ASRecord* Find(std::uint64_t address) const noexcept {
        if(!complete_||!address)return nullptr;
        for(const auto& r:records_)if(r.address==address)return &r;
        return nullptr;
    }
    template<std::size_t N> bool Submit(std::uint64_t queue,std::uint64_t fence,
                                      const ASJournal<N>& journal,std::uint64_t expectedGeneration) noexcept {
        if(!complete_||!queue||!fence||!journal.complete||journal.generation!=expectedGeneration||
           (queue_&&queue_!=queue)||fence<=fence_){Invalidate();return false;}
        // Caller supplies a fresh increasing completion fence for each submission.
        queue_=queue;fence_=fence;
        for(std::size_t i=0;i<journal.size;++i){
            const auto& e=journal.events[i];const auto address=e.record.address;
            if(!address){Invalidate();return false;}
            if(e.operation==ASOperation::Invalidate){
                for(auto& r:records_)if(r.address==address)r={};
                continue;
            }
            ASRecord next=e.record;
            if(e.operation==ASOperation::Copy){
                const auto* source=Find(e.source);
                if(!source){Invalidate();return false;}
                next=*source;next.address=address;
            }else if(e.operation!=ASOperation::Build){Invalidate();return false;}
            auto* slot=Slot(address);
            if(!slot){Invalidate();return false;}
            next.revision=++revision_;*slot=next;
        }
        return true;
    }
    // Copies metadata only after the caller-observed fence reaches this submission.
    // UINT64_MAX is D3D12's device-removed sentinel, never successful completion.
    bool Snapshot(std::uint64_t queue,std::uint64_t completedFence,ASLedger& output) const noexcept {
        if(!complete_||!queue_||queue!=queue_||completedFence==UINT64_MAX||completedFence<fence_)return false;
        output=*this;return true;
    }
};
} // namespace gi_reuse
