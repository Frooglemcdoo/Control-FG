#include "gi_weight_transport.hlsl"
#include "../gi_reuse/scene_capture_grid.h"
// Capture-only evidence from the actual producer, never a reconstructed draw.
RWByteAddressBuffer producerPackets:register(u0,space19);
cbuffer ProducerCapture:register(b1,space19){uint packetWidth,packetHeight,packetStepPacked,packetCapacityPacked;}
uint GIPacketAddress(uint2 pixel){
 if((packetCapacityPacked&65536u)==0)return 0xffffffff;
 uint2 size=min(uint2(packetWidth,packetHeight),512u);
 uint2 start=uint2((packetStepPacked>>9)&16383u,packetCapacityPacked>>17);
 if(packetStepPacked&8388608u){
  uint2 cell=uint2(GISceneCell(pixel.x,packetWidth,size.x),GISceneCell(pixel.y,packetHeight,size.y));
  if(any(cell==0xffffffffu))return 0xffffffff;
  pixel=cell;start=0;
 }
 if(any(start>uint2(packetWidth,packetHeight)-size)||any(pixel<start)||any(pixel>=start+size))return 0xffffffff;
 uint index=(pixel.y-start.y)*size.x+pixel.x-start.x;
 return (packetCapacityPacked&65535u)*64u+size.x*size.y*64u+index*(selection.rayCount+1)*16u;
}

void GIStoreWeight(uint2 pixel,uint ray,float weight){
 producerPackets.Store(GIWeightAddress(pixel,ray,packetWidth,packetHeight,packetCapacityPacked,selection.rayCount),asuint(weight));
}
