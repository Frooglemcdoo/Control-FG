#include "../src/shaders/gi_auto_sample.hlsl"
cbuffer Selection:register(b0,space19){GIAutoSelection selection;}
#ifdef PRODUCER
#include "../src/shaders/gi_producer_packet.hlsl"
[numthreads(8,8,1)]void main(uint3 id:SV_DispatchThreadID){
 if(any(id.xy>=uint2(packetWidth,packetHeight)))return;
 for(uint ray=0;ray<selection.rayCount;++ray){
  // Distinct finite IEEE values, including zero and subnormal; no quantization.
  uint bits=(id.x+id.y*packetWidth+ray)%4==0?0:0x3e000001u+(id.x*37u+id.y*73u+ray*1009u)%0x1000000u;
  if(id.x==1&&id.y==0)bits=ray+1;
  GIStoreWeight(id.xy,ray,asfloat(bits));
 }
}
#else
#include "../src/shaders/gi_auto_training.hlsl"
#include "../src/shaders/gi_weight_read.hlsl"
RWByteAddressBuffer verification:register(u1,space19);
[numthreads(8,8,1)]void main(uint3 id:SV_DispatchThreadID){
 if(any(id.xy>=uint2(trainingWidth,trainingHeight)))return;
 for(uint ray=0;ray<selection.rayCount;++ray)
  verification.Store(((id.y*trainingWidth+id.x)*selection.rayCount+ray)*4,asuint(GIReadWeight(id.xy,ray)));
}
#endif
