// Candidate matching evaluator. Not hooked into gameplay.
#define GI_SECONDARY_LIBRARY_ONLY
#include "gi_evaluator_secondary.generated.hlsl"
#include "gi_frozen_receiver.hlsl"
#include "gi_auto_sample.hlsl"
#include "../gi_reuse/pilot_guide_resolve.h"
#include "gi_auto_training.hlsl"
StructuredBuffer<GIAutoTargets> targets : register(t0,space19);
cbuffer Selection : register(b0,space19) {GIAutoSelection selection;}
[numthreads(8,16,1)]
void main(uint3 id:SV_DispatchThreadID) {
 if(any(id.xy>=uint2(trainingWidth,trainingHeight)))return;
 uint trainingIndex=GIAutoTrainingIndex(id.xy,giCB0[69].z);
 if(trainingIndex!=0xffffffff)autoTraining[trainingIndex]=(GIAutoTraining)0;
 uint trainingRay=GIProducerHash(id.x*73856093u^id.y*19349663u^giCB0[69].z*83492791u^0x19d73bafu)%10;
 GIShadingRegisters receiver;
 if(asint(giCB4[0].z)!=10||selection.rayCount!=10||!GIInitializeReceiver(id.xy,receiver)) {
  g_rwtDiffuseGIColorTarget[id.xy]=0;g_rwtHitInfo[id.xy]=65504;return;
 }
 GIFrozenReceiver source=GIFrozenDecodeReceiver(id.xy,g_tGBuffer1.Load(int3(id.xy,0)),
  g_tLinearDepth.Load(int3(id.xy,0)).x,g_tClipDepth.Load(int3(id.xy,0)).x,giCB0);
 if(!source.valid){g_rwtDiffuseGIColorTarget[id.xy]=0;g_rwtHitInfo[id.xy]=65504;return;}
 GIAutoTargets frozen=targets[0];float alpha=GIAutoAlpha(frozen,selection);
 float3 throughput=asfloat(receiver.r3.xyz);float depth=asfloat(receiver.r1.w);
 float3 nativeSum=0;GIPilotResolve resolve=(GIPilotResolve)0;
 [loop]for(uint slice=0;slice<10;++slice) {
  int4 address=int4(id.xy,slice,0);GISecondaryInput hit;
  hit.material=g_tMaterialId.Load(address).x;
  hit.positionUV=asuint(g_tPosition_TexcoordY.Load(address));hit.normalUV=asuint(g_tNormal_TexcoordX.Load(address));
  GIEvaluateSecondary(receiver,hit);
  float3 rgb=throughput*asfloat(receiver.r15.xyz);
  GIAutoDraw draw=(GIAutoDraw)0;draw.weight=1;
  if(alpha>0&&slice>0)draw=GIAutoSample(id.xy,giCB0[69].z,0,slice,source.origin,source.normal,frozen,selection);
  if(trainingIndex!=0xffffffff&&slice==trainingRay) {
   float3 position=GIFrozenTransform(float4(asfloat(hit.positionUV.xyz),1),6,giCB0).xyz;
   GIAutoTrain(trainingIndex,selection.version,hit.material,position,rgb,draw.weight);
  }
  if(alpha==0)nativeSum+=rgb;
  else if(slice==0)resolve=GIPilotBegin(GIFrozenPoint(rgb),10,alpha,selection.version);
  else {
   resolve=GIPilotAdd(resolve,GIFrozenPoint(rgb),draw.weight,selection.version);
  }
 }
 float3 radiance=nativeSum/10;
 if(alpha>0){GIPilotResult result=GIPilotFinish(resolve);radiance=GIFrozenVector(result.rgb);
  // Invalid estimator input is NOT permission to publish an unweighted average.
  // Host diagnostics/handling must be implemented before this candidate is live.
  if(!result.valid){g_rwtDiffuseGIColorTarget[id.xy]=0;g_rwtHitInfo[id.xy]=65504;return;}}
 float4 fade=asfloat(giCB1[17]);float nearEdge=fade.x-fade.y;
 float width=(fade.y+fade.x)-nearEdge;float t=saturate((depth-nearEdge)*rcp(width));
 float scale=mad(t*t*(3-2*t),fade.w-fade.z,fade.z);
 g_rwtDiffuseGIColorTarget[id.xy]=float4(scale*radiance,1);
 g_rwtHitInfo[id.xy]=asfloat(receiver.r5.w)/10;
}
