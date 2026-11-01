// Unbound prototype. Production integration must link verified native contact
// hit/miss shaders and their alpha/material bindings, as rt_lights_visibility does.
#include "../gi_reuse/connection.h"
RaytracingAccelerationStructure giScene : register(t0,space7);
struct GIVisibilityRequest {
    GIConnection connection;
    float originTMin; float endpointBias; uint instanceMask; uint nativeBindingsVerified;
};
struct GIVisibilityResult {
    uint frameTag; uint connectionId; uint valid; uint visible;
};
StructuredBuffer<GIVisibilityRequest> requests : register(t1,space7);
RWStructuredBuffer<GIVisibilityResult> results : register(u0,space7);
cbuffer GITable : register(b0,space7) {
    uint hitOffset; uint geometryStride; uint missIndex; uint frameTag;
}
struct GIShadowHit { float distance; };
[shader("raygeneration")]
void giConnectionVisibility() {
    if(any(DispatchRaysIndex().yz)) return;
    uint index=DispatchRaysIndex().x,count,stride,outCount,outStride;
    requests.GetDimensions(count,stride);results.GetDimensions(outCount,outStride);
    if(index>=outCount) return;
    GIVisibilityResult result=(GIVisibilityResult)0;
    results[index]=result;
    if(index>=count||stride!=96||outStride!=16||geometryStride==0) return;
    GIVisibilityRequest request=requests[index];
    result.frameTag=request.connection.frameTag;result.connectionId=request.connection.connectionId;
    results[index]=result;
    if(request.nativeBindingsVerified!=1||request.instanceMask==0||request.instanceMask>255||
       !isfinite(request.originTMin)||request.originTMin<0||
       !isfinite(request.endpointBias)||request.endpointBias<0) return;
    GIConnectionResult connection=GIEvaluateConnection(request.connection,frameTag);
    if(connection.valid==0||request.originTMin>=connection.distance-request.endpointBias) return;
    RayDesc ray;
    ray.Origin=float3(request.connection.target.x,request.connection.target.y,request.connection.target.z);
    ray.Direction=float3(connection.direction.x,connection.direction.y,connection.direction.z);
    ray.TMin=request.originTMin;ray.TMax=connection.distance-request.endpointBias;
    GIShadowHit hit;hit.distance=ray.TMax;
    // Retain native any-hit alpha testing. No FORCE_OPAQUE or skipped any-hit.
    TraceRay(giScene,RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,request.instanceMask,
        hitOffset,geometryStride,missIndex,ray,hit);
    if(!isfinite(hit.distance)||hit.distance<0) return;
    result.valid=1;result.visible=hit.distance>=ray.TMax?1:0;results[index]=result;
}
