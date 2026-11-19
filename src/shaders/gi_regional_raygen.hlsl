// Paired variant-zero full-resolution raygen, installed by the owned GI hook.
// Host must admit four or ten rays, batch zero, one pass and the shared-hit variant from
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
#include "gi_regional_lookup.hlsl"
cbuffer Selection : register(b0,space19) {GIAutoSelection selection;}
#include "gi_producer_packet.hlsl"
#include "gi_motion_layout.hlsl"
struct Payload {uint slice;};
[shader("raygeneration")]
void giAutoRaygen() {
 uint2 pixel=DispatchRaysIndex().xy;
 // These are admission requirements, not a safe post-dispatch recovery path.
 if((selection.rayCount!=10&&selection.rayCount!=4)||controls.z!=selection.rayCount||controls.w!=0||passCount!=1)return;
 GIFrozenReceiver receiver=GIFrozenDecodeReceiver(pixel,gbuffer.Load(int3(pixel,0)),
  linearDepth.Load(int3(pixel,0)).x,clipDepth.Load(int3(pixel,0)).x,system);
 bool motion=GIMotionEnabled(packetStepPacked,packetCapacityPacked);
 if(motion){
  if(all(pixel==0)){uint base=GIMotionBase(packetWidth,packetHeight,packetCapacityPacked,selection.rayCount);
   [loop]for(uint c=0;c<71;++c)producerPackets.Store4(base+c*16,system[c]);
   producerPackets.Store4(base+1136,uint4(0x4d343834u,selection.sampleFrame,packetWidth,packetHeight));
   producerPackets.Store4(base+1152,uint4(selection.rayCount,224u,2048u,1u));
  }
  [loop]for(uint r=0;r<selection.rayCount;++r){
   uint at=GIMotionAddress(pixel,r,packetWidth,packetHeight,packetCapacityPacked,selection.rayCount);
   producerPackets.Store4(at,uint4(asuint(receiver.origin),receiver.valid?selection.sampleFrame:0));
   producerPackets.Store4(at+16,0);
  }
 }
 uint packet=GIPacketAddress(pixel);
 if(packet!=0xffffffff)producerPackets.Store4(packet,uint4(asuint(receiver.origin),receiver.valid?selection.sampleFrame:0));
 if(!receiver.valid){[loop]for(uint i=0;i<selection.rayCount;++i){materials[uint3(pixel,i)]=65534;GIStoreWeight(pixel,i,0);}return;}
 GIAutoTargets frozen=GISelectRegional(receiver.origin,receiver.normal,selection);GIAutoPrepared prepared=GIAutoPrepare(receiver.origin,frozen,selection);
 [loop]for(uint rayIndex=0;rayIndex<selection.rayCount;++rayIndex) {
  GIAutoDraw draw=GIAutoSamplePrepared(pixel,system[69].z,0,rayIndex,receiver.normal,prepared,selection);
  GIStoreWeight(pixel,rayIndex,draw.weight);
  if(motion)producerPackets.Store4(GIMotionAddress(pixel,rayIndex,packetWidth,packetHeight,packetCapacityPacked,selection.rayCount)+16,asuint(float4(draw.direction,draw.weight)));
  if(packet!=0xffffffff)producerPackets.Store4(packet+16*(rayIndex+1),asuint(float4(draw.direction,draw.weight)));
  RayDesc ray;ray.Origin=receiver.origin;ray.Direction=draw.direction;ray.TMin=0;
  ray.TMax=asfloat(controls.x)*(.8f+.4f*draw.range);
  Payload payload;payload.slice=0x80000000u|rayIndex;
  // Mask zero executes the native miss shader. Evaluator gives this proposal
  // zero complete RGB weight, while preserving the native distance convention.
  TraceRay(scene,RAY_FLAG_NONE,draw.traceRay?1:0,0,2,0,ray,payload);
 }
}
