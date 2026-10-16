// Synthetic constant inputs legitimately make some finite checks redundant.
#pragma warning(disable: 3577)
// FXC flags aggregate returns on the deliberately non-finite test branch.
// Every returned struct is initialized; GPU readback checks every result.
#pragma warning(disable: 4000)
// Execute the production integrator with deterministic synthetic hit adapters.
struct RayDesc {float3 Origin;float TMin;float3 Direction;float TMax;};
struct PTSceneHit {uint status;float distance;float3 geometricNormal;};
#define PT_PRIMARY_SURFACE_REPLACEMENT 1
#define PT_PATH_PRIMARY_DECODE 1
static uint testCase=0;
#include "../src/shaders/pt_path_integrator.hlsli"
RWStructuredBuffer<float4> output : register(u0);
PTSceneHit PTPathTrace(RayDesc ray){PTSceneHit h=(PTSceneHit)0;h.status=1;h.distance=1;return h;}
PTPathSurface PTPathDecodePrimary(PTSceneHit hit,RayDesc ray){
    PTPathSurface s=(PTPathSurface)0;s.normal=float3(0,0,1);s.spawnOrigin=0;
    s.material.kd=1;s.material.f0=testCase==7?.5:0;s.material.roughness=.5;s.material.lobes=1;
    if(testCase==5){s.material.lobes=0;s.decodeFault=12;s.emission=100;}
    return s;
}
PTPathSurface PTPathDecode(PTSceneHit hit,RayDesc ray){
    PTPathSurface s=(PTPathSurface)0;s.normal=normalize(-ray.Direction+float3(.01,.02,0));s.material.roughness=.5;
    s.emission=float3(2,3,4);s.decodeFault=12;
    if(testCase==1)s.emission=0;
    if(testCase==2)s.normal=ray.Direction;
    if(testCase==3)s.emission=asfloat(0x7fc00000);
    if(testCase==4)s.decodeFault=13;
    if(testCase==6){s.material.lobes=1;s.decodeFault=0;} // Valid absorbing emitter.
    return s;
}
float3 PTPathEnvironment(float3 direction){return 0;}
bool PTPathLight(PTPathSurface surface,float3 outgoingWorld,inout uint rng,out PTDirectSample light,out uint visibility){light=(PTDirectSample)0;light.pdf=1;light.delta=1;light.wi=float3(0,0,1);visibility=1;return false;}
[numthreads(8,1,1)]
void main(uint3 id:SV_DispatchThreadID){
    testCase=id.x;RayDesc ray=(RayDesc)0;ray.Origin=float3(0,0,1);ray.Direction=normalize(float3(.1+.01*id.x,.2,-1));ray.TMin=0;ray.TMax=10;
    PTSceneHit first=(PTSceneHit)0;first.status=1;first.geometricNormal=float3(0,0,1);
    PTPathResult r=PTIntegratePathFirst(ray,4,3,12345,.001,first,1,0);
    output[id.x]=float4(r.radiance,(float)r.primaryValid);
}
