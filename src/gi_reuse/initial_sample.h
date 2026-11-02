#pragma once
#include "connection.h"
// Initial GI sample in solid-angle measure. No spatial/temporal reuse here.
// The adapter supplies secondary radiance BEFORE primary throughput, and the
// primary BRDF*cos separately. Native already-weighted RGB is not radiance.
#ifdef __cplusplus
namespace gi_reuse {
#define GI_SAMPLE_INLINE inline
#else
#define GI_SAMPLE_INLINE
#endif
struct GIInitialInput {
    GIPoint secondaryPosition; float sourcePdf;
    GIPoint secondaryRadiance; uint material;
    GIPoint primaryBrdfCos; uint rayIndex;
    GIPoint fallbackContribution; uint frameTag;
    uint verified; uint configuredCount; float depthScale; uint reserved;
};
struct GIInitialResult {
    GIPoint surfaceContribution; uint valid;
    GIPoint fallbackContribution; uint isSurface;
    float inverseSourcePdf; float targetDensity; uint frameTag; uint rayIndex;
};
GI_SAMPLE_INLINE bool GINonnegative(GIPoint p) {
    return GIFinitePoint(p)&&p.x>=0&&p.y>=0&&p.z>=0;
}
GI_SAMPLE_INLINE GIInitialResult GIMakeInitialSample(GIInitialInput v,uint frameTag) {
#ifdef __cplusplus
    GIInitialResult r{};
#else
    GIInitialResult r=(GIInitialResult)0;
#endif
    if(v.frameTag!=frameTag||v.configuredCount==0||v.configuredCount>64||
       v.rayIndex>=v.configuredCount||!GIFinite(v.depthScale)||v.depthScale<0||v.reserved!=0) return r;
    float scale=v.depthScale/float(v.configuredCount);
    if(v.material==65535) {
        // Misses are not surface reservoirs and remain divided by ALL rays.
        if(!GINonnegative(v.fallbackContribution)) return r;
        r.fallbackContribution.x=v.fallbackContribution.x*scale;
        r.fallbackContribution.y=v.fallbackContribution.y*scale;
        r.fallbackContribution.z=v.fallbackContribution.z*scale;
    } else {
        // Bit 0: proposal density established by producer, including support.
        // Bit 1: radiance is unweighted secondary shading, not primary RGB.
        // Bit 2: supplied primary BRDF*cos uses the SAME proposal convention.
        // These flags do NOT establish geometric normals or reuse visibility.
        if(v.material>=65534||(v.verified&7)!=7||!GIFinitePoint(v.secondaryPosition)||
           !GINonnegative(v.secondaryRadiance)||!GINonnegative(v.primaryBrdfCos)||
           !GIFinite(v.sourcePdf)||v.sourcePdf<=0) return r;
        float inverse=1/v.sourcePdf;
        float x=v.secondaryRadiance.x*v.primaryBrdfCos.x;
        float y=v.secondaryRadiance.y*v.primaryBrdfCos.y;
        float z=v.secondaryRadiance.z*v.primaryBrdfCos.z;
        r.targetDensity=x*.2126f+y*.7152f+z*.0722f;
        r.surfaceContribution.x=x*inverse*scale;
        r.surfaceContribution.y=y*inverse*scale;
        r.surfaceContribution.z=z*inverse*scale;
        r.inverseSourcePdf=inverse;r.isSurface=1;
        if(!GIFinite(inverse)||!GIFinite(r.targetDensity)) {
#ifdef __cplusplus
            return GIInitialResult{};
#else
            return (GIInitialResult)0;
#endif
        }
    }
    if(!GINonnegative(r.surfaceContribution)||!GINonnegative(r.fallbackContribution)) {
#ifdef __cplusplus
        return GIInitialResult{};
#else
        return (GIInitialResult)0;
#endif
    }
    r.valid=1;r.frameTag=frameTag;r.rayIndex=v.rayIndex;return r;
}
#ifdef __cplusplus
static_assert(sizeof(GIInitialInput)==80);
static_assert(sizeof(GIInitialResult)==48);
}
#endif
#undef GI_SAMPLE_INLINE
