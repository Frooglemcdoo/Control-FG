#ifndef CONTROL_PT_PATH_INTEGRATOR
#define CONTROL_PT_PATH_INTEGRATOR
#include "pt_direct_light.hlsli"
// Secondary adapters use coincident geometric/shading normals. Primary camera
// radiance transport may use the raster shading normal with geometric hemisphere
// rejection below. Transmission and importance-mode correction are not admitted. Emitters/environment are BSDF-only; NEE accepts DELTA lights only,
// preventing duplicate non-delta light energy until emitter MIS is implemented.
struct PTPathSurface {
    float3 position, normal, emission, spawnOrigin;
    PTSurfaceMaterial material;
    uint decodeFault;
};
// Adapter supplies these functions before including this file's implementation.
PTSceneHit PTPathTrace(RayDesc ray);
PTPathSurface PTPathDecode(PTSceneHit hit, RayDesc ray);
#ifdef PT_PATH_PRIMARY_DECODE
PTPathSurface PTPathDecodePrimary(PTSceneHit hit,RayDesc ray);
#endif
float3 PTPathEnvironment(float3 direction);
bool PTPathLight(PTPathSurface surface, float3 outgoingWorld, inout uint rng, out PTDirectSample light, out uint visibility);

uint PTPathRandomBits(inout uint state) {
    state ^= state << 13; state ^= state >> 17; state ^= state << 5; return state;
}
float PTPathRandom(inout uint state) { return (PTPathRandomBits(state)>>8)*(1.0/16777216.0); }
struct PTPathResult { float3 radiance; uint hits; uint valid; uint fault; uint primaryValid; uint terminalGIPositive;
#ifdef PT_PATH_TERMINAL_GI
float3 terminalPosition,terminalNormal,terminalWeight,terminalFraction;uint terminalPending;
#endif
 float3 diffuse,specular,kd,f0; float roughness,hitDistance,primaryViewCosine; };
