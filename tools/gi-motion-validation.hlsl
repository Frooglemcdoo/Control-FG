#define CONTROL_GI_REGIONAL 1
#include "../src/shaders/gi_auto_sample.hlsl"
cbuffer Selection:register(b0,space19){GIAutoSelection selection;}
#ifdef PRODUCER
#include "../src/shaders/gi_producer_packet.hlsl"
#include "../src/shaders/gi_motion_producer.hlsl"
[numthreads(8,8,1)]void main(uint3 id:SV_DispatchThreadID){
 if(any(id.xy>=uint2(packetWidth,packetHeight)))return;
 uint4 constants[71];[unroll]for(uint i=0;i<71;++i)constants[i]=uint4(i,i+1,i+2,i+3);
 GIMotionProducerBegin(id.xy,float3(id.xy,3),true,constants);
 for(uint r=0;r<selection.rayCount;++r)GIMotionProducerRay(id.xy,r,float3(0,0,1),1);
}
#else
#include "../src/shaders/gi_auto_training.hlsl"
#include "../src/shaders/gi_red_trace.hlsl"
#include "../src/shaders/gi_motion_evaluator.hlsl"
[numthreads(8,8,1)]void main(uint3 id:SV_DispatchThreadID){
 if(any(id.xy>=uint2(trainingWidth,trainingHeight)))return;
 GIMotionClear(id.xy);
 if(!GIMotionEnabled(trainingStepPacked,trainingCapacityPacked)||all(id.xy==0))return;
 GISecondaryTerms t=(GISecondaryTerms)0;t.texture=1;t.tint=2;t.volume=3;t.surface=4;t.pointLight=5;t.spot=6;t.fallback=7;t.brdf=8;
 for(uint r=0;r<selection.rayCount;++r)GIMotionWrite(id.xy,r,float3(.125,.0625,.03125),1,float3(2,3,4),1885,float3(5,6,7),float3(1,1,1),1,t);
}
#endif
