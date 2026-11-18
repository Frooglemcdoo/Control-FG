// R449 paired evaluator: stratified mixture with balance weights on every ray.
#define GI_SECONDARY_LIBRARY_ONLY
#include "gi_evaluator_secondary.generated.hlsl"
#include "gi_frozen_receiver.hlsl"
#include "gi_auto_sample.hlsl"
#include "gi_auto_training.hlsl"
#include "gi_auto_pixel_audit.hlsl"
StructuredBuffer<GIAutoTargets> targets : register(t0,space19);
cbuffer Selection : register(b0,space19) {GIAutoSelection selection;}
[numthreads(8,16,1)]
void main(uint3 id:SV_DispatchThreadID) {
 if(any(id.xy>=uint2(trainingWidth,trainingHeight)))return;
 uint auditIndex=GIAutoAuditIndex(id.xy);
 GIAutoAuditWrite(auditIndex,0,0,0,0,0,0,65534,0,0,0);
 uint trainingIndex=GIAutoTrainingIndex(id.xy,giCB0[69].z);
 if(trainingIndex!=0xffffffff)autoTraining[trainingIndex]=(GIAutoTraining)0;
 GIAutoTraining bestTraining=(GIAutoTraining)0;
 GIShadingRegisters receiver;
 if(giCB4[0].z!=selection.rayCount||(selection.rayCount!=10&&selection.rayCount!=4)||!GIInitializeReceiver(id.xy,receiver)) {
  g_rwtDiffuseGIColorTarget[id.xy]=0;g_rwtHitInfo[id.xy]=65504;return;
 }
 GIFrozenReceiver source=GIFrozenDecodeReceiver(id.xy,g_tGBuffer1.Load(int3(id.xy,0)),
  g_tLinearDepth.Load(int3(id.xy,0)).x,g_tClipDepth.Load(int3(id.xy,0)).x,giCB0);
 if(!source.valid){g_rwtDiffuseGIColorTarget[id.xy]=0;g_rwtHitInfo[id.xy]=65504;return;}
 GIAutoTargets frozen=targets[0];float alpha=GIAutoAlpha(frozen,selection);
 float3 throughput=asfloat(receiver.r3.xyz);float depth=asfloat(receiver.r1.w);
 float3 weightedSum=0;bool valid=true;
 float3 rawSum=0,peakPosition=0;float rawPeak=0,weightedPeak=0,peakWeight=0;
 uint peakMaterial=65534,peakRay=0,covered=0,peakFlags=0;
 [loop]for(uint slice=0;slice<selection.rayCount;++slice) {
  int4 address=int4(id.xy,slice,0);GISecondaryInput hit;
  hit.material=g_tMaterialId.Load(address).x;
  hit.positionUV=asuint(g_tPosition_TexcoordY.Load(address));hit.normalUV=asuint(g_tNormal_TexcoordX.Load(address));
  GIEvaluateSecondary(receiver,hit);
  float3 rgb=throughput*asfloat(receiver.r15.xyz);
  GIAutoDraw draw=(GIAutoDraw)0;draw.weight=1;
  if(alpha>0)draw=GIAutoSample(id.xy,giCB0[69].z,0,slice,source.origin,source.normal,frozen,selection);
  if(trainingIndex!=0xffffffff) {
   float3 position=GIFrozenTransform(float4(asfloat(hit.positionUV.xyz),1),6,giCB0).xyz;
   bestTraining=GIAutoKeepBrightest(bestTraining,GIAutoCandidate(selection.version,hit.material,position,rgb,draw.weight));
  }
  valid=valid&&all(isfinite(rgb))&&all(rgb>=0)&&isfinite(draw.weight)&&
        draw.weight>=0&&draw.weight<=(selection.rayCount==4?2.00001:1.66667);
  if(auditIndex!=0xffffffff){
   float lum=dot(rgb,float3(.2126,.7152,.0722));rawSum+=rgb;rawPeak=max(rawPeak,lum);
   if(draw.weight<1)++covered;
   if(lum*draw.weight>weightedPeak){weightedPeak=lum*draw.weight;peakWeight=draw.weight;
    peakPosition=GIFrozenTransform(float4(asfloat(hit.positionUV.xyz),1),6,giCB0).xyz;
    peakMaterial=hit.material;peakRay=slice;peakFlags=0;
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
 if(trainingIndex!=0xffffffff)autoTraining[trainingIndex]=bestTraining;
 float3 radiance=weightedSum/float(selection.rayCount);
 // Never replace a weighted result with an unweighted guided average.
 if(!valid||!all(isfinite(radiance))){GIAutoAuditWrite(auditIndex,rawSum/float(selection.rayCount),rawPeak/float(selection.rayCount),radiance,weightedPeak/float(selection.rayCount),peakPosition,peakWeight,peakMaterial,1|(alpha>0?2:0),peakRay,covered);g_rwtDiffuseGIColorTarget[id.xy]=0;g_rwtHitInfo[id.xy]=65504;return;}
 float4 fade=asfloat(giCB1[17]);float nearEdge=fade.x-fade.y;
 float width=(fade.y+fade.x)-nearEdge;float t=saturate((depth-nearEdge)*rcp(width));
 float scale=mad(t*t*(3-2*t),fade.w-fade.z,fade.z);
 GIAutoAuditWrite(auditIndex,scale*rawSum/float(selection.rayCount),scale*rawPeak/float(selection.rayCount),scale*radiance,scale*weightedPeak/float(selection.rayCount),peakPosition,peakWeight,peakMaterial,5|(alpha>0?2:0)|peakFlags,peakRay,covered);
 g_rwtDiffuseGIColorTarget[id.xy]=float4(scale*radiance,1);
 g_rwtHitInfo[id.xy]=asfloat(receiver.r5.w)/float(selection.rayCount);
}
