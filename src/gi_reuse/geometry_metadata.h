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
enum class GeometryReject:std::uint32_t {None,Count,VertexFormat,IndexFormat,Stride,VertexCount,IndexCount,VertexAlignment,IndexAlignment,Flags,VertexRange,IndexRange,Transform,BuildType,Layout,NullArray,NullGeometry,GeometryType};
inline const char* GeometryRejectName(GeometryReject r) noexcept {
 static const char* names[]={"none","count","vertex_format","index_format","stride","vertex_count","index_count","vertex_alignment","index_alignment","flags","vertex_range","index_range","transform","build_type","layout","null_array","null_geometry","geometry_type"};
 const auto n=static_cast<unsigned>(r);return n<18?names[n]:"unknown";
}
class GeometryMetadata final {
    std::vector<TriangleMetadata> triangles_;
    explicit GeometryMetadata(const std::vector<TriangleMetadata>& triangles):triangles_(triangles){}
    static bool Range(std::uint64_t base,std::uint64_t bytes) noexcept {
        return base && bytes && bytes<=UINT64_MAX-base;
    }
public:
    static constexpr std::size_t MaxGeometries=4096;
    static std::uint32_t IndexSize(const TriangleMetadata& t) noexcept {return t.indexFormat==57?2u:t.indexFormat==42?4u:0u;}
    static std::uint64_t IndexBytes(const TriangleMetadata& t) noexcept {return std::uint64_t(t.indexCount)*IndexSize(t);}
    // Indexed float3/uint16 or uint32 metadata. Stride is preserved, never assumed packed.
    static std::uint64_t VertexBytes(const TriangleMetadata& t) noexcept {
        if(!t.vertexCount||t.stride<12||std::uint64_t(t.vertexCount-1)>(UINT64_MAX-12)/t.stride)return 0;
        return std::uint64_t(t.vertexCount-1)*t.stride+12;
    }
    static GeometryReject Validate(const TriangleMetadata& t) noexcept {
        if(t.vertexFormat!=6)return GeometryReject::VertexFormat;
        if(!IndexSize(t))return GeometryReject::IndexFormat;
        if(t.stride<12||t.stride%4)return GeometryReject::Stride;
        if(!t.vertexCount)return GeometryReject::VertexCount;
        if(!t.indexCount||t.indexCount%3)return GeometryReject::IndexCount;
        if(t.vertices%4)return GeometryReject::VertexAlignment;
        if(t.indices%IndexSize(t))return GeometryReject::IndexAlignment;
        if(t.flags&~3u)return GeometryReject::Flags;
        if(!Range(t.vertices,VertexBytes(t)))return GeometryReject::VertexRange;
        if(!Range(t.indices,IndexBytes(t)))return GeometryReject::IndexRange;
        if(t.transform&&(t.transform%16||!Range(t.transform,48)))return GeometryReject::Transform;
        return GeometryReject::None;
    }
    static std::shared_ptr<const GeometryMetadata> Create(const std::vector<TriangleMetadata>& triangles,
        GeometryReject* reason=nullptr,std::uint32_t* badIndex=nullptr) {
        if(reason)*reason=GeometryReject::None;
        if(badIndex)*badIndex=UINT32_MAX;
        if(triangles.empty()||triangles.size()>MaxGeometries){if(reason)*reason=GeometryReject::Count;return {};}
        for(std::size_t i=0;i<triangles.size();++i){const auto failure=Validate(triangles[i]);
            if(failure!=GeometryReject::None){if(reason)*reason=failure;if(badIndex)*badIndex=static_cast<std::uint32_t>(i);return {};}
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
