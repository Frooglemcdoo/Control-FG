// Native variant uses the existing contact-shadow root layout. The host must
// supply the current scene and alpha-aware hit table, not an opaque-only table.
#include "../gi_reuse/visibility.h"
#ifdef GI_NATIVE_VISIBILITY_BINDINGS
RaytracingAccelerationStructure giScene : register(t6);
StructuredBuffer<GIVisibilityRequest> requests : register(t8);
RWStructuredBuffer<GIConnectionVisibility> results : register(u2);
cbuffer GITable : register(b4) { GIVisibilityConstants controls; }
#else
RaytracingAccelerationStructure giScene : register(t0,space7);
StructuredBuffer<GIVisibilityRequest> requests : register(t1,space7);
RWStructuredBuffer<GIConnectionVisibility> results : register(u0,space7);
cbuffer GITable : register(b0,space7) { GIVisibilityConstants controls; }
#endif
struct GIShadowHit { float distance; };
// These exports are also usable with native any-hit material tests. The owned
// miss MUST be used: an ignored any-hit may have modified the distance payload.
[shader("closesthit")]
void giConnectionBlocked(inout GIShadowHit hit, BuiltInTriangleIntersectionAttributes attr) {
    hit.distance=RayTCurrent();
}
[shader("miss")]
void giConnectionMiss(inout GIShadowHit hit) { hit.distance=-1.0f; }
#ifdef GI_VISIBILITY_FIXTURE
[shader("anyhit")]
void giConnectionCutout(inout GIShadowHit hit, BuiltInTriangleIntersectionAttributes attr) {
    hit.distance=RayTCurrent(); // deliberately dirty payload before rejecting
    IgnoreHit();
}
#endif
[shader("raygeneration")]
void giConnectionVisibility() {
    if(any(DispatchRaysIndex().yz)) return;
    uint index=DispatchRaysIndex().x,count,stride,outCount,outStride;
    requests.GetDimensions(count,stride);results.GetDimensions(outCount,outStride);
    // Do not write using an incompatible view stride.
    if(index>=outCount||outStride!=32) return;
    GIConnectionVisibility result=(GIConnectionVisibility)0;
    results[index]=result;
    if(index>=count||stride!=112) return;
    GIVisibilityRequest request=requests[index];
    result.frameTag=request.connection.frameTag;result.connectionId=request.connection.connectionId;
    result.sceneRevision=request.sceneRevision;results[index]=result;
    if(controls.geometryStride==0||controls.geometryStride>15||controls.hitOffset>15||
       controls.missIndex>65535||controls.sceneRevision==0||controls.nativeAlphaPolicy!=1||
       controls.reserved0||controls.reserved1||request.sceneRevision!=controls.sceneRevision||
       request.reserved0||request.reserved1||request.reserved2||
       request.nativeBindingsVerified!=1||request.instanceMask==0||request.instanceMask>255||
       !isfinite(request.originTMin)||request.originTMin<0||
       !isfinite(request.endpointBias)||request.endpointBias<0) return;
    GIConnectionResult connection=GIEvaluateConnection(request.connection,controls.frameTag);
    float end=connection.distance-request.endpointBias;
    if(connection.valid==0||!isfinite(end)||request.originTMin>=end) return;
    RayDesc ray;
    ray.Origin=float3(request.connection.target.x,request.connection.target.y,request.connection.target.z);
    ray.Direction=float3(connection.direction.x,connection.direction.y,connection.direction.z);
    ray.TMin=request.originTMin;ray.TMax=end;
    GIShadowHit hit;hit.distance=ray.TMax;
    TraceRay(giScene,RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,request.instanceMask,
        controls.hitOffset,controls.geometryStride,controls.missIndex,ray,hit);
    // The owned miss writes a distinct sentinel, including at the endpoint.
    bool missed=hit.distance==-1.0f;
    if(!missed&&(!isfinite(hit.distance)||hit.distance<ray.TMin||hit.distance>ray.TMax)) return;
    result.completed=1;result.nativeAlphaPolicy=controls.nativeAlphaPolicy;
    result.visible=missed?1:0;results[index]=result;
}
