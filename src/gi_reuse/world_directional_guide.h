#ifndef CONTROL_GI_WORLD_DIRECTIONAL_GUIDE_H
#define CONTROL_GI_WORLD_DIRECTIONAL_GUIDE_H
// Research-only: world-fixed proposal, fresh receiver cosine, no endpoint reuse.
#include "directional_guide.h"
#ifdef __cplusplus
namespace gi_reuse {
#define GI_WORLD_INLINE inline
inline float GIWorldCos(float x){return std::cos(x);}
inline float GIWorldSin(float x){return std::sin(x);}
inline float GIWorldAtan2(float y,float x){return std::atan2(y,x);}
#else
#define GI_WORLD_INLINE
float GIWorldCos(float x){return cos(x);}
float GIWorldSin(float x){return sin(x);}
float GIWorldAtan2(float y,float x){return atan2(y,x);}
#endif
struct GIWorldProposal { GIPoint direction;float pdf;float throughputWeight;uint trace,valid; };
GI_WORLD_INLINE float GIWorldDensity(GIDirectionalGuide g,GIPoint d){
    float total=0;for(uint i=0;i<64;++i){if(!GIFinite(g.mass[i])||g.mass[i]<=0)return 0;total+=g.mass[i];}
    if(!GIFinite(total)||total<=0)return 0;
    float u=GIWorldAtan2(d.y,d.x)*0.1591549430918953f;if(u<0)u+=1;
    float v=(d.z+1)*0.5f;uint x=uint(u*8),y=uint(v*8);if(x>7)x=7;if(y>7)y=7;
    return (g.mass[y*8+x]/total)*5.092958178940651f; // 64 / (4*pi)
}
GI_WORLD_INLINE GIWorldProposal GISampleWorldGuide(GIDirectionalGuide g,GIPoint n,float fraction,float u,float v){
#ifdef __cplusplus
    GIWorldProposal r{};
#else
    GIWorldProposal r=(GIWorldProposal)0;
#endif
    if(!GIFinitePoint(n)||GIAbs(GIDot(n,n)-1)>0.0001f||!GIFinite(fraction)||fraction<0||fraction>.5f||
       !GIFinite(u)||!GIFinite(v)||u<0||u>=1||v<0||v>=1)return r;
    if(fraction>0&&u<fraction){
        GIGuidedDirection p=GISampleDirectionalGuide(g,u/fraction,v);if(!p.valid)return r;
        float z=2*p.v-1,rad=GISqrt((1-z*z)>0?1-z*z:0),angle=6.283185307179586f*p.u;
        r.direction.x=rad*GIWorldCos(angle);r.direction.y=rad*GIWorldSin(angle);r.direction.z=z;
    }else{
        float angle=6.283185307179586f*((u-fraction)/(1-fraction)),rad=GISqrt(1-v),z=GISqrt(v);
        float x=rad*GIWorldCos(angle),y=rad*GIWorldSin(angle);
        float sign=n.z>=0?1.0f:-1.0f,a=-1/(sign+n.z),b=n.x*n.y*a;
        GIPoint t,bt;t.x=1+sign*n.x*n.x*a;t.y=sign*b;t.z=-sign*n.x;
        bt.x=b;bt.y=sign+n.y*n.y*a;bt.z=-n.y;
        r.direction.x=x*t.x+y*bt.x+z*n.x;r.direction.y=x*t.y+y*bt.y+z*n.y;r.direction.z=x*t.z+y*bt.z+z*n.z;
    }
    float cosine=GIDot(n,r.direction);if(cosine<0)cosine=0;
    float base=cosine*0.3183098861837907f,guide=fraction>0?GIWorldDensity(g,r.direction):0;
    if(fraction>0&&guide<=0)return r;
    r.pdf=(1-fraction)*base+fraction*guide;
    if(!GIFinite(r.pdf)||r.pdf<=0)return r;
    r.throughputWeight=base/r.pdf;r.trace=cosine>0?1u:0u;r.valid=1;return r;
}
// Multiply fresh native surface AND miss contributions by throughputWeight.
// A below-hemisphere guide sample contributes zero and stays in the denominator;
// resampling it, renormalizing by survivors, or restoring a native ray is biased.
// The unchanged finite-range random draw remains independent of this warp.
#ifdef __cplusplus
}
#endif
#undef GI_WORLD_INLINE
#endif