PTPathResult PTIntegratePathFirst(RayDesc ray,uint maxHits,uint rouletteStart,uint seed,float originOffset,
                                  PTSceneHit first,uint useFirst,uint includePrimaryEmission) {
    PTTransportState path=PTBeginTransport();PTPathResult result=(PTPathResult)0;
    if(maxHits==0||maxHits>32||!all(isfinite(ray.Origin))||!all(isfinite(ray.Direction))||
       abs(dot(ray.Direction,ray.Direction)-1)>1e-4||!isfinite(ray.TMin)||!isfinite(ray.TMax)||
       ray.TMin<0||ray.TMax<=ray.TMin||!isfinite(originOffset)||originOffset<=0){result.fault=1;return result;}
    uint rng=seed==0?1:seed; float3 diffuseFraction=1; float3 primaryDirectDiffuse=0,primaryDirectSpecular=0;
#ifdef PT_PATH_TERMINAL_GI
    float3 terminalPosition=0,terminalNormal=0,terminalWeight=0;bool terminalPending=false;
#endif
    for(uint depth=0;depth<maxHits&&path.alive!=0;++depth) {
        PTSceneHit hit;
        if(depth==0&&useFirst!=0)hit=first;else hit=PTPathTrace(ray);
        if(depth==1)result.hitDistance=hit.status==1?hit.distance:1e6;
        if(hit.status==2){path.valid=0;result.fault=2;break;}
        if(hit.status==0){path=PTAccumulate(path,PTPathEnvironment(ray.Direction));break;}
        PTPathSurface surface;
#ifdef PT_PATH_PRIMARY_DECODE
        if(depth==0&&useFirst!=0)surface=PTPathDecodePrimary(hit,ray);else surface=PTPathDecode(hit,ray);
#else
        surface=PTPathDecode(hit,ray);
#endif
        result.hits++;
        PTSurfaceMaterial m=surface.material;
        if(depth==0){result.kd=m.kd;result.f0=m.f0;result.roughness=m.roughness;}
        if(!PTValid(m.kd)||!PTValid(m.f0)||!PTFinite(m.roughness)||m.roughness<0||m.roughness>1||
           any(m.kd<0)||any(m.kd>1)||any(m.f0<0)||any(m.f0>1)||(m.lobes&~3u)!=0||m.lobes==0){path.valid=0;result.fault=surface.decodeFault!=0?surface.decodeFault:3;break;}
#ifdef PT_PRIMARY_SURFACE_REPLACEMENT
        // Primary texture color is evaluated by the original raster material
        // shader at composition. Trace unit-color diffuse transport so black
        // channels and layered materials do not require albedo division.
        if(depth==0){surface.material.kd=1-surface.material.f0;m=surface.material;}
#endif
        float3 n=surface.normal;
        if(!all(isfinite(surface.position))||!all(isfinite(n))||abs(dot(n,n)-1)>1e-4){path.valid=0;result.fault=4;break;}
        float3 tangent=normalize(cross(abs(n.z)<0.9?float3(0,0,1):float3(0,1,0),n));
        float3 bitangent=cross(n,tangent);
        float3 wo=float3(dot(-ray.Direction,tangent),dot(-ray.Direction,bitangent),dot(-ray.Direction,n));
        if(depth==0)result.primaryViewCosine=saturate(wo.z);
        if(wo.z<=0){path.valid=0;result.fault=5;break;}
        if(depth!=0||includePrimaryEmission!=0)path=PTAccumulate(path,surface.emission);
        PTDirectSample light;uint visibility;
        bool sampleDirect=true;
#ifdef PT_PATH_PRIMARY_INDIRECT
        sampleDirect=depth!=0||useFirst==0;
#endif
        if(sampleDirect&&PTPathLight(surface,-ray.Direction,rng,light,visibility)) {
            if(light.delta!=1||visibility>1){path.valid=0;result.fault=visibility==3?14:6;break;}
            light.wi=float3(dot(light.wi,tangent),dot(light.wi,bitangent),dot(light.wi,n));
            PTDirectResult direct=PTEvaluateDirect(surface.material,wo,light,visibility);
            if(direct.valid==0){path.valid=0;result.fault=7;break;}
            path=PTAccumulate(path,direct.radiance);
            if(depth==0){
                PTSurfaceMaterial diffuseMaterial=surface.material;diffuseMaterial.lobes=surface.material.lobes&1u;
                PTDirectResult diffuseDirect=PTEvaluateDirect(diffuseMaterial,wo,light,visibility);
                primaryDirectDiffuse=diffuseDirect.radiance;
                primaryDirectSpecular=max(direct.radiance-primaryDirectDiffuse,0);
            }
        }
        // A valid material alone is insufficient: failed primary lighting
        // must not publish covered black and suppress native illumination.
        if(depth==0&&path.valid!=0)result.primaryValid=1;
        if(depth+1==maxHits){
#ifdef PT_PATH_TERMINAL_GI
            // Replace the omitted indirect tail with the authored diffuse cache.
            // Earlier vertices retain traced transport; never sum this at each
            // bounce or add the native RT GI/reflection output a second time.
            terminalPosition=surface.position;terminalNormal=surface.normal;
            terminalWeight=path.throughput*surface.material.kd;terminalPending=true;
#endif
            break;
        }
        float choose=PTPathRandom(rng),u=PTPathRandom(rng),v=PTPathRandom(rng),rr=PTPathRandom(rng);
        PTSurfaceMaterial continuation=surface.material;
#ifdef PT_PATH_PRIMARY_FILL
        if(depth==0&&useFirst!=0)continuation=PTPrimaryDiffuseContinuation(continuation);
#endif
        PTSurfaceSample sample=PTSampleSurface(continuation,wo,choose,u,v);
        if(depth==0&&sample.valid!=0){
            PTSurfaceMaterial diffuseMaterial=continuation;diffuseMaterial.lobes=continuation.lobes&1u;
            float3 df=PTEvaluateSurface(diffuseMaterial,wo,sample.wi).f;
            float3 full=PTEvaluateSurface(continuation,wo,sample.wi).f;
            diffuseFraction=float3(full.x>0?df.x/full.x:0,full.y>0?df.y/full.y:0,full.z>0?df.z/full.z:0);
        }
        path=PTContinue(path,sample,rr,rouletteStart);
        if(path.alive==0){if(path.valid==0)result.fault=8;break;}
        ray.Origin=surface.spawnOrigin;
        if(!all(isfinite(ray.Origin))){path.valid=0;result.fault=9;break;}
        ray.Direction=normalize(tangent*sample.wi.x+bitangent*sample.wi.y+n*sample.wi.z);
#ifdef PT_PATH_PRIMARY_SHADING_NORMAL
        if(depth==0&&useFirst!=0&&!PTPrimaryReflectionAboveGeometry(first.geometricNormal,ray.Direction))break;
#endif
        ray.TMin=0;
    }
#ifdef PT_PATH_TERMINAL_GI
    if(terminalPending&&path.valid!=0){
        result.terminalPosition=terminalPosition;result.terminalNormal=terminalNormal;
        result.terminalWeight=terminalWeight;result.terminalFraction=diffuseFraction;result.terminalPending=1;
    }
#endif
    if(path.valid==0&&result.fault==0)result.fault=10;
    result.radiance=path.radiance;result.valid=path.valid;
    float3 indirect=max(path.radiance-primaryDirectDiffuse-primaryDirectSpecular,0);
    result.diffuse=primaryDirectDiffuse+indirect*diffuseFraction;
    result.specular=primaryDirectSpecular+indirect*(1-diffuseFraction);
    return result;
}
PTPathResult PTIntegratePath(RayDesc ray,uint maxHits,uint rouletteStart,uint seed,float originOffset) {
    PTSceneHit unused=(PTSceneHit)0;
    return PTIntegratePathFirst(ray,maxHits,rouletteStart,seed,originOffset,unused,0,1);
}
#endif
