#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
namespace gi_reuse {
// One process-owned budget shared by active, preparing and retired idle owners.
// A reservation travels with the whole owner until its GPU/recording retirement.
class FrozenAllocationBudget {
 using U64=std::uint64_t;
 const U64 limit_;
 std::atomic<U64> held_{0};
 std::atomic<bool> failed_{false};
public:
 explicit FrozenAllocationBudget(U64 limit)noexcept:limit_(limit){}
private:
 friend class FrozenAllocationReservation;
 bool Reserve(U64 bytes)noexcept {
  if(!bytes||failed_.load())return false;
  auto held=held_.load();
  do {if(held>limit_||bytes>limit_-held||failed_.load())return false;}
  while(!held_.compare_exchange_weak(held,held+bytes));
  return true;
 }
 void Release(U64 bytes)noexcept {held_.fetch_sub(bytes);}
 void FailClosed()noexcept {failed_.store(true);}
public:
 U64 Held()const noexcept{return held_.load();}
 U64 Limit()const noexcept{return limit_;}
};
class FrozenAllocationReservation {
 using U64=std::uint64_t;
 std::shared_ptr<FrozenAllocationBudget> budget_;
 U64 bytes_=0;
public:
 FrozenAllocationReservation()=default;
 FrozenAllocationReservation(const FrozenAllocationReservation&)=delete;
 FrozenAllocationReservation& operator=(const FrozenAllocationReservation&)=delete;
 ~FrozenAllocationReservation(){if(budget_&&bytes_)budget_->Release(bytes_);}
 bool Peak(const std::shared_ptr<FrozenAllocationBudget>& budget,U64 bytes)noexcept {
  if(!budget||!bytes||(budget_&&budget_!=budget))return false;
  if(bytes>bytes_&&!budget->Reserve(bytes-bytes_))return false;
  if(bytes<bytes_)budget->Release(bytes_-bytes);
  budget_=budget;bytes_=bytes;return true;
 }
 // Settle only AFTER temporary allocations have been released. Failed partial
 // preparation retains its actual storage until the owner is destroyed.
 bool Settle(U64 retained)noexcept {
  if(!budget_)return retained==0;
  if(retained>bytes_){budget_->FailClosed();return false;}
  budget_->Release(bytes_-retained);bytes_=retained;return true;
 }
 U64 Bytes()const noexcept{return bytes_;}
};
}
