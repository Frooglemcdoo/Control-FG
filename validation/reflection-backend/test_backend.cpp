#include "rr_reflection_backend.h"
#include <cassert>
#include <map>
#include <vector>
#include <iostream>
using namespace control_rr_reflection;
struct Api {
 Context current{1,90,91,92,93};
 std::map<Address,unsigned> refs;
 std::vector<Transition> barriers;
 std::vector<std::pair<Address,Address>> copies;
 std::uint64_t sizes[2]{1024,4096},completed=0,signal=0;
 unsigned allocations=0,creates=0,createFail=0,holds=0,holdFail=0,commands=0,commandFail=0,releases=0;
 bool signalFail=false,allocationInfoFail=false,budgetQueryFail=false;
 control_rr::MemoryBudget memory{8ull<<30,2ull<<30};
 unsigned budgetQueries=0;
 bool QueryBudget(Address,control_rr::MemoryBudget& out){++budgetQueries;out=memory;return !budgetQueryFail;}
 bool AllocationSize(Address,const Shape& s,std::uint64_t& size){++allocations;size=sizes[s.format==10];return !allocationInfoFail;}
 bool Create(Address,const Shape&,Address& result){++creates;if(creates==createFail)return false;result=0x10000+creates*0x1000;refs[result]=1;return true;}
 void Release(Address r){assert(refs[r]);--refs[r];++releases;}
 bool Hold(Address r){++holds;++refs[r];return holds!=holdFail;}
 bool Current(const Context& c){return Same(c,current);}
 bool Barriers(const Context&,const Transition* t,std::size_t n){++commands;if(commands==commandFail)return false;barriers.insert(barriers.end(),t,t+n);return true;}
 bool Copy(const Context&,Address d,Address s){++commands;if(commands==commandFail)return false;copies.emplace_back(d,s);return true;}
 bool Signal(Address,Address,std::uint64_t v){if(signalFail)return false;signal=v;return true;}
 std::uint64_t Completed(Address){return completed;}
};
static Shape testMaterialShape{2560,1440,57,2,1,1,0,0,3},testPositionShape{2560,1440,10,2,1,1,0,0,3};
static Snapshot snapshot(const Api& a){Snapshot s{};s.context=a.current;s.material.resource=0x6000;s.position.resource=0x7000;
 s.material.shape=testMaterialShape;s.position.shape=testPositionShape;s.material.state=0xc0;s.position.state=0x40;return s;}
