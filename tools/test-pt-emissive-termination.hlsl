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
#define PT_PATH_SURFACE_GEOMETRY 1
static uint testCase=0;
#include "../src/shaders/pt_material_admission.hlsli"
uint2 TestMaterialFlags(){
    uint mode=(testCase-8u)&3u;
    return uint2(0x1a1a1a00u|mode,mode==0?0x00030008u:0xffff0033u);
}
#include "../src/shaders/pt_path_integrator.hlsli"
RWStructuredBuffer<float4> output : register(u0);
PTSceneHit PTPathTrace(RayDesc ray){PTSceneHit h=(PTSceneHit)0;h.status=1;h.distance=1;return h;}
PTPathSurface PTPathDecodePrimary(PTSceneHit hit,RayDesc ray){
    PTPathSurface s=(PTPathSurface)0;s.normal=float3(0,0,1);s.geometricNormal=s.normal;s.spawnOrigin=0;
    s.material.kd=1;s.material.f0=testCase==7?.5:0;s.material.roughness=.5;s.material.lobes=1;
    if(testCase==5||(testCase>=12&&testCase<16&&!PTMaterialScatteringSupported(TestMaterialFlags().x,TestMaterialFlags().y,true))){s.material.lobes=0;s.decodeFault=12;s.emission=100;}
    return s;
}
PTPathSurface PTPathDecode(PTSceneHit hit,RayDesc ray){
    PTPathSurface s=(PTPathSurface)0;s.normal=normalize(-ray.Direction+float3(.01,.02,0));s.material.roughness=.5;
    s.geometricNormal=s.normal;
    s.emission=float3(2,3,4);s.decodeFault=12;
    if(testCase>=16){
        s.geometricNormal=normalize(-ray.Direction);
        float3 t=normalize(cross(abs(s.geometricNormal.z)<.9?float3(0,0,1):float3(0,1,0),s.geometricNormal));
        float3 mesh=.8*s.geometricNormal+.6*t;
        if(testCase==19)mesh=-mesh;
        s.normal=PTSecondaryShadingNormal(mesh,s.geometricNormal,-ray.Direction);
        s.position=float3(0,0,2);s.material.kd=.5;s.material.f0=0;s.material.lobes=1;s.decodeFault=0;s.emission=0;
        return s;
    }
    if(testCase>=8){
        s.position=float3(0,0,2);s.emission=0;
        if(PTMaterialScatteringSupported(TestMaterialFlags().x,TestMaterialFlags().y,false)){
            s.material.kd=.5;s.material.f0=0;s.material.lobes=1;s.decodeFault=0;
        }
        return s;
    }
    if(testCase==1)s.emission=0;
    if(testCase==2)s.normal=ray.Direction;
    if(testCase==3)s.emission=asfloat(0x7fc00000);
    if(testCase==4)s.decodeFault=13;
    if(testCase==6){s.material.lobes=1;s.decodeFault=0;} // Valid absorbing emitter.
    return s;
}
float3 PTPathEnvironment(float3 direction){return 0;}
bool PTPathLight(PTPathSurface surface,float3 outgoingWorld,inout uint rng,out PTDirectSample light,out uint visibility){light=(PTDirectSample)0;light.pdf=1;light.delta=1;light.wi=float3(0,0,1);visibility=1;
    if(testCase>=8&&surface.position.z==2){light.wi=surface.normal;
        if(testCase==17){float3 ng=surface.geometricNormal;float3 t=normalize(surface.normal-.8*ng);light.wi=sqrt(.99)*t-.1*ng;}
        if(testCase==18||testCase==19)light.wi=surface.geometricNormal;
        light.incidentRadiance=float3(2,4,6)*PTTransportPi;return true;}
    return false;}
[numthreads(20,1,1)]
void main(uint3 id:SV_DispatchThreadID){
    testCase=id.x;RayDesc ray=(RayDesc)0;ray.Origin=float3(0,0,1);ray.Direction=normalize(float3(.1+.01*id.x,.2,-1));ray.TMin=0;ray.TMax=10;
    PTSceneHit first=(PTSceneHit)0;first.status=1;first.geometricNormal=float3(0,0,1);
    PTPathResult r=PTIntegratePathFirst(ray,testCase>=8?2u:4u,3,12345,.001,first,1,0);
    output[id.x]=float4(r.radiance,(float)r.primaryValid);
}
