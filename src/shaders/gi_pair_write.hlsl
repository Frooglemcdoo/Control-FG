#ifndef GI_PAIR_WRITE
#define GI_PAIR_WRITE
#include "gi_weight_transport.hlsl"
bool GIPairEnabled(){return (trainingCapacityPacked&65536u)!=0&&(trainingStepPacked&8388608u)!=0;}
uint GIPairBase(){
 uint weights=(trainingWidth*trainingHeight*selection.rayCount*4u+31u)&~31u;
 return GIWeightBase(trainingWidth,trainingHeight,trainingCapacityPacked,selection.rayCount)+weights;
}
// One 32-byte record per screen pixel: native SampleLevel0 RGB + flags, manual bilinear mip1 RGB + flags.
void GIPairWrite(uint2 pixel,float3 a,uint flagsA,float3 b,uint flagsB){
 if(!GIPairEnabled())return;
 GIAutoTraining v=(GIAutoTraining)0;
 v.position.x=a.x;v.position.y=a.y;v.position.z=a.z;v.energy=asfloat(flagsA);
 v.valid=asuint(b.x);v.version=asuint(b.y);v.reserved0=asuint(b.z);v.reserved1=flagsB;
 autoTraining[GIPairBase()/32u+pixel.y*trainingWidth+pixel.x]=v;
}
#endif
