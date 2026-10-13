#include "../src/shaders/pt_primary_commit.hlsli"
[numthreads(16,1,1)]
void main(uint3 id:SV_DispatchThreadID){
    uint x=id.x;float3 c=float3(.25,.5,.75);uint admission=1;uint2 extent=uint2(16,1);
    if(x==0)admission=0;if(x==1)admission=2;
    if(x==2)c.x=asfloat(0x7fc00000);if(x==4)c.y=-1;
    if(x==5)extent.x=7;if(x==6)c=0;
    float3 fill=0;
    if(x==7)fill=float3(1,2,4);
    if(x==8)fill.x=asfloat(0x7fc00000);
    if(x==9)fill.y=-1;
    if(x==10){c=asfloat(0x7f7fffff).xxx;fill=c;} // sum overflow
    if(x==11){c=0;fill=float3(1,2,4);}
    if(x>=12)admission=0;
    PTCommitPrimaryWithFill(id.xy,extent,c,fill,admission);
}
