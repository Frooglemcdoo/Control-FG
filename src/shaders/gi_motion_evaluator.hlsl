#include "gi_motion_layout.hlsl"
void GIMotionClear(uint2 pixel){
 if(!GIMotionEnabled(trainingStepPacked,trainingCapacityPacked))return;
 [loop]for(uint r=0;r<selection.rayCount;++r){
  uint base=GIMotionAddress(pixel,r,trainingWidth,trainingHeight,trainingCapacityPacked,selection.rayCount)/32u+1;
  [unroll]for(uint t=0;t<6;++t)autoTraining[base+t]=(GIAutoTraining)0;
 }
}
void GIMotionWrite(uint2 pixel,uint ray,float3 rgb,float weight,float3 secondary,uint material,float3 position,float3 throughput,float scale,GISecondaryTerms terms){
 uint base=GIMotionAddress(pixel,ray,trainingWidth,trainingHeight,trainingCapacityPacked,selection.rayCount)/32u+1;
 autoTraining[base]=GIRedPair(float4(rgb,weight),float4(secondary,asfloat(material)));
 autoTraining[base+1]=GIRedPair(float4(position,asfloat(ray|0x80000000u)),float4(throughput,scale));
 autoTraining[base+2]=GIRedPair(terms.texture,terms.tint);
 autoTraining[base+3]=GIRedPair(terms.volume,terms.surface);
 autoTraining[base+4]=GIRedPair(terms.pointLight,terms.spot);
 autoTraining[base+5]=GIRedPair(terms.fallback,terms.brdf);
}
