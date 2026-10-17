#pragma once
#include "pt_scene_dispatch_plan.h"
#include <cmath>
namespace PTEmitters {
struct Geometry {
    std::uint64_t indices=0,attributes=0,positions=0;
    std::array<std::uint32_t,40> constants{};
};
// This plan borrows only addresses from an already retained immutable SBT.
// The caller must keep that Prepared proof and scene lease alive through GPU use.
inline bool Plan(const PTSceneDispatch::Scene& scene,const PTSceneDispatch::Plan& dispatch,
                 const PTShaderTableAdapter::Identifier& alpha,std::vector<Geometry>& out){
    out.clear();if(!scene.version||scene.version!=dispatch.selection.sceneVersion||dispatch.records.empty()||dispatch.records.size()%2)return false;
    std::vector<Geometry> pending;std::uint64_t totalTriangles=0;
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
            // Inverse instance transform supplies the same conservative target
            // offset bound used by closest-hit ray spawning (nonuniform scale).
            const auto* t=instance.transform;
            const double det=double(t[0])*(double(t[5])*t[10]-double(t[6])*t[9])-
                double(t[1])*(double(t[4])*t[10]-double(t[6])*t[8])+double(t[2])*(double(t[4])*t[9]-double(t[5])*t[8]);
            if(!std::isfinite(det)||det==0)return false;
            double inv[12]={
                (double(t[5])*t[10]-double(t[6])*t[9])/det,(double(t[2])*t[9]-double(t[1])*t[10])/det,(double(t[1])*t[6]-double(t[2])*t[5])/det,0,
                (double(t[6])*t[8]-double(t[4])*t[10])/det,(double(t[0])*t[10]-double(t[2])*t[8])/det,(double(t[2])*t[4]-double(t[0])*t[6])/det,0,
                (double(t[4])*t[9]-double(t[5])*t[8])/det,(double(t[1])*t[8]-double(t[0])*t[9])/det,(double(t[0])*t[5]-double(t[1])*t[4])/det,0};
            for(unsigned rowIndex=0;rowIndex<3;++rowIndex){const unsigned at=rowIndex*4;
                inv[at+3]=-(inv[at]*t[3]+inv[at+1]*t[7]+inv[at+2]*t[11]);}
            for(unsigned i=0;i<12;++i){const float v=static_cast<float>(inv[i]);if(!std::isfinite(v))return false;
                std::memcpy(&g.constants[28+i],&v,4);}

            g.constants[24]=source.indexCount/3;
            totalTriangles+=g.constants[24];
            if(g.constants[24]>65535u*64u||totalTriangles>UINT32_MAX)return false;
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
