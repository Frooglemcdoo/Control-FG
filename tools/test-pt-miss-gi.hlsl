#pragma warning(disable: 3577)
#pragma warning(disable: 4000)
struct RayDesc {float3 Origin;float TMin;float3 Direction;float TMax;};
struct PTSceneHit {uint status;float distance;float3 geometricNormal;};
#define PT_PRIMARY_SURFACE_REPLACEMENT 1
#define PT_PATH_PRIMARY_DECODE 1
#define PT_PATH_TERMINAL_GI 1
#define PT_PATH_MISS_GI 1
static uint testCase=0,traceCount=0;
static RayDesc traced;
#include "../src/shaders/pt_native_ray_extent.hlsli"
#include "../src/shaders/pt_path_integrator.hlsli"
RWStructuredBuffer<float4> output : register(u0);
PTSceneHit PTPathTrace(RayDesc ray){
    traced=ray;traceCount++;
    PTSceneHit h=(PTSceneHit)0;
    h.status=testCase==3?2:((testCase==2||testCase==6)&&traceCount==1?1:0);h.distance=1;return h;
}
PTPathSurface PTPathDecodePrimary(PTSceneHit hit,RayDesc ray){
    PTPathSurface s=(PTPathSurface)0;s.normal=float3(0,0,1);
    s.material.kd=1;s.material.roughness=.5;s.material.lobes=1;return s;
}
PTPathSurface PTPathDecode(PTSceneHit hit,RayDesc ray){
    PTPathSurface s=(PTPathSurface)0;s.normal=-ray.Direction;s.position=ray.Origin+ray.Direction;
    s.spawnOrigin=s.position;s.material.kd=testCase==6?0:.25;s.material.roughness=1;s.material.lobes=1;return s;
}
float PTPathContinuationLimit(float roughness,float noise){
    return testCase==1?1e6:(testCase==7?asfloat(0x7fc00000u):PTNativeRayExtent(roughness,noise));
}
float3 PTPathEnvironment(float3 direction){return float3(11,13,17);}
bool PTPathLight(PTPathSurface surface,float3 outgoingWorld,inout uint rng,out PTDirectSample light,out uint visibility){light=(PTDirectSample)0;visibility=1;return false;}
[numthreads(8,1,1)]
void main(uint3 id:SV_DispatchThreadID){
    testCase=id.x;RayDesc ray=(RayDesc)0;ray.Origin=float3(0,0,1);ray.Direction=float3(0,0,-1);ray.TMax=1e6;
    PTSceneHit first=(PTSceneHit)0;first.status=testCase==5?0:1;first.geometricNormal=float3(0,0,1);
    PTPathResult r=PTIntegratePathFirst(ray,testCase==4?1:4,99,12345,.001,first,1,0);
    output[id.x*2]=float4(r.terminalWeight,(float)r.terminalPending);
    float endpointError=0,normalError=0;
    if(r.terminalPending==2){
        endpointError=length(r.terminalPosition-(traced.Origin+traced.Direction*traced.TMax));
        normalError=length(r.terminalNormal-traced.Direction)+length(r.terminalFraction-1)+length(r.terminalFallback-float3(11,13,17));
        // Independently known native bounds over roughness/noise: [4.2,448).
        if(traced.TMax<4.2||traced.TMax>=448)endpointError=100;
    }
    endpointError+=abs(PTNativeRayExtent(0,0)-268.8)+abs(PTNativeRayExtent(1,0)-4.2)+abs(PTNativeRayExtent(.25,.5)-44.8);
    output[id.x*2+1]=float4(endpointError,normalError,dot(r.radiance,1),(float)r.primaryValid);
}
