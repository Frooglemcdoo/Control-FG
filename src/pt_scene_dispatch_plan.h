#pragma once
#include "pt_scene_topology.h"
#include "pt_owned_scene.h"
#include "pt_shader_table_adapter.h"
#include <algorithm>
// Assemble one complete private hit table from explicitly selected snapshots.
// This is CPU preflight, NOT permission to dispatch: the host must retain the
// listed allocation tokens, establish GPU ordering/states, and fence retirement.
// No "latest TLAS" lookup and no partial-table publication are permitted here.
namespace PTSceneDispatch {
using U64=std::uint64_t;
struct Selection {
    U64 tlas=0,sceneVersion=0,hitTable=0,tableVersion=0,pipeline=0;
};
struct Scene {
    U64 address=0,version=0;
    PTSceneTopology::Index topology;
    std::vector<PTSceneTopology::Instance> instances;
};
struct Table {
    U64 address=0,version=0,pipeline=0;
    unsigned rays=0;
    std::vector<PTNativeBinding::Binding> bindings;
    // Index and attribute allocation identities captured with each native row.
    std::vector<std::array<U64,2>> allocationTokens;
    std::vector<unsigned char> bytes;
};
struct Plan {
    Selection selection{};
    U64 privatePipeline=0;
    std::vector<PTShaderTableAdapter::Bytes> records;
    std::vector<U64> allocationTokens;
};
struct ExportSet {
    U64 nativePipeline=0,privatePipeline=0;
    std::vector<PTShaderTableAdapter::Exports> rays;
};
enum class Status {prepared,selection,size,topology,unreferenced,allocation,binding,conversion};
struct Failure {Status status=Status::prepared;std::size_t record=0;};
inline Failure Prepare(const Selection& selected,const Scene& scene,const Table& table,
                       const PTOwnedScene::Index& owners,const ExportSet& exports,
                       Plan& output){
    output={};
    if(!selected.tlas||!selected.sceneVersion||!selected.hitTable||!selected.tableVersion||!selected.pipeline||
       selected.tlas!=scene.address||selected.sceneVersion!=scene.version||selected.hitTable!=table.address||
       selected.tableVersion!=table.version||selected.pipeline!=table.pipeline||exports.nativePipeline!=table.pipeline||!exports.privatePipeline)return {Status::selection,0};
    if(!table.rays||table.rays>2||exports.rays.size()!=table.rays||table.bindings.empty()||table.bindings.size()>65536||
       table.allocationTokens.size()!=table.bindings.size()||table.bytes.size()!=table.bindings.size()*table.rays*PTShaderTableAdapter::NativeStride||
       scene.instances.empty()||scene.instances.size()>32768)return {Status::size,0};
    std::vector<PTSceneTopology::Mapping> mappings;
    scene.topology.Check(scene.instances,table.bindings,table.rays,&mappings);
    if(mappings.size()!=table.bindings.size())return {Status::topology,0};
    Plan pending;pending.selection=selected;pending.privatePipeline=exports.privatePipeline;pending.records.resize(table.bindings.size()*table.rays);
    for(std::size_t i=0;i<table.bindings.size();++i){
        if(!mappings[i].referenced)return {Status::unreferenced,i*table.rays};
        const auto& g=mappings[i].geometry;const auto& binding=table.bindings[i];
        PTOwnedScene::Index::Resolved resolved;
        const PTOwnedScene::Triangle triangle{g.indices,g.positions,g.stride,g.indexToken,g.positionToken,g.indexStride,g.indexCount,g.vertexCount};
        if(owners.ResolveExact(binding,triangle,scene.version,resolved)!=PTOwnedScene::Status::candidate)return {Status::allocation,i*table.rays};
        if(resolved.tokens[0]!=table.allocationTokens[i][0]||resolved.tokens[2]!=table.allocationTokens[i][1])return {Status::allocation,i*table.rays};
        pending.allocationTokens.insert(pending.allocationTokens.end(),std::begin(resolved.tokens),std::end(resolved.tokens));
        for(unsigned ray=0;ray<table.rays;++ray){
            const std::size_t record=i*table.rays+ray;
            const auto* source=table.bytes.data()+record*PTShaderTableAdapter::NativeStride;
            // The byte snapshot must be exactly the same local root that was
            // correlated to the selected BLAS, including unused native fields.
            if(std::memcmp(source+32,&binding.indices,8)||std::memcmp(source+40,&binding.attributes,8)||
               std::memcmp(source+48,binding.constants,40))return {Status::binding,record};
            if(PTShaderTableAdapter::Convert(source,PTShaderTableAdapter::NativeStride,exports.rays[ray],&resolved.geometry,pending.records[record])!=PTShaderTableAdapter::Status::ready)
                return {Status::conversion,record};
        }
    }
    std::sort(pending.allocationTokens.begin(),pending.allocationTokens.end());
    pending.allocationTokens.erase(std::unique(pending.allocationTokens.begin(),pending.allocationTokens.end()),pending.allocationTokens.end());
    output=std::move(pending);return {};
}
}
