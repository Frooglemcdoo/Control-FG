#pragma once
#include "snapshot_retirement.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <new>

namespace gi_reuse {
// Persistent private queue fences, adapted from PT R103. Host serializes these
// methods, NOT the native Execute between Before/After. Each After signal covers
// its preceding native Execute; interleaved work only delays completion. It does
// not establish dependency order or authorize GPU copies by itself.
template<class Owner, unsigned Capacity=256, unsigned QueueCapacity=16,
         std::uint64_t ByteBudget=512ull*1024*1024>
class D3D12SnapshotRetirement {
    using U64=std::uint64_t;
    using Policy=SnapshotRetirement<Owner,Capacity,QueueCapacity,ByteBudget>;
    template<class T> using Ptr=Microsoft::WRL::ComPtr<T>;
    struct Queue {
        Ptr<IUnknown> identity;
        Ptr<ID3D12CommandQueue> queue;
        Ptr<ID3D12Fence> fence;
        U64 issued=0;
    };
    struct State {Policy policy;std::array<Queue,QueueCapacity> queues;};
    std::unique_ptr<State> state_{new(std::nothrow) State};
    U64 FindQueue(ID3D12CommandQueue* queue) noexcept {
        if(!state_ || !queue)return 0;
        Ptr<IUnknown> identity;
        if(FAILED(queue->QueryInterface(IID_PPV_ARGS(&identity))))return 0;
        for(unsigned i=0;i<QueueCapacity;++i)
            if(state_->queues[i].identity.Get()==identity.Get())return U64(i)+1;
        for(unsigned i=0;i<QueueCapacity;++i)if(!state_->queues[i].identity){
            Ptr<ID3D12Device> device;Ptr<ID3D12Fence> fence;
            if(FAILED(queue->GetDevice(IID_PPV_ARGS(&device))) ||
               FAILED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence))))return 0;
            auto& q=state_->queues[i];q.identity=identity;q.queue=queue;q.fence=fence;
            if(!state_->policy.RegisterQueue(U64(i)+1))return 0;
            return U64(i)+1;
        }
        return 0;
    }
public:
    struct Recording {U64 id=0,generation=0;};
    struct Batch {
        U64 queue=0;
        unsigned count=0;
        std::array<typename Policy::Ticket,Capacity> tickets{};
    };
    using Signal=HRESULT (*)(ID3D12CommandQueue*,ID3D12Fence*,UINT64);
    static HRESULT NativeSignal(ID3D12CommandQueue* q,ID3D12Fence* f,UINT64 n){return q->Signal(f,n);}
    ~D3D12SnapshotRetirement(){
        // Unknown GPU use must outlive DLL/runtime teardown. Deliberately pin
        // the bounded state, including queues/fences/owners, rather than release
        // potentially referenced resources. Normal retirement leaves zero bytes.
        if(state_ && state_->policy.RetainedBytes())(void)state_.release();
    }
    U64 Admit(U64 generation,U64 bytes,std::unique_ptr<Owner>& owner) noexcept {
        return state_?state_->policy.Admit(generation,bytes,owner):0;
    }
    bool Seal(Recording r) noexcept {return state_ && state_->policy.Seal(r.id,r.generation);}
    // Must be called for EVERY execution of every admitted recording. Missing
    // queue proof pins owners; it never authorizes skipping native submission.
    Batch Before(ID3D12CommandQueue* queue,const Recording* records,unsigned count) noexcept {
        Batch batch;
        if(!state_ || !records || !count)return batch;
        const U64 id=count<=Capacity?FindQueue(queue):0;
        batch.queue=id;
        for(unsigned i=0;i<count;++i){
            auto ticket=state_->policy.BeforeExecute(records[i].id,records[i].generation,id);
            if(ticket.serial && batch.count<Capacity)batch.tickets[batch.count++]=ticket;
        }
        return batch;
    }
    bool After(Batch& batch,Signal signal=&NativeSignal) noexcept {
        if(!state_ || !batch.queue || !batch.count){batch={};return false;}
        auto& q=state_->queues[static_cast<unsigned>(batch.queue-1)];
        const U64 value=q.issued<UINT64_MAX-1?q.issued+1:0;
        const bool ok=value && signal && SUCCEEDED(signal(q.queue.Get(),q.fence.Get(),value));
        if(ok)q.issued=value;
        bool accepted=ok;
        for(unsigned i=0;i<batch.count;++i)
            if(!state_->policy.AfterExecute(batch.tickets[i],batch.queue,value,ok))accepted=false;
        batch={};return accepted;
    }
    void Poll() noexcept {
        if(!state_)return;
        for(unsigned i=0;i<QueueCapacity;++i){auto& q=state_->queues[i];
            if(q.fence)state_->policy.ObserveCompletion(U64(i)+1,q.fence->GetCompletedValue());
        }
    }
    const Owner* View(U64 id) noexcept {return state_?state_->policy.View(id):nullptr;}
    bool Take(U64 id,std::unique_ptr<Owner>& out) noexcept {return state_ && state_->policy.Retire(id,out);}
    U64 RetainedBytes() const noexcept {return state_?state_->policy.RetainedBytes():0;}
    bool Quarantined(U64 id) noexcept {return state_ && state_->policy.Quarantined(id);}
    // Only after the application has stopped all relevant GPU work. Caller must
    // not hold observation locks: destructors can reenter COM hooks.
    void DeviceStopped() noexcept {if(state_)state_->policy.DeviceStopped();}
};
}
