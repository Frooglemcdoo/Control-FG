// Candidate variant-zero full-resolution raygen. NOT hooked into gameplay.
// Host must admit ten rays, batch zero, one pass and the shared-hit variant from
// gi_single_producer.py in the SAME state object. Native global root is preserved.
#include "gi_frozen_receiver.hlsl"
#include "gi_auto_sample.hlsl"
Texture2D<float4> gbuffer : register(t0);
Texture2D<float4> linearDepth : register(t1);
Texture2D<float4> clipDepth : register(t2);
RaytracingAccelerationStructure scene : register(t4);
RWTexture2DArray<uint> materials : register(u0);
cbuffer System : register(b0) {uint4 system[71];}
cbuffer NativeGI : register(b1) {uint4 controls;uint passCount;}
StructuredBuffer<GIAutoTargets> targets : register(t0,space19);
cbuffer Selection : register(b0,space19) {GIAutoSelection selection;}
struct Payload {uint slice;};
[shader("raygeneration")]
void giAutoRaygen() {
 uint2 pixel=DispatchRaysIndex().xy;
 // These are admission requirements, not a safe post-dispatch recovery path.
 if(controls.z!=10||controls.w!=0||passCount!=1||selection.rayCount!=10)return;
 GIFrozenReceiver receiver=GIFrozenDecodeReceiver(pixel,gbuffer.Load(int3(pixel,0)),
  linearDepth.Load(int3(pixel,0)).x,clipDepth.Load(int3(pixel,0)).x,system);
 if(!receiver.valid){[unroll]for(uint i=0;i<10;++i)materials[uint3(pixel,i)]=65534;return;}
 GIAutoTargets frozen=targets[0];
 [loop]for(uint rayIndex=0;rayIndex<10;++rayIndex) {
  GIAutoDraw draw=GIAutoSample(pixel,system[69].z,0,rayIndex,receiver.origin,receiver.normal,frozen,selection);
  RayDesc ray;ray.Origin=receiver.origin;ray.Direction=draw.direction;ray.TMin=0;
  ray.TMax=asfloat(controls.x)*(.8f+.4f*draw.range);
  Payload payload;payload.slice=0x80000000u|rayIndex;
  // Mask zero executes the native miss shader. Evaluator gives this proposal
  // zero complete RGB weight, while preserving the native distance convention.
  TraceRay(scene,RAY_FLAG_NONE,draw.traceRay?1:0,0,2,0,ray,payload);
 }
}
