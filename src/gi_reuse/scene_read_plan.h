#pragma once
#include "submission_ledger.h"
namespace gi_reuse {
enum class ReadKind:unsigned {Vertex,Index,Transform,Instances};
struct SceneReadRange {
 std::uint64_t asAddress=0,revision=0,address=0,bytes=0;
 std::uint32_t geometryIndex=0;
 ReadKind kind{};
};
// Metadata-only plan. Preserve lineage even when ranges overlap or copies alias.
// No resource state, stable byte generation or active scene membership is implied.
template<std::size_t Limit=65536> class SceneReadPlan {
 std::vector<SceneReadRange> ranges_;
 std::size_t blas_=0,tlas_=0;
 bool complete_=false;
public:
 template<std::size_t N> bool Build(const ASLedger<N>& snapshot) noexcept {
  ranges_.clear();blas_=tlas_=0;complete_=false;
  try {
   const bool valid=snapshot.Visit([&](const ASRecord& r){
    if(!r.revision||!r.count)return false;
    auto append=[&](std::uint64_t address,std::uint64_t bytes,std::uint32_t geometry,ReadKind kind){
     if(!address||!bytes||bytes>UINT64_MAX-address||ranges_.size()==Limit)return false;
     ranges_.push_back({r.address,r.revision,address,bytes,geometry,kind});return true;
    };
    if(r.topLevel){++tlas_;return append(r.instances,std::uint64_t(r.count)*64,0,ReadKind::Instances);}
    ++blas_;if(!r.geometry||r.geometry->Size()!=r.count)return false;
    for(std::uint32_t i=0;i<r.count;++i){const auto* t=r.geometry->At(i);
     if(!append(t->vertices,GeometryMetadata::VertexBytes(*t),i,ReadKind::Vertex)||
        !append(t->indices,std::uint64_t(t->indexCount)*2,i,ReadKind::Index)||
        (t->transform&&!append(t->transform,48,i,ReadKind::Transform)))return false;
    }
    return true;
   });
   if(valid&&!ranges_.empty()){complete_=true;return true;}
  }catch(...){}
  ranges_.clear();blas_=tlas_=0;return false;
 }
 bool Complete() const noexcept {return complete_;}
 const std::vector<SceneReadRange>& Ranges() const noexcept {return ranges_;}
 std::size_t BLASCount() const noexcept {return blas_;}
 std::size_t TLASCount() const noexcept {return tlas_;}
};
}
