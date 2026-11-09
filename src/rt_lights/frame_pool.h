#pragma once
#include "frame_resources.h"
#include "recording_lifetime.h"
#include "frame_recorder.h"
#include <memory>
#include <mutex>
#include <vector>

namespace rt_lights {
class FramePool {
public:
    static constexpr unsigned SlotCount = 3;
    struct Slot {
        std::unique_ptr<FrameResources> resources;
        std::vector<BatchTables> tables;
        ComPtr<ID3D12Fence> fence;
        ComPtr<ID3D12CommandQueue> queue;
        ComPtr<ID3D12CommandList> list, consumer;
        ComPtr<ID3D12StateObject> rayOwner;
        RecordingLifetime lifetime, consumerLifetime;
        UINT64 nextFence = 1;
        bool preparing = false, ownerActive = false;
    };
private:
    std::mutex mutex;
    std::array<std::unique_ptr<Slot>, SlotCount> slots;
    ComPtr<ID3D12Device> device;
    UINT64 generation = 0;
    bool failed = false;

    void RetireLocked(Slot& slot) noexcept {
        if (!slot.fence || slot.lifetime.Available() || slot.ownerActive) return;
        const UINT64 completed = slot.fence->GetCompletedValue();
        if (completed == UINT64_MAX) {
            slot.lifetime.Quarantine(); failed = true;
            return;
        }
        if (!slot.lifetime.CanRetire(completed) ||
            (slot.consumer && !slot.consumerLifetime.CanRetire(completed))) return;
        const bool submitted = slot.lifetime.Submitted();
        const bool consumerSubmitted = slot.consumerLifetime.Submitted();
        if (slot.lifetime.Retire(completed)) {
            if (slot.consumer) slot.consumerLifetime.Retire(completed);
            if (!submitted && !consumerSubmitted) slot.resources->DiscardRecording();
            else if (slot.consumer && !consumerSubmitted) slot.resources->DiscardConsumer();
            else slot.resources->EvidenceRetired();
            slot.list.Reset(); slot.consumer.Reset();
            slot.rayOwner.Reset();
        }
    }
public:
    FramePool() {
        for (auto& slot : slots) slot = std::make_unique<Slot>();
    }
    FramePool(const FramePool&) = delete;
    FramePool& operator=(const FramePool&) = delete;
    ~FramePool() {
        // Process teardown must not destroy objects still referenced by GPU
        // work. Normal worker cleanup retires them without waiting beforehand.
        for (auto& slot : slots)
            if (slot && !slot->lifetime.Available()) (void)slot.release();
    }
    HRESULT Initialize(ID3D12Device* owner) noexcept {
        if (!owner || device) return E_INVALIDARG;
        device = owner;
        for (auto& slot : slots) {
            const HRESULT hr = owner->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&slot->fence));
            if (FAILED(hr)) { failed = true; return hr; }
        }
        return S_OK;
    }
    // One worker calls Prepare/Poll. It constructs resources outside the mutex;
    // render callbacks never wait for resource creation, mapping or destruction.
    HRESULT Prepare(UINT width, UINT height, UINT64 totalBudget) {
        if (!device || !totalBudget) return E_INVALIDARG;
        Slot* target = nullptr;
        std::unique_ptr<FrameResources> old;
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (failed) return E_FAIL;
            for (auto& slot : slots) {
                RetireLocked(*slot);
                if (slot->preparing || !slot->lifetime.Available()) continue;
                if (slot->resources && slot->resources->Width() == width && slot->resources->Height() == height) continue;
                target = slot.get(); target->preparing = true;
                old = std::move(target->resources);
                break;
            }
        }
        if (!target) return S_FALSE;
        old.reset();
        auto fresh = std::make_unique<FrameResources>();
        const HRESULT hr = fresh->Initialize(device.Get(), width, height, totalBudget / SlotCount);
        std::vector<BatchTables> tables;
        if (SUCCEEDED(hr)) tables.resize(1);
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (SUCCEEDED(hr) && !failed) {
                target->resources = std::move(fresh);
                target->tables = std::move(tables);
            }
            target->preparing = false;
        }
        return hr;
    }
    Slot* Acquire(ID3D12CommandList* list, UINT width, UINT height) noexcept {
        if (!list) return nullptr;
        std::lock_guard<std::mutex> lock(mutex);
        if (failed || generation == UINT64_MAX) return nullptr;
        for (auto& slot : slots) {
            // Poll completed fences without waiting; the worker timer must not
            // limit reuse to three frames per half-second. Resources stay allocated.
            RetireLocked(*slot);
            if (failed) return nullptr;
            if (slot->preparing || !slot->lifetime.Available() || !slot->resources ||
                slot->resources->Width() != width || slot->resources->Height() != height) continue;
            if (!slot->lifetime.Begin(++generation)) return nullptr;
            slot->list = list;
            slot->ownerActive = true;
            slot->resources->BeginRecording();
            return slot.get();
        }
        return nullptr; // Native fallback; never wait for a busy slot.
    }
    // Join a second native recording to this resource lease before emitting
    // custom commands on it. Both lists execute on the verified native queue.
    bool AttachConsumer(Slot* slot, ID3D12CommandList* list) noexcept {
        if (!slot || !list) return false;
        std::lock_guard<std::mutex> lock(mutex);
        if (failed || !slot->ownerActive || slot->lifetime.Available()) return false;
        if (slot->list.Get() == list && !slot->lifetime.Detached()) return true;
        if (slot->consumer) return slot->consumer.Get() == list && !slot->consumerLifetime.Detached();
        if (slot->lifetime.Detached() && !slot->lifetime.Submitted()) return false;
        if (!slot->consumerLifetime.Begin(slot->lifetime.Generation())) return false;
        slot->resources->CheckpointHandoff();
        slot->consumer = list;
        return true;
    }
    void ReleaseRecording(Slot* slot) noexcept {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto& owned : slots) if (owned.get() == slot) owned->ownerActive = false;
    }
    // The adapter passes the ACTUAL native queue after its ExecuteCommandLists
    // call returns, plus the exact observed recording-list identities. Do not
    // signal a different queue inferred from Present or a frame counter.
    bool Submitted(ID3D12CommandQueue* queue, UINT count, ID3D12CommandList* const* lists) noexcept {
        if (!queue || !lists) return false;
        std::lock_guard<std::mutex> lock(mutex);
        if (failed) return false;
        for (auto& slot : slots) {
            if (slot->lifetime.Available()) continue;
            bool primaryFound = false, consumerFound = false;
            for (UINT i = 0; i < count; ++i) {
                primaryFound |= !slot->lifetime.Detached() && lists[i] == slot->list.Get();
                consumerFound |= slot->consumer && !slot->consumerLifetime.Detached() && lists[i] == slot->consumer.Get();
            }
            if (!primaryFound && !consumerFound) continue;
            if ((slot->queue && slot->queue.Get() != queue) || slot->nextFence == UINT64_MAX ||
                (consumerFound && !primaryFound && !slot->lifetime.Submitted())) {
                slot->lifetime.Quarantine(); failed = true;
                return false;
            }
            slot->queue = queue;
            const UINT64 value = slot->nextFence++;
            if (FAILED(queue->Signal(slot->fence.Get(), value)) ||
                (primaryFound && !slot->lifetime.Submit(slot->lifetime.Generation(), value)) ||
                (consumerFound && !slot->consumerLifetime.Submit(slot->consumerLifetime.Generation(), value))) {
                slot->lifetime.Quarantine(); failed = true;
                return false;
            }
        }
        return true;
    }
    void ResetSucceeded(ID3D12CommandList* list) noexcept {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto& slot : slots) {
            if (slot->list.Get() == list) slot->lifetime.Reset(slot->lifetime.Generation());
            if (slot->consumer.Get() == list) slot->consumerLifetime.Reset(slot->consumerLifetime.Generation());
        }
    }
    // Worker only. trim is useful while the feature is disabled; in-flight
    // recordings remain owned until reset/discard and actual fence completion.
    void Poll(bool trim = false, void (*report)(const FrameResources::EvidenceSummary&) = nullptr) noexcept {
        // Resource creation/destruction is worker-owned. Render callbacks may
        // reuse scratch buffers while this independent completed copy is read.
        std::array<FrameResources*, SlotCount> evidence{};
        {
            std::lock_guard<std::mutex> lock(mutex);
            for (unsigned i = 0; i < SlotCount; ++i) {
                RetireLocked(*slots[i]); evidence[i] = slots[i]->resources.get();
            }
        }
        for (auto* resource : evidence) if (resource) {
            FrameResources::EvidenceSummary result;
            if (resource->ReadEvidence(result) && report) report(result);
        }
        std::array<std::unique_ptr<FrameResources>, SlotCount> retired;
        if (trim) {
            std::lock_guard<std::mutex> lock(mutex);
            for (unsigned i = 0; i < SlotCount; ++i)
                if (!slots[i]->preparing && slots[i]->lifetime.Available()) retired[i] = std::move(slots[i]->resources);
        }
    }
};
}
