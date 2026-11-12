#pragma once
#include "rt_ptlas_plan.h"
#include <deque>
#include <optional>

namespace rt_ptlas {
// One externally serialized queue and private fence. CPU history advances only
// after Execute + successful Signal, not recording and not GPU completion.
// This permits several ordered updates in flight without a per-update GPU wait.
// Host retains every recording/upload/BLAS/AS owner until its exact fence and
// prevents command-list replay. This journal does not supply resource leases.
class SubmissionJournal {
public:
 using Id=std::uint64_t;
 enum class Stage {Prepared,Recorded,Closed,Executing};
private:
 struct Pending {Id ticket;Plan plan;Stage stage=Stage::Prepared;Id fence=0;};
 struct Flight {Id ticket,fence;};
 Planner planner_;
 Id queue_,fence_,serial_=0,lastSubmitted_=0,completed_=0;
 bool fault_=false;
 std::optional<Pending> pending_;
 std::deque<Flight> flights_;
 bool Is(Id ticket,Stage stage)const noexcept {
  return !fault_&&pending_&&pending_->ticket==ticket&&pending_->stage==stage;
 }
public:
 SubmissionJournal(std::uint32_t capacity,std::uint32_t partitions,Id queue,Id fence)
 :planner_(capacity,partitions),queue_(queue),fence_(fence){
  if(!queue||!fence)throw std::invalid_argument("missing PTLAS queue/fence identity");
 }
 SubmissionJournal(const SubmissionJournal&)=delete;
 SubmissionJournal& operator=(const SubmissionJournal&)=delete;
 Id Prepare(const std::vector<Instance>& instances,const std::vector<std::uint32_t>& partitions){
  if(fault_||pending_||flights_.size()>=128||serial_==UINT64_MAX)
   throw std::logic_error("PTLAS journal unavailable");
  auto plan=planner_.Prepare(instances,partitions);
  const auto ticket=++serial_;pending_.emplace(Pending{ticket,std::move(plan)});return ticket;
 }
 const Plan& Prepared(Id ticket)const {
  if(!Is(ticket,Stage::Prepared))throw std::logic_error("PTLAS plan is not recordable");
  return pending_->plan;
 }
 bool Recorded(Id ticket)noexcept {
  if(!Is(ticket,Stage::Prepared))return false;
  pending_->stage=Stage::Recorded;return true;
 }
 bool Closed(Id ticket)noexcept {
  if(!Is(ticket,Stage::Recorded))return false;
  pending_->stage=Stage::Closed;return true;
 }
 // Call only after successful native Reset/discard and exclusion of submission.
 // This also handles a recorder failure after partial GPU commands were emitted.
 bool DiscardAfterReset(Id ticket)noexcept {
  if(fault_||!pending_||pending_->ticket!=ticket||pending_->stage==Stage::Executing)return false;
  pending_.reset();return true;
 }
 // All potentially allocating bookkeeping is done BEFORE ExecuteCommandLists.
 bool BeginExecute(Id ticket,Id queue,Id fence,Id value){
  if(!Is(ticket,Stage::Closed)||queue!=queue_||fence!=fence_||
     !value||value==UINT64_MAX||value<=lastSubmitted_)return false;
  flights_.push_back({ticket,value});pending_->fence=value;pending_->stage=Stage::Executing;return true;
 }
 // Signal failure after Execute has no safe native retry or resource retirement.
 bool Submitted(Id ticket,bool signalSucceeded) {
  if(!Is(ticket,Stage::Executing))return false;
  if(!signalSucceeded){fault_=true;return false;}
  planner_.Commit(std::move(pending_->plan));lastSubmitted_=pending_->fence;pending_.reset();return true;
 }
 void ExecutionUncertain()noexcept{fault_=true;}
 bool Completed(Id queue,Id fence,Id value)noexcept {
  if(fault_||queue!=queue_||fence!=fence_||value==UINT64_MAX||value>lastSubmitted_)return false;
  if(value<=completed_)return true;
  completed_=value;while(!flights_.empty()&&flights_.front().fence<=value)flights_.pop_front();return true;
 }
 bool Faulted()const noexcept{return fault_;}
 std::size_t InFlight()const noexcept{return flights_.size();}
 Id LastSubmitted()const noexcept{return lastSubmitted_;}
};
}
