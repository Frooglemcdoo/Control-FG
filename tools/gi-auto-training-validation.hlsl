#include "../src/shaders/gi_auto_sample.hlsl"
#include "../src/shaders/gi_auto_training.hlsl"
cbuffer Test:register(b0){uint phase,version;}
[numthreads(8,8,1)]
void main(uint3 id:SV_DispatchThreadID) {
 uint index=GIAutoTrainingIndex(id.xy,211);if(index==0xffffffff)return;
 autoTraining[index]=(GIAutoTraining)0;
 if(index%3==2)return; // Invalid receiver.
 GIAutoTrain(index,version,(phase||index%3==1)?65535:12,float3(index,.5,3),float3(2,4,8),.5);
}
