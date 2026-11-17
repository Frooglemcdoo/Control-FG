#include "../src/gi_reuse/auto_target_binding.h"
StructuredBuffer<GIAutoTargets> targets : register(t0,space19);
cbuffer Selection : register(b0,space19) {GIAutoSelection selection;}
RWStructuredBuffer<uint4> output : register(u0);
[numthreads(1,1,1)]
void main() {
 GIAutoTargets t=targets[0];
 output[0]=uint4(GIAutoUsable(t,selection)?1:0,t.version,t.frame,t.validMask);
 output[2]=uint4(selection.sampleFrame,0,0,0);
 output[1]=uint4(selection.version,selection.sourceFrame,selection.rayCount,asuint(selection.fraction));
}
