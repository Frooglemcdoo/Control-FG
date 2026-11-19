#include "../gi_reuse/scene_capture_grid.h"
// Pixel diagnostics share the training allocation, never the learning records.
uint GIAutoAuditIndex(uint2 pixel) {
 if((trainingCapacityPacked&65536u)==0)return 0xffffffff;
 uint2 size=min(uint2(trainingWidth,trainingHeight),512u);
 uint2 start=uint2((trainingStepPacked>>9)&16383u,trainingCapacityPacked>>17);
 if(trainingStepPacked&8388608u){
  uint2 cell=uint2(GISceneCell(pixel.x,trainingWidth,size.x),GISceneCell(pixel.y,trainingHeight,size.y));
  if(any(cell==0xffffffffu))return 0xffffffff;
  pixel=cell;start=0;
 }
 if(any(start>uint2(trainingWidth,trainingHeight)-size))return 0xffffffff;
 if(any(pixel<start)||any(pixel>=start+size))return 0xffffffff;
 #ifdef CONTROL_GI_REGIONAL
 return 2*trainingCapacity+2*
#else
 return trainingCapacity+2*
#endif
((pixel.y-start.y)*size.x+pixel.x-start.x);
}
void GIAutoAuditWrite(uint index,float3 raw,float rawPeak,float3 weighted,float weightedPeak,
 float3 position,float weight,uint material,uint flags,uint ray,uint covered) {
 if(index==0xffffffff)return;
 GIAutoTraining a=(GIAutoTraining)0,b=(GIAutoTraining)0;
 a.position.x=raw.x;a.position.y=raw.y;a.position.z=raw.z;a.energy=rawPeak;
 a.valid=asuint(weighted.x);a.version=asuint(weighted.y);a.reserved0=asuint(weighted.z);a.reserved1=asuint(weightedPeak);
 b.position.x=position.x;b.position.y=position.y;b.position.z=position.z;b.energy=weight;
 b.valid=material;b.version=flags;b.reserved0=ray;b.reserved1=covered;
 autoTraining[index]=a;autoTraining[index+1]=b;
}
