#define GI_OWNED_SINGLE_USE
#include "gi_dispatch_gate.hlsl"
RWByteAddressBuffer executions : register(u0);
[numthreads(1,1,1)]
void main(){if(!GIOwnedGateOpen())return;uint previous;executions.InterlockedAdd(0,1,previous);}
