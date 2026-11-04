#ifndef CONTROL_GI_OUTPUT_RESOLVE_H
#define CONTROL_GI_OUTPUT_RESOLVE_H
#include "producer_shading.h"
#ifdef __cplusplus
namespace gi_reuse {
#define GI_OUTPUT_INLINE inline
#define GI_OUTPUT_REF &
#else
#define GI_OUTPUT_INLINE
#define GI_OUTPUT_REF
#endif
// Output colour is already normalized by the complete initial batch. Preserve
// native mean hit-distance independently of reservoir selection or luminance.
struct GIOutputState {
    GIPoint color; float distance;
    uint seenLow; uint seenHigh; uint processed; uint valid;
    uint frameTag; uint count; float depthScale; uint reserved;
};
GI_OUTPUT_INLINE GIOutputState GIBeginOutput(GIInitialResult result,uint frameTag,uint count,float depthScale) {
#ifdef __cplusplus
    GIOutputState resolved{};
#else
    GIOutputState resolved=(GIOutputState)0;
#endif
    if(result.valid!=1||result.frameTag!=frameTag||!count||count>64||
       !GIFinite(depthScale)||depthScale<0||!GINonnegative(result.surfaceContribution)||
       !GINonnegative(result.fallbackContribution)||result.isSurface>1||
       (result.isSurface&&result.rayIndex>=count))return resolved;
    resolved.color.x=result.surfaceContribution.x+result.fallbackContribution.x;
    resolved.color.y=result.surfaceContribution.y+result.fallbackContribution.y;
    resolved.color.z=result.surfaceContribution.z+result.fallbackContribution.z;
    if(!GINonnegative(resolved.color))return resolved;
    resolved.frameTag=frameTag;resolved.count=count;resolved.depthScale=depthScale;resolved.valid=1;return resolved;
}
// The initial-only path has already shaded every ray. Display their complete
// Monte Carlo estimate rather than add a second random, single-sample selection.
// Keep the reservoir separately for future validated temporal/spatial reuse.
GI_OUTPUT_INLINE GIOutputState GIBeginBatchOutput(GIInitialResult result,uint frameTag,uint count,float depthScale) {
    GIOutputState resolved=GIBeginOutput(result,frameTag,count,depthScale);
    resolved.color.x=0;resolved.color.y=0;resolved.color.z=0;resolved.reserved=1;
    return resolved;
}
GI_OUTPUT_INLINE bool GIAddOutputDistance(
#ifndef __cplusplus
    inout
#endif
    GIOutputState GI_OUTPUT_REF state,GIShadedProducerSample sample) {
    if(!state.valid)return false;
    const GIInitialInput input=sample.shading;
    GIInitialResult estimate=GIMakeInitialSample(input,state.frameTag);
    if(input.frameTag!=state.frameTag||input.configuredCount!=state.count||
       input.depthScale!=state.depthScale||input.rayIndex>=state.count||
       !GIFinite(sample.hitDistance)||sample.hitDistance<0||sample.reserved1||sample.reserved2||
       !estimate.valid){state.valid=0;return false;}
    const uint bit=1u<<(input.rayIndex&31u);
    if(((input.rayIndex<32?state.seenLow:state.seenHigh)&bit)!=0){state.valid=0;return false;}
    // Divide before summation to avoid overflow from a representable mean.
    state.distance+=sample.hitDistance/float(state.count);
    if(!GIFinite(state.distance)){state.valid=0;return false;}
    if(input.rayIndex<32)state.seenLow|=bit;else state.seenHigh|=bit;
    if(state.reserved==1){
        state.color.x+=estimate.surfaceContribution.x+estimate.fallbackContribution.x;
        state.color.y+=estimate.surfaceContribution.y+estimate.fallbackContribution.y;
        state.color.z+=estimate.surfaceContribution.z+estimate.fallbackContribution.z;
        if(!GINonnegative(state.color)){state.valid=0;return false;}
    }
    ++state.processed;return true;
}
GI_OUTPUT_INLINE bool GIOutputComplete(GIOutputState state) {
    return state.valid==1&&state.count>0&&state.count<=64&&state.processed==state.count;
}
#ifdef __cplusplus
static_assert(sizeof(GIOutputState)==48);
}
#endif
#undef GI_OUTPUT_REF
#undef GI_OUTPUT_INLINE
#endif
