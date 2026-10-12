#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
#include "../src/pt_scene_dispatch_plan.h"
#include <cassert>
#include <cstdio>
using namespace PTSceneDispatch;
int main(){
    PTOwnedScene::Index owners;
    owners.Insert({1,0x1000,8});owners.Insert({2,0x2000,36});owners.Insert({3,0x3000,64});
    PTOwnedScene::Triangle tri{0x1000,0x2000,12,0,0,2,3,3};assert(owners.Observe(tri));
    Scene scene;scene.address=0xa000;scene.version=1;
    assert(scene.topology.Build(0xb000,{{0x1000,0x2000,12,2,3,3,1,2}}));
    PTSceneTopology::Instance instance{};instance.idMask=0xff000001;instance.blas=0xb000;scene.instances={instance};
    Table table;table.address=0xc000;table.version=2;table.pipeline=3;table.rays=2;
    PTNativeBinding::Binding b{};b.indices=0x1000;b.attributes=0x3000;
    b.constants[1]=2;b.constants[2]=16;b.constants[6]=8;b.constants[8]=6;b.constants[9]=64;
    table.bindings={b};table.allocationTokens={{{1,3}}};table.bytes.resize(192);
    ExportSet exports{3,4,std::vector<PTShaderTableAdapter::Exports>(2)};
    for(unsigned ray=0;ray<2;++ray){
        auto& e=exports.rays[ray];e.nativeOpaque[0]=static_cast<unsigned char>(1+ray);e.nativeAlpha[0]=static_cast<unsigned char>(3+ray);
        e.privateOpaque[0]=static_cast<unsigned char>(5+ray);e.privateAlpha[0]=static_cast<unsigned char>(7+ray);
        auto* p=table.bytes.data()+ray*96;std::memcpy(p,e.nativeOpaque.data(),32);
        std::memcpy(p+32,&b.indices,8);std::memcpy(p+40,&b.attributes,8);std::memcpy(p+48,b.constants,40);
    }
    Selection selected{scene.address,scene.version,table.address,table.version,table.pipeline};Plan plan;
    auto prepare=[&](){return Prepare(selected,scene,table,owners,exports,plan);};
    assert(prepare().status==Status::prepared&&plan.records.size()==2&&plan.allocationTokens.size()==3);
    std::uint32_t limit=0;std::memcpy(&limit,plan.records[0].data()+56+8*4,4);assert(limit==8);
    std::memcpy(&limit,plan.records[0].data()+56+9*4,4);assert(limit==64);
    assert(plan.records[0][0]==5&&plan.records[1][0]==6);
    ++selected.sceneVersion;assert(prepare().status==Status::selection&&plan.records.empty());--selected.sceneVersion;
    ++selected.tableVersion;assert(prepare().status==Status::selection);--selected.tableVersion;
    ++selected.pipeline;assert(prepare().status==Status::selection);--selected.pipeline;
    ++exports.nativePipeline;assert(prepare().status==Status::selection);--exports.nativePipeline;
    ++selected.tlas;assert(prepare().status==Status::selection);--selected.tlas;
    ++selected.hitTable;assert(prepare().status==Status::selection);--selected.hitTable;
    table.bytes[96+48+4*4]=9;assert(prepare().status==Status::binding&&plan.records.empty());table.bytes[96+48+4*4]=0;
    table.bytes[96]=99;assert(prepare().status==Status::conversion&&plan.records.empty());table.bytes[96]=2;
    // Freeze the exact scene before a later BLAS update: no global latest lookup.
    auto later=scene;assert(later.topology.Build(0xb000,{{0x1000,0x2400,12,2,3,3,1,4}}));
    assert(prepare().status==Status::prepared);
    assert(Prepare(selected,later,table,owners,exports,plan).status==Status::allocation&&plan.records.empty());
    // Another observed geometry with the same indices must not make exact
    // instance routing ambiguous, but a conflicting reachable instance must.
    owners.Insert({4,0x2400,36});auto other=tri;other.positions=0x2400;assert(owners.Observe(other));
    assert(prepare().status==Status::prepared);
    assert(scene.topology.Build(0xb100,{{0x1000,0x2400,12,2,3,3,1,4}}));
    instance.blas=0xb100;scene.instances.push_back(instance);
    assert(prepare().status==Status::topology&&plan.records.empty());scene.instances.pop_back();
    owners.Remove(2);owners.Insert({5,0x2000,36});
    assert(prepare().status==Status::allocation&&plan.records.empty()); // Reused GPU VA != retained old resource.
    assert(owners.Observe(tri));assert(prepare().status==Status::allocation); // Even identical re-observation cannot revive a frozen scene.
    assert(scene.topology.Build(0xb000,{{0x1000,0x2000,12,2,3,3,1,5}}));
    ++scene.version;selected.sceneVersion=scene.version;assert(prepare().status==Status::prepared);
    owners.Remove(3);owners.Insert({7,0x3000,64});assert(prepare().status==Status::allocation);
    table.allocationTokens[0][1]=7;++table.version;selected.tableVersion=table.version;assert(prepare().status==Status::prepared);
    owners.Remove(1);owners.Insert({6,0x1000,6});assert(owners.Observe(tri));
    assert(prepare().status==Status::allocation&&plan.records.empty()); // No final DWORD padding.
    std::puts("PASS dispatch preflight: selected scene/table/PSO, frozen topology, complete SBT, exact geometry, allocation generations and padded bounds.");
}
