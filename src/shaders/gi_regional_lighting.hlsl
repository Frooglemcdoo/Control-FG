#define CONTROL_GI_REGIONAL 1
// R449 paired evaluator: stratified mixture with balance weights on every ray.
#define GI_SECONDARY_TERMS
float GIMaterialTextureLod();
#define GI_SECONDARY_LIBRARY_ONLY
#include "gi_evaluator_secondary.generated.hlsl"
#include "gi_frozen_receiver.hlsl"
#include "gi_auto_sample.hlsl"
#include "gi_auto_training.hlsl"
#include "gi_auto_pixel_audit.hlsl"
#include "gi_regional_lookup.hlsl"
#include "gi_regional_training_write.hlsl"
cbuffer Selection : register(b0,space19) {GIAutoSelection selection;}
#include "gi_red_trace.hlsl"
#include "gi_weight_transport.hlsl"
float GIMaterialTextureLod(){return (trainingStepPacked&16777216u)?1.0f:0.0f;}
#include "gi_weight_read.hlsl"
#include "gi_motion_evaluator.hlsl"
#ifdef GI_RAY_CLAMP
// R495 ray clamp: current-frame, per-ray impulse limit. A rare ray that lands
// on a small, brightly lit secondary patch can carry ~99% of a receiver's GI
// (PID 716 frame 2841). Each ray's contribution is limited to GI_RAY_CLAMP
// times the robust 5x5 neighbourhood GI level (largest neighbour dropped, self
// excluded), using the brightest RGB channel so red, green and white impulses
// are all caught. Rays under the limit are untouched and hue is preserved.
// No history, no motion vectors: this cannot ghost or smear.
#ifndef GI_RAY_CLAMP_EPS
#define GI_RAY_CLAMP_EPS 0.0001f
#endif
static float3 gRayC[10];
static float gRayScale=0;
static float gRayLevel=-1;
static uint gRayCount=0;
static bool gRayPending=false;
float GIRayMaxChannel(float3 c){return max(c.r,max(c.g,c.b));}
#endif
void EvaluatePixel(uint3 id, bool train, out GIAutoTraining bestTraining, out GIWorldGuideKey key) {
 bestTraining=(GIAutoTraining)0;
 key=(GIWorldGuideKey)0;
 if(any(id.xy>=uint2(trainingWidth,trainingHeight)))return;
 bool motion=GIMotionEnabled(trainingStepPacked,trainingCapacityPacked);
 GIMotionClear(id.xy);
 uint auditIndex=GIAutoAuditIndex(id.xy);
 GIAutoTraining redA=(GIAutoTraining)0,redB=(GIAutoTraining)0;float redScore=0;
 GISecondaryTerms redTerms=(GISecondaryTerms)0;
 GIRedWrite(auditIndex,redA,redB,0,selection.rayCount);
 GIRedTermsWrite(auditIndex,redTerms,selection.rayCount);
 GIAutoAuditWrite(auditIndex,0,0,0,0,0,0,65534,0,0,0);
 GIShadingRegisters receiver;
 if(giCB4[0].z!=selection.rayCount||(selection.rayCount!=10&&selection.rayCount!=4)||!GIInitializeReceiver(id.xy,receiver)) {
  g_rwtDiffuseGIColorTarget[id.xy]=0;g_rwtHitInfo[id.xy]=65504;return;
 }
 GIFrozenReceiver source=GIFrozenDecodeReceiver(id.xy,g_tGBuffer1.Load(int3(id.xy,0)),
  g_tLinearDepth.Load(int3(id.xy,0)).x,g_tClipDepth.Load(int3(id.xy,0)).x,giCB0);
 if(!source.valid){g_rwtDiffuseGIColorTarget[id.xy]=0;g_rwtHitInfo[id.xy]=65504;return;}
 GIAutoTargets frozen=(GIAutoTargets)0;GIAutoPrepared prepared=(GIAutoPrepared)0;
 if(auditIndex!=0xffffffff){frozen=GISelectRegional(source.origin,source.normal,selection);prepared=GIAutoPrepare(source.origin,frozen,selection);}
 float alpha=prepared.alpha;
 float3 throughput=asfloat(receiver.r3.xyz);float depth=asfloat(receiver.r1.w);
 float3 weightedSum=0;bool valid=true;
 float3 rawSum=0,peakPosition=0;float rawPeak=0,weightedPeak=0,peakWeight=0;
 uint peakMaterial=65534,peakRay=0,covered=0,peakFlags=0;
 [loop]for(uint slice=0;slice<selection.rayCount;++slice) {
  int4 address=int4(id.xy,slice,0);GISecondaryInput hit;
  hit.material=g_tMaterialId.Load(address).x;
  hit.positionUV=asuint(g_tPosition_TexcoordY.Load(address));hit.normalUV=asuint(g_tNormal_TexcoordX.Load(address));
  receiver.traceEnabled=motion||auditIndex!=0xffffffff;
  GIEvaluateSecondary(receiver,hit);
  float3 rgb=throughput*asfloat(receiver.r15.xyz);
  GIAutoDraw draw=(GIAutoDraw)0;draw.weight=1;
  if(auditIndex!=0xffffffff&&alpha>0)draw=GIAutoSamplePrepared(id.xy,giCB0[69].z,0,slice,source.normal,prepared,selection);
  draw.weight=GIReadWeight(id.xy,slice);
  if(motion){
   float4 fade=asfloat(giCB1[17]);float edge=fade.x-fade.y;
   float t=saturate((depth-edge)*rcp((fade.y+fade.x)-edge));
   float scale=mad(t*t*(3-2*t),fade.w-fade.z,fade.z);
   float3 pos=GIFrozenTransform(float4(asfloat(hit.positionUV.xyz),1),6,giCB0).xyz;
   GIMotionWrite(id.xy,slice,rgb,draw.weight,asfloat(receiver.r15.xyz),hit.material,pos,throughput,scale,receiver.terms);
  }
  if(train) {
   float3 position=GIFrozenTransform(float4(asfloat(hit.positionUV.xyz),1),6,giCB0).xyz;
   bestTraining=GIAutoKeepBrightest(bestTraining,GIAutoCandidate(selection.version,hit.material,position,rgb,draw.weight));
  }
  valid=valid&&all(isfinite(rgb))&&all(rgb>=0)&&isfinite(draw.weight)&&
        draw.weight>=0&&draw.weight<=(selection.rayCount==4?2.00001:1.66667);
  if(auditIndex!=0xffffffff){
   float score=rgb.r*draw.weight;
   if(all(isfinite(rgb))&&all(rgb>=0)&&isfinite(draw.weight)&&draw.weight>0&&score>redScore){
    redScore=score;redTerms=receiver.terms;float3 secondary=asfloat(receiver.r15.xyz);
    redA.position.x=rgb.x;redA.position.y=rgb.y;redA.position.z=rgb.z;redA.energy=draw.weight;
    redA.valid=asuint(secondary.x);redA.version=asuint(secondary.y);redA.reserved0=asuint(secondary.z);redA.reserved1=hit.material;
    float3 position=GIFrozenTransform(float4(asfloat(hit.positionUV.xyz),1),6,giCB0).xyz;
    redB.position.x=position.x;redB.position.y=position.y;redB.position.z=position.z;redB.energy=asfloat(slice);
    redB.valid=asuint(throughput.x);redB.version=asuint(throughput.y);redB.reserved0=asuint(throughput.z);
   }
   float lum=dot(rgb,float3(.2126,.7152,.0722));rawSum+=rgb;rawPeak=max(rawPeak,lum);
   if(draw.weight<1)++covered;
   if(lum*draw.weight>weightedPeak){weightedPeak=lum*draw.weight;peakWeight=draw.weight;
    peakPosition=GIFrozenTransform(float4(asfloat(hit.positionUV.xyz),1),6,giCB0).xyz;
    peakMaterial=hit.material;peakRay=slice;peakFlags=0;
    uint packetBase=2*trainingCapacity+2*min(trainingWidth,512u)*min(trainingHeight,512u);
    uint pixelIndex=(auditIndex-2*trainingCapacity)/2;
    uint packetIndex=pixelIndex*(selection.rayCount+1);
    GIAutoTraining originPair=autoTraining[packetBase+packetIndex/2];
    float3 actualOrigin=(packetIndex&1)?asfloat(uint3(originPair.valid,originPair.version,originPair.reserved0)):float3(originPair.position.x,originPair.position.y,originPair.position.z);
    uint stamp=(packetIndex&1)?originPair.reserved1:asuint(originPair.energy);
    uint directionIndex=packetIndex+slice+1;
    GIAutoTraining directionPair=autoTraining[packetBase+directionIndex/2];
    float4 actual=(directionIndex&1)?asfloat(uint4(directionPair.valid,directionPair.version,directionPair.reserved0,directionPair.reserved1)):float4(directionPair.position.x,directionPair.position.y,directionPair.position.z,directionPair.energy);
    if(stamp!=selection.sampleFrame)peakFlags|=32;
    else {
     if(abs(actual.w-draw.weight)>1e-5)peakFlags|=64;
     // alpha=0 still needs an expected direction for diagnostics only.
     GIAutoDraw expected=GIAutoSamplePrepared(id.xy,giCB0[69].z,0,slice,source.normal,prepared,selection);
     if(dot(normalize(actual.xyz),normalize(expected.direction))<.999999)peakFlags|=128;
     float3 actualDelta=peakPosition-actualOrigin;
     if(hit.material<65534&&dot(actualDelta,actualDelta)>1e-12&&dot(normalize(actualDelta),normalize(actual.xyz))<.9999)peakFlags|=256;
    }
    if(alpha>0&&hit.material<65534){
     float3 delta=peakPosition-source.origin;
     if(dot(delta,delta)>1e-12&&dot(normalize(delta),normalize(draw.direction))<.9999)peakFlags|=8;
     [unroll]for(uint k=0;k<2;++k)if(frozen.validMask&(1u<<k)){
      float3 c=float3(frozen.bounds[k].center.x,frozen.bounds[k].center.y,frozen.bounds[k].center.z);
      if(distance(peakPosition,c)<frozen.bounds[k].radius-.001&&draw.weight>1.6)peakFlags|=16;
     }
    }
   }
  }
  weightedSum+=rgb*draw.weight;
#ifdef GI_RAY_CLAMP
  gRayC[slice]=rgb*draw.weight;
#endif
 }
 if(bestTraining.valid)key=GIMakeWorldGuideKey(GIFrozenPoint(source.origin*.125f),GIFrozenPoint(source.normal));
 float3 radiance=weightedSum/float(selection.rayCount);
 // Never replace a weighted result with an unweighted guided average.
 if(!valid||!all(isfinite(radiance))){GIAutoAuditWrite(auditIndex,rawSum/float(selection.rayCount),rawPeak/float(selection.rayCount),radiance,weightedPeak/float(selection.rayCount),peakPosition,peakWeight,peakMaterial,1|(alpha>0?2:0),peakRay,covered);g_rwtDiffuseGIColorTarget[id.xy]=0;g_rwtHitInfo[id.xy]=65504;return;}
 float4 fade=asfloat(giCB1[17]);float nearEdge=fade.x-fade.y;
 float width=(fade.y+fade.x)-nearEdge;float t=saturate((depth-nearEdge)*rcp(width));
 float scale=mad(t*t*(3-2*t),fade.w-fade.z,fade.z);
 GIAutoAuditWrite(auditIndex,scale*rawSum/float(selection.rayCount),scale*rawPeak/float(selection.rayCount),scale*radiance,scale*weightedPeak/float(selection.rayCount),peakPosition,peakWeight,peakMaterial,5|(alpha>0?2:0)|peakFlags,peakRay,covered);
 GIRedWrite(auditIndex,redA,redB,scale,selection.rayCount);
 GIRedTermsWrite(auditIndex,redTerms,selection.rayCount);
 #ifdef GI_RAY_CLAMP
 gRayScale=scale/float(selection.rayCount);gRayCount=selection.rayCount;
 gRayLevel=GIRayMaxChannel(scale*radiance);gRayPending=true;
 #else
 g_rwtDiffuseGIColorTarget[id.xy]=float4(scale*radiance,1);
 #endif
 g_rwtHitInfo[id.xy]=asfloat(receiver.r5.w)/float(selection.rayCount);
}