int main(){unsigned checks=0;
 for(unsigned failure=1;failure<=6;++failure){Api a;a.createFail=failure;Backend<Api>b(a);
  assert(!b.Prepare(92,91,94,testMaterialShape,testPositionShape));assert(a.allocations==2&&a.creates==failure&&a.releases==failure-1);
  assert(b.LastPrepare().failure==PrepareFailure::Create&&b.LastPrepare().failedResource==failure);
  assert(b.LastPrepare().requiredBytes==15360&&b.AllocationBytes()==0);
  for(const auto& slot:b.Destinations())for(auto d:slot)assert(!d);
  ++checks;
 }
 for(auto size:{std::uint64_t(0),UINT64_MAX,UINT64_MAX-1}){Api a;a.sizes[0]=size;Backend<Api>b(a);
  assert(!b.Prepare(92,91,94,testMaterialShape,testPositionShape));assert(!a.creates);++checks;
 }
 // Admission respects exact byte boundaries, including the 10% reserve.
 {Api a;Backend<Api>b(a);a.memory={20000,2641};
  assert(!b.Prepare(92,91,94,testMaterialShape,testPositionShape));assert(!a.creates);
  assert(b.LastPrepare().failure==PrepareFailure::Budget&&b.LastPrepare().requiredBytes==15360&&b.LastPrepare().budgetBytes==15359);
  --a.memory.usage;assert(b.Prepare(92,91,94,testMaterialShape,testPositionShape));
  assert(b.AllocationBytes()==15360&&a.budgetQueries==2);assert(b.Dispose(true));++checks;}
 // 4K and 8K four-layer arrays are accepted when DXGI headroom permits them.
 // Only an allocation-size mock: actual device alignment/GPU remains untested.
 for(unsigned width:{3840u,7680u,8192u}){
  Api a;Backend<Api>b(a);auto mm=testMaterialShape,pp=testPositionShape;
  mm.width=pp.width=width;mm.height=pp.height=width==8192?4320:width*9/16;mm.layers=pp.layers=4;
  a.sizes[0]=((mm.width*mm.height*4*2+65535)/65536)*65536;
  a.sizes[1]=((mm.width*mm.height*4*8+65535)/65536)*65536;
  const auto required=3*(a.sizes[0]+a.sizes[1]);
  assert(required>512ull*1024*1024);if(width>3840)assert(required>(1ull<<30));
  a.memory={32ull<<30,8ull<<30};
  assert(b.Prepare(92,91,94,mm,pp));assert(a.creates==6&&b.AllocationBytes()==required);
  assert(b.LastPrepare().failure==PrepareFailure::None&&b.LastPrepare().budgetBytes==a.memory.Allowance());
  // Exercise 8K frame-slot admission and copy/retirement, not just allocation.
  Capture<Backend<Api>> c(b);assert(c.Bind(b.Key()));auto snap=snapshot(a);
  snap.material.shape=mm;snap.position.shape=pp;control_rr::Lease lease;
  assert(b.SetRecording(snap.context)&&c.Record(snap,1,b.Destinations(),lease));
  assert(c.Take(lease,snap.context,1)&&c.AfterSubmission(2));a.completed=a.signal;
  assert(c.Collect()&&c.Idle()&&b.Dispose(c.Idle()));++checks;
 }
 // Same request accepted or rejected as GPU headroom changes, without a fixed cap.
 for(unsigned width:{1920u,2560u,3840u,7680u,8192u})for(unsigned layers:{1u,2u,3u,4u,5u,32u})
 for(unsigned gpuGiB:{8u,16u,32u,64u}){
  Api a;Backend<Api>b(a);auto mm=testMaterialShape,pp=testPositionShape;
  mm.width=pp.width=width;mm.height=pp.height=width*9/16;mm.layers=pp.layers=layers;
  a.sizes[0]=((mm.width*mm.height*layers*2+65535)/65536)*65536;
  a.sizes[1]=((mm.width*mm.height*layers*8+65535)/65536)*65536;
  a.memory={std::uint64_t(gpuGiB)<<30,std::uint64_t(gpuGiB/2)<<30};
  const auto required=3*(a.sizes[0]+a.sizes[1]);const bool admitted=required<=a.memory.Allowance();
  assert(b.Prepare(92,91,94,mm,pp)==admitted);
  assert(b.LastPrepare().requiredBytes==required&&a.creates==(admitted?6u:0u));
  assert(b.LastPrepare().failure==(admitted?PrepareFailure::None:PrepareFailure::Budget));
  assert(b.Dispose(true));++checks;
 }
 for(auto memory:{control_rr::MemoryBudget{0,0},{100,100},{100,101},{100,UINT64_MAX},{100,95}}){
  Api a;a.memory=memory;Backend<Api>b(a);assert(!b.Prepare(92,91,94,testMaterialShape,testPositionShape));
  assert(!a.creates&&b.LastPrepare().failure==PrepareFailure::Budget&&b.LastPrepare().budgetBytes==0);++checks;
 }
 {Api a;a.budgetQueryFail=true;Backend<Api>b(a);assert(!b.Prepare(92,91,94,testMaterialShape,testPositionShape));
  assert(!a.creates&&b.LastPrepare().failure==PrepareFailure::BudgetQuery);++checks;}
 {Api a;a.allocationInfoFail=true;Backend<Api>b(a);
  assert(!b.Prepare(92,91,94,testMaterialShape,testPositionShape));assert(!a.creates);
  assert(b.LastPrepare().failure==PrepareFailure::AllocationInfo&&b.LastPrepare().failedResource==1);++checks;}
 // Overflow cannot wrap into a small request, even with maximum headroom.
 for(unsigned lane=0;lane<2;++lane){Api a;a.sizes[lane]=UINT64_MAX-1;a.memory={UINT64_MAX,0};Backend<Api>b(a);
  assert(!b.Prepare(92,91,94,testMaterialShape,testPositionShape));assert(!a.creates);
  assert(b.LastPrepare().failure==PrepareFailure::Budget&&b.LastPrepare().requiredBytes==UINT64_MAX);++checks;}
 {Api a;a.sizes[0]=UINT64_MAX/6;a.sizes[1]=UINT64_MAX/6+2;Backend<Api>b(a);
  assert(!b.Prepare(92,91,94,testMaterialShape,testPositionShape));assert(!a.creates);
  assert(b.LastPrepare().failure==PrepareFailure::Budget&&b.LastPrepare().requiredBytes==UINT64_MAX);++checks;}
 for(unsigned i=0;i<12;++i){Api a;Backend<Api>b(a);auto mm=testMaterialShape,pp=testPositionShape;
  switch(i){case 0:mm.width=UINT64_MAX;break;case 1:mm.layers=0;break;case 2:pp.layers=33;break;
   case 3:mm.mips=2;break;case 4:mm.samples=2;break;case 5:pp.quality=1;break;
   case 6:mm.dimension=2;break;case 7:pp.format=57;break;case 8:mm.format=10;break;
   case 9:mm.flags=8;break;case 10:pp.height=1080;break;case 11:pp.layers=1;break;}
  assert(!b.Prepare(92,91,94,mm,pp));assert(!a.creates);++checks;
 }
 {Api a;Backend<Api>b(a);Capture<Backend<Api>>c(b);assert(b.Prepare(92,91,94,testMaterialShape,testPositionShape)&&c.Bind(b.Key()));
  auto s=snapshot(a);control_rr::Lease l;assert(b.SetRecording(s.context));assert(c.Record(s,1,b.Destinations(),l));
  assert(a.commands==4&&a.barriers.size()==8&&a.copies.size()==2);assert(a.refs[0x6000]==1&&a.refs[0x7000]==1);
  for(unsigned i=0;i<4;++i){assert(a.barriers[i].resource==a.barriers[i+4].resource);assert(a.barriers[i].before==a.barriers[i+4].after);assert(a.barriers[i].after==a.barriers[i+4].before);}
  assert(!b.Dispose(true));assert(c.Take(l,s.context,1));assert(!c.Take(l,s.context,1));
  assert(c.AfterSubmission(2));assert(c.Collect()&&a.refs[0x6000]==1);a.completed=1;
  assert(c.Collect()&&c.Idle()&&a.refs[0x6000]==0&&a.refs[0x7000]==0);assert(b.Dispose(c.Idle()));assert(a.releases==8);++checks;
 }
 for(unsigned failure=1;failure<=4;++failure){Api a;a.commandFail=failure;Backend<Api>b(a);Capture<Backend<Api>>c(b);
  assert(b.Prepare(92,91,94,testMaterialShape,testPositionShape)&&c.Bind(b.Key()));auto s=snapshot(a);control_rr::Lease l;assert(b.SetRecording(s.context));
  assert(!c.Record(s,1,b.Destinations(),l));assert(c.Stopped()&&b.Poisoned());assert(a.commands==failure&&!a.releases);
  assert(!c.AfterSubmission(2)&&!b.Dispose(true));b.ReleaseSources(0);assert(!a.releases);++checks;
 }
 for(unsigned failure=1;failure<=2;++failure){Api a;a.holdFail=failure;Backend<Api>b(a);Capture<Backend<Api>>c(b);
  assert(b.Prepare(92,91,94,testMaterialShape,testPositionShape)&&c.Bind(b.Key()));auto s=snapshot(a);control_rr::Lease l;assert(b.SetRecording(s.context));
  assert(!c.Record(s,1,b.Destinations(),l));assert(c.Stopped()&&b.Poisoned()&&!a.commands&&!a.releases);assert(!b.Dispose(true));++checks;
 }
 {Api a;Backend<Api>b(a);Capture<Backend<Api>>c(b);assert(b.Prepare(92,91,94,testMaterialShape,testPositionShape)&&c.Bind(b.Key()));
  auto s=snapshot(a);control_rr::Lease l;assert(b.SetRecording(s.context)&&c.Record(s,1,b.Destinations(),l));
  a.signalFail=true;assert(!c.AfterSubmission(2)&&c.Stopped()&&b.Poisoned());assert(!b.Dispose(true)&&!a.releases);++checks;}
 {Api a;Backend<Api>b(a);assert(b.Prepare(92,91,94,testMaterialShape,testPositionShape));auto s=snapshot(a);
  ++s.context.frame;assert(!b.SetRecording(s.context));s.context=a.current;assert(b.SetRecording(s.context));
  ++a.current.list;assert(!b.Current(s.context));assert(b.Dispose(true));++checks;}
 // Repeated GPU stalls fill all three slots. No source can release before completion.
 {Api a;Backend<Api>b(a);Capture<Backend<Api>>c(b);assert(b.Prepare(92,91,94,testMaterialShape,testPositionShape)&&c.Bind(b.Key()));
  for(unsigned cycle=0;cycle<1000;++cycle){
   for(unsigned i=0;i<3;++i){a.current.frame=cycle*4+i+1;auto s=snapshot(a);control_rr::Lease l;
    assert(b.SetRecording(s.context)&&c.Record(s,1,b.Destinations(),l));}
   a.current.frame=cycle*4+4;auto s=snapshot(a);control_rr::Lease l;assert(b.SetRecording(s.context));
   const auto held=a.holds;assert(!c.Record(s,1,b.Destinations(),l)&&a.holds==held&&!c.Stopped());
   assert(c.AfterSubmission(a.current.frame)&&c.Collect());assert(a.refs[0x6000]==3&&a.refs[0x7000]==3);
   a.completed=a.signal;assert(c.Collect()&&c.Idle());assert(!a.refs[0x6000]&&!a.refs[0x7000]);
  }
  assert(b.Dispose(c.Idle()));++checks;
 }
 std::cout<<"PASS "<<checks<<" backend scenarios, including 3000 captured-frame retirements\n";
}
