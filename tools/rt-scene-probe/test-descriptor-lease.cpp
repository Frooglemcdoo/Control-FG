#include "../../src/rt_descriptor_lease.h"
#include <cassert>
#include <cstdio>
#include <map>
#include <vector>
using namespace rt_descriptor_lease;
static int manager,state,timeline,owner;
static Snapshot identity{&manager,&state,&timeline,1,1,32};
static U64 cursor=0,allocations=0,lastCount=0,ownerEpoch=1,ownerVersion=1;
static void Allocate(void*,U64 count,U64* gpu,U64* cpu,void** heap){++allocations;lastCount=count;*gpu=0x100000+cursor*32;*cpu=0x800000+cursor*32;*heap=&owner;cursor+=count;ownerEpoch=identity.epoch;ownerVersion=identity.version;}
static Snapshot Current(){return identity;}
static bool Valid(const Block& block,const Snapshot& value){return block.owner==&owner&&ownerEpoch==value.epoch&&ownerVersion==value.version;}
int main(){
 Api api{Allocate,Current,Valid};U64 tables=0,hits=0;
 for(unsigned j=0;j<2048;++j){
  Lease lease;std::map<U64,unsigned> contents;std::vector<Block> outputs;
  const unsigned n=1+j%127;const auto before=allocations;
  for(unsigned i=0;i<n;++i){Block out;hits+=Take(api,lease,nullptr,49,out)?1u:0u;++tables;outputs.push_back(out);
   for(U64 k=0;k<49;++k)assert(contents.emplace(out.cpu+k*32,i+1).second);
   // Unrelated native reservations interleave without claiming leased slots.
   if(i%5==0){Block external;Allocate(nullptr,49,&external.gpu,&external.cpu,&external.owner);for(U64 k=0;k<49;++k)assert(contents.emplace(external.cpu+k*32,0).second);}
  }
  assert(allocations-before==(n+15)/16+(n+4)/5);
  for(unsigned i=0;i<n;++i)for(U64 k=0;k<49;++k)assert(contents.at(outputs[i].cpu+k*32)==i+1);
 }
 Lease lease;Block out;assert(!Take(api,lease,nullptr,49,out));assert(Take(api,lease,nullptr,49,out));
 ++identity.version;assert(!Take(api,lease,nullptr,49,out));assert(Take(api,lease,nullptr,49,out));
 ++identity.epoch;assert(!Take(api,lease,nullptr,49,out));
 int newState;identity.state=&newState;assert(!Take(api,lease,nullptr,49,out));
 int newTimeline;identity.timeline=&newTimeline;assert(!Take(api,lease,nullptr,49,out));
 ++ownerEpoch;assert(!Take(api,lease,nullptr,49,out));
 ++ownerVersion;assert(!Take(api,lease,nullptr,49,out));
 assert(!Take(api,lease,nullptr,7,out)&&lastCount==7&&lease.used==16);
 identity.epoch=0;assert(!Take(api,lease,nullptr,49,out)&&lastCount==49&&lease.used==16);
 assert(!Span({UINT64_MAX-10,0x800000,&owner},32));assert(!Span({0x100000,0x800000,&owner},UINT64_MAX));
 assert(hits>tables*8/10);
 std::printf("PASS %llu job-scoped tables: disjoint contents survive interleaved native allocations; job/epoch/version/context/timeline/owner boundaries and overflow rejection. Native retirement retained. Not an FPS benchmark.\n",static_cast<unsigned long long>(tables));
}
