#pragma once
#include "scene_traversal_closure.h"
#include "producer_table.h"
#include "frozen_geometry.h"
#include "pt_owned_scene.h"
#include <set>
#include <tuple>
namespace gi_reuse {
// Witnesses must carry the allocation identities observed at the BLAS build,
// not identities recovered by looking up the same address later.
struct GISceneGeometry {
    SceneASReference blas;
    std::uint32_t geometry=0;
    PTOwnedScene::Triangle triangle{};
    std::shared_ptr<const GIFrozenGeometry> frozen;
};
struct GISceneProducerPlan {
    SceneTraversalClosure closure;
    GIProducerTable table;
    std::vector<std::uint64_t> allocationTokens;
    std::uint64_t ownerRevision=0;
};
enum class GISceneProducerError {None,Identity,Closure,Geometry,Table,Mapping,Conflict,Empty,Allocation};
// Called under the owner's lock. Host must acquire all returned allocation
// leases before releasing that lock and retain the complete traversal closure
// through GPU completion. This function does not establish resource lifetime.
template<std::size_t Capacity> GISceneProducerError PrepareGISceneProducer(
    const OrderedASMetadata<Capacity>& history,std::uint64_t cut,std::uint64_t epoch,
    SceneASReference top,const SceneByteView& instances,
    const std::vector<SceneASReference>& perInstance,
    const std::vector<GISceneGeometry>& capturedGeometry,const PTOwnedScene::Index& owners,
    const NativeGITable& native,const std::vector<PTNativeBinding::Binding>& bindings,
    std::uint64_t producerPipeline,const PTShaderTableAdapter::Exports& exports,
    GISceneProducerPlan& output) noexcept {
    using namespace PTShaderTableAdapter;
    if(!epoch||!producerPipeline||!native.pipeline||!native.version||!owners.Revision()||
       bindings.empty()||bindings.size()>65536||capturedGeometry.size()>131072||
       exports.privateOpaque==Identifier{}||exports.privateAlpha==Identifier{}||
       exports.privateOpaque==exports.privateAlpha)return GISceneProducerError::Identity;
    try {
        GISceneProducerPlan next;
        if(ResolveTraversalClosure(history,cut,epoch,top,instances,perInstance,next.closure)!=TraversalError::None)
            return GISceneProducerError::Closure;
        NativeGITable rebuilt;
        if(!BuildNativeGITable(native.pipeline,native.version,1,bindings,exports.nativeOpaque,exports.nativeAlpha,rebuilt)||
           rebuilt.bytes!=native.bytes||rebuilt.active!=native.active)return GISceneProducerError::Table;
        using Key=std::tuple<std::uint64_t,std::uint64_t,std::uint64_t,std::uint32_t>;
        std::map<Key,const GISceneGeometry*> witnesses;
        for(const auto& g:capturedGeometry){
            if(!g.blas.address||!g.blas.token||!g.blas.revision||
               !witnesses.emplace(Key{g.blas.address,g.blas.token,g.blas.revision,g.geometry},&g).second)
                return GISceneProducerError::Geometry;
        }
        ASRecord tlas;
        if(history.Select(cut,top.token,top.address,tlas)!=PTSceneOrder::Status::ready)return GISceneProducerError::Closure;
        const auto* bytes=instances.Read(tlas.instances,std::uint64_t(tlas.count)*64);
        if(!bytes)return GISceneProducerError::Closure;
        auto& table=next.table;
        table.nativePipeline=native.pipeline;table.writerVersion=native.version;
        table.producerPipeline=producerPipeline;table.epoch=epoch;
        table.records.resize(bindings.size()*2);table.geometry.resize(bindings.size());table.bound.resize(bindings.size());
        std::set<std::uint64_t> tokens;
        for(std::uint32_t i=0;i<tlas.count;++i){
            InstanceRecord instance;
            if(!DecodeInstance(bytes+std::size_t(i)*64,instance))return GISceneProducerError::Closure;
            if(!(instance.mask&1)||!instance.blas)continue;
            for(float value:instance.transform)if(!std::isfinite(value))return GISceneProducerError::Geometry;
            const auto& ref=perInstance[i];ASRecord bottom;
            if(history.Select(cut,ref.token,ref.address,bottom)!=PTSceneOrder::Status::ready||
               bottom.revision!=ref.revision||!bottom.geometry)return GISceneProducerError::Closure;
            for(std::uint32_t g=0;g<bottom.count;++g){
                const auto row=std::uint64_t(instance.hitGroup)+std::uint64_t(g)*2;
                if((row&1)||row>=table.records.size())return GISceneProducerError::Mapping;
                const auto found=witnesses.find(Key{ref.address,ref.token,ref.revision,g});
                if(found==witnesses.end())return GISceneProducerError::Geometry;
                const auto& witness=found->second->triangle;const auto* triangle=bottom.geometry->At(g);
                if(!triangle||GeometryMetadata::Validate(*triangle)!=GeometryReject::None||triangle->transform||
                   witness.indices!=triangle->indices||witness.positions!=triangle->vertices||
                   witness.positionStride!=triangle->stride||witness.indexStride!=GeometryMetadata::IndexSize(*triangle)||
                   witness.indexCount!=triangle->indexCount||witness.vertexCount!=triangle->vertexCount)
                    return GISceneProducerError::Geometry;
                const auto binding=static_cast<std::size_t>(row/2);PTOwnedScene::Index::Resolved resolved;
                if(owners.ResolveExact(bindings[binding],witness,epoch,resolved)!=PTOwnedScene::Status::candidate)
                    return GISceneProducerError::Geometry;
                PTShaderTableAdapter::Bytes converted;
                if(Convert(native.bytes.data()+row*NativeStride,NativeStride,exports,&resolved.geometry,converted)!=Status::ready)
                    return GISceneProducerError::Table;
                if(found->second->frozen&&!RebaseConvertedGeometry(*found->second->frozen,resolved.geometry,converted))
                    return GISceneProducerError::Geometry;
                if(table.bound[binding]&&table.records[row]!=converted)return GISceneProducerError::Conflict;
                table.records[row]=converted;table.geometry[binding]=resolved.geometry;table.bound[binding]=1;
                for(auto token:resolved.tokens)tokens.insert(token);
            }
        }
        if(tokens.empty())return GISceneProducerError::Empty;
        next.allocationTokens.assign(tokens.begin(),tokens.end());next.ownerRevision=owners.Revision();
        output=std::move(next);return GISceneProducerError::None;
    }catch(...){return GISceneProducerError::Allocation;}
}
}
