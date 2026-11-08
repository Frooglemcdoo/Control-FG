// Tests root constants and root UAV restoration after the complete NRD chain.
cbuffer Probe : register(b0) { uint4 expected; }
RWStructuredBuffer<uint4> result : register(u0);
[numthreads(1,1,1)]
void main(uint3 id:SV_DispatchThreadID) { result[0]=expected; }
