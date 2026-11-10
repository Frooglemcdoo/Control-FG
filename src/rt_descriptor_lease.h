#pragma once
#include <cstdint>
#include <limits>
namespace rt_descriptor_lease {
using U64=std::uint64_t;
constexpr U64 TableSize=49,TablesPerLease=16;
struct Snapshot {
 void* manager=nullptr;void* state=nullptr;void* timeline=nullptr;
 U64 epoch=0,version=0,step=0;
 bool operator==(const Snapshot& b)const{return manager==b.manager&&state==b.state&&timeline==b.timeline&&epoch==b.epoch&&version==b.version&&step==b.step;}
 bool valid()const{return manager&&state&&timeline&&epoch&&step;}
};
struct Block {U64 gpu=0,cpu=0;void* owner=nullptr;};
struct Lease {Snapshot identity{};Block block{};U64 used=TablesPerLease;};
struct Api {
 void(*allocate)(void*,U64,U64*,U64*,void**);
 Snapshot(*snapshot)();bool(*validBlock)(const Block&,const Snapshot&);
};
inline bool Span(const Block& b,U64 step){
 constexpr auto maximum=(std::numeric_limits<U64>::max)();
 return b.owner&&b.gpu&&b.cpu&&step&&step<=maximum/(TableSize*TablesPerLease)&&
  b.gpu<=maximum-step*TableSize*TablesPerLease&&b.cpu<=maximum-step*TableSize*TablesPerLease;
}
// A lease belongs to one native recording job. Every take returns a fresh,
// disjoint table. Native allocation retains ownership and retirement.
inline bool Take(const Api& api,Lease& lease,void* argument,U64 count,Block& output){
 const auto current=api.snapshot();
 if(count!=TableSize||!current.valid()){
  lease.used=TablesPerLease;api.allocate(argument,count,&output.gpu,&output.cpu,&output.owner);return false;
 }
 if(lease.used<TablesPerLease&&current==lease.identity&&api.validBlock(lease.block,current)){
  output=lease.block;const auto offset=lease.used*TableSize*current.step;
  output.gpu+=offset;output.cpu+=offset;++lease.used;return true;
 }
 lease.used=TablesPerLease;
 api.allocate(argument,TableSize*TablesPerLease,&output.gpu,&output.cpu,&output.owner);
 const auto after=api.snapshot();
 if(current==after&&Span(output,current.step)&&api.validBlock(output,current)){
  lease.identity=current;lease.block=output;lease.used=1;
 }
 return false;
}
}
