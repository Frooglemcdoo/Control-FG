#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
#include "../src/pt_scene_topology.h"
#include <cassert>
#include <cstdio>
int main(){
    using namespace PTSceneTopology;
    Index index;Geometry g{0x1000,0x2000,12,2,3,3};assert(index.Build(0xa000,{g}));assert(index.Compact(0xb000,0xa000));
    std::vector<PTNativeBinding::Binding> b(2);b[1].indices=0x1000;b[1].constants[1]=2;b[1].constants[8]=8;
    Instance instance{};instance.idMask=0xff000001;instance.contributionFlags=2;instance.blas=0xb000;
    auto r=index.Check({instance},b,2);assert(r.matched==1&&!r.mismatch&&!r.outOfRange);
    instance.contributionFlags=1;r=index.Check({instance},b,2);assert(r.outOfRange==1&&!r.matched);
    instance.contributionFlags=4;r=index.Check({instance},b,2);assert(r.outOfRange==1);
    instance.contributionFlags=2;b[1].indices+=2;r=index.Check({instance},b,2);assert(r.mismatch==1);
    b[1].indices-=2;auto other=instance;other.blas=0xc000;g.positions+=0x100;assert(index.Build(other.blas,{g}));
    r=index.Check({instance,other},b,2);assert(r.conflict==1);
    index.Invalidate(0xb000,0x100);r=index.Check({instance},b,2);assert(r.missingBlas==1);
    instance.idMask=1;r=index.Check({instance},b,2);assert(r.masked==1&&!r.missingBlas);
    assert(!index.Compact(0xc000,0xdead));other.idMask=0xff000001;r=index.Check({other},b,2);assert(r.missingBlas==1);
    Index shared;auto immutable=std::make_shared<const std::vector<Geometry>>(std::vector<Geometry>{g});
    std::weak_ptr<const std::vector<Geometry>> weak=immutable;
    assert(shared.BuildShared(0xa000,immutable));auto saved=shared;immutable.reset();
    assert(shared.Same(saved)&&!weak.expired());
    g.positions+=4;assert(shared.Build(0xa000,{g}));assert(!shared.Same(saved));
    Index equal;assert(equal.Build(0xa000,{g}));assert(equal.Same(shared));
    assert(!shared.BuildShared(0xa000,{}));assert(!shared.Same(equal));
    saved.Clear();assert(weak.expired());
    std::puts("PASS topology: record contribution, compaction, index mismatch, conflicting geometry and allocation invalidation.");
}
