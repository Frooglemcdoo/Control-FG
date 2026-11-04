#pragma once
#include "scene_traversal_closure.h"
#include "producer_table.h"
#include "frozen_geometry.h"
#include "pt_owned_scene.h"
#include "validated_mapping_cache.h"
#include <set>
#include <tuple>
#include <unordered_map>
#include <chrono>
namespace gi_reuse {
// Witnesses must carry the allocation identities observed at the BLAS build,
// not identities recovered by looking up the same address later.
struct GISceneGeometry {
    SceneASReference blas;
    std::uint32_t geometry=0;
    PTOwnedScene::Triangle triangle{};
    std::shared_ptr<const GIFrozenGeometry> frozen;
};
struct GISceneProducerTimings {double closureMS=0,tableMS=0,witnessMS=0,rowsMS=0;};
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
 std::uint64_t checked=0,valid=0,reused=0,persistent=0,counts[static_cast<unsigned>(GISceneGeometryStage::Count)]{};
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
    GISceneProducerPlan& output,GISceneProducerDiagnostic* diagnostic=nullptr,GIValidatedMappingCache* persistent=nullptr,GISceneProducerTimings* timings=nullptr) noexcept {
    if(timings)*timings={};
    using Clock=std::chrono::steady_clock;auto stageStart=Clock::now();
    const auto elapsed=[&](){const auto now=Clock::now();const auto ms=std::chrono::duration<double,std::milli>(now-stageStart).count();stageStart=now;return ms;};
    if(diagnostic)*diagnostic={};
    using namespace PTShaderTableAdapter;
    if(!epoch||!producerPipeline||!native.pipeline||!native.version||!owners.Revision()||
       bindings.empty()||bindings.size()>65536||capturedGeometry.size()>131072||
       exports.privateOpaque==Identifier{}||exports.privateAlpha==Identifier{}||
       exports.privateOpaque==exports.privateAlpha)return GISceneProducerError::Identity;
    try {
        GISceneProducerPlan next;
        if(persistent)persistent->Begin(native.pipeline,producerPipeline,exports);
        GISceneProducerError failure=GISceneProducerError::None;
        const auto reject=[&](GISceneGeometrySample sample,GISceneProducerError error){
            if(failure==GISceneProducerError::None)failure=error;
            if(diagnostic)diagnostic->Add(sample);
        };
        if(ResolveTraversalClosure(history,cut,epoch,top,instances,perInstance,next.closure)!=TraversalError::None)
            return GISceneProducerError::Closure;
        const auto closureMS=elapsed();if(timings)timings->closureMS=closureMS;
        if(!MatchesNativeGITable(native,bindings,exports.nativeOpaque,exports.nativeAlpha))return GISceneProducerError::Table;
        const auto tableMS=elapsed();if(timings)timings->tableMS=tableMS;
        using Key=std::tuple<std::uint64_t,std::uint64_t,std::uint64_t,std::uint32_t>;
        std::map<Key,const GISceneGeometry*> witnesses;
        for(const auto& g:capturedGeometry){
            if(!g.blas.address||!g.blas.token||!g.blas.revision||
               !witnesses.emplace(Key{g.blas.address,g.blas.token,g.blas.revision,g.geometry},&g).second)
                {GISceneGeometrySample sample;sample.stage=GISceneGeometryStage::WitnessIdentity;sample.blas=g.blas;sample.geometry=g.geometry;if(!diagnostic)return GISceneProducerError::Geometry;reject(sample,GISceneProducerError::Geometry);}
        }
        const auto witnessMS=elapsed();if(timings)timings->witnessMS=witnessMS;
        const auto& tlas=next.closure.topRecord;
        const auto* bytes=instances.Read(tlas.instances,std::uint64_t(tlas.count)*64);
        if(!bytes)return GISceneProducerError::Closure;
        auto& table=next.table;
        table.nativePipeline=native.pipeline;table.writerVersion=native.version;
        table.producerPipeline=producerPipeline;table.epoch=epoch;
        table.records.resize(bindings.size()*2);table.geometry.resize(bindings.size());table.bound.resize(bindings.size());
        std::set<std::uint64_t> tokens;
        // One binding row is immutable for this owner-locked admission. A repeat
        // of its exact BLAS/revision/geometry needs neither hashing nor conversion.
        std::vector<Key> boundIdentity(bindings.size());
        // Reuse only within this owner-locked transaction. A full source row
        // and allocation/revision/geometry identity distinguish material variants.
        // No result survives a new submit cut, owner change or frozen snapshot.
        using MappingKey=std::pair<Key,std::array<unsigned char,NativeStride>>;
        struct Mapping {PTOwnedScene::Index::Resolved resolved;PTShaderTableAdapter::Bytes record;};
        struct MappingHash {
            std::size_t operator()(const MappingKey& key)const noexcept {
                std::uint64_t hash=std::get<1>(key.first)^(std::get<2>(key.first)*0x9e3779b97f4a7c15ull)^std::get<3>(key.first);
                for(unsigned offset=32;offset<88;offset+=8){std::uint64_t word=0;std::memcpy(&word,key.second.data()+offset,8);
                    hash^=word+0x9e3779b97f4a7c15ull+(hash<<6)+(hash>>2);}
                hash=(hash^(hash>>30))*0xbf58476d1ce4e5b9ull;
                hash=(hash^(hash>>27))*0x94d049bb133111ebull;
                return static_cast<std::size_t>(hash^(hash>>31));
            }
        };
        std::unordered_map<MappingKey,Mapping,MappingHash> mappings;
        mappings.reserve((std::min)(bindings.size(),capturedGeometry.size()*2));
        std::map<std::tuple<std::uint64_t,std::uint64_t,std::uint64_t>,const ASRecord*> selectedBottoms;
        for(std::size_t i=0;i<next.closure.bottoms.size();++i){const auto& ref=next.closure.bottoms[i];
            selectedBottoms.emplace(std::make_tuple(ref.address,ref.token,ref.revision),&next.closure.bottomRecords[i]);}
        for(std::uint32_t i=0;i<tlas.count;++i){
            InstanceRecord instance;
            if(!DecodeInstance(bytes+std::size_t(i)*64,instance))return GISceneProducerError::Closure;
            if(!(instance.mask&1)||!instance.blas)continue;
            GISceneGeometrySample sample;sample.instance=i;sample.blas=perInstance[i];
            bool finite=true;for(float value:instance.transform)finite=finite&&std::isfinite(value);
            if(!finite){sample.stage=GISceneGeometryStage::Transform;if(!diagnostic)return GISceneProducerError::Geometry;reject(sample,GISceneProducerError::Geometry);continue;}
            const auto& ref=perInstance[i];const auto key=std::make_tuple(ref.address,ref.token,ref.revision);
            auto cached=selectedBottoms.find(key);
            if(cached==selectedBottoms.end())return GISceneProducerError::Closure;
            const auto& bottom=*cached->second;
            for(std::uint32_t g=0;g<bottom.count;++g){
                const auto row=std::uint64_t(instance.hitGroup)+std::uint64_t(g)*2;
                if((row&1)||row>=table.records.size())return GISceneProducerError::Mapping;
                const Key identity{ref.address,ref.token,ref.revision,g};
                const auto binding=static_cast<std::size_t>(row/2);
                if(table.bound[binding]&&boundIdentity[binding]==identity){
                    if(diagnostic){++diagnostic->checked;++diagnostic->valid;++diagnostic->reused;}
                    continue;
                }
                sample.geometry=g;sample.row=row;sample.binding=bindings[static_cast<std::size_t>(row/2)];
                sample.witness={};sample.metadata={};sample.metadataError=sample.convertError=0;sample.resolved={};sample.exact={};sample.rebase={};sample.frozenIndices={};sample.frozenPositions={};sample.status=PTOwnedScene::Status::candidate;
                if(diagnostic)++diagnostic->checked;
                const auto found=witnesses.find(Key{ref.address,ref.token,ref.revision,g});
                if(found==witnesses.end()){sample.stage=GISceneGeometryStage::WitnessMissing;if(!diagnostic)return GISceneProducerError::Geometry;reject(sample,GISceneProducerError::Geometry);continue;}
                PTOwnedScene::Index::Resolved resolved;
                PTShaderTableAdapter::Bytes converted;
                MappingKey currentKey{{ref.address,ref.token,ref.revision,g},{}};
                std::memcpy(currentKey.second.data(),native.bytes.data()+row*NativeStride,NativeStride);
                const auto mapped=mappings.find(currentKey);
                const bool reusedMapping=mapped!=mappings.end();
                if(mapped!=mappings.end()){
                    resolved=mapped->second.resolved;converted=mapped->second.record;if(diagnostic)++diagnostic->reused;
                }else{
                    const auto& witness=found->second->triangle;const auto* triangle=bottom.geometry->At(g);
                    sample.witness=witness;sample.metadata=triangle?*triangle:TriangleMetadata{};sample.metadataError=triangle?unsigned(GeometryMetadata::Validate(*triangle)):UINT32_MAX;
                    if(found->second->frozen){sample.frozenIndices=found->second->frozen->indices;sample.frozenPositions=found->second->frozen->positions;}
                    if(!triangle||GeometryMetadata::Validate(*triangle)!=GeometryReject::None||triangle->transform||
                       witness.indices!=triangle->indices||witness.positions!=triangle->vertices||
                       witness.positionStride!=triangle->stride||witness.indexStride!=GeometryMetadata::IndexSize(*triangle)||
                       witness.indexCount!=triangle->indexCount||witness.vertexCount!=triangle->vertexCount)
                        {sample.stage=GISceneGeometryStage::Metadata;if(!diagnostic)return GISceneProducerError::Geometry;reject(sample,GISceneProducerError::Geometry);continue;}
                    const auto persistentKey=GIValidatedMappingCache::MakeKey(ref.address,ref.token,ref.revision,g,
                        witness,found->second->frozen.get(),native.bytes.data()+row*NativeStride);
                    bool acrossTransaction=false;
                    if(persistent&&persistent->Find(persistentKey,found->second->frozen.get(),owners,epoch,resolved,converted,acrossTransaction)){
                        if(diagnostic){if(acrossTransaction)++diagnostic->persistent;else ++diagnostic->reused;}
                    }else{
                        // Typed source bounds are permitted only when a frozen copy
                        // will be rebased and re-admitted for raw DWORD reads below.
                        const auto access=found->second->frozen?PTSceneAdmission::IndexAccess::NativeTyped:PTSceneAdmission::IndexAccess::RawDWORD;
                        // Successful admissions need the exact match, not a census
                        // of unrelated candidates. On failure rerun under the same
                        // owner lock to preserve the complete diagnostic evidence.
                        sample.status=owners.ResolveExact(bindings[binding],witness,epoch,resolved,nullptr,access);
                        if(sample.status!=PTOwnedScene::Status::candidate&&diagnostic)
                            sample.status=owners.ResolveExact(bindings[binding],witness,epoch,resolved,&sample.exact,access);
                        if(sample.status!=PTOwnedScene::Status::candidate){sample.stage=GISceneGeometryStage::Resolve;if(!diagnostic)return GISceneProducerError::Geometry;reject(sample,GISceneProducerError::Geometry);continue;}
                        sample.resolved=resolved.geometry;
                        sample.convertError=unsigned(Convert(native.bytes.data()+row*NativeStride,NativeStride,exports,&resolved.geometry,converted,access));
                        if(sample.convertError!=unsigned(Status::ready))
                            {sample.stage=GISceneGeometryStage::Convert;if(!diagnostic)return GISceneProducerError::Table;reject(sample,GISceneProducerError::Table);continue;}
                        const auto original=resolved;
                        if(found->second->frozen&&!RebaseConvertedGeometry(*found->second->frozen,resolved.geometry,converted,&sample.rebase))
                            {sample.stage=GISceneGeometryStage::Rebase;if(!diagnostic)return GISceneProducerError::Geometry;reject(sample,GISceneProducerError::Geometry);continue;}

                        if(persistent)persistent->Store(persistentKey,found->second->frozen.get(),original,resolved,converted);
                    }
                    // Bound temporary CPU memory; uncached rows still undergo
                    // the identical full validation and conversion.
                    if(mappings.size()<131072)mappings.emplace(std::move(currentKey),Mapping{resolved,converted});
                }
                if(table.bound[binding]&&table.records[row]!=converted){sample.stage=GISceneGeometryStage::Conflict;if(!diagnostic)return GISceneProducerError::Conflict;reject(sample,GISceneProducerError::Conflict);continue;}
                if(diagnostic)++diagnostic->valid;
                table.records[row]=converted;table.geometry[binding]=resolved.geometry;table.bound[binding]=1;boundIdentity[binding]=identity;
                if(!reusedMapping)for(auto token:resolved.tokens)tokens.insert(token);
            }
        }
        if(failure!=GISceneProducerError::None)return failure;
        if(tokens.empty())return GISceneProducerError::Empty;
        next.allocationTokens.assign(tokens.begin(),tokens.end());next.ownerRevision=owners.Revision();
        const auto rowsMS=elapsed();if(timings)timings->rowsMS=rowsMS;
        output=std::move(next);return GISceneProducerError::None;
    }catch(...){return GISceneProducerError::Allocation;}
}
}
