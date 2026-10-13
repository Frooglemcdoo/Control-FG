#include "../src/pt_scene_order.h"
#include <cassert>
#include <cstdio>
#include <chrono>
#include <cstring>
using namespace PTSceneOrder;
static void TestLiveCompaction(){
    using namespace PTSceneOrder;
    Graph reference,compact;
    std::vector<std::pair<U64,U64>> cuts;
    std::map<U64,U64> signalled,completed;
    unsigned random=0x19347u;U64 version=1;
    const auto next=[&](){random=random*1664525u+1013904223u;return random;};
    for(unsigned step=0;step<3000;++step){
        const U64 queue=1+next()%3,key=1+next()%7;
        switch(next()%4){
        case 0: assert(reference.Write(queue,{key,0x1000+key*256},version));assert(compact.Write(queue,{key,0x1000+key*256},version));++version;break;
        case 1: ++signalled[queue];assert(reference.Signal(queue,queue,signalled[queue]));assert(compact.Signal(queue,queue,signalled[queue]));break;
        case 2: {const U64 producer=1+next()%3;if(signalled[producer]){assert(reference.Wait(queue,producer,signalled[producer]));assert(compact.Wait(queue,producer,signalled[producer]));}break;}
        default: cuts.emplace_back(reference.Mark(queue),compact.Mark(queue));break;
        }
        if(cuts.size()>12)cuts.erase(cuts.begin());
        if(step%32)continue;
        std::set<U64> roots;for(const auto& cut:cuts)roots.insert(cut.second);
        std::map<U64,U64> remap;std::set<U64> versions;
        assert(compact.Compact(roots,{},remap,versions));
        for(auto& cut:cuts)cut.second=remap.at(cut.second);
        for(const auto& cut:cuts)for(U64 k=1;k<=7;++k){Selection a,b;
            const auto sa=reference.Select(cut.first,{k,0x1000+k*256},a),sb=compact.Select(cut.second,{k,0x1000+k*256},b);
            assert(sa==sb);if(sa==Status::ready)assert(a.version==b.version);
        }
    }
    // Continuous submissions: completion lags the current tail. No idle point
    // is supplied, yet metadata stays bounded and future CPU submissions may
    // safely observe polled completion. Old cuts do not acquire that new edge.
    Graph busy;U64 done=0;std::map<U64,U64> remap;std::set<U64> versions;
    for(U64 n=1;n<=20000;++n){
        assert(busy.Write(1,{1,0x1000},n));assert(busy.Signal(1,1,n));const auto stamp=busy.TailStamp(1);
        assert(busy.Compact({},{{1,done}},remap,versions));done=stamp;assert(busy.Size()<12);
    }
    const auto final=busy.Mark(1);Selection selected;assert(busy.Select(final,{1,0x1000},selected)==Status::ready&&selected.version==20000);
    Graph concurrent;assert(concurrent.Write(1,{1,0x1000},1));const auto before=concurrent.Mark(2);
    assert(concurrent.Compact({before},{{1,concurrent.TailStamp(1)}},remap,versions));
    assert(concurrent.Select(remap.at(before),{1,0x1000},selected)==Status::unordered);
    const auto after=concurrent.Mark(2);assert(concurrent.Select(after,{1,0x1000},selected)==Status::ready);
    std::puts("PASS live compaction: differential queue ordering, old cuts, concurrent writes and 20,000 busy-queue updates.");
}
static void TestStaticSceneCompaction(){
    Graph graph;std::map<U64,U64> remap;std::set<U64> versions;
    // Many immutable BLAS allocations plus many uncompleted fence signals:
    // the old key-by-root scan repeatedly searched the same one-write vector.
    for(U64 key=1;key<=1500;++key)assert(graph.Write(1,{key,key*256},key));
    for(U64 n=1;n<=3000;++n)assert(graph.Signal(1,1,n));
    const auto start=std::chrono::steady_clock::now();
    assert(graph.Compact({}, {}, remap,versions));
    const auto elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    assert(versions.size()==1500);
    const auto cut=graph.Mark(1);
    for(U64 key:{1ull,750ull,1500ull}){Selection out;
        assert(graph.Select(cut,{key,key*256},out)==Status::ready&&out.version==key);
    }
    std::printf("Static scene compaction: %.3f ms for 1,500 BLAS and 3,000 pending signals (informational)\n",elapsed);
}
static void TestObservedFenceCompletion(){
    const Key as{1,0x1000};Selection selected;
    Graph cpu;cpu.Write(1,as,1);cpu.Wait(1,90,5);const auto cut=cpu.Mark(1);
    assert(cpu.Select(cut,as,selected)==Status::pending);
    const auto blocked=cpu.PendingWait(cut);assert(blocked.fence==90&&blocked.value==5&&!blocked.signalValue);
    assert(cpu.ObserveFenceCompletion(90,4)==0&&cpu.Select(cut,as,selected)==Status::pending);
    assert(cpu.ObserveFenceCompletion(90,UINT64_MAX)==0);
    assert(cpu.ObserveFenceCompletion(90,5)==1&&cpu.Select(cut,as,selected)==Status::ready&&selected.version==1);
    assert(!cpu.PendingWait(cut).fence);
    cpu.Wait(1,90,6);const auto later=cpu.Mark(1);
    assert(cpu.ObserveFenceCompletion(90,5)==0&&cpu.Select(later,as,selected)==Status::pending);
    Graph queued;queued.Write(1,as,2);queued.Signal(1,91,7);queued.Wait(2,91,7);const auto cross=queued.Mark(2);
    assert(queued.ObserveFenceCompletion(91,7)==0);
    assert(queued.Select(cross,as,selected)==Status::ready&&selected.version==2);
    Graph unrelated;unrelated.Write(1,as,3);unrelated.Wait(2,92,1);const auto unknown=unrelated.Mark(2);
    assert(unrelated.ObserveFenceCompletion(92,1)==1);
    assert(unrelated.Select(unknown,as,selected)==Status::unordered); // No invented producer edge.
}
int main(){
    TestObservedFenceCompletion();
    TestStaticSceneCompaction();
    TestLiveCompaction();
    const Key as{10,0xa000};Selection chosen;
    // Native submission can precede the CPU producer of a queued wait. Keep
    // known scene metadata for disarmed recording, but never arm from it.
    Graph preview;
    preview.Write(1,as,90);preview.Wait(1,301,1);const auto previewCut=preview.Mark(1);
    assert(preview.Select(previewCut,as,chosen)==Status::pending);
    assert(preview.Select(previewCut,as,chosen,nullptr,Purpose::recordingPreview)==Status::ready&&chosen.version==90);
    assert(preview.Select(previewCut,as,chosen)==Status::pending); // Preview cannot upgrade the strict cache.
    preview.Signal(2,301,1);
    assert(preview.Select(previewCut,as,chosen)==Status::ready&&chosen.version==90);
    Graph changedPreview;
    changedPreview.Write(1,as,100);changedPreview.Signal(1,302,1);
    changedPreview.Wait(1,303,1);const auto changedCut=changedPreview.Mark(1);
    assert(changedPreview.Select(changedCut,as,chosen,nullptr,Purpose::recordingPreview)==Status::ready&&chosen.version==100);
    changedPreview.Wait(2,302,1);changedPreview.Write(2,as,101);changedPreview.Signal(2,303,1);
    assert(changedPreview.Select(changedCut,as,chosen)==Status::ready&&chosen.version==101); // Old preview must fail version revalidation.
    Graph unknownPreview;
    unknownPreview.Write(1,as,0);unknownPreview.Wait(1,304,1);const auto unknownCut=unknownPreview.Mark(1);
    assert(unknownPreview.Select(unknownCut,as,chosen,nullptr,Purpose::recordingPreview)==Status::missing);
    // R13: checking every future fence signal's complete clock blocked an
    // already-ready scene whenever an unrelated queue awaited a later signal.
    Graph unrelated;
    unrelated.Write(1,as,10);const auto readyCut=unrelated.Mark(1);
    unrelated.Signal(2,201,1);unrelated.Wait(2,202,1);unrelated.Signal(2,201,2);
    assert(unrelated.Select(readyCut,as,chosen)==Status::ready&&chosen.version==10);
    assert(!unrelated.ClosedDependencies()); // We did not invent the signal.
    // A future same-queue AS overwrite is also provably after this cut, even
    // when the overwrite cannot run until a not-yet-submitted signal arrives.
    unrelated.Wait(1,203,1);unrelated.Write(1,as,11);
    assert(unrelated.Select(readyCut,as,chosen)==Status::ready&&chosen.version==10);
    auto blockedCut=unrelated.Mark(1);const char* reason=nullptr;
    assert(unrelated.Select(blockedCut,as,chosen,&reason)==Status::pending);
    assert(std::strcmp(reason,"scene_dispatch_wait_pending")==0);
    unrelated.Signal(3,203,1);
    assert(unrelated.Select(blockedCut,as,chosen)==Status::ready&&chosen.version==11);
    // FIFO cannot excuse a pending competing write on another queue.
    Graph pendingRace;
    pendingRace.Write(1,as,20);const auto raceCut=pendingRace.Mark(1);
    pendingRace.Wait(2,204,1);pendingRace.Write(2,as,21);
    assert(pendingRace.Select(raceCut,as,chosen,&reason)==Status::pending);
    assert(std::strcmp(reason,"scene_competing_write_pending")==0);
    assert(pendingRace.Select(raceCut,as,chosen,nullptr,Purpose::recordingPreview)==Status::pending);
    pendingRace.Signal(3,204,1);
    assert(pendingRace.Select(raceCut,as,chosen)==Status::unordered);
    // Nor can it excuse a higher signal from a different queue releasing a
    // scene wait before its intended producer has completed.
    Graph pendingFenceRace;
    pendingFenceRace.Write(1,as,30);pendingFenceRace.Signal(1,205,1);
    pendingFenceRace.Wait(2,205,1);const auto fenceCut=pendingFenceRace.Mark(2);
    pendingFenceRace.Wait(3,206,1);pendingFenceRace.Signal(3,205,2);
    assert(pendingFenceRace.Select(fenceCut,as,chosen,&reason)==Status::pending);
    assert(std::strcmp(reason,"scene_fence_order_pending")==0);
    assert(pendingFenceRace.Select(fenceCut,as,chosen,nullptr,Purpose::recordingPreview)==Status::pending);
    pendingFenceRace.Signal(4,206,1);
    assert(pendingFenceRace.Select(fenceCut,as,chosen)==Status::unordered);
    Graph graph;
    graph.Write(1,as,100);graph.Signal(1,20,1);
    graph.Write(1,as,200); // Not ordered with queue 2: the older cut cannot freeze the AS.
    graph.Wait(2,20,1);auto ray=graph.Mark(2);
    assert(graph.Select(ray,as,chosen)==Status::unordered);
    graph.Signal(1,20,2);graph.Wait(2,20,2);ray=graph.Mark(2);
    assert(graph.Select(ray,as,chosen)==Status::ready&&chosen.version==200);
    assert(graph.Select(ray,{11,as.address},chosen)==Status::missing&&chosen.version==0);
    graph.Write(2,as,0);ray=graph.Mark(2);assert(graph.Select(ray,as,chosen)==Status::missing);
    Graph late;
    late.Wait(2,30,1);ray=late.Mark(2);assert(!late.ClosedDependencies());
    assert(late.Select(ray,as,chosen)==Status::pending);
    late.Write(1,as,300);late.Signal(1,30,1);assert(late.ClosedDependencies());
    assert(late.Select(ray,as,chosen)==Status::ready&&chosen.version==300);
    Graph race;
    race.Write(1,as,400);race.Signal(1,40,1);
    race.Write(2,as,500);race.Signal(2,50,1);
    race.Wait(3,40,1);race.Wait(3,50,1);ray=race.Mark(3);
    assert(race.Select(ray,as,chosen)==Status::unordered&&chosen.node==0);
    Graph cycle;
    cycle.Wait(1,60,1);cycle.Signal(1,70,1);
    cycle.Wait(2,70,1);cycle.Signal(2,60,1);ray=cycle.Mark(2);
    assert(cycle.Select(ray,as,chosen)==Status::invalid);
    Graph competing;
    competing.Write(1,as,600);competing.Signal(1,90,1);
    competing.Signal(2,90,2);competing.Wait(3,90,1);ray=competing.Mark(3);
    assert(competing.Select(ray,as,chosen)==Status::unordered); // Higher value could release the wait without the build.
    Graph handoff;
    handoff.Write(1,as,700);handoff.Signal(1,100,1);
    handoff.Wait(2,100,1);handoff.Write(2,as,800);handoff.Signal(2,100,2);
    handoff.Wait(3,100,2);ray=handoff.Mark(3);
    assert(handoff.Select(ray,as,chosen)==Status::ready&&chosen.version==800);
    Graph overwrite;
    overwrite.Write(1,as,1);overwrite.Signal(1,110,1);
    overwrite.Write(2,as,2);overwrite.Signal(2,120,1);
    overwrite.Wait(3,110,1);overwrite.Wait(3,120,1);overwrite.Write(3,as,3);ray=overwrite.Mark(3);
    assert(overwrite.Select(ray,as,chosen)==Status::ready&&chosen.version==3);
    Graph protectedUse;
    protectedUse.Write(1,as,100);protectedUse.Signal(1,130,1);
    protectedUse.Wait(2,130,1);ray=protectedUse.Mark(2);
    protectedUse.Signal(2,140,1);protectedUse.Wait(1,140,1);protectedUse.Write(1,as,200);
    assert(protectedUse.Select(ray,as,chosen)==Status::ready&&chosen.version==100);
    Graph rewind;assert(rewind.Signal(1,80,2));assert(!rewind.Signal(1,80,1));
    assert(rewind.Select(1,as,chosen)==Status::invalid);
    // Regression: M12 repeatedly walked the entire ancestry for every write,
    // making this modest sustained-update journal take roughly a second.
    Graph sustained;
    for(U64 frame=0;frame<100;++frame){for(U64 b=1;b<=32;++b)sustained.Write(1,{b,b*4096},frame*32+b);sustained.Signal(1,1,frame+1);}
    const auto begin=std::chrono::steady_clock::now();std::map<Key,Selection> latest;
    assert(sustained.CompletedCheckpoint(latest)&&latest.size()==32);
    for(U64 b=1;b<=32;++b)assert(latest.at({b,b*4096}).version==99*32+b);
    std::printf("Scene history retirement: %.3f ms for 3,200 writes (informational timing)\n",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count());
    std::map<Key,Selection> invalid;
    cycle.Write(2,as,999);assert(!cycle.CompletedCheckpoint(invalid));
    // Released allocations retire only after GPU completion and without cuts.
    Graph dead;const Key oldKey{51,0x9000},newKey{52,0x9000};
    dead.Write(1,oldKey,51);const auto doneStamp=dead.TailStamp(1);
    auto oldCut=dead.Mark(1);std::map<U64,U64> deadRemap;std::set<U64> versions,live{52};
    assert(dead.Compact({oldCut},{{1,doneStamp}},deadRemap,versions,&live)&&versions.count(51));
    oldCut=deadRemap.at(oldCut);assert(dead.Select(oldCut,oldKey,chosen)==Status::ready);
    assert(dead.Compact({},{{1,doneStamp}},deadRemap,versions,&live)&&!versions.count(51));
    dead.Write(1,newKey,52);auto fresh=dead.Mark(1);
    assert(dead.Select(fresh,newKey,chosen)==Status::ready&&chosen.version==52);
    assert(dead.Select(fresh,oldKey,chosen)==Status::missing);
    Graph inFlight;inFlight.Write(1,oldKey,77);
    assert(inFlight.Compact({},{{1,0}},deadRemap,versions,&live)&&versions.count(77));
    assert(inFlight.Compact({},{{1,inFlight.TailStamp(1)}},deadRemap,versions,&live)&&!versions.count(77));
    std::puts("PASS AS queue ordering: exact fence cuts, late signals, allocation identity, unknown writes, unordered producers and cycles.");
}
