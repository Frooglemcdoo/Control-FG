#include "../src/shaders/pt_surface_commit.hlsli"
struct RayDesc {float3 Origin;float TMin;float3 Direction;float TMax;};
struct PTSceneHit {uint status;float3 geometricNormal;};
#define PT_PATH_PRIMARY_DECODE 1
#include "../src/shaders/pt_path_integrator.hlsli"
static uint testMode;
PTSceneHit PTPathTrace(RayDesc ray){PTSceneHit h;h.status=1;h.geometricNormal=-ray.Direction;return h;}
PTPathSurface PTPathDecodePrimary(PTSceneHit hit,RayDesc ray){
    PTPathSurface s=(PTPathSurface)0;s.normal=float3(0,0,1);s.spawnOrigin=float3(0,0,.001);
    s.material.kd=.5;s.material.roughness=.5;s.material.lobes=testMode==10?0:1;s.decodeFault=12;return s;
}
PTPathSurface PTPathDecode(PTSceneHit hit,RayDesc ray){
    PTPathSurface s=(PTPathSurface)0;s.normal=hit.geometricNormal;s.material.roughness=.5;
    s.material.lobes=testMode==11?0:1;s.decodeFault=12;return s;
}
float3 PTPathEnvironment(float3 direction){return 0;}
bool PTPathLight(PTPathSurface surface,inout uint rng,out PTDirectSample light,out uint visibility){
    light=(PTDirectSample)0;light.wi=surface.normal;light.incidentRadiance=1;light.pdf=1;light.delta=1;visibility=1;return true;
}
[numthreads(16,1,1)]
void main(uint3 id:SV_DispatchThreadID){
    uint x=id.x;float3 c=float3(.25,.5,.75);uint admission=1;uint2 extent=uint2(16,1);
    if(x==0)admission=0;if(x==1)admission=2;
    if(x==2)c.x=asfloat(0x7fc00000);if(x==4)c.y=-1;
    if(x==5)extent.x=7;if(x==6)c=0;
    if(x==8)c.x=asfloat(0x7f800000);
    if(x==9){PTClearSurface(uint2(16,0),extent);PTCommitSurface(uint2(16,0),extent,c,1);}
    if(x>=10&&x<=12){
        testMode=x;RayDesc ray;ray.Origin=float3(0,0,1);ray.Direction=float3(0,0,-1);ray.TMin=0;ray.TMax=100;
        PTSceneHit first;first.status=1;first.geometricNormal=float3(0,0,1);
        PTPathResult estimate=PTIntegratePathFirst(ray,2,3,1234,.001,first,1,0);
        PTClearSurface(id.xy,extent);
        if(estimate.primaryValid!=0)PTCommitSurface(id.xy,extent,estimate.radiance,1);
        return;
    }
    if(x>=9)return;
    PTClearSurface(id.xy,extent);
    PTCommitSurface(id.xy,extent,c,admission);
}
