#pragma once
#include "pt_scene_topology.h"
#include "pt_scene_observer.h"
#include <mutex>
namespace PTSceneTopology {
class Runtime {
    std::mutex mutex;Index index;bool active=true;
    struct Upload {U64 serial;std::vector<Instance> instances;};
    std::map<U64,Upload> uploads;std::vector<Instance> latest;
    U64 serial=0,last=0;
public:
    struct Stats {U64 uploads=0,readErrors=0,builds=0,compactions=0,missingCompactions=0,tlas=0,missingUpload=0,tables=0;Result relation{};};
private:
    Stats stats{};
    static bool Copy(void* dst,const void* src,std::size_t bytes)noexcept{
        __try{if(!src)return false;std::memcpy(dst,src,bytes);return true;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}
    }
public:
    void UploadInstances(U64 address,const void* data,U64 bytes){
        if(!address||!bytes||bytes%64||bytes>2ull*1024*1024)return;
        std::lock_guard<std::mutex> guard(mutex);if(!active)return;
        if(uploads.size()>=8&&!uploads.count(address))return;
        Upload u{};u.serial=++serial;u.instances.resize(static_cast<std::size_t>(bytes/64));
        if(!Copy(u.instances.data(),data,static_cast<std::size_t>(bytes))){++stats.readErrors;uploads.erase(address);return;}
        uploads[address]=std::move(u);++stats.uploads;
    }
    void Build(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* desc){
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC d{};if(!PTSceneObserver::ReadBuild(desc,d))return;
        std::lock_guard<std::mutex> guard(mutex);if(!active)return;
        if(d.Inputs.Type==D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL){
            ++stats.tlas;auto u=uploads.find(d.Inputs.InstanceDescs);
            if(d.Inputs.DescsLayout!=D3D12_ELEMENTS_LAYOUT_ARRAY||u==uploads.end()||u->second.instances.size()!=d.Inputs.NumDescs){++stats.missingUpload;latest.clear();last=0;return;}
            if(last!=u->second.serial){latest=u->second.instances;last=u->second.serial;}return;
        }
        if(d.Inputs.NumDescs>4096){index.Invalidate(d.DestAccelerationStructureData,1);return;}
        std::vector<Geometry> geometry;
        for(UINT n=0;n<d.Inputs.NumDescs;++n){D3D12_RAYTRACING_GEOMETRY_DESC g{};
            if(!PTSceneObserver::ReadGeometry(d.Inputs,n,g)||g.Type!=D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES||g.Triangles.Transform3x4||g.Triangles.VertexFormat!=DXGI_FORMAT_R32G32B32_FLOAT||
               (g.Triangles.IndexFormat!=DXGI_FORMAT_R16_UINT&&g.Triangles.IndexFormat!=DXGI_FORMAT_R32_UINT)){index.Invalidate(d.DestAccelerationStructureData,1);return;}
            const auto& t=g.Triangles;geometry.push_back({t.IndexBuffer,t.VertexBuffer.StartAddress,t.VertexBuffer.StrideInBytes,t.IndexFormat==DXGI_FORMAT_R16_UINT?2u:4u,t.IndexCount,t.VertexCount});
        }
        if(index.Build(d.DestAccelerationStructureData,std::move(geometry)))++stats.builds;
    }
    void Compact(U64 dst,U64 src){std::lock_guard<std::mutex> guard(mutex);if(!active)return;if(index.Compact(dst,src))++stats.compactions;else ++stats.missingCompactions;}
    void Invalidate(U64 address,U64 bytes){std::lock_guard<std::mutex> guard(mutex);if(!active)return;index.Invalidate(address,bytes);uploads.erase(address);latest.clear();last=0;}
    void Table(const std::vector<PTNativeBinding::Binding>& bindings,unsigned rays){
        std::lock_guard<std::mutex> guard(mutex);if(!active)return;++stats.tables;
        const auto r=index.Check(latest,bindings,rays);
        stats.relation.instances+=r.instances;stats.relation.matched+=r.matched;stats.relation.missingBlas+=r.missingBlas;stats.relation.outOfRange+=r.outOfRange;stats.relation.mismatch+=r.mismatch;stats.relation.conflict+=r.conflict;stats.relation.masked+=r.masked;
    }
    Stats Stop(){std::lock_guard<std::mutex> guard(mutex);active=false;index.Clear();uploads.clear();latest.clear();return stats;}
};
}
