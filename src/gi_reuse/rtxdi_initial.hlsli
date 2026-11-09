#ifndef CONTROL_GI_RTXDI_INITIAL_HLSLI
#define CONTROL_GI_RTXDI_INITIAL_HLSLI
#include "initial_sample.h"
#include "Rtxdi/GI/Reservoir.hlsli"
// Caller defines RTXDI_GI_RESERVOIR_BUFFER before including this file.
// This seeds ONE candidate, with M=1. It does not combine ray-count samples,
// perform reuse, or pack radiance into a lossy representation.
bool GICreateRTXDIInitial(GIInitialInput input, float3 secondaryGeometricNormal,
    uint geometryVerified, uint currentFrame, out RTXDI_GIReservoir reservoir,
    out GIInitialResult estimate)
{
    reservoir=RTXDI_EmptyGIReservoir();
    estimate=GIMakeInitialSample(input,currentFrame);
    if(!estimate.valid) return false;
    if(!estimate.isSurface) return true; // Fallback remains outside reservoirs.
    if(geometryVerified!=1||!all(isfinite(secondaryGeometricNormal))||
       abs(dot(secondaryGeometricNormal,secondaryGeometricNormal)-1)>0.001) {
        estimate=(GIInitialResult)0;return false;
    }
    reservoir=RTXDI_MakeGIReservoir(
        float3(input.secondaryPosition.x,input.secondaryPosition.y,input.secondaryPosition.z),
        secondaryGeometricNormal,
        float3(input.secondaryRadiance.x,input.secondaryRadiance.y,input.secondaryRadiance.z),
        input.sourcePdf);
    // Reconstruct without reuse from the actual SDK weight, preserving N.
    float3 result=reservoir.radiance*
        float3(input.primaryBrdfCos.x,input.primaryBrdfCos.y,input.primaryBrdfCos.z)*
        reservoir.weightSum*(input.depthScale/float(input.configuredCount));
    if(!all(isfinite(result))||any(result<0)) {
        reservoir=RTXDI_EmptyGIReservoir();estimate=(GIInitialResult)0;return false;
    }
    estimate.surfaceContribution.x=result.x;
    estimate.surfaceContribution.y=result.y;
    estimate.surfaceContribution.z=result.z;
    estimate.inverseSourcePdf=reservoir.weightSum;
    return true;
}
#endif
