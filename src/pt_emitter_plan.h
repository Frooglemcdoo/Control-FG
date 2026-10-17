#pragma once
#include "pt_scene_dispatch_plan.h"
#include <cmath>
namespace PTEmitters {
struct Geometry {
    std::uint64_t indices=0,attributes=0,positions=0;
    std::array<std::uint32_t,28> constants{};
};
// This plan borrows only addresses from an already retained immutable SBT.
// The caller must keep that Prepared proof and scene lease alive through GPU use.
inline bool Plan(const PTSceneDispatch::Scene& scene,const PTSceneDispatch::Plan& dispatch,
                 const PTShaderTableAdapter::Identifier& alpha,std::vector<Geometry>& out){
    out.clear();if(!scene.version||scene.version!=dispatch.selection.sceneVersion||dispatch.records.empty()||dispatch.records.size()%2)return false;
    std::vector<Geometry> pending;
    for(const auto& instance:scene.instances){
        if(((instance.idMask>>24)&1u)==0)continue;
        const auto* geometries=scene.topology.Geometries(instance.blas);if(!geometries)return false;
        const auto first=instance.contributionFlags&0xffffffu;if(first%2)return false;
        for(std::size_t index=0;index<geometries->size();++index){
            const auto row=std::size_t(first)+2*index;if(row>=dispatch.records.size())return false;
            const auto& source=(*geometries)[index];const auto& record=dispatch.records[row];
            if(!source.indexCount||source.indexCount%3)return false;
            Geometry g;std::memcpy(&g.indices,record.data()+32,8);std::memcpy(&g.attributes,record.data()+40,8);std::memcpy(&g.positions,record.data()+48,8);
            if(!g.indices||!g.attributes||!g.positions)return false;
            std::memcpy(g.constants.data(),record.data()+56,48);
            for(float v:instance.transform)if(!std::isfinite(v))return false;
            std::memcpy(g.constants.data()+12,instance.transform,48);
            g.constants[24]=source.indexCount/3;
            const bool forceOpaque=(instance.contributionFlags&0x04000000u)!=0;
            const bool forceAlpha=(instance.contributionFlags&0x08000000u)!=0;
            if(forceOpaque&&forceAlpha)return false;
            g.constants[25]=forceOpaque?0u:(forceAlpha||!std::memcmp(record.data(),alpha.data(),32)?1u:0u);
            pending.push_back(g);if(pending.size()>131072)return false;
        }
    }
    out.swap(pending);return true;
}
}
