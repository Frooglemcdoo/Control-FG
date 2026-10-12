#include "../src/pt_owned_scene.h"
#include <cassert>
#include <cstdio>
int main(){
    using namespace PTOwnedScene;
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
    assert(index.Resolve(binding,record)==Status::invalidGeometry);
    binding.constants[8]=8;index.Remove(3);
    assert(index.Resolve(binding,record)==Status::missingAllocation);
    std::puts("PASS owned scene: candidate bounds, repeated updates, ambiguity, generation reuse, overlapping owners and R16 padding.");
}
