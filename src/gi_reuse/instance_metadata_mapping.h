#pragma once
#include "instance_sample_validation.h"
#include "submission_ledger.h"
namespace gi_reuse {
struct InstanceMappingSummary {std::uint32_t count=0,matched=0,missing=0,nullBLAS=0,wrongType=0,noGeometry=0;};
inline bool MapInstanceMetadata(const unsigned char* bytes,std::uint64_t size,
 const ASMetadataCut& cut,InstanceMappingSummary& out) noexcept {
 out={};if(!bytes||!size||size%64||size>4*1024*1024||!cut.queue||!cut.fence)return false;
 out.count=static_cast<std::uint32_t>(size/64);
 for(std::uint32_t i=0;i<out.count;++i){InstanceRecord instance;DecodeInstance(bytes+std::uint64_t(i)*64,instance);
  if(!instance.blas){++out.nullBLAS;continue;}
  const auto* r=cut.Find(instance.blas);
  if(!r){++out.missing;continue;}if(r->topLevel){++out.wrongType;continue;}
  if(!r->geometry||r->geometry->Size()!=r->count){++out.noGeometry;continue;}
  ++out.matched;
 }return true;
}
}
