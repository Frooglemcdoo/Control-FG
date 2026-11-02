#pragma once
#include <cstdint>
#include <memory>
#include <vector>

namespace gi_reuse {
// Immutable CPU descriptor snapshot. GPU resource ownership and completed bytes
// are separate requirements: an address is never a lifetime or completion token.
struct TriangleMetadata {
    std::uint64_t vertices=0, indices=0, transform=0, stride=0;
    std::uint32_t vertexCount=0, indexCount=0, vertexFormat=0, indexFormat=0, flags=0;
};
class GeometryMetadata final {
    std::vector<TriangleMetadata> triangles_;
    explicit GeometryMetadata(const std::vector<TriangleMetadata>& triangles):triangles_(triangles){}
    static bool Range(std::uint64_t base,std::uint64_t bytes) noexcept {
        return base && bytes && bytes<=UINT64_MAX-base;
    }
public:
    static constexpr std::size_t MaxGeometries=4096;
    // Current decoder accepts only the observed float3 / uint16 layout. Reject
    // the whole build on an unsupported geometry; never shift GeometryIndex.
    static std::shared_ptr<const GeometryMetadata> Create(const std::vector<TriangleMetadata>& triangles) {
        if(triangles.empty()||triangles.size()>MaxGeometries)return {};
        for(const auto& t:triangles){
            if(t.vertexFormat!=6||t.indexFormat!=57||t.stride!=12||
               !t.vertexCount||!t.indexCount||t.indexCount%3||
               t.vertices%4||t.indices%2||(t.flags&~3u)||
               !Range(t.vertices,std::uint64_t(t.vertexCount)*12)||
               !Range(t.indices,std::uint64_t(t.indexCount)*2)||
               (t.transform&&(t.transform%16||!Range(t.transform,48))))return {};
        }
        // Copy prevents caller mutation of already-recorded build metadata.
        return std::shared_ptr<const GeometryMetadata>(new GeometryMetadata(triangles));
    }
    bool CanUpdateFrom(const GeometryMetadata& old) const noexcept {
        if(Size()!=old.Size())return false;
        for(std::size_t i=0;i<Size();++i){const auto& a=triangles_[i];const auto& b=old.triangles_[i];
            if(a.vertexCount!=b.vertexCount||a.indexCount!=b.indexCount||a.vertexFormat!=b.vertexFormat||
               a.indexFormat!=b.indexFormat||a.indices!=b.indices||a.flags!=b.flags||bool(a.transform)!=bool(b.transform))return false;
        }
        return true; // Descriptor compatibility only, not unchanged index bytes.
    }
    std::size_t Size() const noexcept {return triangles_.size();}
    const TriangleMetadata* At(std::uint32_t geometry) const noexcept {
        return geometry<triangles_.size()?&triangles_[geometry]:nullptr;
    }
};
struct TriangleReadPlan {
    TriangleMetadata geometry{};
    std::uint64_t asRevision=0, indexAddress=0;
    std::uint32_t primitive=0;
};
} // namespace gi_reuse
