#include "../src/gi_reuse/live_scene_catalog.h"
#include <cassert>
#include "../src/gi_reuse/attempt_throttle.h"
#include <cstdio>
using namespace gi_reuse;
static std::shared_ptr<GISceneBuild> Bottom(std::uint64_t address=256,std::uint64_t token=1){
 auto s=std::make_shared<GISceneBuild>();TriangleMetadata tri{4096,8192,0,12,3,3,6,57,0};
 s->build={{ASOperation::Build,{address,0,0,1,false,GeometryMetadata::Create({tri}),true},0},token,0};
 s->geometry.push_back({8192,4096,12,11,12,2,3,3});return s;
}
static std::shared_ptr<GISceneBuild> Top(){
 auto s=std::make_shared<GISceneBuild>();
 s->build={{ASOperation::Build,{512,0,12288,1,true,{},true},0},2,0};
 auto bytes=std::make_shared<std::vector<unsigned char>>(64);const float transform[12]={1,0,0,0,0,1,0,0,0,0,1,0};
 std::memcpy(bytes->data(),transform,48);const std::uint32_t mask=0xff000000;const std::uint64_t blas=256;
 std::memcpy(bytes->data()+48,&mask,4);std::memcpy(bytes->data()+56,&blas,8);
 s->instances={12288,3,1,bytes};s->bottoms.push_back({256,1,0});return s;
}
int main(){
 { // Many instances share one BLAS, but no cached revision crosses a cut.
  GILiveSceneCatalog repeated;auto topMany=Top();
  auto bytes=std::make_shared<std::vector<unsigned char>>(64*128);
  for(unsigned i=0;i<128;++i)std::memcpy(bytes->data()+64*i,topMany->instances.bytes->data(),64);
  topMany->instances.bytes=bytes;topMany->bottoms.resize(128,{256,1,0});topMany->build.event.record.count=128;
  assert(repeated.Submit(1,1,Bottom())&&repeated.Submit(1,2,topMany));
  const auto oldCut=repeated.Mark(1);GILiveSceneCatalog::Selection result;
  assert(repeated.Select(oldCut,1,2,512,result)&&result.geometry.size()==1&&result.perInstance.size()==128);
  assert(repeated.Submit(1,3,Bottom()));
  assert(repeated.Select(repeated.Mark(1),2,2,512,result));for(const auto& r:result.perInstance)assert(r.revision==3);
  assert(repeated.Select(oldCut,1,2,512,result));for(const auto& r:result.perInstance)assert(r.revision==1);
 }

 AttemptThrottle retry;assert(retry.Allow(100));retry.Rejected(100);assert(!retry.Allow(100)&&!retry.Allow(349)&&retry.Allow(350));retry.Succeeded();assert(retry.Allow(351));retry.Rejected(UINT64_MAX-10);assert(!retry.Allow(UINT64_MAX-1)&&retry.Allow(UINT64_MAX));
 GILiveSceneCatalog c;GILiveSceneCatalog::Selection selected;
 auto bottom=Bottom(),top=Top();assert(c.Submit(1,1,bottom));assert(c.Signal(1,10,1,true));assert(c.Wait(2,10,1,true));
 assert(c.Submit(2,2,top));auto cut=c.Mark(2);
 assert(c.Select(cut,9,2,512,selected));assert(selected.top.revision==2&&selected.instances.epoch==9&&selected.geometry.size()==1&&selected.perInstance[0].revision==1);
 // Later recording does not change an earlier writer's exact submitted cut.
 auto moved=Bottom();assert(c.Submit(2,3,moved));assert(c.Select(cut,9,2,512,selected)&&selected.perInstance[0].revision==1);
 assert(c.Select(c.Mark(2),10,2,512,selected)&&selected.perInstance[0].revision==3);
 auto clone=std::make_shared<GISceneBuild>();clone->build={{ASOperation::Copy,{768},512},4,2};assert(c.Submit(2,4,clone));
 assert(c.Select(c.Mark(2),10,4,768,selected)&&selected.top.revision==4);
 assert(!c.Checkpoint({{1,c.TailStamp(1)},{2,c.TailStamp(2)}},true));
 assert(c.Checkpoint({{1,c.TailStamp(1)},{2,c.TailStamp(2)}},false));
 assert(c.Select(c.Mark(2),11,4,768,selected));
 auto reused=Bottom(256,5);assert(c.Submit(2,5,reused));
 assert(c.Select(c.Mark(2),12,2,512,selected)&&selected.perInstance[0].token==1); // tokens are never retargeted
 auto bad=std::make_shared<GISceneBuild>();bad->build={{ASOperation::Invalidate,{256},0},1,0};assert(c.Submit(2,6,bad));
 assert(!c.Select(c.Mark(2),12,2,512,selected)); // no previous geometry fallback
 GILiveSceneCatalog::Diagnostic diagnostic;
 assert(!c.Select(c.Mark(2),12,2,512,selected,&diagnostic));
 assert(std::strcmp(diagnostic.stage,"blas_order_or_geometry")==0&&diagnostic.missingBottoms==1&&diagnostic.token==1&&diagnostic.address==256);
 assert(!c.Select(c.Mark(2),12,999,1024,selected,&diagnostic));
 assert(std::strcmp(diagnostic.stage,"tlas_order")==0&&std::strcmp(diagnostic.orderReason,"scene_no_ordered_build")==0);
 // R314 live failure: scene_dispatch_wait_pending despite the native fence
 // already being completed. Exercise the entire catalog forwarding path.
 GILiveSceneCatalog waited;assert(waited.Submit(1,1,Bottom())&&waited.Submit(1,2,Top()));
 assert(waited.Wait(1,90,5,true));const auto waiting=waited.Mark(1);
 assert(!waited.Select(waiting,1,2,512,selected,&diagnostic));
 assert(std::strcmp(diagnostic.orderReason,"scene_dispatch_wait_pending")==0);
 assert(waited.PendingWait(waiting).fence==90);
 assert(waited.ObserveFenceCompletion(90,4)==0);
 assert(waited.ObserveFenceCompletion(90,UINT64_MAX)==0);
 assert(!waited.Select(waiting,1,2,512,selected));
 assert(waited.ObserveFenceCompletion(90,5)==1);
 assert(waited.Select(waiting,1,2,512,selected));
 auto updated=Bottom();updated->build.event.operation=ASOperation::Update;updated->build.event.source=256;updated->build.sourceToken=1;
 assert(waited.Submit(1,3,updated));assert(waited.Select(waited.Mark(1),2,2,512,selected));
 assert(waited.Compact({{1,waited.TailStamp(1)}}));
 assert(waited.Wait(1,90,6,true));const auto future=waited.Mark(1);
 assert(!waited.ObserveFenceCompletion(90,5)&&!waited.Select(future,1,2,512,selected));
 // R317: recording-time wait 37/3 precedes the producer signal. A pending
 // recording snapshot must not permanently discard a future submission.
 GILiveSceneCatalog deferred;
 assert(deferred.Submit(2,1,Bottom())&&deferred.Submit(2,2,Top()));
 assert(deferred.Wait(1,37,3,true));const auto recordCut=deferred.Mark(1);
 assert(deferred.PendingWait(recordCut).value==3);
 assert(!deferred.Select(recordCut,1,2,512,selected));
 auto replacement=Bottom();
 assert(deferred.Submit(2,3,replacement));
 assert(!deferred.Select(deferred.Mark(1),1,2,512,selected));
 assert(deferred.Signal(2,37,4,true));
 assert(deferred.Select(deferred.Mark(1),1,2,512,selected));
 assert(selected.perInstance[0].revision==3);
 assert(deferred.Wait(1,37,7,true));
 assert(!deferred.Select(deferred.Mark(1),1,2,512,selected));
 puts("PASS deferred GI admission: pending during recording, updated producer at submission, unresolved next wait still rejected.");
 // Captured R318 queue sequence: four graphics waits are submitted before
 // the copy queue's four signals; copy work waits on earlier graphics signals.
 // Retain all admission cuts until the corresponding real signal is observed.
 GILiveSceneCatalog split;std::vector<std::uint64_t> held;
 assert(split.Submit(1,1,Bottom())&&split.Submit(1,2,Top()));
 for(std::uint64_t n=1;n<=4;++n){
  assert(split.Wait(1,37,n,true));held.push_back(split.Mark(1));
  assert(!split.Select(held.back(),1,2,512,selected));
  assert(split.Signal(1,38,n,true));
 }
 for(std::uint64_t n=1;n<=4;++n){
  if(n>=3)assert(split.Wait(39,38,n-2,true));
  assert(split.Signal(39,37,n,true));
  for(std::size_t i=0;i<held.size();++i)
   assert(split.Select(held[i],1,2,512,selected)==(i<n));
 }
 // Later graphics writes must not retarget already-held exact cuts.
 assert(split.Submit(1,3,Bottom()));
 assert(split.Select(held.back(),1,2,512,selected)&&selected.perInstance[0].revision==1);
 puts("PASS R318 four-stage upload sequence: retained cuts release only after sufficient signals, no future scene substitution.");
 GILiveSceneCatalog unrelated;assert(unrelated.Submit(1,1,Bottom())&&unrelated.Submit(1,2,Top()));
 assert(unrelated.Wait(2,91,1,true));auto other=unrelated.Mark(2);
 assert(unrelated.ObserveFenceCompletion(91,1)==1);
 assert(!unrelated.Select(other,1,2,512,selected)); // completion is not a guessed cross-queue edge
 GILiveSceneCatalog race;assert(race.Submit(1,1,bottom)&&race.Submit(1,2,top));const auto before=race.Mark(1);
 assert(race.Submit(2,3,moved));assert(!race.Select(before,1,2,512,selected));
 GILiveSceneCatalog streaming;std::uint64_t completed=0;
 for(unsigned n=0;n<300;++n){
  auto payload=Bottom();std::weak_ptr<GISceneBuild> old=payload;
  assert(streaming.Submit(1,n+1,payload));payload.reset();
  assert(streaming.Submit(1,n+1,Top()));
  const auto next=streaming.TailStamp(1);
  assert(streaming.Compact({{1,completed}}));
  assert(streaming.Select(streaming.Mark(1),n+1,2,512,selected));
  assert(!old.expired()); // current producer geometry survives pruning
  assert(streaming.Submit(1,n+1,Bottom()));
  assert(streaming.Compact({{1,next}}));
  assert(old.expired()); // superseded payload released; GPU flights retain their own leases
  completed=next;
 }
 std::puts("PASS live GI scene catalog: ordered writer cuts, cross-queue lineage, shared clone payloads, completed checkpoint, retired identities and conflicting writes");
}
