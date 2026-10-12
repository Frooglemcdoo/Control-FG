#ifndef CONTROL_PT_PATH_INTEGRATOR
#define CONTROL_PT_PATH_INTEGRATOR
#include "pt_direct_light.hlsli"
// Adapter contract: geometric and shading normals must coincide for this initial
// opaque/diffuse/GGX integrator. Transmission and shading-normal correction are
// not admitted. Emitters/environment are BSDF-only; NEE accepts DELTA lights only,
// preventing duplicate non-delta light energy until emitter MIS is implemented.
struct PTPathSurface {
    float3 position, normal, emission;
    PTSurfaceMaterial material;
};
// Adapter supplies these functions before including this file's implementation.
PTSceneHit PTPathTrace(RayDesc ray);
PTPathSurface PTPathDecode(PTSceneHit hit, RayDesc ray);
float3 PTPathEnvironment(float3 direction);
bool PTPathLight(PTPathSurface surface, out PTDirectSample light, out uint visibility);

uint PTPathRandomBits(inout uint state) {
    state ^= state << 13; state ^= state >> 17; state ^= state << 5; return state;
}
float PTPathRandom(inout uint state) { return (PTPathRandomBits(state)>>8)*(1.0/16777216.0); }
struct PTPathResult { float3 radiance; uint hits; uint valid; };
PTPathResult PTIntegratePath(RayDesc ray,uint maxHits,uint rouletteStart,uint seed,float originOffset) {
    PTTransportState path=PTBeginTransport();PTPathResult result=(PTPathResult)0;
    if(maxHits==0||maxHits>32||!all(isfinite(ray.Origin))||!all(isfinite(ray.Direction))||
       abs(dot(ray.Direction,ray.Direction)-1)>1e-4||!isfinite(ray.TMin)||!isfinite(ray.TMax)||
       ray.TMin<0||ray.TMax<=ray.TMin||!isfinite(originOffset)||originOffset<=0) return result;
    uint rng=seed==0?1:seed;
    for(uint depth=0;depth<maxHits&&path.alive!=0;++depth) {
        PTSceneHit hit=PTPathTrace(ray);
        if(hit.status==2){path.valid=0;break;}
        if(hit.status==0){path=PTAccumulate(path,PTPathEnvironment(ray.Direction));break;}
        PTPathSurface surface=PTPathDecode(hit,ray);result.hits++;
        PTSurfaceMaterial m=surface.material;
        if(!PTValid(m.kd)||!PTValid(m.f0)||!PTFinite(m.roughness)||m.roughness<0||m.roughness>1||
           any(m.kd<0)||any(m.kd>1)||any(m.f0<0)||any(m.f0>1)||(m.lobes&~3u)!=0||m.lobes==0){path.valid=0;break;}
        float3 n=surface.normal;
        if(!all(isfinite(surface.position))||!all(isfinite(n))||abs(dot(n,n)-1)>1e-4){path.valid=0;break;}
        float3 tangent=normalize(cross(abs(n.z)<0.9?float3(0,0,1):float3(0,1,0),n));
        float3 bitangent=cross(n,tangent);
        float3 wo=float3(dot(-ray.Direction,tangent),dot(-ray.Direction,bitangent),dot(-ray.Direction,n));
        if(wo.z<=0){path.valid=0;break;}
        path=PTAccumulate(path,surface.emission);
        PTDirectSample light;uint visibility;
        if(PTPathLight(surface,light,visibility)) {
            if(light.delta!=1){path.valid=0;break;}
            light.wi=float3(dot(light.wi,tangent),dot(light.wi,bitangent),dot(light.wi,n));
            PTDirectResult direct=PTEvaluateDirect(surface.material,wo,light,visibility);
            if(direct.valid==0){path.valid=0;break;}
            path=PTAccumulate(path,direct.radiance);
        }
        if(depth+1==maxHits) break;
        float choose=PTPathRandom(rng),u=PTPathRandom(rng),v=PTPathRandom(rng),rr=PTPathRandom(rng);
        PTSurfaceSample sample=PTSampleSurface(surface.material,wo,choose,u,v);
        path=PTContinue(path,sample,rr,rouletteStart);
        if(path.alive==0) break;
        ray.Origin=surface.position+n*originOffset;
        ray.Direction=normalize(tangent*sample.wi.x+bitangent*sample.wi.y+n*sample.wi.z);
        ray.TMin=0;
    }
    result.radiance=path.radiance;result.valid=path.valid;return result;
}
#endif
