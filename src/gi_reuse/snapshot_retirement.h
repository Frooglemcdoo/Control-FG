#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <utility>

namespace gi_reuse {
// Portable ownership/publication policy. Not a D3D12 synchronization primitive.
// Host serializes calls at actual API boundaries, retains queue/fence identities,
// and supplies completion only from its private persistent queue fences.
// Destruction is legal only after device/queues have stopped; quarantine owners
// must otherwise remain alive. No allocations occur in submission/completion.
template<class Owner, unsigned Capacity=256, unsigned QueueCapacity=16,
         std::uint64_t ByteBudget=512ull*1024*1024>
class SnapshotRetirement {
    using U64=std::uint64_t;
    struct Queue {U64 id=0,issued=0,completed=0;bool failed=false;};
    struct Entry {
        U64 id=0,generation=0,bytes=0,ticket=0,minFence=0;
        unsigned pendingQueue=QueueCapacity;
        bool sealed=false,quarantined=false,executed=false;
        std::array<U64,QueueCapacity> required{};
        std::unique_ptr<Owner> owner;
    };
    std::array<Queue,QueueCapacity> queues_{};
    std::array<Entry,Capacity> entries_{};
    U64 nextId_=0,nextTicket_=0,bytes_=0;
    Entry* Find(U64 id) noexcept {
        if(id)for(auto& e:entries_)if(e.id==id)return &e;
        return nullptr;
    }
    unsigned QueueIndex(U64 id) const noexcept {
        if(id)for(unsigned i=0;i<QueueCapacity;++i)if(queues_[i].id==id)return i;
        return QueueCapacity;
    }
    bool Complete(const Entry& e) const noexcept {
        if(e.quarantined || e.ticket)return false;
        for(unsigned i=0;i<QueueCapacity;++i)
            if(e.required[i] && (queues_[i].failed || queues_[i].completed<e.required[i]))return false;
        return true;
    }
    void FailQueue(unsigned index) noexcept {
        queues_[index].failed=true;
        for(auto& e:entries_)if(e.id && (e.required[index] || e.pendingQueue==index))e.quarantined=true;
    }
public:
    struct Ticket {U64 recording=0,serial=0,queue=0;};
    bool RegisterQueue(U64 identity) noexcept {
        if(!identity)return false;
        const auto existing=QueueIndex(identity);
        if(existing!=QueueCapacity)return !queues_[existing].failed;
        for(auto& q:queues_)if(!q.id){q.id=identity;return true;}
        return false;
    }
    // Admission MUST precede inserting GPU commands. Failure leaves the owner's
    // unique_ptr with the caller, who must omit the snapshot commands entirely.
    U64 Admit(U64 generation,U64 bytes,std::unique_ptr<Owner>& owner) noexcept {
        if(!generation || !owner || !bytes || bytes>ByteBudget-bytes_ || nextId_==UINT64_MAX)return 0;
        for(auto& e:entries_)if(!e.id){
            e.id=++nextId_;e.generation=generation;e.bytes=bytes;e.owner=std::move(owner);
            bytes_+=bytes;return e.id;
        }
        return 0;
    }
    // Called before each actual native Execute, including replay. A refused
    // proof never authorizes suppressing native execution. Owners are pinned on
    // uncertainty. This policy alone does NOT make overlapping GPU writes safe.
    Ticket BeforeExecute(U64 recording,U64 generation,U64 queue) noexcept {
        auto* e=Find(recording);if(!e)return {};
        const auto index=QueueIndex(queue);
        if(e->generation!=generation || e->sealed || e->quarantined || e->ticket ||
           index==QueueCapacity || queues_[index].failed || queues_[index].issued>=UINT64_MAX-1 || nextTicket_==UINT64_MAX){
            e->quarantined=true;return {};
        }
        // Same-queue replay is ordered. Cross-queue mutation requires a separate
        // synchronization protocol; do not certify such output even if finished.
        for(unsigned i=0;i<QueueCapacity;++i)if(i!=index && e->required[i]){
            e->quarantined=true;return {};
        }
        e->ticket=++nextTicket_;e->pendingQueue=index;e->minFence=queues_[index].issued;
        return {recording,e->ticket,queue};
    }
    // Supply the successful Signal's value only after the native Execute. A
    // signal covering a batch can certify several records with the same value.
    bool AfterExecute(Ticket ticket,U64 queue,U64 value,bool signalSucceeded) noexcept {
        auto* e=Find(ticket.recording);if(!e)return false;
        const auto index=QueueIndex(queue);
        if(!ticket.serial || e->ticket!=ticket.serial || ticket.queue!=queue ||
           index==QueueCapacity || e->pendingQueue!=index){e->quarantined=true;return false;}
        if(!signalSucceeded || !value || value==UINT64_MAX || queues_[index].failed ||
           value<=e->minFence || value<queues_[index].issued){FailQueue(index);return false;}
        queues_[index].issued=value;e->required[index]=value;e->executed=true;
        e->ticket=0;e->pendingQueue=QueueCapacity;return !e->quarantined;
    }
    bool ObserveCompletion(U64 queue,U64 value) noexcept {
        const auto index=QueueIndex(queue);if(index==QueueCapacity)return false;
        auto& q=queues_[index];
        if(q.failed || value==UINT64_MAX || value<q.completed || value>q.issued){FailQueue(index);return false;}
        q.completed=value;return true;
    }
    // Only successful command-list Reset, final destruction, or another proven
    // end of replay eligibility can seal a recording. Close is NOT sufficient.
    bool Seal(U64 recording,U64 generation) noexcept {
        auto* e=Find(recording);if(!e)return false;
        if(e->generation!=generation){e->quarantined=true;return false;}
        e->sealed=true;return !e->quarantined;
    }
    const Owner* View(U64 recording) noexcept {
        const auto* e=Find(recording);
        return e && e->sealed && e->executed && Complete(*e)?e->owner.get():nullptr;
    }
    // Caller must have finished all consumers of the borrowed View first.
    // Render consumers require their own completion leases; not implemented here.
    // Destroy the returned owner AFTER releasing host locks: COM releases can
    // reenter observation hooks. Detached owners remain the caller's budget responsibility.
    bool Retire(U64 recording,std::unique_ptr<Owner>& retired) noexcept {
        auto* e=Find(recording);if(retired || !e || !e->sealed || !Complete(*e))return false;
        retired=std::move(e->owner);bytes_-=e->bytes;*e={};return true;
    }
    U64 RetainedBytes() const noexcept {return bytes_;}
    bool Quarantined(U64 recording) noexcept {const auto* e=Find(recording);return e && e->quarantined;}
    // Explicit teardown after GPU/device shutdown without host locks held,
    // never a recovery shortcut.
    void DeviceStopped() noexcept {for(auto& e:entries_)e={};bytes_=0;}
};
}
