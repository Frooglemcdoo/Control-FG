#include "../src/rt_spot_shadow_contract.h"
#include <cassert>
#include <limits>
#include <iostream>
int main() {
    using namespace rt_spot_shadow;
    Task task{}; task.normal[1]=1; task.lightCenter[1]=4;
    task.sampleCount=32; task.instanceMask=1; task.originBias=0.001f;
    assert(Valid(task));
    task.sampleCount=0; assert(!Valid(task));
    task.sampleCount=65; assert(!Valid(task));
    task.sampleCount=64; task.instanceMask=256; assert(!Valid(task));
    task.instanceMask=1; task.receiver[0]=std::numeric_limits<float>::infinity(); assert(!Valid(task));
    task.receiver[0]=0; task.endpointBias=-1; assert(!Valid(task));
    task.endpointBias=0; task.normal[1]=0; assert(!Valid(task));
    assert(Resolve(0.25f,{0.75f,1})==0.75f);
    assert(Resolve(0.25f,{0,0})==0.25f);
    assert(Resolve(0.25f,{2,1})==0.25f);
    assert(Resolve(0.25f,{std::numeric_limits<float>::quiet_NaN(),1})==0.25f);
    std::cout << "PASS: shadow input layout, bounds, invalid-result fallback and per-light replacement\n";
}
