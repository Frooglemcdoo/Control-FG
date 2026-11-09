#pragma once
#include "scene_hit_packet.h"
#include <unordered_map>
#include <algorithm>
namespace gi_reuse {
// Offline admission of a COMPLETE instance array at one ordered TLAS version.
// Host-supplied allocation identities must have been captured at this version;
// current-address lookup is not a substitute. This does not retain GPU resources.
struct SceneASReference {
 std::uint64_t address=0,token=0,revision=0;
};
struct SceneTraversalClosure {
 SceneASReference top;
 std::uint64_t cut=0,epoch=0;
 std::vector<SceneASReference> bottoms;
 ASRecord topRecord;
 std::vector<ASRecord> bottomRecords; // Exact records already validated at this cut.
};
enum class TraversalError {None,Identity,Order,Version,Instances,Capacity,Bottom};
template<std::size_t Capacity> TraversalError ResolveTraversalClosure(
 const OrderedASMetadata<Capacity>& history,std::uint64_t cut,std::uint64_t epoch,
 SceneASReference top,const SceneByteView& instances,
 const std::vector<SceneASReference>& perInstance,SceneTraversalClosure& result,std::vector<std::uint32_t>* instanceBottom=nullptr) {
 result={};
 if(!cut||!epoch||!top.address||!top.token||!top.revision)return TraversalError::Identity;
 ASRecord record;
 if(history.Select(cut,top.token,top.address,record)!=PTSceneOrder::Status::ready)return TraversalError::Order;
 if(record.revision!=top.revision)return TraversalError::Version;
 if(!record.topLevel||!record.count||record.count>65536)return TraversalError::Capacity;
 if(instances.epoch!=epoch||perInstance.size()!=record.count||
    std::uint64_t(record.count)>(UINT64_MAX-record.instances)/64)return TraversalError::Instances;
 const auto* bytes=instances.Read(record.instances,std::uint64_t(record.count)*64);
 if(!bytes)return TraversalError::Instances;
 SceneTraversalClosure out;out.top=top;out.cut=cut;out.epoch=epoch;out.topRecord=record;
 std::vector<std::uint32_t> slots;
 if(instanceBottom)slots.resize(record.count);
 std::unordered_map<std::uint64_t,std::uint32_t> unique;
 unique.reserve((std::min)(record.count,8192u));
 for(std::uint32_t i=0;i<record.count;++i){
  InstanceRecord instance;if(!DecodeInstance(bytes+std::size_t(i)*64,instance))return TraversalError::Instances;
  // Retain all non-null BLAS references even when their mask currently excludes
  // GI: the closure must not silently depend on a particular ray mask.
  const auto& ref=perInstance[i];
  if(!instance.blas){if(ref.address||ref.token||ref.revision)return TraversalError::Identity;continue;}
  if(instance.blas%256||ref.address!=instance.blas||!ref.token||!ref.revision)return TraversalError::Identity;
  const auto prior=unique.find(ref.address);
  if(prior!=unique.end()){
   const auto& existing=out.bottoms[prior->second];
   if(existing.token!=ref.token||existing.revision!=ref.revision)return TraversalError::Identity;
   if(instanceBottom)slots[i]=prior->second;
   continue;
  }
  if(unique.size()==8192)return TraversalError::Capacity;
  ASRecord bottom;
  if(history.Select(cut,ref.token,ref.address,bottom)!=PTSceneOrder::Status::ready)return TraversalError::Order;
  if(bottom.revision!=ref.revision)return TraversalError::Version;
  if(bottom.topLevel||!bottom.geometry||bottom.count!=bottom.geometry->Size())return TraversalError::Bottom;
  const auto slot=static_cast<std::uint32_t>(out.bottoms.size());
  unique.emplace(ref.address,slot);if(instanceBottom)slots[i]=slot;out.bottoms.push_back(ref);out.bottomRecords.push_back(std::move(bottom));
 }
 if(instanceBottom)*instanceBottom=std::move(slots);
 result=std::move(out);return TraversalError::None;
}
}
