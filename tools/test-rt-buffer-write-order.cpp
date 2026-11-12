#include "../src/gi_reuse/ordered_buffer_writes.h"
#include <cassert>
#include <iostream>
using History=gi_reuse::OrderedBufferWrites;
using Status=PTSceneOrder::Status;
static History::Recording Copy(std::uint64_t generation,std::uint64_t token,std::uint64_t offset=0,std::uint64_t bytes=64){
 return {generation,true,{{token,offset,bytes,false}}};
}
int main(){
 History h;assert(h.Register(1,4096,256)&&h.Register(2,8192,256));
 History::Stamp stamp,other;
 assert(h.Capture(h.Mark(1),1,stamp)==Status::missing&&!stamp.revision);
 // Producer recorded after consumer on the CPU, submitted first on COPY queue2.
 const auto producer=Copy(10,1);assert(h.Submit(2,{producer}));
 assert(h.Signal(2,100,1,true)&&h.Wait(1,100,1,true));
 const auto first=h.Mark(1);assert(h.Capture(first,1,stamp)==Status::ready&&stamp.revision);
 assert(h.Unchanged(h.Mark(1),stamp)); // no CPU/GPU completion wait needed for order
 // An unordered later writer invalidates a formerly ready selection.
 assert(h.Wait(3,100,1,true)&&h.Submit(3,{Copy(11,1,128,16)}));assert(!h.Unchanged(first,stamp));
 assert(h.Capture(first,1,other)==Status::unordered&&!other.revision);
 // Explicit dependency restores ordering, but cannot restore the old content stamp.
 assert(h.Signal(3,101,1,true)&&h.Wait(1,101,1,true));
 const auto changed=h.Mark(1);assert(h.Capture(changed,1,other)==Status::ready&&!h.Unchanged(changed,stamp));
 // A write after the read, ordered on the same queue, leaves that earlier cut valid.
 const auto saved=other;assert(h.Submit(1,{Copy(12,1)}));assert(h.Unchanged(changed,saved));
 assert(!h.Unchanged(h.Mark(1),saved));
 // Whole-resource writes are explicit; zero-length ranged calls are not writes.
 History whole;assert(whole.Register(1,4096,256));
 assert(whole.Submit(1,{{1,true,{{1,0,0,true}}}}));assert(whole.Capture(whole.Mark(1),1,stamp)==Status::ready);
 assert(whole.Submit(1,{Copy(2,1,256,0)})&&whole.Unchanged(whole.Mark(1),stamp));
 assert(whole.Submit(1,{{3,true,{{1,0,0,true}}}})&&!whole.Unchanged(whole.Mark(1),stamp));
 // Replay gets a new revision; discarding a recording creates no write.
 assert(whole.Capture(whole.Mark(1),1,stamp)==Status::ready);
 const auto discarded=Copy(4,1);(void)discarded;assert(whole.Unchanged(whole.Mark(1),stamp));
 assert(whole.Submit(1,{producer})&&whole.Capture(whole.Mark(1),1,stamp)==Status::ready);
 assert(whole.Submit(1,{producer})&&!whole.Unchanged(whole.Mark(1),stamp));
 // CPU mutation makes this allocation ineligible even if a GPU copy follows.
 assert(whole.CpuWrite(1));assert(whole.Capture(whole.Mark(1),1,other)==Status::invalid&&!other.token);
 assert(whole.Submit(1,{producer})&&whole.Capture(whole.Mark(1),1,other)==Status::invalid);
 assert(whole.Retire(1)&&whole.Register(2,4096,256)); // new lifetime, reused VA
 assert(whole.Submit(1,{Copy(5,2)})&&whole.Capture(whole.Mark(1),2,other)==Status::ready);
 assert(!whole.Unchanged(whole.Mark(1),stamp));
 // CPU and GPU changes to other allocations do not poison this selection.
 assert(whole.Register(3,8192,256)&&whole.CpuWrite(3));assert(whole.Unchanged(whole.Mark(1),other));
 // Pending fence cannot admit a read before its signal is known.
 History late;assert(late.Register(1,4096,256)&&late.Wait(1,20,1,true));
 const auto cut=late.Mark(1);assert(late.Capture(cut,1,stamp)==Status::pending);
 assert(late.Submit(2,{producer})&&late.Signal(2,20,1,true));assert(late.Capture(cut,1,stamp)==Status::ready);
 // Waiting for BOTH racing writers does not order those writers with each other.
 History race;assert(race.Register(1,4096,256));
 assert(race.Submit(2,{producer})&&race.Signal(2,21,1,true));
 assert(race.Submit(3,{Copy(11,1)})&&race.Signal(3,22,1,true));
 assert(race.Wait(1,21,1,true)&&race.Wait(1,22,1,true));
 assert(race.Capture(race.Mark(1),1,stamp)==Status::unordered&&!stamp.token);
 // Invalid recording metadata must fail closed, never preserve stale stamps.
 for(unsigned kind=0;kind<6;++kind){
  History bad;assert(bad.Register(1,4096,256)&&bad.Submit(1,{producer}));
  assert(bad.Capture(bad.Mark(1),1,stamp)==Status::ready);auto r=Copy(20,1);
  if(kind==0)r.complete=false;
  if(kind==1)r.generation=0;
  if(kind==2)r.writes[0].offset=UINT64_MAX;
  if(kind==3)r.writes[0].bytes=257;
  if(kind==4)r.writes[0].token=2;
  if(kind==5)r.writes[0].wholeResource=true; // contradictory nonzero byte count
  assert(!bad.Submit(1,{r})&&!bad.Healthy()&&!bad.Unchanged(bad.Mark(1),stamp));
 }
 History failure;assert(failure.Register(1,4096,256)&&failure.Submit(1,{producer}));
 assert(failure.Capture(failure.Mark(1),1,stamp)==Status::ready);
 assert(!failure.Signal(1,20,1,false)&&!failure.Unchanged(failure.Mark(1),stamp));
 // Reuse existing nonblocking compaction with an outstanding read cut.
 History compact;assert(compact.Register(1,4096,256));
 for(unsigned n=0;n<1000;++n){
  assert(compact.Submit(1,{producer}));const auto at=compact.Mark(1);
  assert(compact.Capture(at,1,stamp)==Status::ready);
  std::map<std::uint64_t,std::uint64_t> remap;
  assert(compact.Compact({at},{{1,compact.TailStamp(1)}},remap));
  assert(remap.count(at)&&compact.Unchanged(remap.at(at),stamp));
 }
 std::cout<<"PASS buffer mutation order: COPY dependencies, late writers, whole copies, replay, CPU rejection, lifetime reuse, invalid input and compaction\n";
}
