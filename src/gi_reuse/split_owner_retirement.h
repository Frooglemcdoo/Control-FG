#pragma once
#include "split_recording_lease.h"
#include <memory>
#include <new>
namespace gi_reuse {
// Host serializes calls. Recording identities come from actual Execute arrays
// and successful Reset generations, never from frame/Present guesses. Owns the
// complete invocation (scratch, snapshots and any separately proven scene lease).
// Does not establish scene immutability or create GPU completion evidence.
template<class Owner,unsigned Capacity=8,std::uint64_t Budget=768ull*1024*1024>
class GISplitOwnerRetirement {
 using U64=std::uint64_t;
public:
 struct Recording {U64 list=0,generation=0;};
 struct Ticket {U64 id=0,serial=0;};
 struct Batch {U64 queue=0;unsigned count=0;std::array<Ticket,Capacity> tickets{};};
private:
 struct Entry {
  U64 id=0,frame=0,bytes=0,queue=0,pending=0;
  GISplitRecordingLease lease;
  std::array<Recording,2> recordings{};
  std::array<bool,2> sealed{};
  std::unique_ptr<Owner> owner;
 };
 struct State {std::array<Entry,Capacity> entries{};U64 bytes=0,next=0,ticket=0;};
 std::unique_ptr<State> state_{new(std::nothrow) State};
 Entry* Find(U64 id)noexcept {
  if(state_&&id)for(auto& e:state_->entries)if(e.id==id)return &e;
  return nullptr;
 }
 static bool Same(Recording a,Recording b)noexcept{return a.list==b.list&&a.generation==b.generation;}
public:
 ~GISplitOwnerRetirement(){
  // No unknown in-flight resource release at DLL teardown. This is bounded by
  // admission and intentionally pins state until process/device teardown.
  if(state_)for(auto& e:state_->entries)if(e.owner&&!e.lease.Retirable()){(void)state_.release();break;}
 }
 U64 Admit(U64 frame,Recording producer,U64 bytes,std::unique_ptr<Owner>& owner)noexcept {
  if(!state_||!frame||!producer.list||!producer.generation||!owner||!bytes||
     bytes>Budget||state_->bytes>Budget-bytes||state_->next==UINT64_MAX)return 0;
  for(auto& e:state_->entries)if(!e.owner){
   Entry next;if(!next.lease.Begin(frame,producer.list,producer.generation))return 0;
   next.id=++state_->next;next.frame=frame;next.bytes=bytes;next.recordings[0]=producer;
   next.owner=std::move(owner);e=std::move(next);state_->bytes+=bytes;return e.id;
  }
  return 0;
 }
 bool Attach(U64 id,U64 frame,Recording consumer)noexcept {
  auto* e=Find(id);if(!e)return false;
  if(consumer.list==e->recordings[0].list&&!e->sealed[0])return false;
  if(!e->lease.AttachConsumer(frame,consumer.list,consumer.generation))return false;
  e->recordings[1]=consumer;return true;
 }
 // Atomic live admission: failed queue/submission checks must not leave an
 // attached consumer that prevents cancellation of the scene reader.
 bool AttachAndClaimConsumer(U64 id,U64 frame,U64 queue,Recording consumer)noexcept {
  auto* e=Find(id);if(!e)return false;
  if(consumer.list==e->recordings[0].list&&!e->sealed[0])return false;
  auto candidate=e->lease;
  if(!candidate.AttachConsumer(frame,consumer.list,consumer.generation)||
     !candidate.ClaimConsumerRecording(frame,queue,consumer.list,consumer.generation))return false;
  e->lease=candidate;e->recordings[1]=consumer;return true;
 }
 bool CanRecordConsumer(U64 id,U64 frame,U64 queue,Recording consumer)noexcept {
  const auto* e=Find(id);
  return e&&e->lease.CanRecordConsumer(frame,queue,consumer.list,consumer.generation);
 }
 bool ClaimConsumerRecording(U64 id,U64 frame,U64 queue,Recording consumer)noexcept {
  auto* e=Find(id);
  return e&&e->lease.ClaimConsumerRecording(frame,queue,consumer.list,consumer.generation);
 }
 bool CancelUnattached(U64 id)noexcept {auto* e=Find(id);return e&&e->lease.CancelUnattachedConsumer();}
 bool Seal(Recording recording)noexcept {
  bool found=false;if(state_)for(auto& e:state_->entries)if(e.owner)
   for(unsigned i=0;i<2;++i)if(recording.list&&Same(recording,e.recordings[i])){
    e.sealed[i]=true;found=e.lease.Seal(recording.list,recording.generation)||found;
   }
  return found;
 }
 Batch Before(U64 queue,const Recording* executed,unsigned count)noexcept {
  Batch batch;batch.queue=queue;if(!state_||!executed||!count)return batch;
  for(auto& e:state_->entries)if(e.owner){
   unsigned mask=0;bool invalid=false;
   for(unsigned j=0;j<count;++j)for(unsigned i=0;i<2;++i)
    if(executed[j].list&&Same(executed[j],e.recordings[i])){
     const unsigned bit=1u<<i;
     if((mask&bit)||e.sealed[i]||(i==0&&(mask&2)))invalid=true;
     mask|=bit;
    }
   if(!mask)continue;
   // This path uses separate submissions: COMMON decay and producer ordering
   // cannot be inferred for two lists in one ExecuteCommandLists call.
   if(invalid||mask==3||state_->ticket==UINT64_MAX||!e.lease.BeforeExecute(mask,queue)){
    e.lease.Quarantine();continue;
   }
   e.queue=queue;e.pending=++state_->ticket;
   batch.tickets[batch.count++]={e.id,e.pending};
  }
  return batch;
 }
 bool After(Batch& batch,U64 queue,U64 fence,bool signalled)noexcept {
  bool ok=batch.count!=0&&batch.count<=Capacity&&queue==batch.queue;
  if(batch.count>Capacity){QuarantineAll();batch={};return false;}
  for(unsigned i=0;i<batch.count;++i){auto* e=Find(batch.tickets[i].id);
   if(!e){ok=false;continue;}
   if(!e->pending||e->pending!=batch.tickets[i].serial||queue!=batch.queue){e->lease.Quarantine();ok=false;continue;}
   e->pending=0;if(!e->lease.AfterExecute(queue,fence,signalled))ok=false;
  }
  batch={};return ok;
 }
 void Complete(U64 queue,U64 fence)noexcept {
  if(state_)for(auto& e:state_->entries)if(e.owner&&e.queue==queue&&queue)e.lease.ObserveCompletion(queue,fence);
 }
 Owner* View(U64 id)noexcept {auto* e=Find(id);return e?e->owner.get():nullptr;}
 bool Take(U64 id,std::unique_ptr<Owner>& out)noexcept {
  auto* e=Find(id);if(!e||out||!e->lease.Retirable())return false;
  state_->bytes-=e->bytes;out=std::move(e->owner);*e=Entry{};return true;
 }
 // Transfer one fully retired owner for host reuse. Never destroy resources
 // while the submission router lock is held; caller owns the returned object.
 bool TakeReady(std::unique_ptr<Owner>& out)noexcept {
  if(!state_||out)return false;
  for(auto& entry:state_->entries)if(entry.owner&&entry.lease.Retirable())return Take(entry.id,out);
  return false;
 }
 bool SceneReleaseReady(U64 id)noexcept {auto* e=Find(id);return e&&e->lease.SceneReleaseReady();}
 bool Quarantined(U64 id)noexcept {auto* e=Find(id);return e&&e->lease.Quarantined();}
 // An untracked recording generation is relevant only if this list is ours.
 // Global queue hooks also see unrelated native/copy/Streamline command lists.
 void QuarantineList(U64 list)noexcept {
  if(state_&&list)for(auto& e:state_->entries)if(e.owner)
   for(const auto& recording:e.recordings)if(recording.list==list)e.lease.Quarantine();
 }
 void QuarantineAll()noexcept {if(state_)for(auto& e:state_->entries)if(e.owner)e.lease.Quarantine();}
 U64 RetainedBytes()const noexcept{return state_?state_->bytes:0;}
 // Explicit application/device stop only; never invoked merely on Present.
 void DeviceStopped()noexcept {if(state_){for(auto& e:state_->entries)e=Entry{};state_->bytes=0;}}
};
}
