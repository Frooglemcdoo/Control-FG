// Private full-surface path producer for the reviewed primary lighting pass.
// Host supplies an immutable scene/SBT, cloned primary table, same-frame native
// environment at t34, TLAS at t35 and private RGBA32F output at u7. Never bind to
// a native 4-byte payload state object or native lighting UAV.
#define PT_GEOMETRY_SPAWN 1
#define PT_SCENE_MATERIAL_REGISTER t8
#include "pt_scene_hits.hlsl"
#include "ControlPrimaryGuideDecode.hlsli"
#include "pt_native_primary_lights.hlsli"
#include "pt_path_integrator.hlsli"
cbuffer PTPrimarySystem : register(b0) { float4 PTSystem[71]; };
Texture2D<float4> PTGBuffer1 : register(t0);
Texture2D<float4> PTGBuffer2 : register(t1);
Texture2D<float4> PTClipDepth : register(t3);
StructuredBuffer<uint2> PTMaterialFlags : register(t7);
TextureCube<float4> PTEnvironment : register(t34);
SamplerState PTEnvironmentSampler : register(s10,space1);
RaytracingAccelerationStructure PTScene : register(t35);
RWTexture2D<float4> PTOutput : register(u7);
RWByteAddressBuffer PTMetrics : register(u6);
void PTMeasure(uint stage,uint2 pixel){if(((pixel.x|pixel.y)&15)==0){uint previous;PTMetrics.InterlockedAdd(stage*4,1,previous);}}
float3 PTViewToWorldVector(float3 v){return v.x*PTSystem[6].xyz+v.y*PTSystem[7].xyz+v.z*PTSystem[8].xyz;}
float3 PTViewToWorldPoint(float3 v){return PTViewToWorldVector(v)+PTSystem[9].xyz;}
float3 PTWorldToViewPoint(float3 v){return v.x*PTSystem[2].xyz+v.y*PTSystem[3].xyz+v.z*PTSystem[4].xyz+PTSystem[5].xyz;}
PTSceneHit PTPathTrace(RayDesc ray){
    PTSceneHit hit=(PTSceneHit)0;TraceRay(PTScene,RAY_FLAG_NONE,1,0,2,0,ray,hit);return hit;
}
PTPathSurface PTPathDecode(PTSceneHit hit,RayDesc ray){
    PTPathSurface s=(PTPathSurface)0;s.position=hit.worldPosition;s.normal=hit.geometricNormal;
    s.spawnOrigin=safeSpawnPoint(s.position,s.normal,hit.spawnOffset);
    uint count,stride;PTMaterialFlags.GetDimensions(count,stride);
    if(stride!=8||hit.material>=count)return s; // lobes==0 rejects the whole path.
    uint packed=PTMaterialFlags[hit.material].x;
    if((packed&255)!=0)return s; // Cloth/hair/special BRDFs stay native.
    PTNativeMaterial m=PTMaterials[hit.material];uint base=hit.material*35;
    float3 color=asfloat(uint3(m.words[13],m.words[14],m.words[15]))*
        PTTextures[NonUniformResourceIndex(base)].SampleLevel(PTLinearWrap,hit.uv,0).rgb;
    float specular=PTTextures[NonUniformResourceIndex(base+4)].SampleLevel(PTLinearWrap,hit.uv,0).x;
    float3 f0=float3((packed>>24)&255,(packed>>16)&255,(packed>>8)&255)*(specular/255.0);
    float smoothness=lerp(asfloat(m.words[17]),asfloat(m.words[18]),
        PTTextures[NonUniformResourceIndex(base+2)].SampleLevel(PTLinearWrap,hit.uv,0).x);
    s.emission=asfloat(m.words[12])*asfloat(uint3(m.words[25],m.words[26],m.words[27]))*
        PTTextures[NonUniformResourceIndex(base+5)].SampleLevel(PTLinearWrap,hit.uv,0).rgb;
    // New transport material model, not a claim of native BRDF equivalence:
    // diffuse coefficient = texture color * (1-F0), GGX/VNDF specular.
    // Do not clamp invalid colors into an apparently valid path.
    if(!all(isfinite(color))||any(color<0)||any(color>1)||!all(isfinite(f0))||any(f0<0)||any(f0>1)||
       !isfinite(smoothness)||smoothness<0||smoothness>1)return s;
    s.material.kd=color*(1-f0);s.material.f0=f0;s.material.roughness=1-smoothness;s.material.lobes=3;return s;
}
float3 PTPathEnvironment(float3 direction){
    // Physical miss uses the sharp cube; no roughness-dependent convolution
    // inside path sampling. Native specular environment scale is b2 byte244.
    return PTEnvironment.SampleLevel(PTEnvironmentSampler,direction,0).rgb*PTPrimaryLightWords[15].y;
}
bool PTPathLight(PTPathSurface surface,inout uint rng,out PTDirectSample light,out uint visibility){
    light=(PTDirectSample)0;visibility=1;
    PTPrimaryLightSample native;PTSamplePrimaryLight(PTWorldToViewPoint(surface.position),PTPathRandom(rng),native);
    // Invalid native data must invalidate the path, not become a dark sample.
    if(native.valid==0){light.pdf=1;light.delta=1;visibility=2;return true;}
    if(native.active==0)return false;
    light.wi=normalize(PTViewToWorldVector(native.directionView));light.incidentRadiance=native.radiance;
    light.pdf=native.selectionPdf;light.delta=1;
    if(dot(surface.normal,light.wi)<=0)return true;
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
[shader("raygeneration")]
void PTPrimaryPathRayGeneration(){
    uint2 pixel=DispatchRaysIndex().xy;
    // Selected only by a queue-ordered clear after exact scene/frame admission.
    // Rejected/replayed lists must never traverse their old acceleration data.
    [branch] if(PTOutput[pixel].a!=1){PTOutput[pixel]=0;return;}
    PTOutput[pixel]=0;PTMeasure(0,pixel);
    uint width,height,g2w,g2h,zw,zh;PTGBuffer1.GetDimensions(width,height);PTGBuffer2.GetDimensions(g2w,g2h);PTClipDepth.GetDimensions(zw,zh);
    if(any(uint2(width,height)!=DispatchRaysDimensions().xy)||width!=g2w||height!=g2h||width!=zw||height!=zh||
       !all(isfinite(PTSystem[0]))||any(abs(PTSystem[0].xy-float2(width,height))>.01))return;
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
    if(first.material!=material)return;PTMeasure(3,pixel);
    if(abs(first.distance-distance)>max(.002,distance*2e-4))return;PTMeasure(4,pixel);
    uint seed=(pixel.x+pixel.y*width)*747796405u+asuint(PTSystem[69].y)*2891336453u+277803737u;
    seed=(seed^(seed>>16))*2246822519u;seed^=seed>>13;
    PTPathResult result=PTIntegratePathFirst(ray,4,3,seed,.001,first,1,0);
    PTMeasure(5,pixel);
    if(result.valid!=0&&all(isfinite(result.radiance))&&all(result.radiance>=0)){PTOutput[pixel]=float4(result.radiance,1);PTMeasure(6,pixel);}
    else PTMeasure(7,pixel);
}
