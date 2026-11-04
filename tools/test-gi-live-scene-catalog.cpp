#include "../src/gi_reuse/live_scene_catalog.h"
#include <cassert>
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
