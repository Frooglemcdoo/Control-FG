#include "../src/pt_scene_catalog.h"
#include <cassert>
#include <cstdio>
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
    assert(c.Scene(100,{{99,top.key.address},256},scene)==Status::missing&&scene.instances.empty());
    // Updating compact's geometry is visible only beyond the new fence cut.
    producer=c.Begin(1);assert(c.Record(1,producer,Blas(compact,0x4000)));
    assert(c.Submit(1,{{1,producer}}));assert(c.Signal(1,1,2));
    ray=c.Begin(2);assert(c.Mark(2,ray,101));assert(c.Wait(2,1,2));assert(c.Submit(2,{{2,ray}}));
    assert(c.Scene(101,top,scene)==Status::ready&&Position(scene)==0x4000);
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
    std::puts("PASS scene catalog: submitted build/copy cuts, exact allocations, late signals, unknown writes, reset and replay rejection.");
}
