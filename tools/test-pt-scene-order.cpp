#include "../src/pt_scene_order.h"
#include <cassert>
#include <cstdio>
#include <chrono>
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
int main(){
    TestLiveCompaction();
    const Key as{10,0xa000};Selection chosen;
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
    std::puts("PASS AS queue ordering: exact fence cuts, late signals, allocation identity, unknown writes, unordered producers and cycles.");
}
