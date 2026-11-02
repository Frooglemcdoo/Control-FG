#include "../src/gi_reuse/buffer_range_index.h"
#include <cassert>
#include <random>
#include <memory>
#include <cstdio>
using namespace gi_reuse;
int main(){
 auto index=std::make_unique<BufferRangeIndex<8192>>();
 for(std::size_t i=0;i<6000;++i)assert(index->Insert(1024+i*256,64,i));
 std::size_t slot=0,examined=0;
 for(std::size_t i=0;i<6000;++i){assert(index->Find(1024+i*256+16,48,slot,&examined)==RangeMatch::Unique&&slot==i&&examined==1);assert(index->Find(1024+i*256+16,49,slot)==RangeMatch::Missing);}
 assert(index->Insert(1024,64,7000));assert(index->Find(1024,64,slot)==RangeMatch::Ambiguous);assert(index->Remove(7000));
 assert(index->Find(UINT64_MAX,2,slot)==RangeMatch::Invalid);assert(index->Find(0,1,slot)==RangeMatch::Invalid);
 // Compare pruning with exhaustive containment over nested/overlapping ranges,
 // including removals and new occupants of recycled slots.
 BufferRangeIndex<128> overlap;struct R{std::uint64_t begin=0,bytes=0;};std::array<R,128> ranges{};std::mt19937 rng(290);
 for(unsigned round=0;round<100;++round){
  const auto changed=rng()%128;if(ranges[changed].bytes)assert(overlap.Remove(changed));
  ranges[changed]={1+rng()%4096,1+rng()%2048};assert(overlap.Insert(ranges[changed].begin,ranges[changed].bytes,changed));
  for(unsigned query=0;query<1000;++query){const std::uint64_t address=1+rng()%8192,bytes=1+rng()%512;unsigned matches=0;std::size_t expected=0;
   for(std::size_t i=0;i<ranges.size();++i){const auto& r=ranges[i];if(r.bytes&&address>=r.begin&&address+bytes<=r.begin+r.bytes){++matches;expected=i;}}
   const auto actual=overlap.Find(address,bytes,slot);assert(actual==(matches>1?RangeMatch::Ambiguous:matches?RangeMatch::Unique:RangeMatch::Missing));if(matches==1)assert(slot==expected);
  }
 }
 std::puts("PASS 6000 indexed ranges, single-range pruning, overlap ambiguity, bounds, removal/reuse, 100000 exhaustive-oracle comparisons");
}
