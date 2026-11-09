#pragma once
#include "rt_as_build_route.h"
#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>

// CPU ownership contract, not an enabled D3D12 adapter. All handles identify a
// generation, never just an address. The adapter supplies real COM owners and
// exact queue/fence identities, and calls PrepareSubmit BEFORE ExecuteCommandLists.
// Destroy this ledger only after device teardown or a proven device-idle drain.
namespace rt_as_ownership {
using Id=std::uint64_t;
enum class Owner {Native, Replacement};
enum class Route {Native, Replacement, Abort};
struct Key {Id address=0,generation=0;};
struct Allocation {
 Key key;Id bytes=0,mappedAddress=0;Owner owner=Owner::Native;
 std::shared_ptr<void> lifetime;
};
using Lease=std::shared_ptr<const Allocation>;
class Ledger {
 struct List {bool closed=false;std::vector<Lease> leases;};
 enum class Stage {Prepared,Executing,Submitted,Quarantined};
 struct Submission {Stage stage=Stage::Prepared;Id queue=0,fence=0,value=0;std::vector<Lease> leases;};
 mutable std::mutex mutex_;
 Id next_=1;
 std::map<Id,Lease> active_;
 std::vector<std::weak_ptr<const Allocation>> retired_;
 std::map<Id,List> lists_;
 std::map<Id,Submission> submissions_;
 std::map<std::pair<Id,Id>,Id> issued_,completed_;
 static bool Overlap(Id a,Id n,Id b,Id m){return a<b+m&&b<a+n;}
 Lease Find(Key key)const {
  auto found=active_.find(key.address);
  if(found==active_.end()||found->second->key.generation!=key.generation)throw std::logic_error("stale AS allocation");
  return found->second;
 }
public:
 Key Register(Id address,Id bytes,std::shared_ptr<void> lifetime,bool requestReplacement,
              const rt_as_build_route::Coverage& coverage,Id mappedAddress=0){
  std::lock_guard<std::mutex> guard(mutex_);
  if(!address||!bytes||bytes>UINT64_MAX-address||!lifetime)throw std::invalid_argument("AS allocation");
  for(const auto& item:active_)if(Overlap(address,bytes,item.first,item.second->bytes))throw std::logic_error("overlapping live AS allocation");
  retired_.erase(std::remove_if(retired_.begin(),retired_.end(),[](const auto& p){return p.expired();}),retired_.end());
  for(const auto& weak:retired_)if(const auto p=weak.lock())if(Overlap(address,bytes,p->key.address,p->bytes))throw std::logic_error("AS alias still retained");
  const bool replace=requestReplacement&&coverage.Complete();
  if(replace&&(!mappedAddress||mappedAddress==address))throw std::invalid_argument("replacement requires separate owned storage");
  auto p=std::make_shared<Allocation>();p->key={address,next_++};p->bytes=bytes;p->owner=replace?Owner::Replacement:Owner::Native;
  p->mappedAddress=replace?mappedAddress:address;p->lifetime=std::move(lifetime);active_.emplace(address,p);return p->key;
 }
 Route Select(Key key,bool replacementReady)const {
  std::lock_guard<std::mutex> guard(mutex_);const auto p=Find(key);
  if(p->owner==Owner::Native)return Route::Native;
  return replacementReady?Route::Replacement:Route::Abort; // Never native fallback.
 }
 Id Resolve(Key key)const {std::lock_guard<std::mutex> guard(mutex_);return Find(key)->mappedAddress;}
 void Retire(Key key){
  std::lock_guard<std::mutex> guard(mutex_);auto p=Find(key);
  retired_.push_back(p);active_.erase(key.address); // Existing recordings/submissions keep their lease.
 }
 Id BeginList(){std::lock_guard<std::mutex> guard(mutex_);const auto id=next_++;lists_.emplace(id,List{});return id;}
 void Reference(Id list,Key key){
  std::lock_guard<std::mutex> guard(mutex_);auto& entry=lists_.at(list);
  if(entry.closed)throw std::logic_error("closed AS recording");
  auto lease=Find(key);if(std::find(entry.leases.begin(),entry.leases.end(),lease)==entry.leases.end())entry.leases.push_back(std::move(lease));
 }
 void Close(Id list){std::lock_guard<std::mutex> guard(mutex_);auto& entry=lists_.at(list);if(entry.closed)throw std::logic_error("already closed");entry.closed=true;}
 void Discard(Id list){std::lock_guard<std::mutex> guard(mutex_);if(!lists_.erase(list))throw std::logic_error("stale AS recording");}
 Id PrepareSubmit(const std::vector<Id>& lists,Id queue,Id fence,Id value){
  std::lock_guard<std::mutex> guard(mutex_);
  if(lists.empty()||!queue||!fence||!value)throw std::invalid_argument("AS submission identity");
  const auto timeline=std::make_pair(queue,fence);const auto previous=issued_.find(timeline);
  if(previous!=issued_.end()&&value<=previous->second)throw std::logic_error("nonmonotonic private fence");
  // Fence identity cannot be shared between queues in this single-writer model.
  for(const auto& item:issued_)if(item.first.second==fence&&item.first.first!=queue)throw std::logic_error("fence has another queue owner");
  Submission pending;pending.queue=queue;pending.fence=fence;pending.value=value;
  for(Id id:lists){const auto& list=lists_.at(id);if(!list.closed)throw std::logic_error("open AS recording");
   for(const auto& lease:list.leases)if(std::find(pending.leases.begin(),pending.leases.end(),lease)==pending.leases.end())pending.leases.push_back(lease);
  }
  // All allocations and references precede native Execute/Signal. A later
  // failure must keep Prepared/Quarantined entries alive, not erase them.
  const auto id=next_++;issued_[timeline]=value;submissions_.emplace(id,std::move(pending));return id;
 }
 void Executing(Id id){std::lock_guard<std::mutex> guard(mutex_);auto& p=submissions_.at(id);if(p.stage!=Stage::Prepared)throw std::logic_error("submission state");p.stage=Stage::Executing;}
 void Submitted(Id id){std::lock_guard<std::mutex> guard(mutex_);auto& p=submissions_.at(id);if(p.stage!=Stage::Executing)throw std::logic_error("submission state");p.stage=Stage::Submitted;}
 void Quarantine(Id id){std::lock_guard<std::mutex> guard(mutex_);submissions_.at(id).stage=Stage::Quarantined;}
 void CancelBeforeExecute(Id id){std::lock_guard<std::mutex> guard(mutex_);auto i=submissions_.find(id);if(i==submissions_.end()||i->second.stage!=Stage::Prepared)throw std::logic_error("cannot cancel executed submission");submissions_.erase(i);}
 void Complete(Id queue,Id fence,Id value){
  std::lock_guard<std::mutex> guard(mutex_);const auto timeline=std::make_pair(queue,fence);
  const auto issued=issued_.find(timeline);if(issued==issued_.end()||value>issued->second)throw std::logic_error("unknown fence completion");
  auto& prior=completed_[timeline];if(value<prior)throw std::logic_error("regressed fence completion");prior=value;
  for(auto i=submissions_.begin();i!=submissions_.end();){const auto& p=i->second;
   if(p.stage==Stage::Submitted&&p.queue==queue&&p.fence==fence&&p.value<=value)i=submissions_.erase(i);else ++i;
  }
 }
 std::size_t Pending()const {std::lock_guard<std::mutex> guard(mutex_);return submissions_.size();}
};
}
