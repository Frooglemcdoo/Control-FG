#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
#include "../src/pt_native_shader_table.h"
#include <cassert>
#include <cstdio>
using namespace PTSceneDispatch;
int main(){
    PTOwnedScene::Index owners;
    owners.Insert({1,0x1000,8});owners.Insert({2,0x2000,36});owners.Insert({3,0x3000,64});
    PTOwnedScene::Triangle tri{0x1000,0x2000,12,0,0,2,3,3};assert(owners.Observe(tri));
    Scene scene;scene.address=0xa000;scene.version=1;
    owners.Insert({10,0xa000,256});owners.Insert({11,0xb000,256});
    scene.tlas={10,256};scene.blas[0xb000]={11,256};
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
    PTNativeShaderTable::Layout layout{0xbf80,320,32,64,32,64,96,2};
    PTNativeShaderTable::Identifiers identifiers;identifiers.pipeline=3;
    identifiers.hit={exports.rays[0].nativeOpaque,exports.rays[1].nativeOpaque,exports.rays[0].nativeAlpha,exports.rays[1].nativeAlpha};
    const auto expected=table;
    assert(PTNativeShaderTable::Build(layout,identifiers,2,2,{b},owners,table));
    assert(table.address==expected.address&&table.bytes==expected.bytes&&table.allocationTokens==expected.allocationTokens);
    auto invalidLayout=layout;invalidLayout.bytes=319;
    assert(!PTNativeShaderTable::Build(invalidLayout,identifiers,2,2,{b},owners,table)&&table.bytes.empty());
    assert(!PTNativeShaderTable::Build(layout,identifiers,2,1,{b},owners,table));
    auto alpha=b;alpha.type=1;
    assert(PTNativeShaderTable::Build(layout,identifiers,2,2,{alpha},owners,table)&&table.bytes[0]==3&&table.bytes[96]==4);
    assert(PTNativeShaderTable::Build(layout,identifiers,2,2,{b},owners,table));
#ifdef _WIN32
    std::array<unsigned char,64> nativeState{};std::memcpy(nativeState.data(),&identifiers.pipeline,8);
    for(unsigned i=0;i<4;++i){const void* id=identifiers.hit[i].data();std::memcpy(nativeState.data()+0x20+i*8,&id,8);}
    PTNativeShaderTable::Identifiers read{};
    assert(PTNativeShaderTable::ReadIdentifiers(nativeState.data(),read)&&read.hit==identifiers.hit&&read.pipeline==3);
    assert(!PTNativeShaderTable::ReadIdentifiers(reinterpret_cast<void*>(1),read)&&read.pipeline==0);
#endif
    auto prepare=[&](){return Prepare(selected,scene,table,owners,exports,plan);};
    assert(prepare().status==Status::prepared&&plan.records.size()==2&&plan.allocationTokens.size()==5);
    assert(!SameDispatch(plan,plan)); // Unversioned fixture cannot authorize a later submission.
    scene.blas[0xb000].version=17;assert(prepare().status==Status::prepared);
    const auto preparedAtRayPass=plan;assert(SameDispatch(preparedAtRayPass,plan));
    ++scene.blas[0xb000].version;assert(prepare().status==Status::prepared);
    assert(plan.records==preparedAtRayPass.records&&!SameDispatch(preparedAtRayPass,plan)); // In-place AS update, same SBT.
    --scene.blas[0xb000].version;assert(prepare().status==Status::prepared);
    auto different=plan;++different.selection.sceneVersion;assert(!SameDispatch(plan,different));
    different=plan;++different.records[0][32];assert(!SameDispatch(plan,different));
    different=plan;++different.allocationTokens[0];assert(!SameDispatch(plan,different));
    different=plan;++different.privatePipeline;assert(!SameDispatch(plan,different));
    owners.Remove(10);owners.Insert({12,0xa000,256});
    assert(prepare().status==Status::allocation&&plan.records.empty());
    scene.tlas.token=12;assert(prepare().status==Status::prepared);
    owners.Remove(11);owners.Insert({13,0xb000,256});
    assert(prepare().status==Status::allocation&&plan.records.empty());
    scene.blas[0xb000].token=13;assert(prepare().status==Status::prepared);
    scene.blas[0xb000].bytes=257;assert(prepare().status==Status::allocation);
    scene.blas[0xb000].bytes=256;
    scene.tlas.bytes=0;assert(prepare().status==Status::allocation);scene.tlas.bytes=256;
    auto masked=instance;masked.idMask=0;masked.blas=0xdead;scene.instances.push_back(masked);
    assert(prepare().status==Status::prepared);scene.instances.pop_back();
    scene.blas.clear();assert(prepare().status==Status::allocation);scene.blas[0xb000]={13,256};
    assert(prepare().status==Status::prepared);
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
    // A build-time GPU copy is authoritative even after original VA reuse.
    auto copy=std::make_shared<PTSceneTopology::FrozenGeometry>();
    copy->indexBase=0x1000;copy->indexBytes=8;copy->positionBase=0x2000;copy->positionBytes=36;
    copy->privateIndex=0x5000;copy->privatePosition=0x6000;copy->owner=std::make_shared<int>(1);
    assert(scene.topology.Build(0xb000,{{0x1000,0x2000,12,2,3,3,1,2,copy}}));
    assert(prepare().status==Status::prepared&&plan.keepAlive.size()==1&&plan.allocationTokens.size()==3);
    U64 copiedAddress=0;std::memcpy(&copiedAddress,plan.records[0].data()+32,8);assert(copiedAddress==0x5000);
    std::memcpy(&copiedAddress,plan.records[0].data()+48,8);assert(copiedAddress==0x6000);
    assert(plan.allocationTokens==std::vector<U64>({7,12,13})); // Only live attributes and AS allocations.
    copy->indexBytes=6;assert(prepare().status==Status::allocation&&plan.records.empty());copy->indexBytes=8;
    table.allocationTokens[0][0]=6;assert(prepare().status==Status::allocation);table.allocationTokens[0][0]=1;
    copy->owner.reset();assert(prepare().status==Status::allocation&&plan.records.empty());
    std::puts("PASS dispatch preflight: selected scene/table/PSO, frozen topology, complete SBT, exact geometry, allocation generations and padded bounds.");
}
