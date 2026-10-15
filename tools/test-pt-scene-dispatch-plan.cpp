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
    // Compare both ray records byte-for-byte with independent full conversion.
    PTOwnedScene::Index::Resolved referenceGeometry;
    const PTOwnedScene::Triangle referenceTriangle{0x1000,0x2000,12,1,2,2,3,3};
    assert(owners.ResolveExact(b,referenceTriangle,scene.version,referenceGeometry)==PTOwnedScene::Status::candidate);
    for(unsigned ray=0;ray<2;++ray){PTShaderTableAdapter::Bytes referenceRecord;
        assert(PTShaderTableAdapter::Convert(table.bytes.data()+ray*96,96,exports.rays[ray],&referenceGeometry.geometry,referenceRecord)==PTShaderTableAdapter::Status::ready);
        assert(plan.records[ray]==referenceRecord);
    }
    const auto originalTable=table;const auto originalExports=exports;
    for(unsigned ray=0;ray<2;++ray)for(unsigned byte=32;byte<88;++byte){
        table=originalTable;table.bytes[ray*96+byte]^=1;
        const auto rejected=prepare();assert(rejected.status==Status::binding&&rejected.record==ray&&plan.records.empty());
    }
    table=originalTable;
    for(unsigned ray=0;ray<2;++ray){
        exports=originalExports;exports.rays[ray].privateAlpha={};
        assert(prepare().status==Status::conversion&&plan.records.empty());
        exports=originalExports;exports.rays[ray].nativeAlpha=exports.rays[ray].nativeOpaque;
        assert(prepare().status==Status::conversion&&plan.records.empty());
    }
    exports=originalExports;
    std::memcpy(table.bytes.data()+96,exports.rays[1].nativeAlpha.data(),32);
    assert(prepare().status==Status::prepared&&plan.records[1][0]==7+1);
    table=originalTable;assert(prepare().status==Status::prepared);
    // Many instances of the same immutable BLAS retain one version/token.
    const auto singleInstancePlan=plan;scene.instances.assign(64,instance);
    assert(prepare().status==Status::prepared&&singleInstancePlan.records==plan.records&&
        singleInstancePlan.allocationTokens==plan.allocationTokens&&singleInstancePlan.bottomVersions==plan.bottomVersions);
    scene.instances={instance};
    // An unowned row is harmless only if the exact submitted TLAS cannot reach
    // it. A later registration must not repair the zero captured owner token.
    auto sparseLayout=layout;sparseLayout.bytes=512;sparseLayout.hitRecords=4;
    auto missing=b;missing.indices=0x7000;missing.attributes=0x8000;
    Table sparse;assert(PTNativeShaderTable::Build(sparseLayout,identifiers,2,2,{b,missing},owners,sparse));
    assert(!sparse.allocationTokens[1][0]&&!sparse.allocationTokens[1][1]);
    Plan sparsePlan;assert(Prepare(selected,scene,sparse,owners,exports,sparsePlan).status==Status::prepared);
    assert(sparsePlan.records.size()==4&&sparsePlan.records[2][0]==5);
    for(std::size_t n=32;n<sparsePlan.records[2].size();++n)assert(!sparsePlan.records[2][n]);
    auto reachable=scene;reachable.instances[0].contributionFlags=2;
    assert(reachable.topology.Build(0xb000,{{0x7000,0x2000,12,2,3,3,99,2}}));
    assert(Prepare(selected,reachable,sparse,owners,exports,sparsePlan).status==Status::rowIndexOwner&&sparsePlan.records.empty());
    owners.Insert({99,0x7000,8});owners.Insert({100,0x8000,64});
    assert(Prepare(selected,reachable,sparse,owners,exports,sparsePlan).status==Status::rowIndexOwner);
    owners.Remove(99);owners.Remove(100);
    sparse.allocationTokens[0][1]=0;
    assert(Prepare(selected,scene,sparse,owners,exports,sparsePlan).status==Status::rowAttributeOwner);
    assert(!SameDispatch(plan,plan)); // Unversioned fixture cannot authorize a later submission.
    scene.blas[0xb000].version=17;assert(prepare().status==Status::prepared);
    // Reuse requires exact scene inputs, including changes that leave versions
    // and shader-table bytes unchanged. Do not rely on a version-only shortcut.
    assert(SameScene(scene,scene));
    auto alteredScene=scene;alteredScene.instances[0].transform[0]=1;assert(!SameScene(scene,alteredScene));
    alteredScene=scene;alteredScene.instances[0].contributionFlags=2;assert(!SameScene(scene,alteredScene));
    alteredScene=scene;alteredScene.instances[0].idMask=0;assert(!SameScene(scene,alteredScene));
    alteredScene=scene;++alteredScene.tlas.token;assert(!SameScene(scene,alteredScene));
    alteredScene=scene;++alteredScene.tlas.bytes;assert(!SameScene(scene,alteredScene));
    alteredScene=scene;++alteredScene.version;assert(!SameScene(scene,alteredScene));
    alteredScene=scene;++alteredScene.blas[0xb000].version;assert(!SameScene(scene,alteredScene));
    alteredScene=scene;++alteredScene.blas[0xb000].token;assert(!SameScene(scene,alteredScene));
    alteredScene=scene;alteredScene.blas.clear();assert(!SameScene(scene,alteredScene));
    alteredScene=scene;assert(alteredScene.topology.Build(0xb000,{{0x1000,0x2010,12,2,3,3,1,2}}));
    assert(!SameScene(scene,alteredScene));
    alteredScene=scene;assert(alteredScene.topology.Build(0xb000,{{0x1000,0x2000,12,2,3,3,1,99}}));
    assert(!SameScene(scene,alteredScene));
    const auto preparedAtRayPass=plan;assert(SameDispatch(preparedAtRayPass,plan));
    assert(SameDispatch(plan,plan));
    // Identity may skip equality work, never the validity contract.
    auto invalidIdentity=plan;invalidIdentity.bottomVersions.begin()->second=0;
    assert(!SameDispatch(invalidIdentity,invalidIdentity));
    invalidIdentity=plan;invalidIdentity.records.clear();assert(!SameDispatch(invalidIdentity,invalidIdentity));
    invalidIdentity=plan;invalidIdentity.selection.tableVersion=0;assert(!SameDispatch(invalidIdentity,invalidIdentity));
    auto invalidScene=scene;invalidScene.address=0;assert(!SameScene(invalidScene,invalidScene));
    invalidScene=scene;invalidScene.version=0;assert(!SameScene(invalidScene,invalidScene));
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
    // Odd triangle count: six native bytes, eight private raw-load bytes.
    copy->indexBytes=6;copy->indexCapacity=8;
    assert(PTNativeShaderTable::Build(layout,identifiers,table.version,2,{b},owners,table));
    assert(table.allocationTokens[0][0]==6);
    assert(scene.topology.Build(0xb000,{{0x1000,0x2000,12,2,3,3,6,2,copy}}));
    assert(prepare().status==Status::prepared);
    copy->indexCapacity=6;assert(prepare().status==Status::allocation);copy->indexCapacity=8;
    copy->indexBytes=5;assert(prepare().status==Status::allocation);copy->indexBytes=6;
    // Large original SBT range, small R16 slice starting on the high halfword.
    // Private roots must not point before the allocated copy or retain the old offset.
    copy->indexBase=0x1000;copy->indexBytes=4096;copy->indexCopyBase=0x1080;copy->indexCopyBytes=8;copy->indexCapacity=8;
    owners.Remove(6);owners.Insert({6,0x1000,4096});
    b.constants[0]=0x82;b.constants[8]=4096;
    assert(PTNativeShaderTable::Build(layout,identifiers,table.version,2,{b},owners,table));
    assert(scene.topology.Build(0xb000,{{0x1082,0x2000,12,2,3,3,6,2,copy}}));
    assert(prepare().status==Status::prepared);
    for(const auto& r:plan.records){
        std::memcpy(&copiedAddress,r.data()+32,8);assert(copiedAddress==0x5000);
        std::memcpy(&limit,r.data()+56,4);assert(limit==2);
        std::memcpy(&limit,r.data()+56+8*4,4);assert(limit==8);
    }
    copy->indexCopyBytes=7;assert(prepare().status==Status::allocation);copy->indexCopyBytes=8;
    copy->indexCopyBase=0x1084;assert(prepare().status==Status::allocation);copy->indexCopyBase=0x1080;
    copy->indexCapacity=7;assert(prepare().status==Status::allocation);copy->indexCapacity=8;
    copy->owner.reset();assert(prepare().status==Status::allocation&&plan.records.empty());
    std::puts("PASS dispatch preflight: selected scene/table/PSO, frozen topology, complete SBT, exact geometry, allocation generations and padded bounds.");
}
