#include "../src/gi_reuse/ordered_as_metadata.h"
#include <cassert>
#include <cstdio>
using namespace gi_reuse;
using PTSceneOrder::Status;
using Adapter=OrderedASMetadata<>;
using Recording=std::shared_ptr<const OrderedASRecording>;
static Recording Build(std::uint64_t address,std::uint64_t token,std::uint64_t instances=4096){
 GrowingASJournal<8> j;j.Reset();
 j.Append({ASOperation::Build,{address,0,instances,2,true,{},true},0});
 return Adapter::Freeze(j,j.generation,{{token,0}});
}
static Recording Change(ASOperation op,std::uint64_t destination,std::uint64_t dstToken,
                        std::uint64_t source,std::uint64_t srcToken){
 GrowingASJournal<8> j;j.Reset();j.Append({op,{destination,0,8192,2,true,{},true},source});
 return Adapter::Freeze(j,j.generation,{{dstToken,srcToken}});
}
int main(){
 ASRecord out;
 Adapter order;
 auto early=Build(256,1,4096),late=Build(256,1,8192);
 assert(order.Submit(1,{late,early}));auto cut=order.Mark(1);
 assert(order.Select(cut,1,256,out)==Status::ready&&out.instances==4096&&out.revision==2);
 assert(order.Submit(1,{early})); // closed-list replay gets a fresh revision
 assert(order.Select(order.Mark(1),1,256,out)==Status::ready&&out.revision==3);
 assert(order.Select(cut,1,256,out)==Status::ready&&out.revision==2);
 assert(order.Select(order.Mark(1),2,256,out)==Status::missing); // reused VA, different owner
 GrowingASJournal<8> j;j.Reset();j.Append({ASOperation::Build,{512,0,4096,2,true},0});
 const auto generation=j.generation;auto frozen=Adapter::Freeze(j,generation,{{2,0}});j.Reset();
 assert(!Adapter::Freeze(j,generation,{})); // stale reset generation rejected
 assert(order.Submit(1,{frozen})); // held recording survives reset
 assert(order.Select(order.Mark(1),2,512,out)==Status::ready);
 assert(!Adapter::Freeze(j,j.generation,{{0,0}}));
 Adapter crossed;
 assert(crossed.Submit(1,{early}));assert(crossed.Signal(1,10,1,true));
 assert(crossed.Wait(2,10,1,true));
 assert(crossed.Submit(2,{Change(ASOperation::Copy,512,2,256,1)}));
 auto copyCut=crossed.Mark(2);
 assert(crossed.Select(copyCut,2,512,out)==Status::ready&&out.instances==4096);
 assert(crossed.Submit(3,{Build(256,1,12288)}));
 // A later-observed unordered producer invalidates an earlier source choice,
 // including the copy derived from it. Frozen metadata is not frozen bytes.
 assert(crossed.Select(copyCut,2,512,out)==Status::invalid&&!out.address);
 Adapter update;
 assert(update.Submit(1,{early,Change(ASOperation::Update,256,1,256,1)}));
 assert(update.Select(update.Mark(1),1,256,out)==Status::ready&&out.instances==8192);
 assert(update.Submit(1,{Change(ASOperation::Copy,512,2,256,1),Change(ASOperation::Invalidate,256,1,0,0)}));
 assert(update.Select(update.Mark(1),1,256,out)==Status::missing);
 assert(update.Select(update.Mark(1),2,512,out)==Status::ready); // source overwritten after copy is ordered
 Adapter pending;
 assert(pending.Wait(2,10,1,true));assert(pending.Submit(2,{late}));auto pendingCut=pending.Mark(2);
 assert(pending.Select(pendingCut,1,256,out)==Status::pending);
 assert(pending.Submit(1,{early}));assert(pending.Signal(1,10,1,true));
 assert(pending.Select(pendingCut,1,256,out)==Status::ready&&out.instances==8192);
 Adapter noSource;
 assert(noSource.Submit(1,{Build(512,2),Change(ASOperation::Copy,512,2,256,1)}));
 assert(noSource.Select(noSource.Mark(1),2,512,out)==Status::missing); // no stale fallback
 Adapter failedSignal;assert(failedSignal.Submit(1,{early}));auto failedCut=failedSignal.Mark(1);
 assert(!failedSignal.Signal(1,10,1,false));assert(!failedSignal.Healthy());
 assert(failedSignal.Select(failedCut,1,256,out)==Status::invalid);
 Adapter failedWait;assert(!failedWait.Wait(1,10,1,false));assert(!failedWait.Submit(1,{early}));
 Adapter missingList;assert(!missingList.Submit(1,{early,{}}));assert(!missingList.Healthy());
 OrderedASMetadata<1> bounded;assert(bounded.Submit(1,{early}));assert(!bounded.Submit(1,{late}));
 assert(!bounded.Healthy());
 TriangleMetadata tri{4096,8192,0,12,3,3,6,57,0};
 auto geometry=GeometryMetadata::Create({tri});assert(geometry);
 GrowingASJournal<8> blas;blas.Reset();
 blas.Append({ASOperation::Build,{256,0,0,1,false,geometry,true},0});
 tri.vertices=12288;auto moved=GeometryMetadata::Create({tri});
 blas.Append({ASOperation::Update,{256,0,0,1,false,moved,true},256});
 Adapter deformation;assert(deformation.Submit(1,{Adapter::Freeze(blas,blas.generation,{{1,0},{1,1}})}));
 assert(deformation.Select(deformation.Mark(1),1,256,out)==Status::ready&&out.geometry->At(0)->vertices==12288);
 blas.Reset();tri.indices=16384;auto changedIndices=GeometryMetadata::Create({tri});
 blas.Append({ASOperation::Update,{256,0,0,1,false,changedIndices,true},256});
 assert(deformation.Submit(1,{Adapter::Freeze(blas,blas.generation,{{1,1}})}));
 assert(deformation.Select(deformation.Mark(1),1,256,out)==Status::missing); // incompatible update tombstones old data
 Adapter nonmonotonic;assert(nonmonotonic.Signal(1,10,2,true));assert(!nonmonotonic.Signal(1,10,1,true));
 Adapter compact;
 std::vector<std::pair<std::uint64_t,std::uint64_t>> revisions;
 assert(compact.Submit(1,{early,Change(ASOperation::Copy,512,2,256,1)},&revisions));
 assert(revisions.size()==2&&revisions[0].first==1&&revisions[0].second==0&&revisions[1].first==2&&revisions[1].second==1);
 std::set<std::uint64_t> kept;
 const auto tail=compact.TailStamp(1);
 assert(!compact.Checkpoint({{1,tail-1}},false,kept));
 assert(!compact.Checkpoint({{1,tail}},true,kept));
 assert(compact.Checkpoint({{1,tail}},false,kept)&&kept==std::set<std::uint64_t>({1,2}));
 for(unsigned n=0;n<1000;++n){
  assert(compact.Submit(1,{late,Change(ASOperation::Copy,512,2,256,1)}));
  assert(compact.Checkpoint({{1,compact.TailStamp(1)}},false,kept)&&kept.size()==2&&compact.VersionCount()==2);
  assert(compact.Select(compact.Mark(1),2,512,out)==Status::ready&&out.instances==8192);
 }
 // A completed producer cannot authorize a checkpoint of another unfinished queue.
 assert(compact.Signal(1,11,1,true)&&compact.Wait(2,11,1,true));
 assert(!compact.Checkpoint({{1,compact.TailStamp(1)}},false,kept));
 assert(compact.Checkpoint({{1,compact.TailStamp(1)},{2,compact.TailStamp(2)}},false,kept));
 std::puts("PASS GI ordered AS adapter: batch/list order, replay/reset, allocation identities, cross-queue copy lineage, updates, pending/failed fences and bounded rejection");
}
