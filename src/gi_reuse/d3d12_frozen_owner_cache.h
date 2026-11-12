#pragma once
#include "d3d12_frozen_invocation.h"
#include "d3d12_split_submission_router.h"
namespace gi_reuse {
// Only the submission router can supply reusable owners. Never cache from a
// frame counter, producer completion, scope exit or command-list Close.
class D3D12FrozenOwnerCache {
 using Owner=D3D12FrozenInvocation;
 static constexpr unsigned Capacity=8;
 using Owners=std::array<std::unique_ptr<Owner>,Capacity>;
 std::mutex mutex_;
 Owners idle_{};
 const UINT64 limit_;
 std::shared_ptr<FrozenAllocationBudget> budget_;
public:
 explicit D3D12FrozenOwnerCache(UINT64 limit):limit_(limit),budget_(std::make_shared<FrozenAllocationBudget>(limit)){}
 void TrimIdle()noexcept {
  Owners discarded;
  {std::lock_guard<std::mutex> lock(mutex_);discarded=std::move(idle_);}
  // D3D Release and allocation-reservation destruction occur without our lock.
 }
 void Collect(D3D12SplitSubmissionRouter<Owner>& router)noexcept {
  Owners retired;router.CollectReady(retired);
  for(auto& owner:retired)if(owner&&!owner->RearmAfterRetirement())owner.reset();
  {std::lock_guard<std::mutex> lock(mutex_);
   for(auto& owner:retired)if(owner)for(auto& slot:idle_)if(!slot){slot=std::move(owner);break;}
  }
  // Excess fully retired owners are released outside both locks.
 }
 HRESULT Prepare(D3D12SplitSubmissionRouter<Owner>& router,Owner::Preparation input,
                 std::unique_ptr<Owner>& output)noexcept {
  if(output||!input.budget||!limit_)return E_INVALIDARG;
  Collect(router);
  std::unique_ptr<Owner> candidate;
  {std::lock_guard<std::mutex> lock(mutex_);
   for(auto& slot:idle_)if(slot&&slot->ReusableFor(input)){candidate=std::move(slot);break;}
  }
  input.memoryBudget=budget_;input.budget=(std::min)(input.budget,limit_);
  if(!candidate)candidate.reset(new(std::nothrow) Owner);
  if(!candidate)return E_OUTOFMEMORY;
  auto hr=candidate->Prepare(input);
  if(hr==E_OUTOFMEMORY){
   // An idle oversized allocation or its growth overlap can deny a new frame.
   // Release only retired/unrecorded storage, then try one fresh preparation.
   candidate.reset();TrimIdle();candidate.reset(new(std::nothrow) Owner);
   if(!candidate)return E_OUTOFMEMORY;hr=candidate->Prepare(input);
  }
  if(SUCCEEDED(hr))output=std::move(candidate);
  return hr;
 }
 UINT64 HeldBytes()const noexcept{return budget_->Held();}
};
}
