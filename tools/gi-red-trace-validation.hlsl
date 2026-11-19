#include "../src/gi_reuse/auto_target_contract.h"
RWStructuredBuffer<GIAutoTraining> autoTraining:register(u0);
cbuffer Controls:register(b0){uint trainingWidth,trainingHeight,trainingCapacity,rays;}
#include "../src/shaders/gi_red_trace.hlsl"
[numthreads(64,1,1)]void main(uint3 id:SV_DispatchThreadID){
 if(id.x>=trainingWidth*trainingHeight)return;
 GIAutoTraining a=(GIAutoTraining)0,b=(GIAutoTraining)0;
 a.position.x=2;a.position.y=3;a.position.z=4;a.energy=1.25;
 a.valid=asuint(5.0);a.version=asuint(6.0);a.reserved0=asuint(7.0);a.reserved1=123;
 b.position.x=8;b.position.y=9;b.position.z=10;b.energy=asfloat(rays-1);
 b.valid=asuint(11.0);b.version=asuint(12.0);b.reserved0=asuint(13.0);
 GIRedWrite(2*trainingCapacity+2*id.x,a,b,.5,rays);
 GIRedWrite(0xffffffff,a,b,1,rays); // Disabled capture must not write.
}
