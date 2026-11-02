#pragma once
#include <d3d12.h>
#include "geometry_metadata.h"
namespace gi_reuse {
// Copy at command recording time, while the application's descriptor pointers
// are valid. This never maps buffers, changes the AS build, or retains pointers
// into native arrays. Caller still owns resource lifetime and submission order.
inline std::shared_ptr<const GeometryMetadata> CopyBLASGeometry(
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs) {
    if(inputs.Type!=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL||
       !inputs.NumDescs||inputs.NumDescs>GeometryMetadata::MaxGeometries)return {};
    const bool array=inputs.DescsLayout==D3D12_ELEMENTS_LAYOUT_ARRAY;
    if(!array&&inputs.DescsLayout!=D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS)return {};
    if(array?!inputs.pGeometryDescs:!inputs.ppGeometryDescs)return {};
    std::vector<TriangleMetadata> metadata;metadata.reserve(inputs.NumDescs);
    for(UINT i=0;i<inputs.NumDescs;++i){
        const auto* g=array?inputs.pGeometryDescs+i:inputs.ppGeometryDescs[i];
        if(!g||g->Type!=D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES)return {};
        const auto& t=g->Triangles;
        metadata.push_back({t.VertexBuffer.StartAddress,t.IndexBuffer,t.Transform3x4,
            t.VertexBuffer.StrideInBytes,t.VertexCount,t.IndexCount,
            static_cast<std::uint32_t>(t.VertexFormat),static_cast<std::uint32_t>(t.IndexFormat),
            static_cast<std::uint32_t>(g->Flags)});
    }
    return GeometryMetadata::Create(metadata);
}
} // namespace gi_reuse
