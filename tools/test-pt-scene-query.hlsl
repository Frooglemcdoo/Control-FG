// Link/compile fixture for private ray types. Requires a host-created two-type
// SBT; not a shader to install into Control's native state object.
#include "../src/shaders/pt_scene_hits.hlsl"
RaytracingAccelerationStructure PTTestScene : register(t5);
struct PTTestRay { float3 origin; float tMin; float3 direction; float tMax; };
StructuredBuffer<PTTestRay> PTTestRays : register(t0,space4);
struct PTTestResult { PTSceneHit hit; uint visibility; };
RWStructuredBuffer<PTTestResult> PTTestResults : register(u0,space4);
[shader("raygeneration")]
void PTSceneQueryRayGeneration() {
    uint i=DispatchRaysIndex().x;PTTestRay input=PTTestRays[i];
    PTTestResult result=(PTTestResult)0;
    result.hit.status=2;result.visibility=2;
    float n=dot(input.direction,input.direction);
    if(all(isfinite(input.origin))&&all(isfinite(input.direction))&&n>1e-12&&
       isfinite(input.tMin)&&isfinite(input.tMax)&&input.tMin>=0&&input.tMax>input.tMin) {
        RayDesc ray;ray.Origin=input.origin;ray.Direction=input.direction*rsqrt(n);
        ray.TMin=input.tMin;ray.TMax=input.tMax;
        result.hit.status=0;
        TraceRay(PTTestScene,RAY_FLAG_NONE,1,0,2,0,ray,result.hit);
        PTVisibility shadow;shadow.status=0;
        TraceRay(PTTestScene,RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,1,1,2,1,ray,shadow);
        result.visibility=shadow.status;
    }
    PTTestResults[i]=result;
}
