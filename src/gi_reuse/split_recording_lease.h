#pragma once
#include <array>
#include <cstdint>
namespace gi_reuse {
// One split invocation, serialized by the host. Owns no GPU resources itself:
// its owner must retain ALL resources until Retirable(), or device shutdown.
// Queue completion comes from a private fence signalled after actual Execute.
// Close/Present/frame advance are never retirement evidence.
class GISplitRecordingLease {
 using U64=std::uint64_t;
 struct Recording {U64 list=0,generation=0;bool sealed=false,executed=false;};
 std::array<Recording,2> recordings_{};
 U64 frame_=0,queue_=0,required_=0,completed_=0;
 unsigned pending_=0;
 bool cancelled_=false,quarantined_=false;
public:
 bool Begin(U64 frame,U64 list,U64 generation)noexcept {
  if(frame_||!frame||!list||!generation)return false;
  frame_=frame;recordings_[0]={list,generation,false,false};return true;
 }
 bool AttachConsumer(U64 frame,U64 list,U64 generation)noexcept {
  if(!frame_||frame!=frame_||!list||!generation||cancelled_||quarantined_||recordings_[1].list||
     (list==recordings_[0].list&&generation==recordings_[0].generation))return false;
  recordings_[1]={list,generation,false,false};return true;
 }
 // mask 1=producer, 2=consumer, 3=one batch in producer->consumer order.
 // Caller derives mask/order from the REAL Execute array, not record order.
 // Replay/cross-queue/overlapping submissions fail closed, retaining the owner.
 bool BeforeExecute(unsigned mask,U64 queue)noexcept {
  if(!frame_||!mask||mask>3||!queue||pending_||quarantined_||
     (queue_&&queue_!=queue)||(mask&2&&cancelled_))return Quarantine();
  for(unsigned i=0;i<2;++i)if(mask&(1u<<i)){
   const auto& r=recordings_[i];if(!r.list||r.sealed||r.executed)return Quarantine();
  }
  if(mask==2&&!recordings_[0].executed)return Quarantine();
  queue_=queue;pending_=mask;return true;
 }
 bool AfterExecute(U64 queue,U64 fence,bool signalled)noexcept {
  if(!pending_||quarantined_||queue!=queue_||!signalled||!fence||fence==UINT64_MAX||fence<=required_)
   return Quarantine();
  for(unsigned i=0;i<2;++i)if(pending_&(1u<<i))recordings_[i].executed=true;
  required_=fence;pending_=0;return true;
 }
 bool ObserveCompletion(U64 queue,U64 fence)noexcept {
  if(quarantined_||!queue_||queue!=queue_||fence==UINT64_MAX||fence<completed_)return Quarantine();
  // The private fence may also cover newer invocations: values above this
  // invocation's required cut are valid. It must not be a native shared fence.
  completed_=fence;return true;
 }
 bool Seal(U64 list,U64 generation)noexcept {
  bool found=false;
  for(auto& r:recordings_)if(r.list==list&&r.generation==generation){r.sealed=true;found=true;}
  return found&&!quarantined_;
 }
 // Only after the host guarantees no consumer commands were/will be recorded.
 // An already attached consumer must instead be sealed after successful Reset.
 bool CancelUnattachedConsumer()noexcept {
  if(!frame_||recordings_[1].list||quarantined_)return false;
  cancelled_=true;return true;
 }
 bool Retirable()const noexcept {
  if(!frame_||quarantined_||pending_||!recordings_[0].sealed||completed_<required_)return false;
  return cancelled_||(recordings_[1].list&&recordings_[1].sealed);
 }
 bool SubmittedBoth()const noexcept {return !quarantined_&&recordings_[0].executed&&recordings_[1].executed;}
 bool Quarantine()noexcept {quarantined_=true;return false;}
 bool Quarantined()const noexcept{return quarantined_;}
 U64 RequiredFence()const noexcept{return required_;}
};
}