// Each 8x16 dispatch group contains two independent, aligned 8x8 blocks.
// A larger training tile still publishes one record: only the block containing
// its original selected pixel competes. Partial blocks include inactive lanes
// in each barrier, with zero energy.
groupshared uint blockSlot[2], blockWinner[2], score[128];
#ifdef GI_RAY_CLAMP
groupshared float rayLevel[128];
#endif
[numthreads(8,16,1)]
void main(uint3 id:SV_DispatchThreadID,uint lane:SV_GroupIndex) {
 uint block=lane/64, first=block*64;
 if(lane==first)blockSlot[block]=0xffffffff;
 GroupMemoryBarrierWithGroupSync();
 uint slot=GIAutoTrainingIndex(id.xy,giCB0[69].z);
 if(slot!=0xffffffff)blockSlot[block]=slot;
 GroupMemoryBarrierWithGroupSync();
 slot=blockSlot[block];
 GIAutoTraining candidate;
 GIWorldGuideKey receiverKey;
 EvaluatePixel(id,slot!=0xffffffff,candidate,receiverKey);
 score[lane]=(candidate.valid&&receiverKey.valid)?asuint(candidate.energy):0;
#ifdef GI_RAY_CLAMP
 rayLevel[lane]=gRayPending?gRayLevel:-1.0f;
#endif
 GroupMemoryBarrierWithGroupSync();
#ifdef GI_RAY_CLAMP
 if(gRayPending){
  const int lx=int(lane%8u),ly=int(lane/8u);
  float levelSum=0,levelTop=0;uint levelCount=0;
  [unroll]for(int dy=-2;dy<=2;++dy){
   [unroll]for(int dx=-2;dx<=2;++dx){
    const int qx=lx+dx,qy=ly+dy;
    if((dx==0&&dy==0)||qx<0||qx>=8||qy<0||qy>=16)continue;
    const float v=rayLevel[qy*8+qx];
    if(v>=0){levelSum+=v;levelTop=max(levelTop,v);++levelCount;}
   }
  }
  float3 clamped=0;
  const bool limit=levelCount>=3u;
  const float limitValue=limit?GI_RAY_CLAMP*((levelSum-levelTop)/float(levelCount-1u))+GI_RAY_CLAMP_EPS:0;
  [loop]for(uint r=0;r<gRayCount;++r){
   const float3 c=gRayC[r]*gRayScale;
   const float m=GIRayMaxChannel(c);
   clamped+=(limit&&m>limitValue)?c*(limitValue/m):c;
  }
  g_rwtDiffuseGIColorTarget[id.xy]=float4(clamped,1);
 }
#endif
 if(lane==first&&slot!=0xffffffff){
  uint winner=first;
  [loop]for(uint i=first+1;i<first+64;i++)if(score[i]>score[winner])winner=i;
  blockWinner[block]=winner;
  if(score[winner]==0){
   autoTraining[2*slot]=(GIAutoTraining)0;
   autoTraining[2*slot+1]=(GIAutoTraining)0;
  }
 }
 GroupMemoryBarrierWithGroupSync();
 if(slot!=0xffffffff&&score[lane]>0&&blockWinner[block]==lane)
  GIRegionalStore(slot,receiverKey,candidate,selection.version);
}
