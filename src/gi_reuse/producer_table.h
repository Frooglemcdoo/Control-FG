#pragma once
#include "native_gi_table.h"
namespace gi_reuse {
// Private-table preparation from the matched native GI writer. Geometry must
// already be resolved against the current scene cut, not by index address alone.
struct GIProducerTable {
    std::uint64_t nativePipeline=0,producerPipeline=0,writerVersion=0,epoch=0;
    std::vector<PTShaderTableAdapter::Bytes> records;
    std::vector<PTSceneAdmission::Geometry> geometry;
    std::vector<unsigned char> bound; // Unreferenced rows remain zero; never compact the native layout.
};
enum class GIProducerTableError {None,Identity,Shape,ActiveRows,Epoch,Exports,Geometry,Allocation};
inline GIProducerTableError PrepareGIProducerTable(const NativeGITable& native,
    std::uint64_t expectedPipeline,std::uint64_t expectedVersion,
    std::uint64_t producerPipeline,std::uint64_t epoch,
    const PTShaderTableAdapter::Exports& exports,
    const std::vector<PTSceneAdmission::Geometry>& geometry,GIProducerTable& output,
    std::size_t* failedBinding=nullptr) noexcept {
    if(failedBinding)*failedBinding=0;
    using namespace PTShaderTableAdapter;
    if(!expectedPipeline||!expectedVersion||!producerPipeline||!epoch||
       native.pipeline!=expectedPipeline||native.version!=expectedVersion)
        return GIProducerTableError::Identity;
    if(geometry.empty()||geometry.size()>65536||
       native.active.size()!=geometry.size()*2||native.bytes.size()!=geometry.size()*2*NativeStride)
        return GIProducerTableError::Shape;
    if(exports.nativeOpaque==Identifier{}||exports.nativeAlpha==Identifier{}||
       exports.privateOpaque==Identifier{}||exports.privateAlpha==Identifier{}||
       exports.nativeOpaque==exports.nativeAlpha||exports.privateOpaque==exports.privateAlpha)
        return GIProducerTableError::Exports;
    try {
        GIProducerTable next;
        next.nativePipeline=expectedPipeline;next.producerPipeline=producerPipeline;
        next.writerVersion=expectedVersion;next.epoch=epoch;
        next.records.resize(geometry.size()*2);next.geometry=geometry;
        next.bound.assign(geometry.size(),1);
        for(std::size_t i=0;i<geometry.size();++i){
            if(failedBinding)*failedBinding=i;
            if(native.active[i*2]!=1||native.active[i*2+1]!=0)return GIProducerTableError::ActiveRows;
            if(geometry[i].epoch!=epoch)return GIProducerTableError::Epoch;
            // Do not run the generic all-active converter over reserved rows.
            // TLAS instance contributions and geometry multiplier two stay intact.
            if(Convert(native.bytes.data()+i*2*NativeStride,NativeStride,exports,&geometry[i],
                       next.records[i*2])!=Status::ready)return GIProducerTableError::Geometry;
        }
        output=std::move(next);if(failedBinding)*failedBinding=geometry.size();
        return GIProducerTableError::None;
    }catch(...){return GIProducerTableError::Allocation;}
}
}
