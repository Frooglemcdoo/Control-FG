#pragma warning(disable: 3577)
#pragma warning(disable: 4000)
#ifndef __HLSL_VERSION
struct RayDesc {float3 Origin;float TMin;float3 Direction;float TMax;};
#endif
struct PTSceneHit {uint status;float distance;};
#define PT_PRIMARY_SURFACE_REPLACEMENT 1
#define PT_PATH_PRIMARY_DECODE 1
#define PT_PATH_EMITTER_NEE 1
static uint testCase=0;
static float hitPdf=0;
#include "../src/shaders/pt_path_integrator.hlsli"
RWStructuredBuffer<float4> output:register(u0);
PTSceneHit PTPathTrace(RayDesc ray){PTSceneHit h;h.status=1;h.distance=1;hitPdf=ray.Direction.z/PTTransportPi;return h;}
PTPathSurface PTPathDecodePrimary(PTSceneHit hit,RayDesc ray){
    PTPathSurface s=(PTPathSurface)0;s.normal=float3(0,0,1);s.material.kd=1;s.material.roughness=.5;s.material.lobes=1;return s;
}
PTPathSurface PTPathDecode(PTSceneHit hit,RayDesc ray){
    PTPathSurface s=(PTPathSurface)0;s.position=float3(0,0,1);s.normal=-ray.Direction;s.emission=2;
    s.material.roughness=.5;s.material.lobes=testCase==4?0:1;s.decodeFault=testCase==4?12:0;return s;
}
float3 PTPathEnvironment(float3 direction){return 0;}
bool PTPathLight(PTPathSurface surface,float3 outgoing,inout uint rng,out PTDirectSample light,out uint visibility){
    light=(PTDirectSample)0;visibility=1;return false;
}
float PTPathEmitterPdf(float3 previous,PTSceneHit hit){return testCase==1?0:hitPdf;}
bool PTPathEmitterLight(PTPathSurface surface,inout uint rng,out PTDirectSample light,out uint visibility){
    light=(PTDirectSample)0;visibility=testCase==2?0:1;
    light.wi=float3(0,0,1);light.incidentRadiance=PTTransportPi;light.pdf=1;
    return testCase!=1&&testCase!=5&&surface.position.z==0;
}
[numthreads(6,1,1)]
void main(uint3 id:SV_DispatchThreadID){
    testCase=id.x;RayDesc ray=(RayDesc)0;ray.Origin=float3(0,0,1);ray.Direction=float3(0,0,-1);ray.TMax=10;
    PTSceneHit first;first.status=1;first.distance=1;
    PTPathResult r=PTIntegratePathFirst(ray,testCase==3||testCase==5?1:2,3,12345,.001,first,1,0);
    output[id.x]=float4(r.radiance,r.primaryValid);
}
