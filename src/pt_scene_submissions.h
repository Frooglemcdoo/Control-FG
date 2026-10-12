#pragma once
#include "pt_scene_leases.h"
#include <array>
#include <mutex>
// Host contract: Hold before recording GPU use; Submitted only AFTER the actual
// ExecuteCommandLists on this queue. Serialize that boundary with list reset.
// A generation cannot be reused or submitted again. No frame-count retirement.
namespace PTOwnedScene {
class Submissions {
    enum class Phase {empty,recording,submitted,quarantined};
    struct Slot {LeaseBatch batch;std::uint64_t recording=0,fence=0;Phase phase=Phase::empty;};
    std::mutex mutex;
    std::array<Slot,16> slots;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    std::uint64_t nextFence=1;
    bool failed=false;
    void PollLocked(){
        if(!fence)return;
        const auto completed=fence->GetCompletedValue();
        if(completed==UINT64_MAX){failed=true;return;}
        for(auto& slot:slots)if(slot.phase==Phase::submitted&&completed>=slot.fence)slot=Slot{};
    }
public:
    bool Initialize(ID3D12CommandQueue* nativeQueue){
        std::lock_guard<std::mutex> guard(mutex);if(queue||!nativeQueue)return false;
        Microsoft::WRL::ComPtr<ID3D12Device> device;
        if(FAILED(nativeQueue->GetDevice(IID_PPV_ARGS(&device)))||
           FAILED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence))))return false;
        queue=nativeQueue;return true;
    }
    bool Hold(std::uint64_t recording,LeaseBatch& batch){
        std::lock_guard<std::mutex> guard(mutex);PollLocked();
        if(!queue||failed||!recording||!batch.Count())return false;
        for(const auto& slot:slots)if(slot.recording==recording&&slot.phase!=Phase::recording)return false;
        for(auto& slot:slots)if(slot.phase==Phase::empty){slot.batch=std::move(batch);slot.recording=recording;slot.phase=Phase::recording;return true;}
        return false; // Caller keeps the batch on rejection.
    }
    bool Submitted(std::uint64_t recording){
        std::lock_guard<std::mutex> guard(mutex);
        bool found=false;for(const auto& slot:slots)if(slot.phase==Phase::recording&&slot.recording==recording)found=true;
        if(!found)return false;
        const auto value=nextFence;
        const bool signalled=!failed&&value!=UINT64_MAX&&SUCCEEDED(queue->Signal(fence.Get(),value));
        if(signalled)++nextFence;else failed=true;
        for(auto& slot:slots)if(slot.phase==Phase::recording&&slot.recording==recording){
            slot.phase=signalled?Phase::submitted:Phase::quarantined;slot.fence=value;
        }
        return signalled;
    }
    void Discarded(std::uint64_t recording){
        std::lock_guard<std::mutex> guard(mutex);
        for(auto& slot:slots)if(slot.phase==Phase::recording&&slot.recording==recording)slot=Slot{};
    }
    void Poll(){std::lock_guard<std::mutex> guard(mutex);PollLocked();}
    ~Submissions(){
        PollLocked();bool outstanding=false;
        for(auto& slot:slots)if(slot.phase==Phase::submitted||slot.phase==Phase::quarantined){
            // No completion proof: intentionally quarantine the references AND
            // their budget charge for process lifetime instead of freeing live GPU memory.
            (void)slot.batch.payload.release();outstanding=true;
        }
        if(outstanding){(void)queue.Detach();(void)fence.Detach();}
    }
};
}
