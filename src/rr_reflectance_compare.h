#pragma once
#include "rr_reflectance_projection.h"
namespace control_rr_reflectance {
struct CandidateSource {
    unsigned width,height;
    const unsigned char *g1,*g2,*albedo,*part1;
    std::size_t g1Pitch,g2Pitch,albedoPitch,part1Bytes;
    float sx,sy;
};
struct CandidateComparison {bool layout=false;std::uint64_t diffuseErrors=0,specularErrors=0;double maximumError=0;};
inline CandidateComparison CompareCandidate(const CandidateSource& v,const void* output,std::size_t outputBytes) noexcept {
    CandidateComparison result{};
    const auto pixels=std::uint64_t(v.width)*v.height;
    if(!pixels || pixels>8388608 || !v.g1 || !v.g2 || !v.albedo || !v.part1 || !output ||
       v.g1Pitch<std::size_t(v.width)*4 || v.g2Pitch<std::size_t(v.width)*4 || v.albedoPitch<std::size_t(v.width)*8 ||
       !v.part1Bytes || v.part1Bytes>524288 || v.part1Bytes%8 || outputBytes!=pixels*24 ||
       !std::isfinite(v.sx) || !std::isfinite(v.sy) || v.sx<1e-4 || v.sy<1e-4 || v.sx>1e4 || v.sy>1e4) return result;
    result.layout=true;const auto* bytes=static_cast<const unsigned char*>(output);
    for(unsigned y=0;y<v.height;++y) for(unsigned x=0;x<v.width;++x) {
        const std::size_t pixel=std::size_t(y)*v.width+x;
        const auto* a=v.albedo+std::size_t(y)*v.albedoPitch+std::size_t(x)*8;
        if(std::memcmp(a,bytes+pixel*8,6) || bytes[pixel*8+6]!=0 || bytes[pixel*8+7]!=0x3c) ++result.diffuseErrors;
        const auto* g1=v.g1+std::size_t(y)*v.g1Pitch+std::size_t(x)*4;
        const auto* g2=v.g2+std::size_t(y)*v.g2Pitch+std::size_t(x)*4;
        const unsigned id=(unsigned(g2[2])<<8)|g2[3];
        if(id>=v.part1Bytes/8) {++result.specularErrors;continue;}
        std::uint32_t packed=0;std::memcpy(&packed,v.part1+id*8,4);
        const auto f0=DecodeControlMaterialF0(packed,float(g2[1])/255.0f);
        const auto expected=SpecularReflectanceFromLinearRoughness(f0,1-float(g1[2])/255.0f,
            PixelViewCosine(g1,x,y,v.width,v.height,v.sx,v.sy));
        const float reference[]={expected.x,expected.y,expected.z};
        float actual[4];std::memcpy(actual,bytes+std::size_t(pixels)*8+pixel*16,16);
        if(actual[3]!=1) ++result.specularErrors;
        for(unsigned k=0;k<3;++k) {
            const double error=std::fabs(double(actual[k])-reference[k]);
            if(!std::isfinite(error) || error>0.0002+0.0002*std::fabs(reference[k])) ++result.specularErrors;
            if(std::isfinite(error) && error>result.maximumError) result.maximumError=error;
        }
    }
    return result;
}
}
