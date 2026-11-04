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
enum class GISceneGeometryStage {WitnessIdentity,Transform,WitnessMissing,Metadata,Resolve,Convert,Rebase,Conflict,Count};
struct GISceneGeometrySample {
 GISceneGeometryStage stage{};std::uint32_t instance=0,geometry=0;std::uint64_t row=0;
 SceneASReference blas{};PTNativeBinding::Binding binding{};PTOwnedScene::Triangle witness{};
 TriangleMetadata metadata{};unsigned metadataError=0,convertError=0;PTSceneAdmission::Geometry resolved{};PTOwnedScene::Index::ExactDiagnostic exact{};
 PTOwnedScene::Status status=PTOwnedScene::Status::candidate;GIRebaseDiagnostic rebase{};
 GIFrozenRange frozenIndices{},frozenPositions{};
};
struct GISceneProducerDiagnostic {
 std::uint64_t checked=0,valid=0,reused=0,counts[static_cast<unsigned>(GISceneGeometryStage::Count)]{};
 std::vector<GISceneGeometrySample> samples;
 void Add(const GISceneGeometrySample& sample){
  auto& count=counts[static_cast<unsigned>(sample.stage)];++count;
  if(count<=4)samples.push_back(sample); // bounded examples per stage; count every failure
 }
};
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
    GISceneProducerPlan& output,GISceneProducerDiagnostic* diagnostic=nullptr) noexcept {
    if(diagnostic)*diagnostic={};
    using namespace PTShaderTableAdapter;
    if(!epoch||!producerPipeline||!native.pipeline||!native.version||!owners.Revision()||
       bindings.empty()||bindings.size()>65536||capturedGeometry.size()>131072||
       exports.privateOpaque==Identifier{}||exports.privateAlpha==Identifier{}||
       exports.privateOpaque==exports.privateAlpha)return GISceneProducerError::Identity;
    try {
        GISceneProducerPlan next;
        GISceneProducerError failure=GISceneProducerError::None;
        const auto reject=[&](GISceneGeometrySample sample,GISceneProducerError error){
            if(failure==GISceneProducerError::None)failure=error;
            if(diagnostic)diagnostic->Add(sample);
        };
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
                {GISceneGeometrySample sample;sample.stage=GISceneGeometryStage::WitnessIdentity;sample.blas=g.blas;sample.geometry=g.geometry;if(!diagnostic)return GISceneProducerError::Geometry;reject(sample,GISceneProducerError::Geometry);}
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
        // Repeated adjacent instances commonly share all local-root data.
        // Cache only a fully validated result inside this owner-locked call.
        bool cachedMapping=false;Key mappingKey{};std::array<unsigned char,NativeStride> mappingSource{};
        PTOwnedScene::Index::Resolved mappingResolved{};PTShaderTableAdapter::Bytes mappingRecord{};
        std::map<std::tuple<std::uint64_t,std::uint64_t,std::uint64_t>,ASRecord> selectedBottoms;
        for(std::uint32_t i=0;i<tlas.count;++i){
            InstanceRecord instance;
            if(!DecodeInstance(bytes+std::size_t(i)*64,instance))return GISceneProducerError::Closure;
            if(!(instance.mask&1)||!instance.blas)continue;
            GISceneGeometrySample sample;sample.instance=i;sample.blas=perInstance[i];
            bool finite=true;for(float value:instance.transform)finite=finite&&std::isfinite(value);
            if(!finite){sample.stage=GISceneGeometryStage::Transform;if(!diagnostic)return GISceneProducerError::Geometry;reject(sample,GISceneProducerError::Geometry);continue;}
            const auto& ref=perInstance[i];const auto key=std::make_tuple(ref.address,ref.token,ref.revision);
            auto cached=selectedBottoms.find(key);
            if(cached==selectedBottoms.end()){
                ASRecord selected;
                if(history.Select(cut,ref.token,ref.address,selected)!=PTSceneOrder::Status::ready||
                   selected.revision!=ref.revision||!selected.geometry)return GISceneProducerError::Closure;
                cached=selectedBottoms.emplace(key,std::move(selected)).first;
            }
            const auto& bottom=cached->second;
            for(std::uint32_t g=0;g<bottom.count;++g){
                const auto row=std::uint64_t(instance.hitGroup)+std::uint64_t(g)*2;
                if((row&1)||row>=table.records.size())return GISceneProducerError::Mapping;
                sample.geometry=g;sample.row=row;sample.binding=bindings[static_cast<std::size_t>(row/2)];
                sample.witness={};sample.metadata={};sample.metadataError=sample.convertError=0;sample.resolved={};sample.exact={};sample.rebase={};sample.frozenIndices={};sample.frozenPositions={};sample.status=PTOwnedScene::Status::candidate;
                if(diagnostic)++diagnostic->checked;
                const auto found=witnesses.find(Key{ref.address,ref.token,ref.revision,g});
                if(found==witnesses.end()){sample.stage=GISceneGeometryStage::WitnessMissing;if(!diagnostic)return GISceneProducerError::Geometry;reject(sample,GISceneProducerError::Geometry);continue;}
                const auto& witness=found->second->triangle;const auto* triangle=bottom.geometry->At(g);
                sample.witness=witness;sample.metadata=triangle?*triangle:TriangleMetadata{};sample.metadataError=triangle?unsigned(GeometryMetadata::Validate(*triangle)):UINT32_MAX;
                if(found->second->frozen){sample.frozenIndices=found->second->frozen->indices;sample.frozenPositions=found->second->frozen->positions;}
                if(!triangle||GeometryMetadata::Validate(*triangle)!=GeometryReject::None||triangle->transform||
                   witness.indices!=triangle->indices||witness.positions!=triangle->vertices||
                   witness.positionStride!=triangle->stride||witness.indexStride!=GeometryMetadata::IndexSize(*triangle)||
                   witness.indexCount!=triangle->indexCount||witness.vertexCount!=triangle->vertexCount)
                    {sample.stage=GISceneGeometryStage::Metadata;if(!diagnostic)return GISceneProducerError::Geometry;reject(sample,GISceneProducerError::Geometry);continue;}
                const auto binding=static_cast<std::size_t>(row/2);PTOwnedScene::Index::Resolved resolved;
                PTShaderTableAdapter::Bytes converted;
                const Key currentKey{ref.address,ref.token,ref.revision,g};
                if(cachedMapping&&mappingKey==currentKey&&
                   !std::memcmp(mappingSource.data(),native.bytes.data()+row*NativeStride,NativeStride)){
                    resolved=mappingResolved;converted=mappingRecord;if(diagnostic)++diagnostic->reused;
                }else{
                    // Typed source bounds are permitted only when a frozen copy
                    // will be rebased and re-admitted for raw DWORD reads below.
                    const auto access=found->second->frozen?PTSceneAdmission::IndexAccess::NativeTyped:PTSceneAdmission::IndexAccess::RawDWORD;
                    sample.status=owners.ResolveExact(bindings[binding],witness,epoch,resolved,diagnostic?&sample.exact:nullptr,access);
                    if(sample.status!=PTOwnedScene::Status::candidate){sample.stage=GISceneGeometryStage::Resolve;if(!diagnostic)return GISceneProducerError::Geometry;reject(sample,GISceneProducerError::Geometry);continue;}
                    sample.resolved=resolved.geometry;
                    sample.convertError=unsigned(Convert(native.bytes.data()+row*NativeStride,NativeStride,exports,&resolved.geometry,converted,access));
                    if(sample.convertError!=unsigned(Status::ready))
                        {sample.stage=GISceneGeometryStage::Convert;if(!diagnostic)return GISceneProducerError::Table;reject(sample,GISceneProducerError::Table);continue;}
                    if(found->second->frozen&&!RebaseConvertedGeometry(*found->second->frozen,resolved.geometry,converted,&sample.rebase))
                        {sample.stage=GISceneGeometryStage::Rebase;if(!diagnostic)return GISceneProducerError::Geometry;reject(sample,GISceneProducerError::Geometry);continue;}

                    cachedMapping=true;mappingKey=currentKey;std::memcpy(mappingSource.data(),native.bytes.data()+row*NativeStride,NativeStride);
                    mappingResolved=resolved;mappingRecord=converted;
                }
                if(table.bound[binding]&&table.records[row]!=converted){sample.stage=GISceneGeometryStage::Conflict;if(!diagnostic)return GISceneProducerError::Conflict;reject(sample,GISceneProducerError::Conflict);continue;}
                if(diagnostic)++diagnostic->valid;
                table.records[row]=converted;table.geometry[binding]=resolved.geometry;table.bound[binding]=1;
                for(auto token:resolved.tokens)tokens.insert(token);
            }
        }
        if(failure!=GISceneProducerError::None)return failure;
        if(tokens.empty())return GISceneProducerError::Empty;
        next.allocationTokens.assign(tokens.begin(),tokens.end());next.ownerRevision=owners.Revision();
        output=std::move(next);return GISceneProducerError::None;
    }catch(...){return GISceneProducerError::Allocation;}
}
}
