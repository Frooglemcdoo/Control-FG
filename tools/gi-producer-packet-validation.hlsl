#include "../src/shaders/gi_auto_sample.hlsl"
cbuffer Selection:register(b0,space19){GIAutoSelection selection;}
#include "../src/shaders/gi_producer_packet.hlsl"
[numthreads(8,8,1)]void main(uint3 id:SV_DispatchThreadID){
 uint2 pixel=id.xy+uint2((packetStepPacked>>9)&16383u,packetCapacityPacked>>17);
 uint address=GIPacketAddress(pixel);if(address==0xffffffff)return;
 producerPackets.Store4(address,uint4(asuint(float3(1,2,3)),selection.sampleFrame));
 GIAutoPrepared prepared=(GIAutoPrepared)0;
 for(uint i=0;i<selection.rayCount;++i){
  GIAutoDraw draw=GIAutoSamplePrepared(pixel,0,0,i,float3(0,0,1),prepared,selection);
  producerPackets.Store4(address+16*(i+1),asuint(float4(draw.direction,draw.weight)));
 }
}
