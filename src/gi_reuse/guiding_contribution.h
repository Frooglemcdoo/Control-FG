#ifndef CONTROL_GI_GUIDING_CONTRIBUTION_H
#define CONTROL_GI_GUIDING_CONTRIBUTION_H
// Research bridge at the owned evaluator's throughput * secondary boundary.
// Native throughput already contains the native cosine sampling cancellation.
// Apply p_native / p_guided ONCE, not 1 / p_guided. No live bindings here.
#include "connection.h"
#ifdef __cplusplus
namespace gi_reuse {
#define GI_CONTRIBUTION_INLINE inline
#else
#define GI_CONTRIBUTION_INLINE
#endif
struct GIGuidedContribution { GIPoint rgb; float training; uint valid; };
GI_CONTRIBUTION_INLINE GIGuidedContribution GIApplyGuideCorrection(
    GIPoint throughput, GIPoint secondary, float nativePdf, float guidedPdf) {
#ifdef __cplusplus
    GIGuidedContribution r{};
#else
    GIGuidedContribution r=(GIGuidedContribution)0;
#endif
    if(!GIFinitePoint(throughput)||!GIFinitePoint(secondary)||
       throughput.x<0||throughput.y<0||throughput.z<0||
       secondary.x<0||secondary.y<0||secondary.z<0||
       !GIFinite(nativePdf)||!GIFinite(guidedPdf)||nativePdf<0||guidedPdf<=0)return r;
    float correction=nativePdf/guidedPdf;
    if(!GIFinite(correction))return r;
    r.rgb.x=(throughput.x*correction)*secondary.x;
    r.rgb.y=(throughput.y*correction)*secondary.y;
    r.rgb.z=(throughput.z*correction)*secondary.z;
    r.training=r.rgb.x*.2126f+r.rgb.y*.7152f+r.rgb.z*.0722f;
    if(!GIFinitePoint(r.rgb)||!GIFinite(r.training)){
        r.rgb.x=r.rgb.y=r.rgb.z=r.training=0;return r;
    }
    // A legitimate zero still counts in both lighting and learning denominators.
    r.valid=1;return r;
}
#ifdef __cplusplus
}
#endif
#undef GI_CONTRIBUTION_INLINE
#endif
