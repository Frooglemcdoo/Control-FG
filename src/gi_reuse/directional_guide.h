#ifndef CONTROL_GI_DIRECTIONAL_GUIDE_H
#define CONTROL_GI_DIRECTIONAL_GUIDE_H
// Research-only proposal in native cosine-hemisphere (u,v) coordinates.
// NOT wired into live rendering. No cached radiance is returned as lighting.
#include "connection.h"
#ifdef __cplusplus
namespace gi_reuse {
#define GI_GUIDE_INLINE inline
#else
#define GI_GUIDE_INLINE
#endif
struct GIDirectionalGuide { float mass[64]; };
struct GIGuidedDirection { float u,v,relativePdf,inverseRelativePdf; uint cell,valid; };
// Keep at least half the proposal uniform. A stale guide cannot remove support.
// Normalize after scaling to avoid overflowing HDR training sums.
GI_GUIDE_INLINE GIDirectionalGuide GIBuildDirectionalGuide(float energy[64],float learnedFraction) {
    GIDirectionalGuide g;
    float peak=0;
    bool good=GIFinite(learnedFraction)&&learnedFraction>=0&&learnedFraction<=0.5f;
    for(uint index1=0;index1<64;++index1){good=good&&GIFinite(energy[index1])&&energy[index1]>=0;if(energy[index1]>peak)peak=energy[index1];}
    float total=0;
    if(good&&peak>0)for(uint index2=0;index2<64;++index2)total+=energy[index2]/peak;
    if(!good||!(total>0))learnedFraction=0;
    for(uint index3=0;index3<64;++index3)g.mass[index3]=learnedFraction>0?
        (1-learnedFraction)/64+learnedFraction*((energy[index3]/peak)/total):1.0f/64;
    return g;
}
GI_GUIDE_INLINE GIGuidedDirection GISampleDirectionalGuide(GIDirectionalGuide g,float choose,float vertical) {
#ifdef __cplusplus
    GIGuidedDirection r{};
#else
    GIGuidedDirection r=(GIGuidedDirection)0;
#endif
    if(!GIFinite(choose)||!GIFinite(vertical)||choose<0||choose>=1||vertical<0||vertical>=1)return r;
    float total=0;
    for(uint index4=0;index4<64;++index4){if(!GIFinite(g.mass[index4])||g.mass[index4]<=0)return r;total+=g.mass[index4];}
    if(!GIFinite(total)||total<=0)return r;
    // Normalize the actual stored masses; evaluation must use this SAME table.
    float target=choose*total,lo=0;uint selected=63;
    for(uint index5=0;index5<63;++index5){float hi=lo+g.mass[index5];if(target<hi){selected=index5;break;}lo=hi;}
    float fraction=(target-lo)/g.mass[selected];
    if(fraction<0)fraction=0;
    if(fraction>0.9999999403953552f)fraction=0.9999999403953552f;
    r.u=(float(selected%8)+fraction)/8;r.v=(float(selected/8)+vertical)/8;
    if(r.u>0.9999999403953552f)r.u=0.9999999403953552f;
    if(r.v>0.9999999403953552f)r.v=0.9999999403953552f;
    r.relativePdf=64*(g.mass[selected]/total);r.inverseRelativePdf=1/r.relativePdf;
    r.cell=selected;r.valid=1;return r;
}
// Native cosine PDF cancels throughput only for the original uniform UV law.
// Multiply EVERY fresh contribution, including miss/volume fallback, by this
// inverse relative density. Preserve native ray-count/depth normalization.
// Keep native hit-distance ABI separate; do not silently weight it as radiance.
#ifdef __cplusplus
}
#endif
#undef GI_GUIDE_INLINE
#endif
