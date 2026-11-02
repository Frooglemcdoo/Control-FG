#include "../src/gi_reuse/submission_ledger.h"
#include <cassert>
#include <cstdio>
#include "../src/gi_reuse/scene_read_plan.h"
using namespace gi_reuse;
static ASEvent Build(std::uint64_t address,std::uint64_t instances=0){return {ASOperation::Build,{address,0,instances,3,instances!=0},0};}
int main(){
 ASJournal<4> early,late;early.Reset();late.Reset();early.Append(Build(256,100));late.Append(Build(256,200));
 ASLedger<4> live,snapshot;
 // Recorded early, submitted late: final metadata MUST follow submission order.
 assert(live.Submit(1,1,late,late.generation));assert(live.Submit(1,2,early,early.generation));assert(live.Find(256)->instances==100);
 assert(!live.Snapshot(1,1,snapshot));assert(!live.Snapshot(2,2,snapshot));assert(!live.Snapshot(1,UINT64_MAX,snapshot));assert(live.Snapshot(1,2,snapshot));
 ASJournal<4> copy;copy.Reset();copy.Append({ASOperation::Copy,{512},256});assert(live.Submit(1,3,copy,copy.generation));assert(live.Find(512)->instances==100);
 // In-place updates assign a new revision. Earlier completed snapshot stays immutable.
 early.Reset();early.Append(Build(256,300));assert(live.Submit(1,4,early,early.generation));assert(live.Find(256)->instances==300);assert(snapshot.Find(256)->instances==100);assert(live.Find(512)->instances==100);
 const auto generation=early.generation;early.Reset();assert(!live.Submit(1,5,early,generation));assert(!live.Find(256));
 ASLedger<1> full;ASJournal<4> two;two.Append(Build(256));two.Append(Build(512));assert(!full.Submit(1,1,two,0));assert(!full.Find(256));
 ASLedger<4> multi;assert(multi.Submit(1,1,late,late.generation));assert(!multi.Submit(2,2,early,early.generation));
 ASLedger<4> missing;assert(!missing.Submit(1,1,copy,copy.generation));
 ASJournal<1> overflow;assert(overflow.Append(Build(256)));assert(!overflow.Append(Build(512)));ASLedger<4> reject;assert(!reject.Submit(1,1,overflow,0));
 ASLedger<4> invalidated;ASJournal<4> reset;reset.Append(Build(256));reset.Append({ASOperation::Invalidate,{256},0});assert(invalidated.Submit(1,1,reset,0));assert(!invalidated.Find(256));
 GrowingASJournal<1024> growing;
 for(unsigned i=0;i<1024;++i)assert(growing.Append(Build(256,i+1)));
 auto frozen=growing;growing.Reset();assert(growing.size==0&&growing.events.empty());
 ASLedger<4> growingLedger;assert(growingLedger.Submit(1,1,frozen,0));assert(growingLedger.Find(256)->instances==1024);
 assert(!frozen.Append(Build(512)));assert(!growingLedger.Submit(1,2,frozen,0));
 TriangleMetadata t{4096,8192,12288,12,3,3,6,57,0};
 auto geometry=GeometryMetadata::Create({t});assert(geometry);
 ASLedger<4> scene,sceneSnapshot;ASJournal<4> sceneEvents;
 sceneEvents.Append({ASOperation::Build,{256,0,0,1,false,geometry},0});
 sceneEvents.Append({ASOperation::Copy,{512},256});
 sceneEvents.Append({ASOperation::Build,{768,0,16384,2,true},0});
 assert(scene.Submit(1,1,sceneEvents,0));assert(scene.Snapshot(1,1,sceneSnapshot));
 SceneReadPlan<> plan;assert(plan.Build(sceneSnapshot)&&plan.Complete());
 assert(plan.BLASCount()==2&&plan.TLASCount()==1&&plan.Ranges().size()==7);
 assert(plan.Ranges()[0].asAddress==256&&plan.Ranges()[0].revision==1&&plan.Ranges()[0].bytes==36);
 assert(plan.Ranges()[1].kind==ReadKind::Index&&plan.Ranges()[1].bytes==6);
 assert(plan.Ranges()[2].kind==ReadKind::Transform&&plan.Ranges()[2].bytes==48);
 assert(plan.Ranges()[3].asAddress==512&&plan.Ranges()[3].revision==2);
 assert(plan.Ranges()[6].kind==ReadKind::Instances&&plan.Ranges()[6].bytes==128);
 SceneReadPlan<6> tooSmall;assert(!tooSmall.Build(sceneSnapshot)&&tooSmall.Ranges().empty());
 scene.Invalidate();assert(!plan.Build(scene)&&!plan.Complete()&&plan.Ranges().empty());
 assert(plan.Build(sceneSnapshot)); // prior immutable snapshot remains usable as metadata
 ASLedger<4> malformed;ASJournal<1> bad;bad.Append({ASOperation::Build,{256,0,UINT64_MAX-10,2,true},0});
 assert(malformed.Submit(1,1,bad,0));assert(!plan.Build(malformed)&&plan.Ranges().empty());
 std::puts("PASS R292 scene read plan: AS revisions, copy lineage, all four range kinds, capacity/invalid rejection and prior snapshot isolation");
 std::puts("PASS submission ordering, reset generations, copy lineage, immutable snapshots, fence gating, capacity and multi-queue rejection");
}
