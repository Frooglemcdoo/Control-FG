#pragma once
#include "pt_scene_order.h"
#include <limits>

namespace gi_reuse {
// Mutation evidence only. Reuses the existing queue/fence graph; COPY queues
// have exactly the same ordering requirements as direct/compute queues.
// The host serializes calls, supplies lifetime IDs, observes every relevant
// writer, and retains resources/recordings with the existing ownership ledger.
// A stamp does not own bytes, establish completion, or protect a future read.
class OrderedBufferWrites {
 using U64=std::uint64_t;
 struct Allocation {U64 address=0,bytes=0;bool cpuWritten=false,retired=false;};
 PTSceneOrder::Graph order_;
 std::map<U64,Allocation> allocations_;
 U64 revision_=0;
 bool healthy_=true;
 const char* failure_="none";
 bool Fail(const char* reason){healthy_=false;failure_=reason;return false;}
 const Allocation* Find(U64 token)const {
  const auto i=allocations_.find(token);
  return i==allocations_.end()||i->second.retired?nullptr:&i->second;
 }
public:
 struct Write {U64 token=0,offset=0,bytes=0;bool wholeResource=false;};
 struct Recording {U64 generation=0;bool complete=true;std::vector<Write> writes;};
 struct Stamp {U64 token=0,revision=0;};
 static constexpr std::size_t MaximumAllocations=65536;
 bool Healthy()const{return healthy_;}
 const char* FailureReason()const{return failure_;}
 void Invalidate(const char* reason){Fail(reason);}
 bool Register(U64 token,U64 address,U64 bytes){
  if(!healthy_)return false;
  if(!token||!address||!bytes||bytes>UINT64_MAX-address||allocations_.count(token))return Fail("buffer_identity");
  if(allocations_.size()>=MaximumAllocations)return Fail("buffer_identity_capacity");
  allocations_.emplace(token,Allocation{address,bytes});return true;
 }
 // Tokens are never recycled, even after retirement. VA reuse with a NEW token
 // is supported, but the host must independently exclude physical alias races.
 bool Retire(U64 token){
  if(!healthy_)return false;
  const auto i=allocations_.find(token);
  if(i==allocations_.end()||i->second.retired)return Fail("buffer_retire_identity");
  i->second.retired=true;return true;
 }
 // Call before exposing CPU-writable storage. This only rejects admission;
 // it cannot stop native CPU writes or repair already executing GPU reads.
 // No EndWrite unpoisons this allocation: a CPU return/Unmap is not a GPU edge.
 bool CpuWrite(U64 token){
  if(!healthy_)return false;
  const auto i=allocations_.find(token);
  if(i==allocations_.end()||i->second.retired)return Fail("cpu_write_identity");
  i->second.cpuWritten=true;return true;
 }
 // Immutable recordings arrive in actual Execute/list order, not Close order.
 // Replay is a new write revision. Reset/discarded recordings are NOT submitted.
 // No byte-content provenance is inferred from the source of a copy.
 bool Submit(U64 queue,const std::vector<Recording>& batch){
  if(!healthy_)return false;
  if(!queue)return Fail("buffer_queue_identity");
  // Validate the complete batch before adding its first graph write.
  for(const auto& r:batch){
   if(!r.generation||!r.complete)return Fail("buffer_recording_incomplete");
   for(const auto& w:r.writes){const auto* a=Find(w.token);
    if(!a)return Fail("buffer_write_identity");
    if(w.wholeResource){if(w.offset||w.bytes)return Fail("buffer_whole_copy_arguments");}
    else if(w.offset>a->bytes||w.bytes>a->bytes-w.offset)return Fail("buffer_write_range");
   }
  }
  for(const auto& r:batch)for(const auto& w:r.writes){
   if(!w.wholeResource&&!w.bytes)continue;
   if(revision_==UINT64_MAX)return Fail("buffer_revision_overflow");
   const auto* a=Find(w.token);
   // Conservatively invalidate the entire allocation, including writes outside
   // the sampled subrange. Range-aware reuse may narrow this only with proof.
   if(!order_.Write(queue,{w.token,a->address},++revision_))return Fail("buffer_order_capacity");
  }
  return true;
 }
 U64 Mark(U64 queue){if(!healthy_)return 0;const auto cut=order_.Mark(queue);if(!cut)Fail("buffer_mark_failed");return cut;}
 bool Signal(U64 queue,U64 fence,U64 value,bool succeeded){
  if(!healthy_)return false;
  return succeeded?(order_.Signal(queue,fence,value)!=0||Fail("buffer_signal_contract")):Fail("buffer_signal_failed");
 }
 bool Wait(U64 queue,U64 fence,U64 value,bool succeeded){
  if(!healthy_)return false;
  return succeeded?(order_.Wait(queue,fence,value)!=0||Fail("buffer_wait_contract")):Fail("buffer_wait_failed");
 }
 U64 ObserveFenceCompletion(U64 fence,U64 completed){return healthy_?order_.ObserveFenceCompletion(fence,completed):0;}
 U64 TailStamp(U64 queue)const{return order_.TailStamp(queue);}
 PTSceneOrder::Status Capture(U64 cut,U64 token,Stamp& out)const {
  out={};const auto* a=Find(token);
  if(!healthy_||!a||a->cpuWritten)return PTSceneOrder::Status::invalid;
  PTSceneOrder::Selection selected;
  const auto status=order_.Select(cut,{token,a->address},selected);
  if(status==PTSceneOrder::Status::ready)out={token,selected.version};
  return status;
 }
 bool Unchanged(U64 cut,const Stamp& before)const {
  Stamp current;return before.token&&before.revision&&Capture(cut,before.token,current)==PTSceneOrder::Status::ready&&
   current.revision==before.revision;
 }
 // Reuse the graph's nonblocking compaction, including remapping external cuts.
 // Allocation registrations are bounded and are deliberately not age-evicted.
 bool Compact(const std::set<U64>& cuts,const std::map<U64,U64>& completed,std::map<U64,U64>& remap){
  remap.clear();if(!healthy_)return false;std::set<U64> retained;
  return order_.Compact(cuts,completed,remap,retained);
 }
};
}
