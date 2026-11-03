#pragma once
#include "submission_ledger.h"
#include "pt_scene_order.h"
#include <map>
#include <set>

namespace gi_reuse {
// CPU metadata adapter only. Host serializes actual queue operations and supplies
// lifetime IDs (not pointers) for allocations, queues and fences. No GPU-state,
// byte-snapshot, residency or completion claim follows from Select().
struct OrderedASEvent { ASEvent event; std::uint64_t destinationToken=0,sourceToken=0; };
struct OrderedASRecording {
 std::uint64_t generation=0;
 std::vector<OrderedASEvent> events;
};
template<std::size_t Capacity=65536> class OrderedASMetadata {
 using U64=std::uint64_t;
 using Key=PTSceneOrder::Key;
 struct Dependency {Key key;U64 cut=0,version=0;};
 struct Version {ASRecord record;Dependency dependency;};
 PTSceneOrder::Graph order_;
 std::map<U64,Version> versions_;
 std::set<U64> queues_;
 U64 serial_=0;
 bool healthy_=true;
 const char* failure_="none";
 bool Fail(const char* reason) {healthy_=false;failure_=reason;return false;}
 bool Valid(const ASRecord& r) const {
  if(!r.address||!r.count)return false;
  if(r.topLevel)return !r.geometry&&r.instances&&U64(r.count)<=((UINT64_MAX-r.instances)/64);
  return r.geometry&&r.count==r.geometry->Size();
 }
 bool DependencyValid(U64 version) const {
  std::set<U64> seen;
  while(version){
   if(!seen.insert(version).second)return false;
   const auto it=versions_.find(version);if(it==versions_.end())return false;
   const auto& d=it->second.dependency;if(!d.version)return true;
   PTSceneOrder::Selection selected;
   if(order_.Select(d.cut,d.key,selected)!=PTSceneOrder::Status::ready||selected.version!=d.version)return false;
   version=d.version;
  }
  return false;
 }
public:
 bool Healthy() const {return healthy_;}
 const char* FailureReason() const {return failure_;}
 void Invalidate(const char* reason="host_observation_failure"){Fail(reason);}
 // Freeze while recording is stable. The extra token vector is mandatory:
 // current R294 address-only journals cannot establish allocation identity.
 template<class Journal> static std::shared_ptr<const OrderedASRecording> Freeze(
  const Journal& journal,U64 expectedGeneration,const std::vector<std::pair<U64,U64>>& tokens) {
  if(!expectedGeneration||journal.generation!=expectedGeneration||!journal.complete||
     journal.size>Journal::Limit||tokens.size()!=journal.size)return {};
  auto out=std::make_shared<OrderedASRecording>();out->generation=expectedGeneration;
  out->events.reserve(journal.size);
  for(std::size_t i=0;i<journal.size;++i){
   const auto& e=journal.events[i];
   if(!tokens[i].first||!e.record.address)return {};
   if((e.operation==ASOperation::Copy||e.operation==ASOperation::Update)&&(!tokens[i].second||!e.source))return {};
   out->events.push_back({e,tokens[i].first,tokens[i].second});
  }
  return out;
 }
 // Feed immutable recordings in the exact Execute batch/list order. A closed
 // list may be replayed; each submission receives new metadata revisions.
 bool Submit(U64 queue,const std::vector<std::shared_ptr<const OrderedASRecording>>& batch,
             std::vector<std::pair<U64,U64>>* revisions=nullptr) {
  if(!healthy_)return false;
  if(!queue)return Fail("queue_identity");
  queues_.insert(queue);if(revisions)revisions->clear();
  for(const auto& r:batch)if(!r||!r->generation)return Fail("recording_missing");
  for(const auto& r:batch)for(const auto& item:r->events){
   const auto& e=item.event;const Key destination{item.destinationToken,e.record.address};
   if(!destination.token||!destination.address)return Fail("destination_identity");
   ASRecord next=e.record;Dependency dep{};bool admitted=false;
   if(e.operation==ASOperation::Build)admitted=Valid(next);
   else if(e.operation==ASOperation::Copy||e.operation==ASOperation::Update){
    const U64 cut=order_.Mark(queue);if(!cut)return Fail("order_capacity");
    const Key source{item.sourceToken,e.source};PTSceneOrder::Selection selected;
    if(source.token&&source.address&&order_.Select(cut,source,selected)==PTSceneOrder::Status::ready&&DependencyValid(selected.version)){
     const auto& previous=versions_.at(selected.version).record;
     if(e.operation==ASOperation::Copy){next=previous;next.address=destination.address;admitted=true;}
     else if(Valid(next)&&previous.allowUpdate&&next.topLevel==previous.topLevel&&next.count==previous.count&&
       (next.topLevel||next.geometry->CanUpdateFrom(*previous.geometry))){next.allowUpdate=true;admitted=true;}
     if(admitted)dep={source,cut,selected.version};
    }
   }else if(e.operation!=ASOperation::Invalidate)return Fail("unknown_operation");
   U64 version=0;
   if(admitted){
    if(versions_.size()>=Capacity||serial_==UINT64_MAX)return Fail("version_capacity");
    next.revision=version=++serial_;versions_.emplace(version,Version{next,dep});
   }
   // Unsupported/unresolved writes hide older data. Never silently retain an
   // earlier geometry version when the new write cannot be reconstructed.
   if(!order_.Write(queue,destination,version,"gi_unavailable_write"))return Fail("order_capacity");
   if(revisions)revisions->emplace_back(version,dep.version);
  }
  return true;
 }
 U64 Mark(U64 queue){if(!healthy_)return 0;queues_.insert(queue);const auto cut=order_.Mark(queue);if(!cut)Fail("mark_failed");return cut;}
 bool Signal(U64 queue,U64 fence,U64 value,bool succeeded){
  if(!healthy_)return false;
  if(!succeeded)return Fail("signal_failed");
  queues_.insert(queue);return order_.Signal(queue,fence,value)!=0||Fail("signal_contract");
 }
 bool Wait(U64 queue,U64 fence,U64 value,bool succeeded){
  if(!healthy_)return false;
  if(!succeeded)return Fail("wait_failed");
  queues_.insert(queue);return order_.Wait(queue,fence,value)!=0||Fail("wait_contract");
 }
 U64 TailStamp(U64 queue) const {return order_.TailStamp(queue);}
 std::size_t VersionCount() const {return versions_.size();}
 // Caller supplies fence-proven completed tail stamps and no outstanding cuts.
 // This is polling only: never wait on the GPU to obtain a checkpoint.
 // Return retained revision IDs so the host prunes matching payload snapshots.
 bool Checkpoint(const std::map<U64,U64>& completed,bool outstandingCuts,std::set<U64>& retained) {
  retained.clear();if(!healthy_||outstandingCuts)return false;
  for(const auto q:queues_){const auto found=completed.find(q);
   if(found==completed.end()||found->second<order_.TailStamp(q))return false;}
  std::map<Key,PTSceneOrder::Selection> latest;
  if(!order_.CompletedCheckpoint(latest))return false;
  std::map<U64,Version> next;std::map<Key,U64> baseline;
  for(const auto& item:latest){const auto revision=item.second.version;baseline.emplace(item.first,revision);
   if(!revision)continue;
   if(!DependencyValid(revision))return false;
   const auto found=versions_.find(revision);if(found==versions_.end())return false;
   next.emplace(revision,Version{found->second.record,{}});retained.insert(revision);
  }
  if(!order_.RebaseCompleted(baseline)){retained.clear();return false;}
  versions_=std::move(next);return true;
 }
 PTSceneOrder::Status Select(U64 cut,U64 token,U64 address,ASRecord& out) const {
  out={};if(!healthy_||!token||!address)return PTSceneOrder::Status::invalid;
  PTSceneOrder::Selection selected;const auto status=order_.Select(cut,{token,address},selected);
  if(status!=PTSceneOrder::Status::ready)return status;
  if(!DependencyValid(selected.version))return PTSceneOrder::Status::invalid;
  out=versions_.at(selected.version).record;return status;
 }
};
}
