#pragma once
#include <d3d12.h>
#include "geometry_metadata.h"
namespace gi_reuse {
// Copy at command recording time, while the application's descriptor pointers
// are valid. This never maps buffers, changes the AS build, or retains pointers
// into native arrays. Caller still owns resource lifetime and submission order.
struct GeometryDiagnostic {
 GeometryReject reason=GeometryReject::None;std::uint32_t index=UINT32_MAX,type=UINT32_MAX;
 TriangleMetadata triangle{};
};
inline std::shared_ptr<const GeometryMetadata> CopyBLASGeometry(
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs,GeometryDiagnostic* diagnostic=nullptr) {
    GeometryDiagnostic local;auto& d=diagnostic?*diagnostic:local;d={};
    auto reject=[&](GeometryReject reason)->std::shared_ptr<const GeometryMetadata>{d.reason=reason;return {};};
    if(inputs.Type!=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL)return reject(GeometryReject::BuildType);
    if(!inputs.NumDescs||inputs.NumDescs>GeometryMetadata::MaxGeometries)return reject(GeometryReject::Count);
    const bool array=inputs.DescsLayout==D3D12_ELEMENTS_LAYOUT_ARRAY;
    if(!array&&inputs.DescsLayout!=D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS)return reject(GeometryReject::Layout);
    if(array?!inputs.pGeometryDescs:!inputs.ppGeometryDescs)return reject(GeometryReject::NullArray);
    std::vector<TriangleMetadata> metadata;metadata.reserve(inputs.NumDescs);
    for(UINT i=0;i<inputs.NumDescs;++i){
        d.index=i;const auto* g=array?inputs.pGeometryDescs+i:inputs.ppGeometryDescs[i];
        if(!g)return reject(GeometryReject::NullGeometry);
        d.type=static_cast<std::uint32_t>(g->Type);
        if(g->Type!=D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES)return reject(GeometryReject::GeometryType);
        const auto& t=g->Triangles;
        d.triangle={t.VertexBuffer.StartAddress,t.IndexBuffer,t.Transform3x4,
            t.VertexBuffer.StrideInBytes,t.VertexCount,t.IndexCount,
            static_cast<std::uint32_t>(t.VertexFormat),static_cast<std::uint32_t>(t.IndexFormat),static_cast<std::uint32_t>(g->Flags)};
        const auto failure=GeometryMetadata::Validate(d.triangle);
        if(failure!=GeometryReject::None)return reject(failure);
        metadata.push_back(d.triangle);
    }
    d={};return GeometryMetadata::Create(metadata);
}
} // namespace gi_reuse
