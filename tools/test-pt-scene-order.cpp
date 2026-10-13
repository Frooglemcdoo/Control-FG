#include "../src/pt_scene_order.h"
#include <cassert>
#include <cstdio>
using namespace PTSceneOrder;
int main(){
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
    late.Wait(2,30,1);ray=late.Mark(2);
    assert(late.Select(ray,as,chosen)==Status::pending);
    late.Write(1,as,300);late.Signal(1,30,1);
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
    std::puts("PASS AS queue ordering: exact fence cuts, late signals, allocation identity, unknown writes, unordered producers and cycles.");
}
