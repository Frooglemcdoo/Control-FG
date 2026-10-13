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
    // Captured with the AS build/version, never recovered from a latest-VA lookup.
    struct Acceleration {U64 token=0,bytes=0,version=0;};
    Acceleration tlas;
    std::map<U64,Acceleration> blas;
    PTSceneTopology::Index topology;
    std::vector<PTSceneTopology::Instance> instances;
};
// Exact immutable-input comparison, not merely a version-number shortcut.
// Frozen geometry is shared_ptr<const> and retained by the prepared scene.
inline bool SameScene(const Scene& a,const Scene& b){
    const auto sameAS=[](const Scene::Acceleration& x,const Scene::Acceleration& y){
        return x.token==y.token&&x.bytes==y.bytes&&x.version==y.version;
    };
    if(!a.address||!a.version||a.address!=b.address||a.version!=b.version||!sameAS(a.tlas,b.tlas)||
       a.instances.size()!=b.instances.size()||a.blas.size()!=b.blas.size())return false;
    if(!a.instances.empty()&&std::memcmp(a.instances.data(),b.instances.data(),a.instances.size()*sizeof(PTSceneTopology::Instance)))return false;
    auto next=b.blas.begin();for(const auto& item:a.blas){
        if(item.first!=next->first||!sameAS(item.second,next->second))return false;++next;
    }
    return a.topology.Same(b.topology);
}
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
    U64 ownerRevision=0; // Set only by the locked resource owner after successful preflight.
    std::vector<PTShaderTableAdapter::Bytes> records;
    std::vector<U64> allocationTokens;
    std::map<U64,U64> bottomVersions;
    std::vector<std::shared_ptr<void>> keepAlive;
};
struct ExportSet {
    U64 nativePipeline=0,privatePipeline=0;
    std::vector<PTShaderTableAdapter::Exports> rays;
};
enum class Status {prepared,selection,size,topology,unreferenced,allocation,binding,conversion,rowIndexOwner,rowAttributeOwner};
struct Failure {Status status=Status::prepared;std::size_t record=0;};
inline const char* FailureName(Status status){
    switch(status){
    case Status::prepared:return "scene_prepared";
    case Status::selection:return "scene_selection";
    case Status::size:return "scene_table_size";
    case Status::topology:return "scene_topology";
    case Status::unreferenced:return "scene_unreferenced";
    case Status::allocation:return "scene_allocation";
    case Status::binding:return "scene_binding";
    case Status::conversion:return "scene_conversion";
    case Status::rowIndexOwner:return "scene_row_index_owner";
    case Status::rowAttributeOwner:return "scene_row_attribute_owner";
    }
    return "scene_preflight_unknown";
}
// A later submission may reuse the prepared SBT only if every referenced AS
// version is still the same. Identical records alone do not detect an in-place
// BLAS update that leaves its addresses and local-root constants unchanged.
inline bool SameDispatch(const Plan& a,const Plan& b){
    const auto& x=a.selection;const auto& y=b.selection;
    if(!x.tlas||!x.sceneVersion||!x.hitTable||!x.tableVersion||!x.pipeline||!a.privatePipeline||a.records.empty()||a.bottomVersions.empty())return false;
    for(const auto& bottom:a.bottomVersions)if(!bottom.first||!bottom.second)return false;
    return x.tlas==y.tlas&&x.sceneVersion==y.sceneVersion&&x.hitTable==y.hitTable&&x.tableVersion==y.tableVersion&&
        x.pipeline==y.pipeline&&a.privatePipeline==b.privatePipeline&&a.records==b.records&&
        a.allocationTokens==b.allocationTokens&&a.bottomVersions==b.bottomVersions;
}
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
    Plan pending;
    const auto retainAS=[&](U64 address,const Scene::Acceleration& as){
        if(!as.token||!as.bytes||owners.Find(address,as.bytes)!=as.token)return false;
        pending.allocationTokens.push_back(as.token);return true;
    };
    if(!retainAS(scene.address,scene.tlas))return {Status::allocation,0};
    for(const auto& instance:scene.instances){
        if(!(instance.idMask>>24))continue;
        const auto found=scene.blas.find(instance.blas);
        if(found==scene.blas.end()||!retainAS(instance.blas,found->second))return {Status::allocation,0};
        pending.bottomVersions.emplace(instance.blas,found->second.version);
    }
    pending.selection=selected;pending.privatePipeline=exports.privatePipeline;pending.records.resize(table.bindings.size()*table.rays);
    for(std::size_t i=0;i<table.bindings.size();++i){
        if(!mappings[i].referenced){
            // Check() proved every visible instance and geometry maps elsewhere.
            // Keep row numbers intact with valid private identifiers and zero
            // roots; these rows cannot execute for this immutable scene cut.
            for(unsigned ray=0;ray<table.rays;++ray){const auto& e=exports.rays[ray];
                if(e.privateOpaque==PTShaderTableAdapter::Identifier{})return {Status::conversion,i*table.rays+ray};
                auto& record=pending.records[i*table.rays+ray];record.fill(0);
                std::memcpy(record.data(),e.privateOpaque.data(),32);
            }
            continue;
        }
        if(!table.allocationTokens[i][0])return {Status::rowIndexOwner,i*table.rays};
        if(!table.allocationTokens[i][1])return {Status::rowAttributeOwner,i*table.rays};
        const auto& g=mappings[i].geometry;const auto& binding=table.bindings[i];
        PTOwnedScene::Index::Resolved resolved;
        const PTOwnedScene::Triangle triangle{g.indices,g.positions,g.stride,g.indexToken,g.positionToken,g.indexStride,g.indexCount,g.vertexCount};
        if(g.frozen){
            const auto& f=*g.frozen;
            if(!f.owner||!f.privateIndex||!f.privatePosition||g.stride>UINT32_MAX)return {Status::allocation,i*table.rays};
            // Reconstruct the ORIGINAL build inputs for validation. The current
            // GPU address may now name a different allocation; never use it to
            // recover released positions. Attribute identity remains live.
            const auto attributeToken=owners.Find(binding.attributes,binding.constants[9]);
            if(!attributeToken||attributeToken!=table.allocationTokens[i][1]||g.indexToken!=table.allocationTokens[i][0])return {Status::allocation,i*table.rays};
            const auto& c=binding.constants;U64 indexAddress=0;
            const U64 padded=(U64(c[8])+3)&~U64(3);
            if(f.indexCopyBase&&f.indexBytes>UINT64_MAX-3)return {Status::allocation,i*table.rays};
            const U64 capacity=f.indexCopyBase?((f.indexBytes+3)&~U64(3)):(f.indexCapacity?f.indexCapacity:f.indexBytes);
            if(!PTNativeBinding::IndexAddress(binding,indexAddress)||indexAddress!=g.indices||
               binding.indices<f.indexBase||binding.indices-f.indexBase>f.indexBytes||
               c[8]>f.indexBytes-(binding.indices-f.indexBase)||
               binding.indices-f.indexBase>capacity||padded>capacity-(binding.indices-f.indexBase))return {Status::allocation,i*table.rays};
            if(f.indexCopyBase){
                const U64 needed=U64(g.indexCount)*g.indexStride;
                if(!f.indexCopyBytes||!f.indexCapacity||f.indexCopyBase%4||f.indexCopyBase<f.indexBase||
                   f.indexCopyBase-f.indexBase>f.indexBytes||f.indexCopyBytes>f.indexBytes-(f.indexCopyBase-f.indexBase)||g.indices<f.indexCopyBase||
                   g.indices-f.indexCopyBase>f.indexCopyBytes||needed>f.indexCopyBytes-(g.indices-f.indexCopyBase)||
                   g.indices-f.indexCopyBase>UINT32_MAX||f.indexCapacity>UINT32_MAX||
                   f.indexCopyBytes>f.indexCapacity||f.indexCopyBytes>UINT64_MAX-3||
                   ((f.indexCopyBytes+3)&~U64(3))>f.indexCapacity)return {Status::allocation,i*table.rays};
            }
            resolved.geometry={{{binding.indices,padded,scene.version},g.indices},{{f.positionBase,f.positionBytes,scene.version},g.positions},
                {{binding.attributes,c[9],scene.version},binding.attributes},g.indexStride,g.indexCount,g.vertexCount,
                static_cast<std::uint32_t>(g.stride),c[2],c[5],c[6],c[3],244000u/35u,scene.version,true,true,true};
            resolved.tokens[0]=g.indexToken;resolved.tokens[1]=g.positionToken;resolved.tokens[2]=attributeToken;
            pending.keepAlive.push_back(f.owner);pending.allocationTokens.push_back(attributeToken);
        }else{
            if(owners.ResolveExact(binding,triangle,scene.version,resolved)!=PTOwnedScene::Status::candidate)return {Status::allocation,i*table.rays};
            pending.allocationTokens.insert(pending.allocationTokens.end(),std::begin(resolved.tokens),std::end(resolved.tokens));
        }
        if(resolved.tokens[0]!=table.allocationTokens[i][0]||resolved.tokens[2]!=table.allocationTokens[i][1])return {Status::allocation,i*table.rays};
        for(unsigned ray=0;ray<table.rays;++ray){
            const std::size_t record=i*table.rays+ray;
            const auto* source=table.bytes.data()+record*PTShaderTableAdapter::NativeStride;
            // The byte snapshot must be exactly the same local root that was
            // correlated to the selected BLAS, including unused native fields.
            if(std::memcmp(source+32,&binding.indices,8)||std::memcmp(source+40,&binding.attributes,8)||
               std::memcmp(source+48,binding.constants,40))return {Status::binding,record};
            if(PTShaderTableAdapter::Convert(source,PTShaderTableAdapter::NativeStride,exports.rays[ray],&resolved.geometry,pending.records[record])!=PTShaderTableAdapter::Status::ready)
                return {Status::conversion,record};
            if(g.frozen){
                const auto& f=*g.frozen;
                const U64 indices=f.indexCopyBase?f.privateIndex:f.privateIndex+(binding.indices-f.indexBase),positions=f.privatePosition+(g.positions-f.positionBase);
                std::memcpy(pending.records[record].data()+32,&indices,8);
                std::memcpy(pending.records[record].data()+48,&positions,8);
                if(f.indexCopyBase){
                    const auto offset=static_cast<std::uint32_t>(g.indices-f.indexCopyBase);
                    const auto bytes=static_cast<std::uint32_t>(f.indexCapacity);
                    std::memcpy(pending.records[record].data()+56,&offset,4);
                    std::memcpy(pending.records[record].data()+56+8*4,&bytes,4);
                }
            }
        }
    }
    std::sort(pending.allocationTokens.begin(),pending.allocationTokens.end());
    pending.allocationTokens.erase(std::unique(pending.allocationTokens.begin(),pending.allocationTokens.end()),pending.allocationTokens.end());
    output=std::move(pending);return {};
}
}
