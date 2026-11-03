#pragma once
#include "scene_traversal_closure.h"
#include "d3d12_as_owners.h"
namespace gi_reuse {
// Consume only a ResolveTraversalClosure success with its evidence kept alive.
// Host serializes scene metadata and preserves the snapshot's provenance. This
// transaction excludes registry retirement but does not prevent native GPU writes.
template<std::size_t Capacity,UINT64 Budget>
bool RetainTraversalClosure(ASResourceOwners<Capacity,Budget>& owners,
 const SceneTraversalClosure& closure,
 std::vector<typename ASResourceOwners<Capacity,Budget>::Lease>& retained) noexcept {
 retained.clear();
 if(!closure.cut||!closure.epoch||!closure.top.address||!closure.top.token||
    !closure.top.revision||closure.bottoms.size()>8192)return false;
 using Reference=typename ASResourceOwners<Capacity,Budget>::Reference;
 std::vector<Reference> refs;
 try {
  refs.reserve(closure.bottoms.size()+1);refs.push_back({closure.top.address,closure.top.token});
  for(const auto& b:closure.bottoms){if(!b.address||!b.token||!b.revision)return false;refs.push_back({b.address,b.token});}
 }catch(...){return false;}
 return owners.ResolveBatch(refs,retained);
}
}
