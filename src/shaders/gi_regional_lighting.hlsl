#define CONTROL_GI_REGIONAL 1
// R449 paired evaluator: stratified mixture with balance weights on every ray.
#define GI_SECONDARY_TERMS
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
[numthreads(8,16,1)]
void main(uint3 id:SV_DispatchThreadID) {
 if(any(id.xy>=uint2(trainingWidth,trainingHeight)))return;
 uint auditIndex=GIAutoAuditIndex(id.xy);
 GIAutoTraining redA=(GIAutoTraining)0,redB=(GIAutoTraining)0;float redScore=0;
 GISecondaryTerms redTerms=(GISecondaryTerms)0;
 GIRedWrite(auditIndex,redA,redB,0,selection.rayCount);
 GIRedTermsWrite(auditIndex,redTerms,selection.rayCount);
 GIAutoAuditWrite(auditIndex,0,0,0,0,0,0,65534,0,0,0);
 uint trainingIndex=GIAutoTrainingIndex(id.xy,giCB0[69].z);
 if(trainingIndex!=0xffffffff){autoTraining[2*trainingIndex]=(GIAutoTraining)0;autoTraining[2*trainingIndex+1]=(GIAutoTraining)0;}
 GIAutoTraining bestTraining=(GIAutoTraining)0;
 GIShadingRegisters receiver;
 if(giCB4[0].z!=selection.rayCount||(selection.rayCount!=10&&selection.rayCount!=4)||!GIInitializeReceiver(id.xy,receiver)) {
  g_rwtDiffuseGIColorTarget[id.xy]=0;g_rwtHitInfo[id.xy]=65504;return;
 }
 GIFrozenReceiver source=GIFrozenDecodeReceiver(id.xy,g_tGBuffer1.Load(int3(id.xy,0)),
  g_tLinearDepth.Load(int3(id.xy,0)).x,g_tClipDepth.Load(int3(id.xy,0)).x,giCB0);
 if(!source.valid){g_rwtDiffuseGIColorTarget[id.xy]=0;g_rwtHitInfo[id.xy]=65504;return;}
 GIAutoTargets frozen=GISelectRegional(source.origin,source.normal,selection);GIAutoPrepared prepared=GIAutoPrepare(source.origin,frozen,selection);float alpha=prepared.alpha;
 float3 throughput=asfloat(receiver.r3.xyz);float depth=asfloat(receiver.r1.w);
 float3 weightedSum=0;bool valid=true;
 float3 rawSum=0,peakPosition=0;float rawPeak=0,weightedPeak=0,peakWeight=0;
 uint peakMaterial=65534,peakRay=0,covered=0,peakFlags=0;
 [loop]for(uint slice=0;slice<selection.rayCount;++slice) {
  int4 address=int4(id.xy,slice,0);GISecondaryInput hit;
  hit.material=g_tMaterialId.Load(address).x;
  hit.positionUV=asuint(g_tPosition_TexcoordY.Load(address));hit.normalUV=asuint(g_tNormal_TexcoordX.Load(address));
  receiver.traceEnabled=auditIndex!=0xffffffff;
  GIEvaluateSecondary(receiver,hit);
  float3 rgb=throughput*asfloat(receiver.r15.xyz);
  GIAutoDraw draw=(GIAutoDraw)0;draw.weight=1;
  if(alpha>0)draw=GIAutoSamplePrepared(id.xy,giCB0[69].z,0,slice,source.normal,prepared,selection);
  if(trainingIndex!=0xffffffff) {
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
 }
 if(trainingIndex!=0xffffffff&&bestTraining.valid){
  GIWorldGuideKey key=GIMakeWorldGuideKey(GIFrozenPoint(source.origin*.125f),GIFrozenPoint(source.normal));
  GIRegionalStore(trainingIndex,key,bestTraining,selection.version);
 }
 float3 radiance=weightedSum/float(selection.rayCount);
 // Never replace a weighted result with an unweighted guided average.
 if(!valid||!all(isfinite(radiance))){GIAutoAuditWrite(auditIndex,rawSum/float(selection.rayCount),rawPeak/float(selection.rayCount),radiance,weightedPeak/float(selection.rayCount),peakPosition,peakWeight,peakMaterial,1|(alpha>0?2:0),peakRay,covered);g_rwtDiffuseGIColorTarget[id.xy]=0;g_rwtHitInfo[id.xy]=65504;return;}
 float4 fade=asfloat(giCB1[17]);float nearEdge=fade.x-fade.y;
 float width=(fade.y+fade.x)-nearEdge;float t=saturate((depth-nearEdge)*rcp(width));
 float scale=mad(t*t*(3-2*t),fade.w-fade.z,fade.z);
 GIAutoAuditWrite(auditIndex,scale*rawSum/float(selection.rayCount),scale*rawPeak/float(selection.rayCount),scale*radiance,scale*weightedPeak/float(selection.rayCount),peakPosition,peakWeight,peakMaterial,5|(alpha>0?2:0)|peakFlags,peakRay,covered);
 GIRedWrite(auditIndex,redA,redB,scale,selection.rayCount);
 GIRedTermsWrite(auditIndex,redTerms,selection.rayCount);
 g_rwtDiffuseGIColorTarget[id.xy]=float4(scale*radiance,1);
 g_rwtHitInfo[id.xy]=asfloat(receiver.r5.w)/float(selection.rayCount);
}
