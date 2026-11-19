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
 GISecondaryTerms t;
 t.texture=float4(1,2,3,4);t.tint=float4(5,6,7,8);t.volume=float4(9,10,11,12);t.surface=float4(13,14,15,16);
 t.pointLight=float4(17,18,19,20);t.spot=float4(21,22,23,24);t.fallback=float4(25,26,27,28);t.brdf=float4(29,30,31,32);
 GIRedTermsWrite(2*trainingCapacity+2*id.x,t,rays);
 GIRedTermsWrite(0xffffffff,t,rays);
 GIRedWrite(2*trainingCapacity+2*id.x,a,b,.5,rays);
 GIRedWrite(0xffffffff,a,b,1,rays); // Disabled capture must not write.
}
