#pragma once
#include "scene_producer_table.h"
namespace gi_reuse {
// CPU scene payloads attached to the actual submission revision. Nothing here
// reads GPU storage or substitutes recording order for queue execution order.
struct GISceneBuild {
 OrderedASEvent build;
 SceneByteView instances;
 std::vector<SceneASReference> bottoms; // build-time identities, revisions resolved at use
 std::vector<PTOwnedScene::Triangle> geometry;
 std::vector<std::shared_ptr<const GIFrozenGeometry>> frozen;
};
class GILiveSceneCatalog {
 using U64=std::uint64_t;
 OrderedASMetadata<> order_;
 std::map<U64,std::shared_ptr<const GISceneBuild>> payloads_;
public:
 struct Selection {
  SceneASReference top;
  SceneByteView instances;
  std::vector<SceneASReference> perInstance;
  std::vector<GISceneGeometry> geometry;
 };
 const OrderedASMetadata<>& History() const {return order_;}
 bool Healthy() const {return order_.Healthy();}
 void Invalidate(const char* reason){order_.Invalidate(reason);}
 U64 Mark(U64 queue){return order_.Mark(queue);}
 U64 TailStamp(U64 queue)const{return order_.TailStamp(queue);}
 bool Signal(U64 queue,U64 fence,U64 value,bool ok){return order_.Signal(queue,fence,value,ok);}
 bool Wait(U64 queue,U64 fence,U64 value,bool ok){return order_.Wait(queue,fence,value,ok);}
 bool Submit(U64 queue,U64 generation,std::shared_ptr<const GISceneBuild> input) {
  if(!input||!generation){Invalidate("live_payload_missing");return false;}
  auto recording=std::make_shared<OrderedASRecording>();recording->generation=generation;recording->events.push_back(input->build);
  std::vector<std::pair<U64,U64>> revisions;
  if(!order_.Submit(queue,{recording},&revisions))return false;
  if(revisions.size()!=1){Invalidate("live_revision_missing");return false;}
  const auto revision=revisions[0].first;
  if(!revision)return true; // Tombstone: old content stays inaccessible.
  if(input->build.event.operation==ASOperation::Copy){
   const auto source=payloads_.find(revisions[0].second);
   if(source==payloads_.end()){Invalidate("live_copy_payload_missing");return false;}
   input=source->second; // Share immutable source payload, preserve destination revision in order_.
  }
  payloads_.emplace(revision,std::move(input));return true;
 }
 bool Select(U64 cut,U64 epoch,U64 token,U64 address,Selection& output) const {
  Selection next;ASRecord top;
  if(!epoch||order_.Select(cut,token,address,top)!=PTSceneOrder::Status::ready||!top.topLevel)return false;
  const auto found=payloads_.find(top.revision);if(found==payloads_.end())return false;
  const auto& payload=*found->second;
  if(!payload.instances.Read(top.instances,U64(top.count)*64)||payload.bottoms.size()!=top.count)return false;
  next.top={address,token,top.revision};next.instances=payload.instances;next.instances.epoch=epoch;next.perInstance=payload.bottoms;
  std::set<std::pair<U64,U64>> visited;
  for(auto& ref:next.perInstance){
   if(!ref.address){if(ref.token)return false;ref.revision=0;continue;}
   ASRecord bottom;if(order_.Select(cut,ref.token,ref.address,bottom)!=PTSceneOrder::Status::ready||bottom.topLevel||!bottom.geometry)return false;
   ref.revision=bottom.revision;if(!visited.emplace(ref.token,ref.address).second)continue;
   const auto source=payloads_.find(bottom.revision);
   if(source==payloads_.end()||source->second->geometry.size()!=bottom.count)return false;
   const auto& frozen=source->second->frozen;
   if(!frozen.empty()&&frozen.size()!=bottom.count)return false;
   for(std::uint32_t g=0;g<bottom.count;++g)next.geometry.push_back({ref,g,source->second->geometry[g],frozen.empty()?nullptr:frozen[g]});
  }
  SceneTraversalClosure closure;
  if(ResolveTraversalClosure(order_,cut,epoch,next.top,next.instances,next.perInstance,closure)!=TraversalError::None)return false;
  output=std::move(next);return true;
 }
 // Must run between admission calls, with no external CPU cut retained.
 bool Compact(const std::map<U64,U64>& completed) {
  std::set<U64> retained;if(!order_.Compact(completed,retained))return false;
  for(auto p=payloads_.begin();p!=payloads_.end();)if(!retained.count(p->first))p=payloads_.erase(p);else ++p;
  return true;
 }
 bool Checkpoint(const std::map<U64,U64>& completed,bool outstandingCuts) {
  std::set<U64> retained;if(!order_.Checkpoint(completed,outstandingCuts,retained))return false;
  for(auto p=payloads_.begin();p!=payloads_.end();)if(!retained.count(p->first))p=payloads_.erase(p);else ++p;
  return true;
 }
};
}
