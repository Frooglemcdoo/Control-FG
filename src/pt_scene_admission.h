#pragma once
#include <cstdint>
#include <limits>
// Pure validation for private SBT records. Caller retains resources until the
// submission fence completes and supplies tracked allocation/view metadata.
// This does not discover native buffers, infer matching indices, or hook Control.
namespace PTSceneAdmission {
struct Allocation {std::uint64_t address,bytes,epoch;};
struct View {Allocation allocation;std::uint64_t address;};
struct Geometry {
    View indices,positions,attributes;
    std::uint32_t indexStride,indexCount,vertexCount,positionStride,attributeStride;
    std::uint32_t normalOffset,uvOffset,material,materialCount;
    std::uint64_t epoch;
    bool float3Positions,sharedVertexIndexing,noGeometryTransform;
};
struct Record {
    std::uint64_t indices=0,attributes=0,positions=0;
    std::uint32_t constants[12]{};
};
enum class Error {none,format,epoch,allocation,alignment,range,material};
inline bool AddFits(std::uint64_t a,std::uint64_t b){return b<=std::numeric_limits<std::uint64_t>::max()-a;}
inline bool Contains(View v,std::uint64_t bytes){
    return v.allocation.address&&v.allocation.bytes&&AddFits(v.allocation.address,v.allocation.bytes)&&
        v.address>=v.allocation.address&&v.address-v.allocation.address<=v.allocation.bytes&&
        bytes<=v.allocation.bytes-(v.address-v.allocation.address);
}
inline Error Admit(const Geometry& g,Record& out){
    out={};
    if(!g.float3Positions||!g.sharedVertexIndexing||!g.noGeometryTransform||
       (g.indexStride!=2&&g.indexStride!=4)||!g.indexCount||g.indexCount%3||!g.vertexCount||
       g.positionStride<12||g.attributeStride<8)return Error::format;
    if(!g.epoch||g.indices.allocation.epoch!=g.epoch||g.positions.allocation.epoch!=g.epoch||g.attributes.allocation.epoch!=g.epoch)return Error::epoch;
    if(g.material>=g.materialCount||g.material>=244000u/35u)return Error::material;
    if((g.indices.allocation.address&3)||(g.indices.address%g.indexStride)||
       (g.positions.address&3)||(g.attributes.address&3)||(g.positionStride&3)||(g.attributeStride&3)||
       (g.normalOffset&3)||(g.uvOffset&3))return Error::alignment;
    if(g.normalOffset>g.attributeStride-8||g.uvOffset>g.attributeStride-4)return Error::range;
    const std::uint64_t indexBytes=std::uint64_t(g.indexCount)*g.indexStride;
    const std::uint64_t positionBytes=std::uint64_t(g.vertexCount-1)*g.positionStride+12;
    const std::uint64_t attributeBytes=std::uint64_t(g.vertexCount)*g.attributeStride;
    if(!Contains(g.indices,indexBytes)||!Contains(g.positions,positionBytes)||!Contains(g.attributes,attributeBytes))return Error::allocation;
    const auto indexOffset=g.indices.address-g.indices.allocation.address;
    const auto end=indexOffset+indexBytes;
    // Raw 16-bit fetches read aligned 32-bit words, including the last index.
    if(end>UINT32_MAX-3u||((end+3u)&~std::uint64_t(3))>g.indices.allocation.bytes||
       g.indices.allocation.bytes>UINT32_MAX||positionBytes>UINT32_MAX||attributeBytes>UINT32_MAX)return Error::range;
    out.indices=g.indices.allocation.address;out.attributes=g.attributes.address;out.positions=g.positions.address;
    const std::uint32_t c[12]={static_cast<std::uint32_t>(indexOffset),g.indexStride,g.attributeStride,g.material,
        0,g.normalOffset,g.uvOffset,0,static_cast<std::uint32_t>(g.indices.allocation.bytes),
        static_cast<std::uint32_t>(attributeBytes),g.positionStride,static_cast<std::uint32_t>(positionBytes)};
    for(unsigned i=0;i<12;++i)out.constants[i]=c[i];
    return Error::none;
}
}
