#include "../../src/rr_reflectance_compare.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
using namespace control_rr_reflectance;
int main() {
    unsigned char g1[8]={128,128,90,0,9,240,180,37},g2[8]={0,80,0,0,0,200,0,0};
    unsigned char albedo[16]={1,0,2,0,3,0,0,0,4,0,5,0,6,0,0,0},table[8]{};
    unsigned char out[48]{};CandidateSource source{2,1,g1,g2,albedo,table,8,8,16,8,1.2f,2.1f};
    assert(CopyRGBA16FToDiffuseReflectance(albedo,16,out,16,2,1));
    for(unsigned x=0;x<2;++x) {
        auto f0=DecodeControlMaterialF0(0,float(g2[x*4+1])/255);
        auto s=SpecularReflectanceFromLinearRoughness(f0,1-float(g1[x*4+2])/255,PixelViewCosine(g1+x*4,x,0,2,1,source.sx,source.sy));
        float rgba[]={s.x,s.y,s.z,1};std::memcpy(out+16+x*16,rgba,16);
    }
    auto c=CompareCandidate(source,out,sizeof(out));assert(c.layout && !c.diffuseErrors && !c.specularErrors);
    for(unsigned i=0;i<16;++i) {
        out[i]^=1;c=CompareCandidate(source,out,sizeof(out));assert(c.diffuseErrors);out[i]^=1;
    }
    unsigned char saved[4];std::memcpy(saved,out+16,4);
    for(float invalid:{-1.0f,100.0f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}) {
        std::memcpy(out+16,&invalid,4);assert(CompareCandidate(source,out,sizeof(out)).specularErrors);
    }
    std::memcpy(out+16,saved,4);out[31]=0;assert(CompareCandidate(source,out,sizeof(out)).specularErrors);out[31]=0x3f;
    g2[3]=1;assert(CompareCandidate(source,out,sizeof(out)).specularErrors);g2[3]=0;
    assert(!CompareCandidate(source,out,47).layout);source.part1Bytes=7;assert(!CompareCandidate(source,out,48).layout);
    puts("PASS: actual runtime comparator detects diffuse bit corruption, specular error/nonfinite/alpha, material bounds and invalid lengths");
}
