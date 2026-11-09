#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>
#include <stdexcept>
#include <cmath>

namespace rt_ptlas {
// Byte layout of a conventional DXR instance. Slots remain native InstanceIndex
// values; user InstanceID is not a unique object/lifetime identifier.
struct Instance {
 std::array<float,12> transform{};
 std::uint32_t idMask=0, contributionFlags=0;
 std::uint64_t blas=0;
};
static_assert(sizeof(Instance)==64);
struct Write {
 std::array<float,12> transform{};
 std::array<float,6> bounds{};
 std::uint32_t id=0,mask=0,contribution=0,flags=0,index=0,partition=0;
 std::uint64_t blas=0;
};
static_assert(sizeof(Write)==104);
struct Plan {
 std::vector<Write> writes;
 std::vector<Instance> next;
 std::vector<std::uint32_t> partitions;
 std::uint64_t revision=0;
 bool initialize=false;
};
class Planner {
 std::vector<Instance> current_;
 std::vector<std::uint32_t> partitions_;
 std::uint32_t capacity_,partitionCount_;
 std::uint64_t revision_=0;
public:
 Planner(std::uint32_t capacity,std::uint32_t partitions):capacity_(capacity),partitionCount_(partitions) {
  if(!capacity||capacity>(1u<<24)||!partitions||partitions>(1u<<24))throw std::invalid_argument("PTLAS capacity");
 }
 Plan Prepare(const std::vector<Instance>& next,const std::vector<std::uint32_t>& partitions) const {
  if(next.size()>capacity_||next.size()!=partitions.size())throw std::invalid_argument("PTLAS input count");
  Plan p;p.next=next;p.partitions=partitions;p.revision=revision_;p.initialize=revision_==0;
  // Initialize EVERY reserved slot, including inactive slots. Shrinking a scene
  // explicitly deactivates removed slots; an omitted write preserves old data.
  const auto count=p.initialize?capacity_:static_cast<std::uint32_t>((std::max)(next.size(),current_.size()));
  p.writes.reserve(count);
  for(std::uint32_t i=0;i<count;++i){
   Instance value{};value.transform[0]=value.transform[5]=value.transform[10]=1;
   std::uint32_t partition=0;
   if(i<next.size()) {value=next[i];partition=partitions[i];}
   if(partition>=partitionCount_&&partition!=0xffffffffu)throw std::invalid_argument("PTLAS partition");
   if(value.blas%256 || (value.contributionFlags>>24)>15)throw std::invalid_argument("PTLAS instance flags/address");
   for(float f:value.transform)if(!std::isfinite(f))throw std::invalid_argument("PTLAS nonfinite transform");
   if(!p.initialize&&i<current_.size()&&i<next.size()&&partitions_[i]==partition&&
      std::memcmp(&current_[i],&value,sizeof(value))==0)continue;
   Write w;w.transform=value.transform;w.id=value.idMask&0xffffffu;w.mask=value.idMask>>24;
   w.contribution=value.contributionFlags&0xffffffu;w.flags=value.contributionFlags>>24;
   w.index=i;w.partition=partition;w.blas=value.blas;p.writes.push_back(w);
  }
  return p;
 }
 // Caller commits only after successful GPU submission/completion policy. A
 // discarded command list must not advance the history used by the next plan.
 void Commit(Plan&& p) {
  if(p.revision!=revision_)throw std::logic_error("PTLAS stale plan");
  current_=std::move(p.next);partitions_=std::move(p.partitions);++revision_;
 }
};
} // namespace rt_ptlas
