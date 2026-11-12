#pragma once
#ifdef CONTROL_GI_LONG_SEQUENCE
#include "gi_reuse/d3d12_frozen_invocation.h"
#include "gi_reuse/d3d12_split_submission_router.h"
#include "gi_reuse/d3d12_frozen_owner_cache.h"
#include "gi_frozen_submission_bridge.h"
// Included after native_lighting and the exact native scope/window adapters.
// Process-lifetime observer wiring. Installation alone records no replacement
// rays and grants no scene/content lease. Admission requires a prepared owner
// and a scene token already acquired by the live dispatch coordinator.
namespace gi_frozen_lifetime {
using Owner=gi_reuse::D3D12FrozenInvocation;
using Router=gi_reuse::D3D12SplitSubmissionRouter<Owner>;
// Destruction notifications can arrive during other static destructors. Keep
// their callback target alive until process exit; never destroy its mutex first.
inline Router* const router=new(std::nothrow) Router;
inline std::mutex initializeMutex;
inline Microsoft::WRL::ComPtr<ID3D12CommandQueue> nativeQueue;
inline gi_reuse::D3D12FrozenOwnerCache* cache=nullptr;
inline std::atomic<bool> installed{false},initialized{false};
inline void ReleaseScene(UINT64 token)noexcept {
 if(!gi_native_scene_window::ReleaseAfterConsumerOrCancellation(token))
  Log("GI_FROZEN_R428 lifetime_fault=scene_release token=%llu",token);
}
inline UINT64 Before(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList*const* lists)noexcept {
 return initialized.load(std::memory_order_acquire)?router->Before(queue,count,lists):0;
}
inline void After(ID3D12CommandQueue* queue,UINT64 token,bool submitted)noexcept {
 if(initialized.load(std::memory_order_acquire))router->After(queue,token,submitted);
}
inline void Seal(ID3D12CommandList* list,UINT64 generation)noexcept {
 if(initialized.load(std::memory_order_acquire))router->Reset(list,generation);
}
inline const gi_frozen_submission::Callbacks callbacks{&Before,&After};
// Called once during startup, before render workers. Existing feature callbacks
// are independent and must never be overwritten to enable this path.
inline bool Install()noexcept {
 if(installed.load(std::memory_order_acquire))return true;
 auto& hooks=native_lighting::gi_compute_hooks::onFrozenRecordingReset;
 auto& destroyed=native_lighting::gi_compute_hooks::onFrozenRecordingDestroyed;
 if(!router||!gi_native_scene_window::ready.load(std::memory_order_acquire)||
    gi_frozen_submission::callbacks.load()||hooks.load()||destroyed.load())return false;
 native_lighting::gi_compute_hooks::EnableForDenoiser();
 hooks.store(&Seal,std::memory_order_release);destroyed.store(&Seal,std::memory_order_release);
 gi_frozen_submission::callbacks.store(&callbacks,std::memory_order_release);
 installed.store(true,std::memory_order_release);return true;
}
// Queue comes from the original native recording accessor. The first call only
// allocates the private fence; it does not flush, submit or wait for GPU work.
inline bool Initialize(ID3D12CommandQueue* queue)noexcept {
 if(!installed.load(std::memory_order_acquire)||!queue)return false;
 std::lock_guard<std::mutex> lock(initializeMutex);
 if(initialized.load(std::memory_order_acquire))return queue==nativeQueue.Get();
 std::unique_ptr<gi_reuse::D3D12FrozenOwnerCache> next;
 try {next=std::make_unique<gi_reuse::D3D12FrozenOwnerCache>(768ull*1024*1024);}catch(...){return false;}
 if(FAILED(router->Initialize(queue,&native_lighting::gi_compute_hooks::RecordingGeneration,
     &Router::Pool::NativeSignal,&ReleaseScene)))return false;
 cache=next.release();nativeQueue=queue;initialized.store(true,std::memory_order_release);return true;
}
// The coordinator uses this before admission/commands. All allocation remains
// outside the queue/router lock and all owners use one active+idle budget.
inline HRESULT PrepareOwner(Owner::Preparation input,std::unique_ptr<Owner>& output)noexcept {
 if(!initialized.load(std::memory_order_acquire)||!cache)return E_UNEXPECTED;
 return cache->Prepare(*router,std::move(input),output);
}
// No ownership transfer on failure. The caller must release its acquired but
// unrecorded scene token. Actual command recording/suppression is separate.
inline UINT64 Admit(const gi_native_render_scope::Scope& scope,ID3D12CommandList* list,
                    UINT64 generation,std::unique_ptr<Owner>& owner)noexcept {
 if(!initialized.load(std::memory_order_acquire)||!scope.token||!scope.frame||!scope.owner||
    !owner||!owner->AllocationBytes()||owner->OriginalProducer()!=list||owner->NativeQueue()!=nativeQueue.Get()||
    gi_native_render_scope::current.token!=scope.token||gi_native_render_scope::current.frame!=scope.frame||
    gi_native_render_scope::current.owner!=scope.owner||
    gi_native_scene_window::window.Inspect().reader!=scope.token||
    !native_lighting::gi_compute_hooks::ObserveDestruction(list,generation))return 0;
 return router->AdmitScene(scope.frame,{reinterpret_cast<UINT64>(list),generation},owner->AllocationBytes(),owner,scope.token);
}
}
#endif
