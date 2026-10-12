#pragma once
#include <d3d12.h>
#include <cstdint>
// CPU recording-time evidence only; not GPU completion or lifetime tracking.
namespace PTSceneObserver {
constexpr UINT MaximumGeometries=4096;
struct Event {
    std::uint64_t sequence=0,list=0,dest=0,source=0,index=0,position=0,transform=0,stride=0;
    UINT site=0,kind=0,buildFlags=0,ordinal=0,count=0,geometryType=0,geometryFlags=0;
    UINT indexFormat=0,vertexFormat=0,indexCount=0,vertexCount=0,layout=0;
    std::uint64_t attributes=0;UINT constants[10]{};
};
using Sink=void(*)(const Event&) noexcept;
inline bool ReadBuild(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* desc,D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& out) noexcept {
    __try {if(!desc)return false;out=*desc;return true;} __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
inline bool ReadGeometry(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& in,UINT i,D3D12_RAYTRACING_GEOMETRY_DESC& out) noexcept {
    __try {
        if(i>=in.NumDescs||in.NumDescs>MaximumGeometries)return false;
        if(in.DescsLayout==D3D12_ELEMENTS_LAYOUT_ARRAY){if(!in.pGeometryDescs)return false;out=in.pGeometryDescs[i];}
        else if(in.DescsLayout==D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS){if(!in.ppGeometryDescs||!in.ppGeometryDescs[i])return false;out=*in.ppGeometryDescs[i];}
        else return false;
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
inline void Observe(ID3D12GraphicsCommandList4* list,const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* desc,UINT site,std::uint64_t sequence,Sink sink) noexcept {
    Event e{};e.sequence=sequence;e.list=reinterpret_cast<std::uintptr_t>(list);e.site=site;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC d{};
    if(!ReadBuild(desc,d)){e.kind=4;sink(e);return;}
    e.dest=d.DestAccelerationStructureData;e.source=d.SourceAccelerationStructureData;
    e.buildFlags=static_cast<UINT>(d.Inputs.Flags);e.count=d.Inputs.NumDescs;e.layout=static_cast<UINT>(d.Inputs.DescsLayout);
    e.kind=d.Inputs.Type==D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL?1u:2u;
    if(e.kind==2){e.index=d.Inputs.InstanceDescs;sink(e);return;}
    sink(e);
    if(e.count>MaximumGeometries){e.kind=4;sink(e);return;}
    for(UINT i=0;i<e.count;++i){
        D3D12_RAYTRACING_GEOMETRY_DESC g{};Event row=e;row.ordinal=i;row.kind=3;
        if(!ReadGeometry(d.Inputs,i,g)){row.kind=4;sink(row);return;}
        row.geometryType=static_cast<UINT>(g.Type);row.geometryFlags=static_cast<UINT>(g.Flags);
        if(g.Type==D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES){
            const auto& t=g.Triangles;row.index=t.IndexBuffer;row.position=t.VertexBuffer.StartAddress;
            row.transform=t.Transform3x4;row.stride=t.VertexBuffer.StrideInBytes;
            row.indexFormat=static_cast<UINT>(t.IndexFormat);row.vertexFormat=static_cast<UINT>(t.VertexFormat);
            row.indexCount=t.IndexCount;row.vertexCount=t.VertexCount;
        }
        sink(row);
    }
}
using Build=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList4*,const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*,UINT,const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC*);
inline void Forward(ID3D12GraphicsCommandList4* list,const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* desc,UINT count,const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC* info) {
    reinterpret_cast<Build>((*reinterpret_cast<void***>(list))[72])(list,desc,count,info);
}
}
