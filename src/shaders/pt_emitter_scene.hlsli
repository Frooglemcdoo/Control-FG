#ifndef CONTROL_PT_EMITTER_SCENE
#define CONTROL_PT_EMITTER_SCENE
// Immutable per-dispatch registry. Invalid or incomplete extraction disables
// BOTH techniques' MIS weights, leaving ordinary BSDF-hit emission unbiased.
ByteAddressBuffer PTEmitters:register(t34);
uint PTEmitterCount(){
    uint bytes;PTEmitters.GetDimensions(bytes);if(bytes<16)return 0;
    uint3 header=PTEmitters.Load3(0);
    return header.y==0&&header.z==0&&header.x<=(bytes-16)/80?header.x:0;
}
float PTPathEmitterPdf(float3 previousPosition,PTSceneHit hit){
    uint count=PTEmitterCount();if(count==0||hit.emitterArea<=5e-11)return 0;
    float3 delta=hit.worldPosition-previousPosition;float d2=dot(delta,delta);
    if(!isfinite(d2)||d2<=0)return 0;
    return PTEmitterSolidAnglePdf(hit.emitterArea,count,d2,saturate(abs(dot(hit.geometricNormal,delta*rsqrt(d2)))));
}
bool PTPathEmitterLight(PTPathSurface surface,inout uint rng,out PTDirectSample light,out uint visibility){
    light=(PTDirectSample)0;visibility=1;uint count=PTEmitterCount();if(count==0)return false;
    uint index=min(uint(PTPathRandom(rng)*count),count-1),at=16+80*index;
    float4 a=asfloat(PTEmitters.Load4(at)),b=asfloat(PTEmitters.Load4(at+16)),c=asfloat(PTEmitters.Load4(at+32));
    uint4 meta=PTEmitters.Load4(at+48);uint alpha=PTEmitters.Load(at+64);
    float u=PTPathRandom(rng),v=PTPathRandom(rng);
    PTEmitterPoint point=PTSampleEmitterTriangle(a.xyz,b.xyz,c.xyz,count,u,v);
    if(point.valid==0)return false;
    float3 delta=point.position-surface.position;float d2=dot(delta,delta);if(!isfinite(d2)||d2<=0)return false;
    light.wi=delta*rsqrt(d2);light.pdf=PTEmitterSolidAnglePdf(point.area,count,d2,saturate(abs(dot(point.normal,-light.wi))));
    if(light.pdf<=0)return false;
    uint material=meta.w,materials,stride;PTMaterials.GetDimensions(materials,stride);
    if(stride!=336||material>=materials||material>=244000/35){visibility=2;return true;}
    float root=sqrt(u);float3 bary=float3(1-root,root*(1-v),root*v);
    float2 uv=float2(dot(float3(a.w,b.w,c.w),bary),dot(asfloat(meta.xyz),bary));
    PTNativeMaterial m=PTMaterials[material];uint base=material*35;
    if(alpha!=0&&PTTextures[NonUniformResourceIndex(base)].SampleLevel(PTLinearWrap,uv,0).a*asfloat(m.words[16])<.5)return false;
    light.incidentRadiance=asfloat(m.words[12])*asfloat(uint3(m.words[25],m.words[26],m.words[27]))*
        PTTextures[NonUniformResourceIndex(base+5)].SampleLevel(PTLinearWrap,uv,0).rgb;
    light.delta=0;light.roughness=surface.material.roughness;
    if(!all(isfinite(light.incidentRadiance))||any(light.incidentRadiance<0)){visibility=2;return true;}
    if(!any(light.incidentRadiance>0)||dot(surface.normal,light.wi)<=0||!PTPrimaryReflectionAboveGeometry(surface.geometricNormal,light.wi)){
        visibility=0;return true;
    }
    RayDesc shadow;shadow.Origin=surface.spawnOrigin;
    delta=point.position-shadow.Origin;float distance=length(delta);
    if(!isfinite(distance)||distance<=0){visibility=2;return true;}
    shadow.Direction=delta/distance;shadow.TMin=0;shadow.TMax=asfloat(asuint(distance)-1);
    PTVisibility result;result.status=0;
    TraceRay(PTScene,RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,1,1,2,1,shadow,result);
    visibility=result.status;return true;
}
#endif
