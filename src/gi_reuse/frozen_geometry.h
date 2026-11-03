#pragma once
#include "pt_shader_table_adapter.h"
#include <memory>
namespace gi_reuse {
// Immutable BLAS-build-time ranges. A storage owner must keep both copies alive;
// submission retains the transfer's source leases separately through completion.
struct GIFrozenRange {
 std::uint64_t original=0,bytes=0,address=0,capacity=0;
 bool Rebase(const PTSceneAdmission::View& source,std::uint64_t required,
             PTSceneAdmission::View& destination) const noexcept {
  if(!original||!bytes||!address||!capacity||bytes>capacity||
     !PTSceneAdmission::AddFits(original,bytes)||!PTSceneAdmission::AddFits(address,capacity)||
     source.address<original||source.address-original>bytes||required>bytes-(source.address-original)||
     !PTSceneAdmission::Contains(source,required))return false;
  destination={{address,capacity,source.allocation.epoch},address+(source.address-original)};return true;
 }
};
struct GIFrozenGeometry {
 GIFrozenRange indices,positions;
 std::shared_ptr<void> owner;
};
// Call ONLY after Convert has validated the original native row against its
// original allocation. Rewriting first would hide a stale/mismatched native SBT.
inline bool RebaseConvertedGeometry(const GIFrozenGeometry& frozen,
 PTSceneAdmission::Geometry& geometry,PTShaderTableAdapter::Bytes& record) noexcept {
 if(!frozen.owner)return false;
 auto next=geometry;PTSceneAdmission::Record original;
 if(PTSceneAdmission::Admit(geometry,original)!=PTSceneAdmission::Error::none)return false;
 std::uint64_t index=0,position=0;std::uint32_t offset=0;
 std::memcpy(&index,record.data()+32,8);std::memcpy(&position,record.data()+48,8);std::memcpy(&offset,record.data()+56,4);
 if(index!=original.indices||position!=original.positions||offset!=original.constants[0])return false;
 if(!frozen.indices.Rebase(geometry.indices,std::uint64_t(geometry.indexCount)*geometry.indexStride,next.indices)||
    !frozen.positions.Rebase(geometry.positions,std::uint64_t(geometry.vertexCount-1)*geometry.positionStride+12,next.positions))return false;
 PTSceneAdmission::Record admitted;if(PTSceneAdmission::Admit(next,admitted)!=PTSceneAdmission::Error::none)return false;
 auto converted=record;
 std::memcpy(converted.data()+32,&admitted.indices,8);std::memcpy(converted.data()+48,&admitted.positions,8);
 for(unsigned i:{0u,8u,10u,11u})std::memcpy(converted.data()+56+i*4,admitted.constants+i,4);
 geometry=next;record=converted;return true;
}
}
