#ifndef CONTROL_GI_INITIAL_RESERVOIR_H
#define CONTROL_GI_INITIAL_RESERVOIR_H
#include "initial_sample.h"
#ifdef __cplusplus
namespace gi_reuse {
#define GI_RES_INLINE inline
#else
#define GI_RES_INLINE
#endif
// Initial RIS over a COMPLETE ray batch. Not temporal/spatial reservoir merging.
// Miss fallback stays separate; the denominator includes black hits and misses.
struct GIInitialReservoir {
    GIInitialInput selected;
    GIPoint geometricNormal; uint hasSelection;
    float weightSum; float selectedTarget; uint processed; uint valid;
    uint seenLow; uint seenHigh; uint frameTag; uint configuredCount;
    GIPoint fallback; float depthScale;
};
GI_RES_INLINE GIInitialReservoir GIBeginInitialReservoir(uint frameTag,uint count,float depthScale) {
#ifdef __cplusplus
    GIInitialReservoir r{};
#else
    GIInitialReservoir r=(GIInitialReservoir)0;
#endif
    r.frameTag=frameTag;r.configuredCount=count;r.depthScale=depthScale;
    r.valid=count>0&&count<=64&&GIFinite(depthScale)&&depthScale>=0?1:0;
    return r;
}
GI_RES_INLINE bool GIStreamInitialReservoir(
#ifdef __cplusplus
    GIInitialReservoir& r,
#else
    inout GIInitialReservoir r,
#endif
    GIInitialInput sample,GIPoint geometricNormal,uint geometryVerified,float uniformRandom) {
    if(!r.valid) return false;
    if(sample.frameTag!=r.frameTag||sample.configuredCount!=r.configuredCount||
       sample.depthScale!=r.depthScale||sample.rayIndex>=r.configuredCount||
       !GIFinite(uniformRandom)||uniformRandom<0||uniformRandom>=1) {r.valid=0;return false;}
    uint bit=1u<<(sample.rayIndex&31u);
    if(((sample.rayIndex<32?r.seenLow:r.seenHigh)&bit)!=0) {r.valid=0;return false;}
    GIInitialResult estimate=GIMakeInitialSample(sample,r.frameTag);
    if(!estimate.valid) {r.valid=0;return false;}
    if(estimate.isSurface) {
        if(geometryVerified!=1||!GIFinitePoint(geometricNormal)||
           GIAbs(GIDot(geometricNormal,geometricNormal)-1)>0.001f) {r.valid=0;return false;}
        float weight=estimate.targetDensity*estimate.inverseSourcePdf;
        float total=r.weightSum+weight;
        if(!GIFinite(weight)||weight<0||!GIFinite(total)) {r.valid=0;return false;}
        if(weight>0&&(!r.hasSelection||uniformRandom*total<weight)) {
            r.selected=sample;r.geometricNormal=geometricNormal;
            r.selectedTarget=estimate.targetDensity;r.hasSelection=1;
        }
        r.weightSum=total;
    } else {
        r.fallback.x+=estimate.fallbackContribution.x;
        r.fallback.y+=estimate.fallbackContribution.y;
        r.fallback.z+=estimate.fallbackContribution.z;
        if(!GINonnegative(r.fallback)) {r.valid=0;return false;}
    }
    if(sample.rayIndex<32)r.seenLow|=bit;else r.seenHigh|=bit;
    ++r.processed;return true;
}
GI_RES_INLINE GIInitialResult GIResolveInitialReservoir(GIInitialReservoir r) {
#ifdef __cplusplus
    GIInitialResult result{};
#else
    GIInitialResult result=(GIInitialResult)0;
#endif
    if(!r.valid||r.processed!=r.configuredCount||r.configuredCount==0||r.configuredCount>64) return result;
    // Stream admission enforces unique in-range indices; count equality proves
    // complete coverage without shifting a 32-bit value by 32 or 64.
    if(r.hasSelection) {
        if(r.selectedTarget<=0||!GIFinite(r.selectedTarget)) return result;
        float weight=r.weightSum/r.selectedTarget;
        float scale=r.depthScale/float(r.configuredCount);
        result.surfaceContribution.x=r.selected.secondaryRadiance.x*r.selected.primaryBrdfCos.x*weight*scale;
        result.surfaceContribution.y=r.selected.secondaryRadiance.y*r.selected.primaryBrdfCos.y*weight*scale;
        result.surfaceContribution.z=r.selected.secondaryRadiance.z*r.selected.primaryBrdfCos.z*weight*scale;
        if(!GIFinite(weight)||!GINonnegative(result.surfaceContribution)) {
#ifdef __cplusplus
            return GIInitialResult{};
#else
            return (GIInitialResult)0;
#endif
        }
        // This is a RIS reconstruction weight, NOT the selected source 1/pdf.
        // Keep inverseSourcePdf zero; do not mislabel this as an initial sample.
        result.isSurface=1;result.targetDensity=r.selectedTarget;result.rayIndex=r.selected.rayIndex;
    }
    result.fallbackContribution=r.fallback;result.frameTag=r.frameTag;result.valid=1;return result;
}
#ifdef __cplusplus
static_assert(sizeof(GIInitialReservoir)==144);
}
#endif
#undef GI_RES_INLINE
#endif
