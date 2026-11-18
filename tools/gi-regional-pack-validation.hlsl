#include "../src/gi_reuse/regional_target_build.h"
StructuredBuffer<GIRegionalTraining> records:register(t0);
RWStructuredBuffer<GIAutoTraining> autoTraining:register(u0);
#include "../src/shaders/gi_regional_training_write.hlsl"
cbuffer Limits:register(b0){uint count;}
[numthreads(64,1,1)]void main(uint3 id:SV_DispatchThreadID){
 if(id.x>=count)return;GIRegionalTraining r=records[id.x];GIAutoTraining best=(GIAutoTraining)0;
 best.position=r.source.center;best.energy=r.score;GIRegionalStore(id.x,r.receiver,best,r.version);
}
