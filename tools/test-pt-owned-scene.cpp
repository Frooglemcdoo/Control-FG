#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
#include "../src/pt_owned_scene.h"
#include <cassert>
#include <cstdio>
int main(){
    using namespace PTOwnedScene;
    Index revisions;auto revision=revisions.Revision();assert(revision);
    revisions.Insert({1,4096,256});assert(revisions.Revision()!=revision);revision=revisions.Revision();
    assert(revisions.Find(4096,8)==1);assert(revisions.Revision()==revision);
    revisions.Insert({1,4096,256});assert(revisions.Revision()!=revision);revision=revisions.Revision();
    revisions.Remove(1);assert(revisions.Revision()!=revision);revision=revisions.Revision();
    assert(!revisions.Observe({}));assert(revisions.Revision()!=revision);revision=revisions.Revision();
    revisions.Clear();assert(revisions.Revision()!=revision);
    Index index;index.Insert({1,0x1000,0x100});index.Insert({2,0x2000,0x100});index.Insert({3,0x3000,0x100});
    Triangle triangle{0x1000,0x2000,12,0,0,2,3,3};assert(index.Observe(triangle));
    PTNativeBinding::Binding binding{};binding.indices=0x1000;binding.attributes=0x3000;
    binding.constants[1]=2;binding.constants[2]=16;binding.constants[6]=8;binding.constants[8]=8;binding.constants[9]=48;
    PTSceneAdmission::Record record{};
    assert(index.Resolve(binding,record)==Status::candidate&&record.positions==0x2000);
    for(unsigned i=0;i<100;++i)assert(index.Observe(triangle)); // Updates do not invent ambiguity.
    assert(index.Resolve(binding,record)==Status::candidate);
    auto alternate=triangle;alternate.positions+=48;assert(index.Observe(alternate));
    assert(index.Resolve(binding,record)==Status::ambiguousGeometry&&record.positions==0);
    index.Remove(2);index.Insert({4,0x2000,0x100}); // Reused VA must not revive stale geometry.
    assert(index.Resolve(binding,record)==Status::missingGeometry);
    assert(index.Observe(triangle));assert(index.Resolve(binding,record)==Status::candidate);
    index.Insert({5,0x1000,0x100});assert(index.Resolve(binding,record)==Status::missingAllocation);
    index.Remove(5);binding.constants[0]=10;
    assert(index.Resolve(binding,record)==Status::invalidBinding);
    binding.constants[0]=0;binding.constants[9]=16;
    assert(index.Resolve(binding,record)==Status::invalidGeometry&&record.positions==0);
    binding.constants[9]=48;binding.constants[8]=6; // R16 raw final fetch needs padding.
    assert(index.Resolve(binding,record)==Status::candidate&&record.constants[8]==8);
    index.Remove(1);index.Insert({6,0x1000,6});assert(index.Observe(triangle));
    assert(index.Resolve(binding,record)==Status::invalidGeometry); // No backing padding.
    index.Remove(6);index.Insert({7,0x1000,0x100});assert(index.Observe(triangle));
    binding.constants[8]=8;index.Remove(3);
    assert(index.Resolve(binding,record)==Status::missingAllocation);
    // A long earlier interval must still create ambiguity past many short ones.
    Index ranges;for(U64 n=1;n<=10000;++n)ranges.Insert({n,n*4096,1024});
    for(U64 n=1;n<=10000;++n){assert(ranges.Find(n*4096+1023,1)==n);assert(ranges.Find(n*4096+1023,2)==0);}
    ranges.Insert({10001,4096,10000*4096});assert(ranges.Find(9999*4096,4)==0);
    ranges.Remove(9999);assert(ranges.Find(9999*4096,4)==10001);
    ranges.Remove(10001);assert(ranges.Find(9999*4096,4)==0);
    ranges.Insert({9999,9999*4096,1024});assert(ranges.Find(9999*4096,4)==9999);
    ranges.Insert({10002,UINT64_MAX-3,4});assert(ranges.Find(UINT64_MAX,1)==10002);assert(ranges.Find(UINT64_MAX,2)==0);
    ranges.Clear();assert(ranges.Find(UINT64_MAX,1)==0);
    std::puts("PASS owned scene: candidate bounds, repeated updates, ambiguity, generation reuse, overlapping owners and R16 padding.");
}
