#pragma once
#include "d3d12_split_owner_retirement.h"
#include <mutex>
namespace gi_reuse {
// Bounded bridge from the process-wide Execute observer's opaque token to the
// exact owner batch. No TLS lookup or allocation at submission. Native Execute
// runs outside this lock; overlapping owned submissions are rejected by Pool.
// Install callbacks only after Initialize and retain the router for their life.
template<class Owner,unsigned Capacity=8,UINT64 Budget=768ull*1024*1024>
class D3D12SplitSubmissionRouter {
public:
 using Pool=D3D12SplitOwnerRetirement<Owner,Capacity,Budget>;
 using Generation=typename Pool::Generation;
 using Signal=typename Pool::Signal;
 using Recording=typename Pool::Recording;
 using ReleaseScene=void (*)(UINT64)noexcept;
private:
 struct Pending {UINT64 token=0;ID3D12CommandQueue* queue=nullptr;typename Pool::Batch batch{};};
 std::mutex mutex_;
 Pool pool_;
 std::array<Pending,Capacity> pending_{};
 struct Scene {UINT64 id=0,token=0;};
 std::array<Scene,Capacity> scenes_{};
 ReleaseScene release_=nullptr;
 UINT64 serial_=0;
 // Called under the router lock. The host callback only releases its scene
 // gate; it must never call back into this router or invoke native rendering.
 void ReleaseReadyScenes()noexcept {
  if(release_)for(auto& scene:scenes_)if(scene.id&&pool_.SceneReleaseReady(scene.id)){
   const auto token=scene.token;scene={};release_(token);
  }
 }
 Generation generation_=nullptr;
 Signal signal_=&Pool::NativeSignal;
public:
 HRESULT Initialize(ID3D12CommandQueue* queue,Generation generation,Signal signal=&Pool::NativeSignal,ReleaseScene release=nullptr)noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  if(!generation||!signal)return E_INVALIDARG;
  const auto hr=pool_.Initialize(queue);
  if(SUCCEEDED(hr)){generation_=generation;signal_=signal;release_=release;}return hr;
 }
 // Host admissions/claims/recording/reset/recycling share this same lock.
 // Never call native Execute or wait for scene preparation inside Access.
 template<class F>auto Access(F&& operation) -> decltype(operation(pool_)) {
  std::lock_guard<std::mutex> lock(mutex_);return operation(pool_);
 }
 // Atomically transfers an already acquired native scene token together with
 // the complete owner. On failure the caller retains BOTH and must release its
 // unrecorded scene token itself. No GPU commands are recorded by admission.
 UINT64 AdmitScene(UINT64 frame,Recording producer,UINT64 bytes,
                   std::unique_ptr<Owner>& owner,UINT64 sceneToken)noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  if(!release_||!sceneToken)return 0;
  for(const auto& scene:scenes_)if(scene.token==sceneToken)return 0;
  for(auto& scene:scenes_)if(!scene.id){
   const auto id=pool_.Admit(frame,producer,bytes,owner);
   if(id)scene={id,sceneToken};return id;
  }
  return 0;
 }
 // Scope exit may cancel only a consumer that was never attached. This does
 // NOT erase the producer recording or substitute for its Execute/Reset seal.
 bool CancelUnattached(UINT64 id)noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  const bool cancelled=pool_.CancelUnattached(id);ReleaseReadyScenes();return cancelled;
 }
 UINT64 Before(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList*const* lists)noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  auto batch=pool_.Before(queue,count,lists,generation_);
  if(!batch.count)return 0;
  if(serial_!=UINT64_MAX)for(auto& p:pending_)if(!p.token){
   p.token=++serial_;p.queue=queue;p.batch=batch;return p.token;
  }
  // Cannot retain the exact completion ticket: pin this batch, never pretend
  // its Execute was observed or signal an unrelated empty completion fence.
  pool_.FailedExecute(queue,batch);return 0;
 }
 void After(ID3D12CommandQueue* queue,UINT64 token,bool submitted)noexcept {
  if(!token)return;
  std::lock_guard<std::mutex> lock(mutex_);
  for(auto& p:pending_)if(p.token==token){
   if(queue==p.queue&&submitted)pool_.After(queue,p.batch,signal_);
   else pool_.FailedExecute(p.queue,p.batch);
   p={};ReleaseReadyScenes();return;
  }
  // A repeated/stale callback has no live ticket and cannot affect a new one.
 }
 void Reset(ID3D12CommandList* list,UINT64 previous)noexcept {
  if(!list||!previous)return;
  std::lock_guard<std::mutex> lock(mutex_);
  pool_.Seal({reinterpret_cast<UINT64>(list),previous});ReleaseReadyScenes();
 }
};
}
