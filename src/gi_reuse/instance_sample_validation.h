#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
#include <array>
namespace gi_reuse {
struct InstanceRecord {float transform[12]{};std::uint32_t id=0,mask=0,hitGroup=0,flags=0;std::uint64_t blas=0;};
inline bool DecodeInstance(const unsigned char* bytes,InstanceRecord& out) noexcept {
 if(!bytes){return false;}std::memcpy(out.transform,bytes,48);std::uint32_t a=0,b=0;
 std::memcpy(&a,bytes+48,4);std::memcpy(&b,bytes+52,4);std::memcpy(&out.blas,bytes+56,8);
 out.id=a&0xffffff;out.mask=a>>24;out.hitGroup=b&0xffffff;out.flags=b>>24;return true;
}
struct InstanceSampleSummary {std::uint64_t hash=14695981039346656037ull;std::uint32_t count=0,masked=0,nullBLAS=0,badAlignment=0,nonfinite=0,unknownFlags=0;std::array<InstanceRecord,8> first{};};
inline bool ValidateInstanceSample(const unsigned char* bytes,std::uint64_t size,InstanceSampleSummary& out) noexcept {
 out={};if(!bytes||!size||size%64||size>4*1024*1024)return false;out.count=static_cast<std::uint32_t>(size/64);
 for(std::uint64_t i=0;i<size;++i){out.hash^=bytes[i];out.hash*=1099511628211ull;}
 for(std::uint32_t i=0;i<out.count;++i){InstanceRecord r;DecodeInstance(bytes+std::uint64_t(i)*64,r);if(i<out.first.size())out.first[i]=r;
  if(!r.mask){++out.masked;}if(!r.blas)++out.nullBLAS;else if(r.blas%256)++out.badAlignment;
  bool finite=true;for(float x:r.transform)if(!std::isfinite(x))finite=false;if(!finite)++out.nonfinite;
  if(r.flags&~15u)++out.unknownFlags;
 }return true;
}
}
