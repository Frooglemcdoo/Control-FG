#include "../src/shaders/pt_scene_hits.hlsl"
#include "../src/shaders/pt_path_integrator.hlsli"
RaytracingAccelerationStructure PTTestScene : register(t5);
struct PTTestRay { float3 origin; float tMin; float3 direction; float tMax; };
StructuredBuffer<PTTestRay> PTTestRays : register(t0,space4);
struct PTTestResult {float3 radiance;float3 expected;uint hits,valid,samples;};
RWStructuredBuffer<PTTestResult> PTTestResults : register(u0,space4);
PTSceneHit PTPathTrace(RayDesc ray) {
    PTSceneHit hit=(PTSceneHit)0;
    TraceRay(PTTestScene,RAY_FLAG_NONE,1,0,2,0,ray,hit);return hit;
}
PTPathSurface PTPathDecode(PTSceneHit hit,RayDesc ray) {
    PTPathSurface s=(PTPathSurface)0;s.position=ray.Origin+ray.Direction*hit.distance;s.normal=hit.shadingNormal;
#ifdef PT_GEOMETRY_SPAWN
    s.position=hit.worldPosition;s.normal=hit.geometricNormal;
    s.spawnOrigin=safeSpawnPoint(s.position,s.normal,hit.spawnOffset);
#else
    s.spawnOrigin=s.position+s.normal*.001;
#endif
    s.material.kd=float3(.5,.25,.75);s.material.f0=0;s.material.roughness=.5;s.material.lobes=1;
    s.emission=DispatchRaysIndex().x<4?float3(1,.5,.25):float3(0,0,0);return s;
}
float3 PTPathEnvironment(float3 direction){return float3(.2,.4,.8);}
bool PTPathLight(PTPathSurface s,float3 outgoingWorld,inout uint rng,out PTDirectSample light,out uint visibility) {
    uint i=DispatchRaysIndex().x;light=(PTDirectSample)0;visibility=1;
    if(i!=4&&i!=5)return false;
    float3 target=PTTestRays[i].origin+(i==4?float3(0,0,0):float3(0,0,-4));
    float3 delta=target-s.position;float distance=length(delta);
    light.wi=delta/distance;light.incidentRadiance=4/(distance*distance);light.pdf=1;light.delta=1;
    RayDesc ray;ray.Origin=s.spawnOrigin;ray.Direction=normalize(target-ray.Origin);ray.TMin=0;ray.TMax=length(target-ray.Origin)-.001;
    PTVisibility v;v.status=0;TraceRay(PTTestScene,RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,1,1,2,1,ray,v);visibility=v.status;return true;
}
[shader("raygeneration")]
void PTSceneQueryRayGeneration() {
    uint i=DispatchRaysIndex().x;PTTestRay input=PTTestRays[i];
    RayDesc ray;ray.Origin=input.origin;ray.Direction=input.direction;ray.TMin=input.tMin;ray.TMax=input.tMax;
    uint depths[8]={1,2,4,8,1,1,1,1};
    PTTestResult output=(PTTestResult)0;output.valid=1;output.samples=64;
    for(uint s=0;s<output.samples;++s){
        PTPathResult p=PTIntegratePath(ray,depths[i],33,1234+s*7919,.001);
        output.radiance+=p.radiance/output.samples;output.hits=max(output.hits,p.hits);output.valid&=p.valid;output.expected.x=max(output.expected.x,(float)p.fault);
    }
    // Expected values are independently checked by the CPU; never accepted from
    // this shader. Keep ABI reserved fields zero.
    PTTestResults[i]=output;
}
