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
private:
 struct Pending {UINT64 token=0;ID3D12CommandQueue* queue=nullptr;typename Pool::Batch batch{};};
 std::mutex mutex_;
 Pool pool_;
 std::array<Pending,Capacity> pending_{};
 UINT64 serial_=0;
 Generation generation_=nullptr;
 Signal signal_=&Pool::NativeSignal;
public:
 HRESULT Initialize(ID3D12CommandQueue* queue,Generation generation,Signal signal=&Pool::NativeSignal)noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  if(!generation||!signal)return E_INVALIDARG;
  const auto hr=pool_.Initialize(queue);
  if(SUCCEEDED(hr)){generation_=generation;signal_=signal;}return hr;
 }
 // Host admissions/claims/recording/reset/recycling share this same lock.
 // Never call native Execute or wait for scene preparation inside Access.
 template<class F>auto Access(F&& operation) -> decltype(operation(pool_)) {
  std::lock_guard<std::mutex> lock(mutex_);return operation(pool_);
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
   p={};return;
  }
  // A repeated/stale callback has no live ticket and cannot affect a new one.
 }
 void Reset(ID3D12CommandList* list,UINT64 previous)noexcept {
  if(!list||!previous)return;
  std::lock_guard<std::mutex> lock(mutex_);
  pool_.Seal({reinterpret_cast<UINT64>(list),previous});
 }
};
}
