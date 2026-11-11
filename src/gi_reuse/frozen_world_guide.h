#ifndef CONTROL_GI_FROZEN_WORLD_GUIDE_H
#define CONTROL_GI_FROZEN_WORLD_GUIDE_H
// Replacement-pipeline component; not connected to the game's dispatch hook yet.
// Immutable CDF built after training, before any guided ray is sampled.
#include "world_directional_guide.h"
#ifdef __cplusplus
namespace gi_reuse {
#define GI_FROZEN_INLINE inline
#else
#define GI_FROZEN_INLINE
#endif
struct GIFrozenWorldGuide { float cdf[65]; uint frame,epoch,samples,valid; };
struct GIFrozenRay { GIPoint direction; float weight; float pdf; uint frame,epoch,trace,valid; };
GI_FROZEN_INLINE GIFrozenWorldGuide GIFreezeWorldGuide(float energy[64],uint samples,uint frame,uint epoch) {
    GIFrozenWorldGuide g;
    g.frame=frame;g.epoch=epoch;g.samples=samples;g.valid=0;g.cdf[0]=0;
    float peak=0;bool good=samples>=256&&epoch!=0;
    for(uint index1=0;index1<64;++index1){good=good&&GIFinite(energy[index1])&&energy[index1]>=0;if(energy[index1]>peak)peak=energy[index1];}
    float sum=0;if(good&&peak>0)for(uint index2=0;index2<64;++index2)sum+=energy[index2]/peak;
    good=good&&GIFinite(sum)&&sum>0;
    for(uint index3=0;index3<64;++index3){
        float mass=good?0.01f/64+0.99f*(energy[index3]/peak)/sum:1.0f/64;
        g.cdf[index3+1]=g.cdf[index3]+mass;
    }
    // Evaluate the stored intervals, not the pre-rounding training masses.
    float total=g.cdf[64];for(uint index4=1;index4<64;++index4)g.cdf[index4]/=total;g.cdf[64]=1;
    g.valid=good?1u:0u;return g;
}
GI_FROZEN_INLINE bool GIFrozenValid(GIFrozenWorldGuide g,uint frame,uint epoch){
    if(g.valid!=1||g.samples<256||!epoch||g.frame!=frame||g.epoch!=epoch||g.cdf[0]!=0||g.cdf[64]!=1)return false;
    for(uint index5=0;index5<64;++index5)if(!GIFinite(g.cdf[index5+1])||g.cdf[index5+1]<=g.cdf[index5])return false;
    return true;
}
GI_FROZEN_INLINE uint GIFrozenCell(GIPoint d){
    float u=GIWorldAtan2(d.y,d.x)*0.1591549430918953f;if(u<0)u+=1;
    uint x=uint(u*8),y=uint((d.z+1)*4);if(x>7)x=7;if(y>7)y=7;return y*8+x;
}
GI_FROZEN_INLINE GIFrozenRay GISampleFrozenWorld(GIFrozenWorldGuide g,GIPoint n,
    uint frame,uint epoch,float fraction,float u,float v){
#ifdef __cplusplus
    GIFrozenRay r{};
#else
    GIFrozenRay r=(GIFrozenRay)0;
#endif
    if(!GIFinitePoint(n)||GIAbs(GIDot(n,n)-1)>0.0001f||!GIFinite(fraction)||fraction<0||fraction>.5f||
       !GIFinite(u)||!GIFinite(v)||u<0||u>=1||v<0||v>=1)return r;
    // Fallback is decided BEFORE sampling, never after seeing a hit or its light.
    if(g.valid!=1||g.samples<256||!epoch||g.frame!=frame||g.epoch!=epoch)fraction=0;
    r.frame=frame;r.epoch=epoch;
    uint selected=0;bool guided=fraction>0&&u<fraction;
    if(guided){
        float choose=u/fraction;
        uint lo=0,hi=64;
        for(uint step=0;step<6;++step){uint mid=(lo+hi)/2;if(choose<g.cdf[mid])hi=mid;else lo=mid;}
        selected=lo;float interval=g.cdf[lo+1]-g.cdf[lo];
        if(!GIFinite(interval)||interval<=0)return r;
        float local=(choose-g.cdf[lo])/interval;
        if(local<0)local=0;
        if(local>0.9999999403953552f)local=0.9999999403953552f;
        float angle=6.283185307179586f*(float(lo%8)+local)/8;
        float z=2*(float(lo/8)+v)/8-1,rad=GISqrt((1-z*z)>0?1-z*z:0);
        r.direction.x=rad*GIWorldCos(angle);r.direction.y=rad*GIWorldSin(angle);r.direction.z=z;
    }else{
        float angle=6.283185307179586f*((u-fraction)/(1-fraction)),rad=GISqrt(1-v),z=GISqrt(v);
        float x=rad*GIWorldCos(angle),y=rad*GIWorldSin(angle),sign=n.z>=0?1.0f:-1.0f;
        float a=-1/(sign+n.z),b=n.x*n.y*a;
        r.direction.x=x*(1+sign*n.x*n.x*a)+y*b+z*n.x;
        r.direction.y=x*sign*b+y*(sign+n.y*n.y*a)+z*n.y;
        r.direction.z=-x*sign*n.x-y*n.y+z*n.z;
        selected=GIFrozenCell(r.direction);
    }
    float cosine=GIDot(n,r.direction);if(cosine<0)cosine=0;
    float base=cosine*0.3183098861837907f;
    float guide=fraction>0?(g.cdf[selected+1]-g.cdf[selected])*5.092958178940651f:0;
    if(!GIFinite(guide)||guide<0)return r;
    r.pdf=(1-fraction)*base+fraction*guide;
    // The representable endpoint v=0 is a zero-measure grazing native sample.
    if(r.pdf<=0){if(fraction==0&&cosine==0){r.weight=1;r.trace=1;r.valid=1;}return r;}
    r.weight=base/r.pdf;r.trace=cosine>0?1u:0u;r.valid=1;r.frame=frame;r.epoch=epoch;return r;
}
// Publish this weight with the ray. Never infer it from quantized hit positions
// or a later guide. Apply it to ALL surface and miss radiance, not hit distance.
// Below-hemisphere samples publish zero and remain in the native denominator.
#ifdef __cplusplus
static_assert(sizeof(GIFrozenWorldGuide)==276);
static_assert(sizeof(GIFrozenRay)==36);
}
#endif
#undef GI_FROZEN_INLINE
#endif
