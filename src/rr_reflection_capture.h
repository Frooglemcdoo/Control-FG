#pragma once
#include "rr_reflection_binding.h"
#include "rr_frame_slots.h"
namespace control_rr_reflection {
// Caller serializes all calls under the render/retirement lock. Backend is owned
// alongside this object. No destructor frees GPU resources: shutdown must first
// retire successfully submitted work; ambiguous recording/signal failures retain.
// Backend requirements, in addition to RecordCopies methods:
// ReleaseSources(slot), Select(slot), Signal(fence,value), Completed(fence).
// Select binds HoldSources to that slot. Destinations are preallocated off-thread,
// owned externally until Idle(), and always start/end in NON_PIXEL_SHADER_RESOURCE.
template<class Backend> class Capture {
 Backend& backend_;
 control_rr::FrameSlots policy_{};
 control_rr::FenceKey fence_{};
 std::array<Context,3> contexts_{};
 std::array<bool,3> held_{};
 std::array<bool,3> primaryQueued_{};
 bool stopped_=false;
 std::uint64_t signal_=0;
public:
 explicit Capture(Backend& backend):backend_(backend){}
 Capture(const Capture&)=delete;
 Capture& operator=(const Capture&)=delete;
 bool Bind(control_rr::FenceKey fence){
  if(stopped_||!Idle()||!policy_.Bind(fence))return false;
  if(!control_rr::SameFence(fence_,fence))signal_=0;
  fence_=fence;return true;
 }
 bool Idle() const {
  for(const auto&s:policy_.Inspect())if(s.state!=control_rr::SlotState::Free)return false;
  for(bool h:held_)if(h)return false;
  return true;
 }
 bool Stopped() const{return stopped_;}
 const auto& Inspect() const{return policy_.Inspect();}
 // destinations[slot][material/position]; rejects any cross-slot resource alias.
 bool Record(const Snapshot&s,std::uint64_t epoch,const Address (&destinations)[3][2],control_rr::Lease&lease){
  lease={};
  if(stopped_||s.context.device!=fence_.device||s.context.queue!=fence_.queue)return false;
  for(unsigned i=0;i<6;++i){
   Address d=destinations[i/2][i%2];
   if(!d||d==s.material.resource||d==s.position.resource)return false;
   for(unsigned j=0;j<i;++j)if(d==destinations[j/2][j%2])return false;
  }
  if(s.material.shape.width>UINT32_MAX)return false;
  const control_rr::FrameKey key{s.context.frame,epoch,static_cast<std::uint32_t>(s.material.shape.width),s.material.shape.height};
  if(!policy_.Begin(key,lease))return false;
  backend_.Select(lease.index);
  // Mark before any backend recording attempt. False positives retain rather
  // than risk releasing GPU-visible resources after an ambiguous call failure.
  if(!policy_.CommandsStarted(lease)){stopped_=true;return false;}
  CopyProgress p{};
  const bool ok=RecordCopies(backend_,s,destinations[lease.index][0],destinations[lease.index][1],p);
  held_[lease.index]=p.sourcesHeld;
  if(!ok){policy_.Abort(lease);stopped_=true;return false;}
  contexts_[lease.index]=s.context;primaryQueued_[lease.index]=false;
  if(!policy_.GuidesReady(lease)){policy_.Abort(lease);stopped_=true;return false;}
  return true;
 }
 bool CaptureBound(const Access&a,Address rendererBase,Address callRva,Address owner,
                   std::uint64_t epoch,const Address (&destinations)[3][2],control_rr::Lease&lease){
  lease={};Snapshot snapshot{};
  return ReadBound(a,rendererBase,callRva,owner,snapshot)&&Record(snapshot,epoch,destinations,lease);
 }
 // Can be consumed once, only by the same frame/view/device/queue/list. A different
 // list requires an independently proven submission-order protocol, not guessing.
 bool Take(control_rr::Lease lease,const Context&current,std::uint64_t epoch){
  if(stopped_||lease.index>=3||!Same(contexts_[lease.index],current))return false;
  const auto&s=policy_.Inspect()[lease.index];
  return policy_.TakeEvaluation(lease,{current.frame,epoch,s.key.width,s.key.height});
 }
 // Called only by the exact primary-Direct enqueue observer AFTER append.
 // The observer verifies queue identity, closed list, frame, Present, and thread.
 bool ObservePrimaryQueued(control_rr::Lease lease,const Context& producer){
  if(stopped_||lease.index>=3||!lease.serial||!Same(contexts_[lease.index],producer))return false;
  const auto& slot=policy_.Inspect()[lease.index];
  if(slot.serial!=lease.serial||slot.state!=control_rr::SlotState::Ready||primaryQueued_[lease.index])return false;
  primaryQueued_[lease.index]=true;return true;
 }
 // Distinct command lists are permitted only after the producer was observed in
 // Control's FIFO Direct queue. Both callers must be verified primary contexts.
 const char* PrimaryHandoffReason(control_rr::Lease lease,const Context& current,std::uint64_t epoch) const {
  if(stopped_||lease.index>=3||!lease.serial)return "invalid_reflection_lease";
  const auto& producer=contexts_[lease.index];const auto& slot=policy_.Inspect()[lease.index];
  if(slot.serial!=lease.serial)return "stale_reflection_lease";
  if(producer.frame!=current.frame)return "producer_frame_differs";
  if(producer.queue!=current.queue)return "producer_queue_differs";
  if(producer.device!=current.device)return "producer_device_differs";
  if(producer.view!=current.view)return "producer_view_differs";
  if(!current.list)return "consumer_list_missing";
  if(!policy_.CanEvaluate(lease,{current.frame,epoch,slot.key.width,slot.key.height}))return "reflection_not_evaluable";
  if(producer.list==current.list)return primaryQueued_[lease.index]?"producer_list_recycled":nullptr;
  return primaryQueued_[lease.index]?nullptr:"producer_list_not_queued";
 }
 bool TakeOnPrimaryQueue(control_rr::Lease lease,const Context& current,std::uint64_t epoch){
  if(PrimaryHandoffReason(lease,current,epoch))return false;
  const auto& s=policy_.Inspect()[lease.index];
  return policy_.TakeEvaluation(lease,{current.frame,epoch,s.key.width,s.key.height});
 }
 // Invoke only AFTER the engine has submitted all lists referencing these slots.
 // Exact-lease retirement for hosts that prove submission using a separate
 // Present-token ledger. Never convert Present tokens to engine-frame numbers.
 bool SubmitCaptured(control_rr::Lease lease){
  if(stopped_||lease.index>=3||!lease.serial)return false;
  const auto s=policy_.Inspect()[lease.index];
  if(s.serial!=lease.serial||s.state!=control_rr::SlotState::Ready)return false;
  if(signal_>=UINT64_MAX-1){policy_.Abort(lease);stopped_=true;return false;}
  const auto next=signal_+1;
  if(!backend_.Signal(fence_,next)){policy_.Abort(lease);stopped_=true;return false;}
  if(!policy_.Submit(lease,next)){stopped_=true;return false;}
  signal_=next;return true;
 }
 // afterFrame is the first frame whose submission is NOT covered by this point.
 bool AfterSubmission(std::uint64_t afterFrame){
  if(stopped_)return false;
  for(std::size_t i=0;i<3;++i){
   const auto s=policy_.Inspect()[i];
   if(s.state!=control_rr::SlotState::Ready||s.key.frame>=afterFrame)continue;
   if(!SubmitCaptured({i,s.serial}))return false;
  }
  return true;
 }
 bool Collect(){
  if(stopped_||!policy_.Collect(fence_,backend_.Completed(fence_))){stopped_=true;return false;}
  for(std::size_t i=0;i<3;++i)if(held_[i]&&policy_.Inspect()[i].state==control_rr::SlotState::Free){
   backend_.ReleaseSources(i);held_[i]=false;contexts_[i]={};primaryQueued_[i]=false;
  }
  return true;
 }
};
}
