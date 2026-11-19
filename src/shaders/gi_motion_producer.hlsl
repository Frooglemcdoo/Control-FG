#include "gi_motion_layout.hlsl"
void GIMotionProducerBegin(uint2 pixel,float3 origin,bool valid,uint4 constants[71]){
 if(!GIMotionEnabled(packetStepPacked,packetCapacityPacked))return;
 if(all(pixel==0)){
  uint base=GIMotionBase(packetWidth,packetHeight,packetCapacityPacked,selection.rayCount);
  [loop]for(uint c=0;c<71;++c)producerPackets.Store4(base+c*16,constants[c]);
  producerPackets.Store4(base+1136,uint4(0x4d343834u,selection.sampleFrame,packetWidth,packetHeight));
  producerPackets.Store4(base+1152,uint4(selection.rayCount,224u,2048u,1u));
 }
 [loop]for(uint r=0;r<selection.rayCount;++r){
  uint at=GIMotionAddress(pixel,r,packetWidth,packetHeight,packetCapacityPacked,selection.rayCount);
  producerPackets.Store4(at,uint4(asuint(origin),valid?selection.sampleFrame:0));producerPackets.Store4(at+16,0);
 }
}
void GIMotionProducerRay(uint2 pixel,uint ray,float3 direction,float weight){
 if(GIMotionEnabled(packetStepPacked,packetCapacityPacked))producerPackets.Store4(GIMotionAddress(pixel,ray,packetWidth,packetHeight,packetCapacityPacked,selection.rayCount)+16,asuint(float4(direction,weight)));
}
