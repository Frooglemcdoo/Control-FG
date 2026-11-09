#pragma once
#include "rr_reflectance.h"
namespace control_rr_reflectance {
// Supported captured camera shape only. Pixel centers use the unjittered
// symmetric perspective matrix; this does not establish gameplay jitter or
// foreground coverage for RR consumption.
inline bool SupportedProjection(const double* m,float* sx,float* sy) noexcept {
    if(!m || !sx || !sy) return false;
    for(unsigned i=0;i<16;++i) if(!std::isfinite(m[i])) return false;
    const unsigned zeros[]={1,2,3,4,6,7,8,9,12,13,15};
    for(unsigned i:zeros) if(std::fabs(m[i])>1e-8) return false;
    if(m[0]<1e-4 || m[0]>1e4 || m[5]<1e-4 || m[5]>1e4 ||
       std::fabs(m[11]-1)>1e-8 || m[10]<=0 || m[14]>=0) return false;
    *sx=static_cast<float>(m[0]);*sy=static_cast<float>(m[5]);return true;
}
inline float PixelViewCosine(const std::uint8_t* g1,unsigned x,unsigned y,
    unsigned width,unsigned height,float sx,float sy) noexcept {
    const unsigned nx=(unsigned(g1[0])<<3)|((g1[3]>>1)&7);
    const unsigned ny=(unsigned(g1[1])<<4)|(g1[3]>>4);
    const float qx=1.3f*(2.0f*float(nx)/2047.0f-1.0f);
    const float qy=1.3f*(2.0f*float(ny)/4095.0f-1.0f);
    const float q2=qx*qx+qy*qy;
    const Float3 normal{2*qx/(q2+1),2*qy/(q2+1),(q2-1)/(q2+1)};
    const float nn=std::sqrt(normal.x*normal.x+normal.y*normal.y+normal.z*normal.z);
    const float vx=(2*(float(x)+0.5f)/float(width)-1)/sx;
    const float vy=(1-2*(float(y)+0.5f)/float(height))/sy;
    const float vv=std::sqrt(vx*vx+vy*vy+1);
    const float dot=-(normal.x*vx+normal.y*vy+normal.z)/(nn*vv);
    return std::fmax(-1.0f,std::fmin(1.0f,dot));
}
}
