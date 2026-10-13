#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
#include "../src/pt_scene_catalog.h"
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace PTSceneCatalog;
static const Allocation bottom{{10,0xb000},256},compact{{11,0xc000},256},top{{12,0xa000},256};
static Snapshot Blas(Allocation allocation,U64 positions){
    Snapshot s;s.kind=Snapshot::Kind::blas;s.destination=allocation;
    s.geometry={{0x1000,positions,12,2,3,3,20,21}};return s;
}
static Snapshot Tlas(Allocation allocation){
    Snapshot s;s.kind=Snapshot::Kind::tlas;s.destination=top;
    PTSceneTopology::Instance i{};i.idMask=0xff000001;i.blas=allocation.key.address;i.transform[0]=1;
    s.instances={i};s.bottom.emplace(i.blas,allocation);return s;
}
static U64 Position(const PTSceneDispatch::Scene& scene){
    PTNativeBinding::Binding b{};b.indices=0x1000;b.constants[1]=2;b.constants[8]=6;
    std::vector<PTSceneTopology::Mapping> map;scene.topology.Check(scene.instances,{b},2,&map);
    assert(map.size()==1&&map[0].referenced);return map[0].geometry.positions;
}
int main(){
    Catalog c;PTSceneDispatch::Scene scene;
    // PT rays use inclusion mask 1. Mask-2 objects cannot reach their SBT, so
    // unavailable geometry/owners there must not disable visible mask-1 data.
    Catalog maskedScene;auto maskedGeneration=maskedScene.Begin(1);
    assert(maskedScene.Record(1,maskedGeneration,Blas(bottom,0x2000)));
    auto maskedTop=Tlas(bottom);maskedTop.instances[0].idMask=0x01000001;
    auto excluded=maskedTop.instances[0];excluded.idMask=0x02000002;excluded.blas=0xd000;excluded.contributionFlags=8;
    maskedTop.instances.push_back(excluded);maskedTop.bottom.emplace(0xd000,Allocation{{99,0xd000},256});
    auto unowned=excluded;unowned.blas=0xe000;maskedTop.instances.push_back(unowned);maskedTop.bottom.emplace(0xe000,Allocation{});
    assert(maskedScene.Record(1,maskedGeneration,maskedTop));assert(maskedScene.Mark(1,maskedGeneration,800));
    assert(maskedScene.Submit(1,{{1,maskedGeneration}}));
    const char* maskedReason=nullptr;
    assert(maskedScene.Scene(800,top,scene,&maskedReason)==Status::missing);
    assert(maskedScene.Scene(800,top,scene,&maskedReason,PTSceneOrder::Purpose::submission,1)==Status::ready);
    assert(scene.instances.size()==1&&scene.blas.size()==1&&Position(scene)==0x2000);
    assert(maskedScene.Scene(800,top,scene,&maskedReason,PTSceneOrder::Purpose::recordingPreview,1)==Status::ready);
    // Making the unavailable geometry reachable must block both stages.
    maskedGeneration=maskedScene.Begin(1);maskedTop.instances[1].idMask=0x03000002;
    assert(maskedScene.Record(1,maskedGeneration,maskedTop));assert(maskedScene.Mark(1,maskedGeneration,801));
    assert(maskedScene.Submit(1,{{1,maskedGeneration}}));
    assert(maskedScene.Scene(801,top,scene,&maskedReason,PTSceneOrder::Purpose::submission,1)==Status::missing);
    assert(std::strcmp(maskedReason,"scene_no_ordered_build")==0);
    assert(maskedScene.Scene(801,top,scene,&maskedReason,PTSceneOrder::Purpose::recordingPreview,1)==Status::missing);
    Snapshot rejectedBuild;rejectedBuild.destination={{99,0xd000},256};rejectedBuild.unavailable="scene_update_while_pt_off";
    maskedGeneration=maskedScene.Begin(1);assert(maskedScene.Record(1,maskedGeneration,rejectedBuild));
    assert(maskedScene.Mark(1,maskedGeneration,802));assert(maskedScene.Submit(1,{{1,maskedGeneration}}));
    assert(maskedScene.Scene(802,top,scene,&maskedReason,PTSceneOrder::Purpose::submission,1)==Status::missing);
    assert(std::strcmp(maskedReason,"scene_update_while_pt_off")==0);
    assert(maskedScene.RetireHistory({}));
    assert(maskedScene.Scene(802,top,scene,&maskedReason,PTSceneOrder::Purpose::submission,1)==Status::missing);
    assert(std::strcmp(maskedReason,"scene_update_while_pt_off")==0);
    maskedTop.instances[1].idMask=0x02000002;maskedTop.instances[2].idMask=0x01000003;
    maskedGeneration=maskedScene.Begin(1);assert(maskedScene.Record(1,maskedGeneration,maskedTop));
    assert(maskedScene.Record(1,maskedGeneration,Blas({{100,0xe000},256},0x4000))); // Later owner cannot repair captured zero.
    assert(maskedScene.Mark(1,maskedGeneration,803));assert(maskedScene.Submit(1,{{1,maskedGeneration}}));
    assert(maskedScene.Scene(803,top,scene,&maskedReason,PTSceneOrder::Purpose::submission,1)==Status::missing);
    assert(std::strcmp(maskedReason,"scene_blas_owner_missing")==0);
    // A ready TLAS/BLAS cut survives unrelated future queued waits. The real
    // waiting cut must still be pending, with a reason reaching the caller.
    Catalog future;auto futureGeneration=future.Begin(1);const char* reason=nullptr;
    assert(future.Record(1,futureGeneration,Blas(bottom,0x2000)));
    assert(future.Record(1,futureGeneration,Tlas(bottom)));
    assert(future.Mark(1,futureGeneration,700));
    assert(future.Scene(700,top,scene,&reason)==Status::pending);
    assert(std::strcmp(reason,"scene_submission_pending")==0);
    assert(future.Submit(1,{{1,futureGeneration}}));
    assert(future.Signal(2,201,1));assert(future.Wait(2,202,1));assert(future.Signal(2,201,2));
    assert(future.Scene(700,top,scene,&reason)==Status::ready&&Position(scene)==0x2000);
    futureGeneration=future.Begin(2);assert(future.Mark(2,futureGeneration,701));
    assert(future.Submit(2,{{2,futureGeneration}}));
    assert(future.Scene(701,top,scene,&reason)==Status::pending);
    assert(std::strcmp(reason,"scene_dispatch_wait_pending")==0);
    Catalog preview;auto previewGeneration=preview.Begin(1);
    assert(preview.Record(1,previewGeneration,Blas(bottom,0x2000)));
    assert(preview.Record(1,previewGeneration,Tlas(bottom)));
    assert(preview.Submit(1,{{1,previewGeneration}}));assert(preview.Signal(1,301,1));
    assert(preview.Wait(1,302,1));previewGeneration=preview.Begin(1);
    assert(preview.Mark(1,previewGeneration,702));assert(preview.Submit(1,{{1,previewGeneration}}));
    assert(preview.Scene(702,top,scene)==Status::pending);
    assert(preview.Scene(702,top,scene,nullptr,PTSceneOrder::Purpose::recordingPreview)==Status::ready);
    const auto previewBottom=scene.blas.at(bottom.key.address).version;
    assert(Position(scene)==0x2000);
    assert(preview.Scene(702,top,scene)==Status::pending&&scene.instances.empty());
    // A producer that arrives later can change BLAS contents while keeping the
    // TLAS unchanged. Strict admission must expose the changed BLAS version.
    assert(preview.Wait(2,301,1));previewGeneration=preview.Begin(2);
    assert(preview.Record(2,previewGeneration,Blas(bottom,0x3000)));
    assert(preview.Submit(2,{{2,previewGeneration}}));assert(preview.Signal(2,302,1));
    assert(preview.Scene(702,top,scene)==Status::ready&&Position(scene)==0x3000);
    assert(scene.blas.at(bottom.key.address).version!=previewBottom);
    Catalog lateUnchanged;auto lg=lateUnchanged.Begin(1);
    assert(lateUnchanged.Record(1,lg,Blas(bottom,0x2000))&&lateUnchanged.Record(1,lg,Tlas(bottom)));
    assert(lateUnchanged.Submit(1,{{1,lg}}));assert(lateUnchanged.Signal(1,400,1));
    assert(lateUnchanged.Wait(1,401,1)&&lateUnchanged.Wait(1,401,2));
    lg=lateUnchanged.Begin(1);assert(lateUnchanged.Mark(1,lg,703)&&lateUnchanged.Submit(1,{{1,lg}}));
    assert(lateUnchanged.Scene(703,top,scene,nullptr,PTSceneOrder::Purpose::recordingPreview)==Status::ready);
    const auto unchangedVersion=scene.blas.at(bottom.key.address).version;
    assert(lateUnchanged.Scene(703,top,scene)==Status::pending);
    assert(lateUnchanged.Wait(2,400,1)&&lateUnchanged.Signal(2,401,1));
    assert(lateUnchanged.Scene(703,top,scene)==Status::pending);
    assert(lateUnchanged.Signal(2,401,2));
    assert(lateUnchanged.Scene(703,top,scene)==Status::ready&&scene.blas.at(bottom.key.address).version==unchangedVersion);
    auto producer=c.Begin(1);assert(producer);
    assert(c.Record(1,producer,Blas(bottom,0x2000)));
    Snapshot copy;copy.kind=Snapshot::Kind::copy;copy.destination=compact;copy.source=bottom;
    assert(c.Record(1,producer,copy));
    assert(c.Record(1,producer,Tlas(compact)));
    assert(c.Submit(1,{{1,producer}}));assert(c.Signal(1,1,1));
    // Record a later build before the earlier ray list is submitted. A compacted
    // BLAS must retain the geometry from its source at the copy operation.
    producer=c.Begin(1);assert(c.Record(1,producer,Blas(bottom,0x3000)));
    assert(c.Submit(1,{{1,producer}}));
    auto ray=c.Begin(2);assert(c.Mark(2,ray,100));assert(c.Scene(100,top,scene)==Status::pending);
    assert(c.Wait(2,1,1));assert(c.Submit(2,{{2,ray}}));
    assert(c.Scene(100,top,scene)==Status::ready&&Position(scene)==0x2000);
    assert(scene.blas.at(compact.key.address).token==compact.key.token);
    const auto raySceneVersion=scene.version,rayBottomVersion=scene.blas.at(compact.key.address).version;
    assert(rayBottomVersion);
    assert(c.Scene(100,{{99,top.key.address},256},scene)==Status::missing&&scene.instances.empty());
    // Updating compact's geometry is visible only beyond the new fence cut.
    producer=c.Begin(1);assert(c.Record(1,producer,Blas(compact,0x4000)));
    assert(c.Submit(1,{{1,producer}}));assert(c.Signal(1,1,2));
    ray=c.Begin(2);assert(c.Mark(2,ray,101));assert(c.Wait(2,1,2));assert(c.Submit(2,{{2,ray}}));
    assert(c.Scene(101,top,scene)==Status::ready&&Position(scene)==0x4000);
    assert(scene.version==raySceneVersion&&scene.blas.at(compact.key.address).version!=rayBottomVersion);
    // A submitted unsupported build must invalidate the old good geometry.
    producer=c.Begin(1);auto bad=Blas(compact,0x5000);bad.geometry[0].positionToken=0;
    assert(c.Record(1,producer,bad));assert(c.Submit(1,{{1,producer}}));assert(c.Signal(1,1,3));
    ray=c.Begin(2);assert(c.Mark(2,ray,102));assert(c.Wait(2,1,3));assert(c.Submit(2,{{2,ray}}));
    assert(c.Scene(102,top,scene)==Status::missing&&scene.instances.empty());
    // Reset cancels unsent marks and geometry; stale generations cannot publish.
    Catalog reset;auto old=reset.Begin(1);assert(reset.Record(1,old,Blas(bottom,0x2000)));assert(reset.Mark(1,old,200));
    const auto fresh=reset.Begin(1);assert(!reset.Record(1,old,Tlas(bottom)));
    assert(reset.Scene(200,top,scene)==Status::missing);assert(reset.Record(1,fresh,Tlas(bottom)));
    assert(reset.Mark(1,fresh,201));assert(reset.Submit(1,{{1,fresh}}));assert(reset.Scene(201,top,scene)==Status::missing);
    // Late CPU signaling resolves an already-submitted waiting consumer.
    Catalog late;ray=late.Begin(2);assert(late.Mark(2,ray,300));assert(late.Wait(2,2,1));assert(late.Submit(2,{{2,ray}}));
    assert(late.Scene(300,top,scene)==Status::pending);
    producer=late.Begin(1);assert(late.Record(1,producer,Blas(bottom,0x6000)));assert(late.Record(1,producer,Tlas(bottom)));
    assert(late.Submit(1,{{1,producer}}));assert(late.Signal(1,2,1));
    assert(late.Scene(300,top,scene)==Status::ready&&Position(scene)==0x6000);
    // Copy chains resolve at their own source cuts, including address reuse.
    producer=late.Begin(1);assert(late.Record(1,producer,Blas({{77,bottom.key.address},256},0x7000)));
    assert(late.Submit(1,{{1,producer}}));assert(late.Signal(1,2,2));
    ray=late.Begin(2);assert(late.Mark(2,ray,301));assert(late.Wait(2,2,2));assert(late.Submit(2,{{2,ray}}));
    assert(late.Scene(301,top,scene)==Status::ready&&scene.blas.at(bottom.key.address).token==10);
    // Ownership preflight must subsequently reject that token if released. This
    // catalog cannot turn an address into a new allocation identity.
    assert(!late.Submit(2,{{2,ray}}));assert(late.Scene(300,top,scene)==Status::invalid);
    Catalog cancelled;const auto cancelledGeneration=cancelled.Begin(1);
    assert(cancelled.Mark(1,cancelledGeneration,400));cancelled.Forget(400);
    assert(cancelled.Submit(1,{{1,cancelledGeneration}}));
    assert(cancelled.Scene(400,top,scene)==Status::missing); // Submission cannot resurrect cancellation.
    Catalog prune;auto generation=prune.Begin(1);
    auto storage=std::make_shared<int>(17);std::weak_ptr<int> released=storage;
    auto frozen=std::make_shared<PTSceneTopology::FrozenGeometry>();frozen->owner=storage;
    auto initial=Blas(bottom,0x8000);initial.geometry[0].frozen=frozen;
    assert(prune.Record(1,generation,initial));initial.geometry.clear();frozen.reset();storage.reset();
    assert(prune.Record(1,generation,copy));assert(prune.Record(1,generation,Tlas(compact)));
    assert(prune.Submit(1,{{1,generation}}));assert(prune.Signal(1,99,1));
    generation=prune.Begin(2);assert(prune.Wait(2,99,1));assert(prune.Mark(2,generation,500));assert(prune.Submit(2,{{2,generation}}));
    assert(!prune.CompletedCheckpoint([](U64){return true;})); // Pending historical consumers prohibit pruning.
    prune.Forget(500);
    assert(prune.CompletedCheckpoint([](U64 token){return token!=bottom.key.token;}));
    assert(!released.expired()); // Compaction still needs its source geometry.
    generation=prune.Begin(3);assert(prune.Wait(3,99,1));assert(prune.Mark(3,generation,501));assert(prune.Submit(3,{{3,generation}}));
    assert(prune.Scene(501,top,scene)==Status::ready&&Position(scene)==0x8000);scene={};prune.Forget(501);
    generation=prune.Begin(1);assert(prune.Record(1,generation,Blas(compact,0x9000)));assert(prune.Submit(1,{{1,generation}}));
    assert(prune.CompletedCheckpoint([](U64){return true;}));assert(released.expired());
    // Repeated completion checkpoints retain the current scene without growing
    // graph history until its fixed node limit. Old fence waits remain valid.
    for(unsigned n=0;n<20000;++n){
        generation=prune.Begin(1);assert(prune.Record(1,generation,Blas(compact,0x9000+n*4)));
        assert(prune.Submit(1,{{1,generation}}));assert(prune.CompletedCheckpoint([](U64){return true;}));
    }
    generation=prune.Begin(9);assert(prune.Wait(9,99,1));assert(prune.Mark(9,generation,502));assert(prune.Submit(9,{{9,generation}}));
    assert(prune.Scene(502,top,scene)==Status::ready&&Position(scene)==0x9000+19999*4);
    Catalog busy;U64 completed=0;
    generation=busy.Begin(1);assert(busy.Record(1,generation,Blas(bottom,0x2000)));
    assert(busy.Record(1,generation,copy));assert(busy.Record(1,generation,Tlas(compact)));assert(busy.Submit(1,{{1,generation}}));
    for(U64 n=0;n<20000;++n){
        generation=busy.Begin(1);assert(busy.Record(1,generation,Blas(bottom,0x3000+n*4)));assert(busy.Submit(1,{{1,generation}}));
        const auto stamp=busy.TailStamp(1);assert(busy.RetireHistory({{1,completed}}));completed=stamp;
        const auto stats=busy.StatisticsNow();assert(stats.rows<32&&stats.nodes<32&&stats.versions<10);
    }
    generation=busy.Begin(1);assert(busy.Mark(1,generation,600));assert(busy.Submit(1,{{1,generation}}));
    assert(busy.RetireHistory({{1,completed}}));
    assert(busy.Scene(600,top,scene)==Status::ready&&Position(scene)==0x2000); // Compacted static source remains exact.
    Catalog deadHistory;auto deadStorage=std::make_shared<int>(9);std::weak_ptr<int> deadWeak=deadStorage;
    auto deadFrozen=std::make_shared<PTSceneTopology::FrozenGeometry>();deadFrozen->owner=deadStorage;
    auto deadBlas=Blas(bottom,0x9000);deadBlas.geometry[0].frozen=deadFrozen;
    generation=deadHistory.Begin(1);assert(deadHistory.Record(1,generation,deadBlas));
    assert(deadHistory.Submit(1,{{1,generation}}));
    deadBlas.geometry.clear();deadFrozen.reset();deadStorage.reset();std::set<U64> noLive;
    assert(deadHistory.RetireHistory({{1,0}},&noLive));assert(!deadWeak.expired());
    assert(deadHistory.RetireHistory({{1,deadHistory.TailStamp(1)}},&noLive));assert(deadWeak.expired());
    std::puts("PASS scene catalog: submitted build/copy cuts, exact allocations, late signals, unknown writes, reset and replay rejection.");
}
