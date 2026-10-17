#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
#include "../src/pt_emitter_plan.h"
#include <cassert>
#include <cstdio>
int main(){
    PTSceneDispatch::Scene s;s.version=4;
    PTSceneDispatch::Plan p;p.selection.sceneVersion=4;p.records.resize(2);
    PTShaderTableAdapter::Identifier alpha{};alpha[0]=7;
    auto& r=p.records[0];std::memcpy(r.data(),alpha.data(),32);
    const std::uint64_t addresses[]={100,200,300};std::memcpy(r.data()+32,addresses,24);
    std::uint32_t constants[12]={0,4,16,9,0,4,12,0,12,48,12,36};std::memcpy(r.data()+56,constants,48);
    PTSceneTopology::Geometry geometry{};geometry.indexCount=3;
    assert(s.topology.Build(900,{geometry}));
    PTSceneTopology::Instance instance{};instance.blas=900;instance.idMask=0x01000000;
    instance.transform[0]=instance.transform[5]=instance.transform[10]=1;
    s.instances.push_back(instance);instance.transform[3]=12;s.instances.push_back(instance);
    std::vector<PTEmitters::Geometry> out;assert(PTEmitters::Plan(s,p,alpha,out)&&out.size()==2);
    assert(out[0].indices==100&&out[0].attributes==200&&out[0].positions==300);
    assert(out[0].constants[24]==1&&out[0].constants[25]==1);
    float translation=0;std::memcpy(&translation,out[1].constants.data()+15,4);assert(translation==12);
    s.instances[1].idMask=0x02000000;assert(PTEmitters::Plan(s,p,alpha,out)&&out.size()==1);
    s.instances[0].contributionFlags=0x04000000;assert(PTEmitters::Plan(s,p,alpha,out)&&out[0].constants[25]==0);
    s.instances[0].contributionFlags=0x0c000000;assert(!PTEmitters::Plan(s,p,alpha,out)&&out.empty());
    s.instances[0].contributionFlags=2;assert(!PTEmitters::Plan(s,p,alpha,out));
    s.instances[0].contributionFlags=0;s.version=5;assert(!PTEmitters::Plan(s,p,alpha,out));
    s.version=4;s.instances[0].blas=901;assert(!PTEmitters::Plan(s,p,alpha,out));
    puts("PASS: retained emitter geometry mapping, distinct instances, transforms, masks, alpha flags and stale/out-of-range rejection");
}
