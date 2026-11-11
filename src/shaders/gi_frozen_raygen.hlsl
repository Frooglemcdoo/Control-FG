// Audited variant-zero, ONE ray per receiver. Host admission must establish
// that ABI before replacing native work. Uses the ORIGINAL hit/miss records.
#include "gi_frozen_receiver.hlsl"
Texture2D<float4> gbuffer : register(t0);
Texture2D<float4> linearDepth : register(t1);
Texture2D<float4> clipDepth : register(t2);
RaytracingAccelerationStructure scene : register(t4);
RWTexture2DArray<uint> materials : register(u0);
RWTexture2DArray<float4> positions : register(u2);
cbuffer System : register(b0) {uint4 system[71];}
cbuffer NativeGI : register(b1) {uint4 controls;uint passCount;}
// Raygen-local root. No change to native hit/miss local root arguments.
StructuredBuffer<GIKeyedGuide> guides : register(t0,space18);
RWStructuredBuffer<float> weights : register(u0,space18);
cbuffer Limits : register(b0,space18){
 uint width,height,frame,epoch;
 uint guideSlots,parity,useGuide;float guideFraction;
}
struct Payload {uint slice;};
[shader("raygeneration")]
void giFrozenRaygen(){
 uint2 compact=DispatchRaysIndex().xy;
 uint2 pixel=uint2(compact.x*2+parity,compact.y);
 if(any(DispatchRaysIndex().zz)||parity>1||width<2||(width&1)||pixel.x>=width||pixel.y>=height)return;
 uint index=compact.y*(width/2)+compact.x;
 weights[index]=0;
 if(controls.z!=1||controls.w!=0||passCount!=1||!frame||!epoch){
  materials[uint3(pixel,0)]=65534;return;
 }
 GIFrozenReceiver receiver=GIFrozenDecodeReceiver(pixel,gbuffer.Load(int3(pixel,0)),
     linearDepth.Load(int3(pixel,0)).x,clipDepth.Load(int3(pixel,0)).x,system);
 if(!receiver.valid){materials[uint3(pixel,0)]=65534;return;}
 // The native sampling counter and host invocation identity are separate.
 GIProducerCoordinates random=GIProducerSample(pixel.x,pixel.y,system[69].z,0,0,1);
 GIPoint normal=GIFrozenPoint(receiver.normal);
 GIWorldGuideKey key=GIMakeWorldGuideKey(GIFrozenPoint(receiver.world),normal);
 GIKeyedGuide guide=(GIKeyedGuide)0;
 if(useGuide&&guideSlots)guide=guides[GITrainingHash(key)%guideSlots];
 GIFrozenRay draw=GISampleKeyedFrozenWorld(guide.guide,guide.key,key,normal,frame,epoch,
     useGuide?guideFraction:0,random.u,random.v);
 // A malformed guide follows the core's pre-draw native fallback. No retry or
 // post-hit rejection. The range draw remains independent and unchanged.
 RayDesc ray;ray.Origin=receiver.origin;ray.Direction=GIFrozenVector(draw.direction);
 ray.TMin=0;ray.TMax=asfloat(controls.x)*(.8f+.4f*random.range);
 if(!draw.valid||!isfinite(ray.TMax)||ray.TMax<=0){materials[uint3(pixel,0)]=65534;return;}
 weights[index]=draw.weight;
 if(draw.trace){
  Payload payload;payload.slice=0xa0000000u|(parity<<28);
  TraceRay(scene,RAY_FLAG_NONE,1,0,2,0,ray,payload);
 }else{
  // Zero-contribution below-hemisphere proposals stay in the denominator.
  // Populate native miss metadata; GI6 applies weight zero to ALL radiance.
  float3 endpoint=ray.Origin+ray.Direction*ray.TMax;
  materials[uint3(pixel,0)]=65535;
  positions[uint3(pixel,0)]=float4(GIFrozenTransform(float4(endpoint,1),2,system).xyz,0);
 }
}
