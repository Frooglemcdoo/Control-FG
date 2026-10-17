// Private full-surface path producer for the reviewed primary lighting pass.
// Host supplies an immutable scene/SBT, cloned primary table, same-frame native
// environment at t34 and TLAS at t35. Validated standard-output pixels add
// separate diffuse/specular transport for reviewed raster composition (or the
// legacy additive variant). Never bind to a native 4-byte payload state object.
#define PT_GEOMETRY_SPAWN 1
#define PT_SCENE_MATERIAL_REGISTER t8
#include "pt_scene_hits.hlsl"
#include "pt_material_admission.hlsli"
#include "ControlPrimaryGuideDecode.hlsli"
#include "pt_native_primary_lights.hlsli"
#define PT_PATH_PRIMARY_DECODE 1
#define PT_DIRECT_LIGHT_ROUGHNESS 1
#include "pt_native_light_roughness.hlsli"
#ifndef PT_PRIMARY_SURFACE_REPLACEMENT
#define PT_PATH_PRIMARY_FILL 1
#define PT_PATH_PRIMARY_INDIRECT 1
#endif
#define PT_PATH_PRIMARY_SHADING_NORMAL 1
#define PT_PATH_SURFACE_GEOMETRY 1
#ifdef PT_PRIMARY_SURFACE_REPLACEMENT
// Full transport: no authored diffuse-cache termination or bounded miss GI.
#define PT_PATH_PRIMARY_LIGHT 1
#endif
#include "pt_path_integrator.hlsli"
#include "pt_light_reservoir.hlsli"
#ifdef PT_PRIMARY_SURFACE_REPLACEMENT
#include "pt_surface_commit.hlsli"
#else
#include "pt_primary_commit.hlsli"
#endif
cbuffer PTPrimarySystem : register(b0) { float4 PTSystem[71]; };
Texture2D<float4> PTGBuffer1 : register(t0);
Texture2D<float4> PTGBuffer2 : register(t1);
Texture2D<float4> PTClipDepth : register(t3);
StructuredBuffer<uint2> PTMaterialFlags : register(t7);
RaytracingAccelerationStructure PTScene : register(t35);
RWByteAddressBuffer PTMetrics : register(u6);
void PTMeasure(uint stage,uint2 pixel){if(((pixel.x|pixel.y)&63)==0){uint previous;PTMetrics.InterlockedAdd(stage*4,1,previous);}}
float3 PTViewToWorldVector(float3 v){return v.x*PTSystem[6].xyz+v.y*PTSystem[7].xyz+v.z*PTSystem[8].xyz;}
float3 PTViewToWorldPoint(float3 v){return PTViewToWorldVector(v)+PTSystem[9].xyz;}
float3 PTWorldToViewPoint(float3 v){return v.x*PTSystem[2].xyz+v.y*PTSystem[3].xyz+v.z*PTSystem[4].xyz+PTSystem[5].xyz;}
PTSceneHit PTPathTrace(RayDesc ray){
    PTSceneHit hit=(PTSceneHit)0;TraceRay(PTScene,RAY_FLAG_NONE,1,0,2,0,ray,hit);return hit;
}
PTPathSurface PTDecodeSurface(PTSceneHit hit,RayDesc ray,bool primary){
    PTPathSurface s=(PTPathSurface)0;s.position=hit.worldPosition;s.normal=hit.geometricNormal;
    s.geometricNormal=hit.geometricNormal;
    s.spawnOrigin=safeSpawnPoint(s.position,s.geometricNormal,hit.spawnOffset);
    if(!primary)s.normal=PTSecondaryShadingNormal(hit.shadingNormal,s.geometricNormal,-ray.Direction);
    uint count,stride;PTMaterialFlags.GetDimensions(count,stride);
    if(stride!=8||hit.material>=count){s.decodeFault=11;return s;} // lobes==0 rejects the whole path.
    uint2 flags=PTMaterialFlags[hit.material];uint packed=flags.x;
    PTNativeMaterial m=PTMaterials[hit.material];uint base=hit.material*35;
    // Emission is independent of scattering. Native reflected-hit shading uses
    // these common material fields even when our secondary BRDF is unsupported.
    s.emission=asfloat(m.words[12])*asfloat(uint3(m.words[25],m.words[26],m.words[27]));
    if(any(s.emission!=0))s.emission*=PTTextures[NonUniformResourceIndex(base+5)].SampleLevel(PTLinearWrap,hit.uv,0).rgb;
    if(!all(isfinite(s.emission))||any(s.emission<0)){s.emission=0;s.decodeFault=13;return s;}
    // Only primary surfaces require the specialized raster BRDF contract.
    // Secondary hits use the reviewed common native material approximation.
    if(!PTMaterialScatteringSupported(packed,flags.y,primary)){s.decodeFault=12;return s;}
    float3 color=asfloat(uint3(m.words[13],m.words[14],m.words[15]))*
        PTTextures[NonUniformResourceIndex(base)].SampleLevel(PTLinearWrap,hit.uv,0).rgb;
    float specular=PTTextures[NonUniformResourceIndex(base+4)].SampleLevel(PTLinearWrap,hit.uv,0).x;
    float3 f0=float3((packed>>24)&255,(packed>>16)&255,(packed>>8)&255)*(specular/255.0);
    float smoothness=lerp(asfloat(m.words[17]),asfloat(m.words[18]),
        PTTextures[NonUniformResourceIndex(base+2)].SampleLevel(PTLinearWrap,hit.uv,0).x);
    if(primary){
        uint2 pixel=DispatchRaysIndex().xy;
        float3 shading=ControlNormalViewToWorld(ControlDecodeNormalView(PTGBuffer1.Load(int3(pixel,0))),
            PTSystem[6].xyz,PTSystem[7].xyz,PTSystem[8].xyz);
        if(!PTValidPrimaryShadingNormal(shading,hit.geometricNormal,-ray.Direction)){s.decodeFault=4;return s;}
        s.normal=shading; // Spawn offset above deliberately keeps the geometric normal.
        f0=ControlDecodeMaterialF0(packed,PTGBuffer2.Load(int3(pixel,0)).g);
        smoothness=1-ControlDecodeMaterialRoughness(PTGBuffer1.Load(int3(pixel,0)));
    }
    // New transport material model, not a claim of native BRDF equivalence:
    // diffuse coefficient = texture color * (1-F0), GGX/VNDF specular.
    // Do not clamp invalid colors into an apparently valid path.
    if(!all(isfinite(color))||any(color<0)||any(color>1)||!all(isfinite(f0))||any(f0<0)||any(f0>1)||
       !isfinite(smoothness)||smoothness<0||smoothness>1){s.decodeFault=13;return s;}
    s.material.kd=color*(1-f0);s.material.f0=f0;s.material.roughness=1-smoothness;s.material.lobes=3;return s;
}
PTPathSurface PTPathDecode(PTSceneHit hit,RayDesc ray){return PTDecodeSurface(hit,ray,false);}
PTPathSurface PTPathDecodePrimary(PTSceneHit hit,RayDesc ray){return PTDecodeSurface(hit,ray,true);}
float3 PTPathEnvironment(float3 direction){
    // Reflection-probe cubemaps may contain baked scene lighting. Until a
    // verified physical sky source is imported, a true scene miss is black.
    return 0;
}
bool PTPathLightCandidates(PTPathSurface surface,float3 outgoingWorld,uint primary,inout uint rng,out PTDirectSample light,out uint visibility){
    light=(PTDirectSample)0;visibility=1;
    PTPrimaryLightSample native;float selectedRoughness=surface.material.roughness;
#ifdef PT_PRIMARY_SURFACE_REPLACEMENT
    PTLightReservoir reservoir=PTEmptyLightReservoir();native=(PTPrimaryLightSample)0;native.valid=1;
    float3 positionView=PTWorldToViewPoint(surface.position);
    // Match the integrator's local shading frame for candidate and final BRDF.
    float3 n=surface.normal;
    float3 tangent=normalize(cross(abs(n.z)<0.9?float3(0,0,1):float3(0,1,0),n));
    float3 bitangent=cross(n,tangent);
    float3 wo=float3(dot(outgoingWorld,tangent),dot(outgoingWorld,bitangent),dot(outgoingWorld,n));
    uint3 counts=asuint(PTPrimaryLightWords[16].xyz);
    // Invalid counts still flow through the authenticated sampler's rejection.
    uint total=counts.x<=65535&&counts.y<=65535&&counts.z<=1?counts.x+counts.y+counts.z:0;
    uint candidates=PTLightCandidateCount(total,primary);
    [loop] for(uint candidateIndex=0;candidateIndex<candidates;++candidateIndex){
        float random=PTPathRandom(rng);
        float choose=PTLightCandidateChoose(total,primary,candidateIndex,random);
        PTPrimaryLightSample candidate;PTSamplePrimaryLight(positionView,choose,candidate);
        if(candidate.valid==0){native.valid=0;break;}
        // No lights is a valid zero estimator; do not pass a zero PDF to RIS.
        if(candidate.selectionPdf==0)break;
        float target=0,candidateRoughness=surface.material.roughness;
        if(candidate.active!=0){
            float3 wi=normalize(PTViewToWorldVector(candidate.directionView));
            float3 localWi=float3(dot(wi,tangent),dot(wi,bitangent),dot(wi,n));
            candidateRoughness=PTNativeLightRoughness(surface.material.roughness,candidate.radius,candidate.distance,localWi.z);
            if(candidateRoughness<0){native.valid=0;break;}
            PTSurfaceMaterial directMaterial=surface.material;directMaterial.roughness=candidateRoughness;
            if(PTPrimaryReflectionAboveGeometry(surface.geometricNormal,wi))
                target=PTLightTarget(directMaterial,wo,localWi,candidate.radiance);
        }
        reservoir=PTAddLightCandidate(reservoir,target,candidate.selectionPdf,PTPathRandom(rng),candidateIndex);
        if(reservoir.valid==0){native.valid=0;break;}
        if(reservoir.target>0&&reservoir.selected==candidateIndex){native=candidate;selectedRoughness=candidateRoughness;}
    }
    if(native.valid!=0){native.selectionPdf=PTLightDenominator(reservoir);native.active=native.selectionPdf>0?1u:0u;}
#else
    PTSamplePrimaryLight(PTWorldToViewPoint(surface.position),PTPathRandom(rng),native);
#endif
    // Invalid native data must invalidate the path, not become a dark sample.
    if(native.valid==0){light.pdf=1;light.delta=1;visibility=3;return true;}
    if(native.active==0)return false;
    light.wi=normalize(PTViewToWorldVector(native.directionView));light.incidentRadiance=native.radiance;
    light.pdf=native.selectionPdf;light.delta=1;
#ifndef PT_PRIMARY_SURFACE_REPLACEMENT
    selectedRoughness=PTNativeLightRoughness(surface.material.roughness,native.radius,native.distance,dot(surface.normal,light.wi));
#endif
    light.roughness=selectedRoughness;
    if(dot(surface.normal,light.wi)<=0||!PTPrimaryReflectionAboveGeometry(surface.geometricNormal,light.wi)){visibility=0;return true;}
    RayDesc shadow;shadow.Origin=surface.spawnOrigin;shadow.Direction=light.wi;shadow.TMin=0;shadow.TMax=1e6;
    if(native.directional==0){
        float3 delta=PTViewToWorldPoint(native.targetView)-shadow.Origin;float distance=length(delta);
        if(!isfinite(distance)||distance<=0){visibility=2;return true;}
        shadow.Direction=delta/distance;
        // Exclude the endpoint by one representable float, not a scene-scale
        // bias that could skip thin occluders near a light.
        shadow.TMax=asfloat(asuint(distance)-1);
    }
    PTVisibility result;result.status=0;
    TraceRay(PTScene,RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,1,1,2,1,shadow,result);
    visibility=result.status;return true;
}
bool PTPathLight(PTPathSurface surface,float3 outgoingWorld,inout uint rng,out PTDirectSample light,out uint visibility){
    return PTPathLightCandidates(surface,outgoingWorld,0,rng,light,visibility);
}
#ifdef PT_PATH_PRIMARY_LIGHT
bool PTPathPrimaryLight(PTPathSurface surface,float3 outgoingWorld,inout uint rng,out PTDirectSample light,out uint visibility){
    return PTPathLightCandidates(surface,outgoingWorld,1,rng,light,visibility);
}
#endif
[shader("raygeneration")]
void PTPrimaryPathRayGeneration(){
    uint2 pixel=DispatchRaysIndex().xy;
    // A four-byte copy precedes the native signal releasing this dispatch.
    // Rejected/replayed lists must never traverse their old acceleration data.
    uint admission=PTMetrics.Load(32);
    [branch] if(admission==0||(admission==2&&((pixel.x|pixel.y)&15)!=0))return;
#ifdef PT_PRIMARY_SURFACE_REPLACEMENT
    // Clear coverage before any depth/material rejection. Admission 0 must not
    // touch resources: replay may refer to an output whose scene was retired.
    // Host initializes the whole output to zero before its first submission.
    if(admission==1&&!PTClearSurface(pixel,DispatchRaysDimensions().xy))return;
#endif
    PTMeasure(0,pixel);
    uint width,height,g2w,g2h,zw,zh;PTGBuffer1.GetDimensions(width,height);PTGBuffer2.GetDimensions(g2w,g2h);PTClipDepth.GetDimensions(zw,zh);
    if(any(uint2(width,height)!=DispatchRaysDimensions().xy)||width!=g2w||height!=g2h||width!=zw||height!=zh||
       !all(isfinite(PTSystem[0].zw))||any(PTSystem[0].zw<=0)||
       any(abs(PTSystem[0].zw*float2(width,height)-1)>1e-6))return;
    PTMeasure(1,pixel);
    float depth=PTClipDepth.Load(int3(pixel,0)).x;
    if(!isfinite(depth)||depth<0||depth>1)return;
    float2 screen=(float2(pixel)+.5)*PTSystem[0].zw;
    float4 view=(screen.x*2-1)*PTSystem[14]+(1-screen.y*2)*PTSystem[15]+depth*PTSystem[16]+PTSystem[17];
    if(!all(isfinite(view))||view.w==0)return;
    float3 primaryPosition=PTViewToWorldPoint(view.xyz/view.w),origin=PTSystem[9].xyz;
    float distance=length(primaryPosition-origin);
    if(!all(isfinite(primaryPosition))||!isfinite(distance)||distance<=0)return;
    RayDesc ray;ray.Origin=origin;ray.Direction=(primaryPosition-origin)/distance;ray.TMin=0;ray.TMax=1e6;
    PTSceneHit first=PTPathTrace(ray);
    // Raster visibility may include unsupported geometry. Require its exact
    // material and a close depth before selecting any PT radiance for that pixel.
    uint material=ControlDecodeMaterialID(PTGBuffer2.Load(int3(pixel,0)));
    if(first.status!=1)return;PTMeasure(2,pixel);
    bool sameMaterial=first.material==material;
    bool sameDepth=abs(first.distance-distance)<=max(.002,distance*2e-4);
    // Independent categories: distinguish wrong geometry from slot disagreement.
    if(((pixel.x|pixel.y)&63)==0&&!sameMaterial){uint old;PTMetrics.InterlockedAdd(sameDepth?148:152,1,old);}
    if(((pixel.x|pixel.y)&63)==0&&sameMaterial&&!sameDepth){uint old;PTMetrics.InterlockedAdd(156,1,old);}
    if(!sameMaterial)return;PTMeasure(3,pixel);
    if(!sameDepth)return;PTMeasure(4,pixel);
    uint seed=(pixel.x+pixel.y*width)*747796405u+asuint(PTSystem[69].z)*2891336453u+277803737u;
    seed=(seed^(seed>>16))*2246822519u;seed^=seed>>13;
    // Keep the first two continuations before roulette to reduce reflection variance.
    PTPathResult result=PTIntegratePathFirst(ray,4,3,seed,.001,first,1,0);
    PTMeasure(5,pixel);
    if(result.terminalGIPositive!=0&&((pixel.x|pixel.y)&63)==0){uint old;PTMetrics.InterlockedAdd(96,1,old);}
    // The legacy variant adds diffuse-first indirect. The replacement variant
    // includes primary direct and both continuation lobes, and writes complete
    // material-weighted surface radiance. Primary emission stays in the native
    // material compositor in BOTH variants; secondary emission is integrated.
#ifdef PT_PRIMARY_SURFACE_REPLACEMENT
    // Primary ownership is decided before sampling continuations. An unsupported
    // secondary BRDF terminates this estimate at its last finite contribution;
    // it must not select a different primary lighting model on this frame.
    if(result.primaryValid!=0&&all(isfinite(result.radiance))&&all(result.radiance>=0)){
#else
    if(result.valid!=0&&all(isfinite(result.radiance))&&all(result.radiance>=0)){
#endif
#ifdef PT_PRIMARY_SURFACE_REPLACEMENT
        if(admission==2)PTMeasure(6,pixel);
        else if(PTCommitSignals(pixel,uint2(width,height),result,admission)){
            PTMeasure(6,pixel);
        }
        if(result.valid==0&&((pixel.x|pixel.y)&63)==0){uint old;PTMetrics.InterlockedAdd(36,1,old);}
#else
        if(admission==2||PTCommitIndirect(pixel,uint2(width,height),result.radiance,admission))PTMeasure(6,pixel);
#endif
    }
    else {PTMeasure(7,pixel);if(((pixel.x|pixel.y)&63)==0){
        uint old;PTMetrics.InterlockedAdd(36+4*min(result.fault,15u),1,old);
        // One representative rejection per screen tile; bounded sparse writes.
        uint tile=min(pixel.x*8/width,7u)+8*min(pixel.y*4/height,3u),at=160+tile*32;
        uint bytes;PTMetrics.GetDimensions(bytes);
        if(bytes>=1184){
            PTMetrics.InterlockedAdd(at,1,old);
            if(old==0){uint count,stride;PTMaterialFlags.GetDimensions(count,stride);
                uint2 flags=uint2(0xffffffffu,0xffffffffu);if(stride==8&&first.material<count)flags=PTMaterialFlags[first.material];
                PTMetrics.Store4(at+4,uint4(pixel,first.material,flags.x));
                PTMetrics.Store3(at+20,uint3(flags.y,result.fault,0));}
        }
    }}
}
