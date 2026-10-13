#include "../src/shaders/pt_primary_commit.hlsli"
[numthreads(8,1,1)]
void main(uint3 id:SV_DispatchThreadID){
    uint x=id.x;float3 c=float3(.25,.5,.75);uint admission=1;uint2 extent=uint2(8,1);
    if(x==0)admission=0;if(x==1)admission=2;
    if(x==2)c.x=asfloat(0x7fc00000);if(x==4)c.y=-1;
    if(x==5)extent.x=7;if(x==6)c=0;
    PTCommitPrimary(id.xy,extent,c,admission);
}
