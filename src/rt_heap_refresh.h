#pragma once
#include <array>
#include <cstdint>
#include <algorithm>
namespace rt_heap_refresh {
using U64=std::uint64_t;using U32=std::uint32_t;
constexpr U32 Capacity=0x3b920,PageSize=256,Pages=(Capacity+PageSize-1)/PageSize;
struct Key {std::uintptr_t heap=0,object=0,cpu=0;};
struct Range {U32 start=0,count=0;};
struct Plan {std::array<Range,Pages> ranges{};U32 rangeCount=0,copied=0;bool full=true;};
struct Record {Key key{};U64 epoch=0,generation=0;U32 count=0;};
class Tracker {
 std::uintptr_t manager=0;U64 epoch=0,generation=0;
 std::array<U64,Pages> dirty{};
 std::array<Record,64> records{};
public:
 U64 resets=0,publications=0;
 void Observe(std::uintptr_t owner,U64 version){
  if(owner!=manager||version!=generation){manager=owner;generation=version;++epoch;++resets;dirty.fill(0);records.fill({});}
 }
 void Publish(std::uintptr_t owner,U64 before,U32 index){
  Observe(owner,before);
  if(before==UINT64_MAX){Observe(owner,0);++publications;return;}
  generation=before+1;++publications;
  if(index<Capacity)dirty[index/PageSize]=generation;
  else {++epoch;records.fill({});}
 }
 static bool Same(const Key& a,const Key& b){return a.heap==b.heap&&a.object==b.object&&a.cpu==b.cpu;}
 Plan Prepare(std::uintptr_t owner,Key key,U64 current,U64 heapVersion,U32 count,U32 oldCount){
  Observe(owner,current);Plan plan;
  const U32 total=std::max(count,oldCount);
  plan.copied=total;plan.ranges[0]={0,total};plan.rangeCount=total?1u:0u;
  if(total>Capacity||!key.heap||!key.object||!key.cpu)return plan;
  const Record* record=nullptr;
  for(const auto& entry:records)if(entry.epoch==epoch&&Same(entry.key,key)){record=&entry;break;}
  if(!record||record->generation!=heapVersion||record->generation>current||record->count!=oldCount||count!=oldCount)return plan;
  Plan partial;partial.full=false;
  for(U32 page=0;page<Pages&&page*PageSize<total;++page){
   if(dirty[page]<=record->generation)continue;
   const U32 start=page*PageSize,n=std::min(PageSize,total-start);
   if(partial.rangeCount&&partial.ranges[partial.rangeCount-1].start+partial.ranges[partial.rangeCount-1].count==start)partial.ranges[partial.rangeCount-1].count+=n;
   else partial.ranges[partial.rangeCount++]={start,n};
   partial.copied+=n;
  }
  // Avoid replacing one large driver call with many small ones.
  if(partial.copied>total/2||partial.rangeCount>32)return plan;
  return partial;
 }
 void Commit(Key key,U64 version,U32 count){
  if(count>Capacity||version!=generation)return;
  for(auto& entry:records)if(entry.epoch==epoch&&Same(entry.key,key)){entry={key,epoch,version,count};return;}
  for(auto& entry:records)if(entry.epoch!=epoch){entry={key,epoch,version,count};return;}
  // Bounded cache: an untracked heap keeps the full native copy.
 }
};
}
