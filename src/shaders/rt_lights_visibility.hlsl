#include "../rt_lights/layout.h"
#ifdef RT_LIGHTS_NATIVE_BINDINGS
// Captured 45116212... global root already covers these registers. Native hit
// handlers and their space-1 material / space-3 local bindings stay intact.
RaytracingAccelerationStructure scene : register(t6);
StructuredBuffer<ShadowTask> tasks : register(t8);
// Native contact hit shaders use t0..t7; t9 is private to this raygen.
StructuredBuffer<Contribution> contributions : register(t9);
RWStructuredBuffer<Visibility> visibility : register(u2);
cbuffer ShaderTableLayout : register(b4) {
    uint hitOffset; uint geometryStride; uint missIndex; uint unused;
}
#else
RaytracingAccelerationStructure scene : register(t0,space7);
StructuredBuffer<ShadowTask> tasks : register(t1,space7);
RWStructuredBuffer<Visibility> visibility : register(u0,space7);
cbuffer ShaderTableLayout : register(b0,space7) {
    uint hitOffset; uint geometryStride; uint missIndex; uint unused;
}
#endif
// Same float-distance payload layout as Control's captured contact-shadow library.
struct ShadowHit { float distance; };
[shader("closesthit")]
void blocked(inout ShadowHit hit, BuiltInTriangleIntersectionAttributes attr) { hit.distance=RayTCurrent(); }
[shader("miss")]
void visible(inout ShadowHit hit) {}
#ifdef RT_LIGHTS_FIXTURE
// Test-only cutout. Production must use verified native material alpha handling.
[shader("anyhit")]
void fixtureCutout(inout ShadowHit hit, BuiltInTriangleIntersectionAttributes attr) { IgnoreHit(); }
#endif
bool Valid(ShadowTask task) {
    float n=dot(task.normal,task.normal);
    return all(isfinite(task.receiver)) && all(isfinite(task.normal)) &&
        all(isfinite(task.lightCenter)) && all(isfinite(task.emitterAxisU)) &&
        all(isfinite(task.emitterAxisV)) && isfinite(n) && n>0.5 && n<1.5 &&
        isfinite(task.originBias) && task.originBias>=0 &&
        isfinite(task.endpointBias) && task.endpointBias>=0 &&
        task.sampleCount>0 && task.sampleCount<=64 && task.instanceMask>0 && task.instanceMask<=255;
}
[shader("raygeneration")]
void perLightVisibility() {
    if(any(DispatchRaysIndex().yz)) return;
    uint index=DispatchRaysIndex().x, count,stride,outCount,outStride;
    tasks.GetDimensions(count,stride); visibility.GetDimensions(outCount,outStride);
    if(index>=outCount) return;
    Visibility result=(Visibility)0;
    visibility[index]=result;
    if(index>=count || stride!=96 || outStride!=32 || geometryStride==0) return;
    ShadowTask task=tasks[index];
    result.lightId=task.lightId; result.receiverId=task.receiverId; result.frameTag=task.frameTag;
    visibility[index]=result;
    if(!Valid(task)) return;
#ifdef RT_LIGHTS_NATIVE_BINDINGS
    uint contributionCount, contributionStride;
    contributions.GetDimensions(contributionCount, contributionStride);
    if(index>=contributionCount || contributionStride!=48) return;
    Contribution contribution=contributions[index];
    if(contribution.visibilityIndex!=index || contribution.receiverId!=task.receiverId ||
       contribution.lightId!=task.lightId || contribution.frameTag!=task.frameTag ||
       !all(isfinite(contribution.diffuse)) || !all(isfinite(contribution.specular))) return;
    // Exact zero only: never cull faint lights or change BRDF/cone membership.
    if(all(contribution.diffuse==0) && all(contribution.specular==0)) {
        result.value=1;result.valid=1;result.nearestBlocker=3.402823466e+38;
        result.reserved1=1;visibility[index]=result;return;
    }
#endif
    float3 normal=normalize(task.normal);
    if(dot(normal,task.lightCenter-task.receiver)<0) normal=-normal;
    float3 origin=task.receiver+normal*task.originBias;
    float sum=0, nearest=3.402823466e+38;
    for(uint sample=0;sample<task.sampleCount;++sample) {
        float radius=task.sampleCount==1 ? 0 : sqrt((sample+0.5)/task.sampleCount);
        float angle=sample*2.399963229728653;
        float3 target=task.lightCenter+radius*(cos(angle)*task.emitterAxisU+sin(angle)*task.emitterAxisV);
        float3 delta=target-origin;
        float lengthToLight=length(delta);
        if(!isfinite(lengthToLight) || lengthToLight<=task.endpointBias) return;
        RayDesc ray;
        ray.Origin=origin; ray.Direction=delta/lengthToLight;
        ray.TMin=0; ray.TMax=lengthToLight-task.endpointBias;
        ShadowHit hit; hit.distance=ray.TMax;
        // Only binary visibility is consumed. Native alpha testing still runs;
        // terminate after the first ACCEPTED hit, not the first cutout candidate.
        TraceRay(scene,RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,task.instanceMask,hitOffset,geometryStride,missIndex,ray,hit);
        bool occluded=hit.distance<ray.TMax;
        sum+=occluded ? 0 : 1;
        if(occluded) nearest=min(nearest,hit.distance);
    }
    result.reserved0=task.sampleCount; // actual rays, for sparse GPU evidence
    result.value=sum/task.sampleCount; result.nearestBlocker=nearest; result.valid=1;
    visibility[index]=result;
}
