#include "../../src/rr_reflectance_compare.h"
using namespace control_rr_reflectance;
extern "C" int reference(const unsigned char* g1,const unsigned char* g2,const unsigned char* albedo,
    const unsigned char* table,std::size_t tableBytes,unsigned width,unsigned height,float sx,float sy,unsigned char* out,float* cosines) {
    if(!CopyRGBA16FToDiffuseReflectance(albedo,std::size_t(width)*8,out,std::size_t(width)*8,width,height)) return 1;
    const std::size_t n=std::size_t(width)*height;
    for(unsigned y=0;y<height;++y) for(unsigned x=0;x<width;++x) {
        const auto i=std::size_t(y)*width+x;const auto id=(unsigned(g2[i*4+2])<<8)|g2[i*4+3];
        if(id>=tableBytes/8) return 2;
        std::uint32_t packed;std::memcpy(&packed,table+id*8,4);
        const auto f0=DecodeControlMaterialF0(packed,float(g2[i*4+1])/255);
        cosines[i]=PixelViewCosine(g1+i*4,x,y,width,height,sx,sy);
        const auto s=SpecularReflectanceFromLinearRoughness(f0,1-float(g1[i*4+2])/255,cosines[i]);
        float rgba[]={s.x,s.y,s.z,1};std::memcpy(out+n*8+i*16,rgba,16);
    }
    const CandidateSource source{width,height,g1,g2,albedo,table,std::size_t(width)*4,std::size_t(width)*4,std::size_t(width)*8,tableBytes,sx,sy};
    const auto c=CompareCandidate(source,out,n*24);return c.layout && !c.diffuseErrors && !c.specularErrors?0:3;
}
