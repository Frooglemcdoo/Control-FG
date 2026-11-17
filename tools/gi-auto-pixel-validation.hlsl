#include "../src/gi_reuse/auto_target_contract.h"
#include "../src/gi_reuse/producer_sampling.h"
#include "../src/shaders/gi_auto_training.hlsl"
#include "../src/shaders/gi_auto_pixel_audit.hlsl"
[numthreads(8,8,1)]
void main(uint3 id:SV_DispatchThreadID){
 if(any(id.xy>=uint2(trainingWidth,trainingHeight)))return;
 uint i=GIAutoAuditIndex(id.xy);
 GIAutoAuditWrite(i,float3(id.xy,3),4,float3(5,6,7),8,float3(9,10,11),.25,1913,7,4,6);
 if((id.x+id.y)%3==0)GIAutoAuditWrite(i,0,0,0,0,0,0,65534,0,0,0);
}
